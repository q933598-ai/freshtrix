/*
Freshtrix Larptrix WebSocket transport.
*/
#include "larptrix/larptrix_ws_client.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonParseError>
#include <QtCore/QTimer>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QUrl>
#include <QtWebSockets/QWebSocket>

namespace Larptrix {

WebSocketClient::WebSocketClient(QObject *parent)
: QObject(parent)
, _socket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
, _heartbeat(new QTimer(this)) {
	_heartbeat->setInterval(25000);
	connect(_heartbeat, &QTimer::timeout, this, [this] {
		sendPing();
	});
	connect(_socket, &QWebSocket::connected, this, [this] {
		_connecting = false;
		_heartbeat->start();
		emit connected();
	});
	connect(_socket, &QWebSocket::disconnected, this, [this] {
		_heartbeat->stop();
		if (_connecting) {
			_connecting = false;
			const auto reason = _socket->errorString();
			emit connectionFailed(reason.isEmpty()
				? QStringLiteral("Could not establish the Larptrix live connection.")
				: reason);
		}
		emit disconnected();
	});
	connect(_socket, &QWebSocket::textMessageReceived, this,
		[this](const QString &text) {
			QJsonParseError parseError;
			const auto document = QJsonDocument::fromJson(text.toUtf8(), &parseError);
			if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
				emit protocolError(QStringLiteral("Larptrix sent an invalid JSON WebSocket event."));
				return;
			}
			emit serverEvent(document.object());
		});
}

WebSocketClient::~WebSocketClient() = default;

bool WebSocketClient::open(
		const QString &serverUrl,
		const QByteArray &cookieHeader,
		QString *error) {
	QUrl url(serverUrl);
	const auto scheme = url.scheme().toLower();
	if (!url.isValid()
		|| url.host().isEmpty()
		|| (scheme != QStringLiteral("https") && scheme != QStringLiteral("http"))) {
		if (error) *error = QStringLiteral("Configure a valid HTTP(S) Larptrix server first.");
		return false;
	}
	if (cookieHeader.isEmpty()) {
		if (error) *error = QStringLiteral("The Larptrix session cookie is missing. Sign in again.");
		return false;
	}

	// Preserve a possible server path prefix, but always target the /ws route.
	auto path = url.path();
	while (path.endsWith('/')) path.chop(1);
	url.setPath(path + QStringLiteral("/ws"));
	url.setScheme(scheme == QStringLiteral("https")
		? QStringLiteral("wss")
		: QStringLiteral("ws"));
	url.setQuery(QString());
	url.setFragment(QString());

	const auto origin = scheme + QStringLiteral("://") + QUrl(serverUrl).authority();
	QNetworkRequest request(url);
	request.setRawHeader("Origin", origin.toUtf8());
	request.setRawHeader("Cookie", cookieHeader);

	_heartbeat->stop();
	_connecting = false;
	_socket->abort();
	_connecting = true;
	_socket->open(request);
	return true;
}

void WebSocketClient::close() {
	_connecting = false;
	_heartbeat->stop();
	if (_socket->state() != QAbstractSocket::UnconnectedState) {
		_socket->close();
	}
}

bool WebSocketClient::isConnected() const {
	return _socket->state() == QAbstractSocket::ConnectedState;
}

void WebSocketClient::sendPing() {
	if (isConnected()) {
		sendEvent({ { QStringLiteral("type"), QStringLiteral("ping") } });
	}
}

void WebSocketClient::openChat(const QString &peerId) {
	if (peerId.isEmpty()) return;
	sendEvent({
		{ QStringLiteral("type"), QStringLiteral("open") },
		{ QStringLiteral("peer_id"), peerId }
	});
}

void WebSocketClient::setPresence(const QString &status) {
	if (status != QStringLiteral("online")
		&& status != QStringLiteral("dnd")
		&& status != QStringLiteral("invisible")) {
		return;
	}
	sendEvent({
		{ QStringLiteral("type"), QStringLiteral("set_presence") },
		{ QStringLiteral("status"), status }
	});
}

void WebSocketClient::sendEvent(const QJsonObject &event) {
	if (!isConnected()) return;
	_socket->sendTextMessage(QString::fromUtf8(
		QJsonDocument(event).toJson(QJsonDocument::Compact)));
}

} // namespace Larptrix
