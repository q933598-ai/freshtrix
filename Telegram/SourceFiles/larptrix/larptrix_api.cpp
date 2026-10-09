/*
Freshtrix Larptrix API bridge.
*/
#include "larptrix/larptrix_api.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonParseError>
#include <QtCore/QRegularExpression>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkCookie>
#include <QtNetwork/QNetworkCookieJar>

namespace Larptrix {
namespace {

QString normalizedServerUrl(const QString &input, QString *error) {
	auto value = input.trimmed();
	if (value.isEmpty()) {
		if (error) *error = QStringLiteral("Enter a Larptrix server URL.");
		return {};
	}

	if (!value.contains(QStringLiteral("://"))) {
		value.prepend(QStringLiteral("https://"));
	}

	QUrl url(value);
	const auto scheme = url.scheme().toLower();
	if (!url.isValid()
		|| url.host().isEmpty()
		|| (scheme != QStringLiteral("https")
			&& scheme != QStringLiteral("http"))
		|| !url.userInfo().isEmpty()
		|| !url.query().isEmpty()
		|| !url.fragment().isEmpty()) {
		if (error) {
			*error = QStringLiteral(
				"Use a server URL such as https://chat.example.com.");
		}
		return {};
	}

	url.setScheme(scheme);
	url.setPath(url.path().replace(QRegularExpression(QStringLiteral("/+$")), QString()));
	return url.toString(QUrl::RemoveTrailingSlash);
}

} // namespace

Api::Api(QObject *parent) : QObject(parent) {
}

bool Api::setServerUrl(const QString &url, QString *error) {
	auto normalized = normalizedServerUrl(url, error);
	if (normalized.isEmpty()) {
		return false;
	}
	_serverUrl = std::move(normalized);
	return true;
}

QString Api::serverUrl() const {
	return _serverUrl;
}

bool Api::isConfigured() const {
	return !_serverUrl.isEmpty();
}

QByteArray Api::sessionCookieHeader() const {
	if (_serverUrl.isEmpty() || !_network.cookieJar()) {
		return {};
	}
	const auto cookies = _network.cookieJar()->cookiesForUrl(QUrl(_serverUrl));
	QByteArray result;
	for (const auto &cookie : cookies) {
		const auto value = cookie.toRawForm(QNetworkCookie::NameAndValueOnly);
		if (value.isEmpty()) {
			continue;
		}
		if (!result.isEmpty()) {
			result += QByteArrayLiteral("; ");
		}
		result += value;
	}
	return result;
}

void Api::loginWithPassword(
		const QString &email,
		const QString &password,
		const QString &deviceName) {
	postJson(QStringLiteral("/api/login"), {
		{ QStringLiteral("email"), email },
		{ QStringLiteral("password"), password },
		{ QStringLiteral("device_name"), deviceName }
	});
}

void Api::loginWithAccessKey(
		const QString &accessKey,
		const QString &deviceName) {
	postJson(QStringLiteral("/api/login"), {
		{ QStringLiteral("access_key"), accessKey },
		{ QStringLiteral("device_name"), deviceName }
	});
}

void Api::fetchCurrentUser() {
	getJson(QStringLiteral("/api/me"));
}

void Api::fetchFriends() {
	getJson(QStringLiteral("/api/friends"));
}

void Api::logout() {
	postJson(QStringLiteral("/api/logout"), {});
}

void Api::postJson(const QString &route, const QJsonObject &body) {
	sendJsonRequest(route, QByteArrayLiteral("POST"), body);
}

void Api::getJson(const QString &route) {
	sendJsonRequest(route, QByteArrayLiteral("GET"));
}

void Api::sendJsonRequest(
		const QString &route,
		const QByteArray &method,
		const QJsonObject &body) {
	if (!isConfigured()) {
		emit requestFailed(route, QStringLiteral("Configure the Larptrix server URL first."));
		return;
	}
	const auto url = QUrl(_serverUrl + route);
	QNetworkRequest request(url);
	request.setHeader(
		QNetworkRequest::ContentTypeHeader,
		QStringLiteral("application/json"));
	request.setRawHeader("Accept", "application/json");

	QNetworkReply *reply = nullptr;
	if (method == QByteArrayLiteral("GET")) {
		reply = _network.get(request);
	} else {
		reply = _network.sendCustomRequest(
			request, method, QJsonDocument(body).toJson(QJsonDocument::Compact));
	}
	connect(reply, &QNetworkReply::finished, this, [this, route, reply] {
		handleReply(route, reply);
	});
}

void Api::handleReply(const QString &route, QNetworkReply *reply) {
	const auto status = reply->attribute(
		QNetworkRequest::HttpStatusCodeAttribute).toInt();
	const auto bytes = reply->readAll();
	const auto networkError = reply->error();
	const auto networkMessage = reply->errorString();
	reply->deleteLater();

	QJsonParseError parseError;
	const auto document = QJsonDocument::fromJson(bytes, &parseError);
	if (networkError != QNetworkReply::NoError || status < 200 || status >= 300) {
		QString message = QStringLiteral("Larptrix request failed (HTTP %1).").arg(status);
		if (document.isObject()) {
			const auto object = document.object();
			if (object.value(QStringLiteral("message")).isString()) {
				message = object.value(QStringLiteral("message")).toString();
			} else if (object.value(QStringLiteral("error")).isString()) {
				message = object.value(QStringLiteral("error")).toString();
			}
		} else if (status == 0) {
			message = networkMessage;
		}
		if (route == QStringLiteral("/api/logout")) {
			_authenticated = false;
			emit authenticationChanged(false, {});
		}
		emit requestFailed(route, message);
		return;
	}

	const auto payload = document.isObject()
		? document.object()
		: QJsonObject();
	if (route == QStringLiteral("/api/login")
		|| route == QStringLiteral("/api/me")) {
		// Larptrix returns UserInfo directly for /api/me and successful
		// password/access-key login; registration uses a separate wrapper.
		const auto user = payload.value(QStringLiteral("user")).isObject()
			? payload.value(QStringLiteral("user")).toObject()
			: payload;
		_authenticated = user.contains(QStringLiteral("user_id"));
		emit authenticationChanged(_authenticated, _authenticated ? user : QJsonObject{});
	} else if (route == QStringLiteral("/api/logout")) {
		_authenticated = false;
		emit authenticationChanged(false, {});
	}
	emit requestSucceeded(route, payload);
}

} // namespace Larptrix
