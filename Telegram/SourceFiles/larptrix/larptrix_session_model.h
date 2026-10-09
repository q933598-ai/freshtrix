/*
Freshtrix-side session model for Larptrix wire events.
*/
#pragma once

#include <QtCore/QHash>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QObject>
#include <QtCore/QString>

namespace Larptrix {

// A transport-independent cache of Larptrix users, groups and encrypted
// message envelopes. It intentionally does not decrypt message bodies.
class SessionModel final : public QObject {
	Q_OBJECT

public:
	explicit SessionModel(QObject *parent = nullptr);

	void setCurrentUser(const QJsonObject &user);
	void setFriends(const QJsonArray &friends);
	void applyEvent(const QJsonObject &event);

	[[nodiscard]] QJsonObject currentUser() const;
	[[nodiscard]] QString currentUserId() const;
	[[nodiscard]] QJsonArray friends() const;
	[[nodiscard]] QJsonArray directory() const;
	[[nodiscard]] QJsonArray groups() const;
	[[nodiscard]] QJsonArray historyFor(const QString &peerId) const;

signals:
	void sessionReady();
	void friendsChanged(const QJsonArray &friends);
	void directoryChanged(const QJsonArray &users);
	void groupsChanged(const QJsonArray &groups);
	void chatHistoryChanged(const QString &peerId, const QJsonArray &history);
	void newMessageReceived(const QString &peerId, const QJsonObject &message);
	void presenceChanged(const QString &userId, const QString &status);
	void protocolError(const QString &message);

private:
	QJsonArray mergeUserArray(const QJsonArray &users);
	void cacheUser(const QJsonObject &user);
	QString peerIdForMessage(const QJsonObject &message) const;
	void updateHistoryObject(
		const QString &peerId,
		const QString &messageId,
		const QString &key,
		const QJsonValue &value);

	QJsonObject _currentUser;
	QString _currentUserId;
	QJsonArray _friends;
	QJsonArray _directory;
	QJsonArray _groups;
	QHash<QString, QJsonObject> _usersById;
	QHash<QString, QJsonArray> _historyByPeer;
};

} // namespace Larptrix
