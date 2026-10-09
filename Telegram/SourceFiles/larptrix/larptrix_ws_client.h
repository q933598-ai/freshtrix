/*
Freshtrix Larptrix WebSocket transport.
*/
#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QString>

class QTimer;
class QWebSocket;

namespace Larptrix {

// Authenticated, JSON-only transport for the Larptrix /ws protocol.
// It intentionally does not send chat bodies: the E2E layer must encrypt
// them before the Send event is exposed to the UI.
class WebSocketClient final : public QObject {
	Q_OBJECT

public:
	explicit WebSocketClient(QObject *parent = nullptr);
	~WebSocketClient() override;

	bool open(const QString &serverUrl, const QByteArray &cookieHeader, QString *error = nullptr);
	void close();
	[[nodiscard]] bool isConnected() const;

	void sendPing();
	void openChat(const QString &peerId);
	void setPresence(const QString &status);

signals:
	void connected();
	void disconnected();
	void connectionFailed(const QString &message);
	void serverEvent(const QJsonObject &event);
	void protocolError(const QString &message);

private:
	void sendEvent(const QJsonObject &event);

	QWebSocket *_socket = nullptr;
	QTimer *_heartbeat = nullptr;
	bool _connecting = false;
};

} // namespace Larptrix
