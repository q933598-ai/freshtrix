/*
Freshtrix Larptrix API bridge.
This file is part of the Freshtrix experimental port; retain upstream license
notices for any upstream-derived code.
*/
#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

namespace Larptrix {

// Small HTTP/session foundation for the Larptrix backend.
// QNetworkAccessManager owns the in-memory cookie jar, so the session cookie
// issued by /api/login is sent automatically on subsequent API requests.
class Api final : public QObject {
	Q_OBJECT

public:
	explicit Api(QObject *parent = nullptr);

	bool setServerUrl(const QString &url, QString *error = nullptr);
	[[nodiscard]] QString serverUrl() const;
	[[nodiscard]] bool isConfigured() const;

	void loginWithPassword(
		const QString &email,
		const QString &password,
		const QString &deviceName = QString());
	void loginWithAccessKey(
		const QString &accessKey,
		const QString &deviceName = QString());
	void fetchCurrentUser();
	void fetchFriends();
	void fetchFriends();
	void logout();

signals:
	// Emitted for successful JSON API responses. The route is the path passed
	// to request(), e.g. "/api/login" or "/api/me".
	void requestSucceeded(const QString &route, const QJsonObject &payload);
	void requestFailed(const QString &route, const QString &message);
	void authenticationChanged(bool authenticated, const QJsonObject &user);

private:
	void postJson(const QString &route, const QJsonObject &body);
	void getJson(const QString &route);
	void sendJsonRequest(
		const QString &route,
		const QByteArray &method,
		const QJsonObject &body = QJsonObject());
	void handleReply(const QString &route, QNetworkReply *reply);

	QNetworkAccessManager _network;
	QString _serverUrl;
	bool _authenticated = false;
};

} // namespace Larptrix
