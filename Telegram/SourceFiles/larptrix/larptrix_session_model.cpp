/*
Freshtrix-side session model for Larptrix wire events.
*/
#include "larptrix/larptrix_session_model.h"

#include <QtCore/QJsonValue>

namespace Larptrix {
namespace {

QString userId(const QJsonObject &user) {
	return user.value(QStringLiteral("user_id")).toString();
}

QJsonObject mergeObjects(const QJsonObject &base, const QJsonObject &changes) {
	auto result = base;
	for (auto i = changes.constBegin(); i != changes.constEnd(); ++i) {
		result.insert(i.key(), i.value());
	}
	return result;
}

} // namespace

SessionModel::SessionModel(QObject *parent)
: QObject(parent) {
}

void SessionModel::setCurrentUser(const QJsonObject &user) {
	_currentUser = user;
	_currentUserId = userId(user);
	cacheUser(user);
}

void SessionModel::setFriends(const QJsonArray &friends) {
	QJsonArray merged;
	for (const auto &value : friends) {
		const auto friendUser = value.toObject();
		const auto id = userId(friendUser);
		if (id.isEmpty()) {
			merged.append(friendUser);
			continue;
		}
		const auto previous = _usersById.value(id);
		const auto combined = mergeObjects(previous, friendUser);
		_usersById.insert(id, combined);
		merged.append(combined);
	}
	_friends = merged;
	emit friendsChanged(_friends);
}

void SessionModel::applyEvent(const QJsonObject &event) {
	const auto type = event.value(QStringLiteral("type")).toString();
	if (type == QStringLiteral("welcome")) {
		setCurrentUser(event.value(QStringLiteral("user")).toObject());
		_directory = mergeUserArray(event.value(QStringLiteral("users")).toArray());
		emit directoryChanged(_directory);
		emit sessionReady();
	} else if (type == QStringLiteral("directory")) {
		_directory = mergeUserArray(event.value(QStringLiteral("users")).toArray());
		emit directoryChanged(_directory);
	} else if (type == QStringLiteral("groups")) {
		_groups = event.value(QStringLiteral("groups")).toArray();
		emit groupsChanged(_groups);
	} else if (type == QStringLiteral("chat")) {
		const auto peer = event.value(QStringLiteral("peer")).toObject();
		auto peerId = userId(peer);
		if (peerId.isEmpty()) {
			peerId = peer.value(QStringLiteral("group_id")).toString();
		}
		if (peerId.isEmpty()) {
			emit protocolError(QStringLiteral("Larptrix returned chat history without a peer ID."));
			return;
		}
		cacheUser(peer);
		const auto history = event.value(QStringLiteral("history")).toArray();
		_historyByPeer.insert(peerId, history);
		emit chatHistoryChanged(peerId, history);
	} else if (type == QStringLiteral("message")) {
		const auto message = event.value(QStringLiteral("message")).toObject();
		const auto peerId = peerIdForMessage(message);
		if (peerId.isEmpty()) {
			emit protocolError(QStringLiteral("Cannot determine the peer for a Larptrix message."));
			return;
		}
		const auto id = message.value(QStringLiteral("id")).toString();
		auto history = _historyByPeer.value(peerId);
		bool exists = false;
		if (!id.isEmpty()) {
			for (const auto &entry : history) {
				if (entry.toObject().value(QStringLiteral("id")).toString() == id) {
					exists = true;
					break;
				}
			}
		}
		if (!exists) {
			history.append(message);
			_historyByPeer.insert(peerId, history);
			emit chatHistoryChanged(peerId, history);
			emit newMessageReceived(peerId, message);
		}
	} else if (type == QStringLiteral("message_deleted")) {
		const auto peerId = event.value(QStringLiteral("peer_id")).toString();
		const auto messageId = event.value(QStringLiteral("message_id")).toString();
		auto history = _historyByPeer.value(peerId);
		QJsonArray filtered;
		for (const auto &entry : history) {
			if (entry.toObject().value(QStringLiteral("id")).toString() != messageId) {
				filtered.append(entry);
			}
		}
		_historyByPeer.insert(peerId, filtered);
		emit chatHistoryChanged(peerId, filtered);
	} else if (type == QStringLiteral("message_reaction")) {
		updateHistoryObject(
			event.value(QStringLiteral("peer_id")).toString(),
			event.value(QStringLiteral("message_id")).toString(),
			QStringLiteral("reactions"),
			event.value(QStringLiteral("reactions")));
	} else if (type == QStringLiteral("message_view_update")) {
		updateHistoryObject(
			event.value(QStringLiteral("peer_id")).toString(),
			event.value(QStringLiteral("message_id")).toString(),
			QStringLiteral("view_count"),
			event.value(QStringLiteral("view_count")));
	} else if (type == QStringLiteral("presence")) {
		const auto id = event.value(QStringLiteral("user_id")).toString();
		const auto status = event.value(QStringLiteral("status")).toString();
		if (!id.isEmpty()) {
			auto user = _usersById.value(id);
			user.insert(QStringLiteral("user_id"), id);
			user.insert(QStringLiteral("presence"), status);
			_usersById.insert(id, user);
			emit presenceChanged(id, status);
		}
	} else if (type == QStringLiteral("error")) {
		const auto message = event.value(QStringLiteral("message")).toString();
		emit protocolError(message.isEmpty()
			? QStringLiteral("Larptrix reported a WebSocket protocol error.")
			: message);
	}
}

QJsonObject SessionModel::currentUser() const {
	return _currentUser;
}

QString SessionModel::currentUserId() const {
	return _currentUserId;
}

QJsonArray SessionModel::friends() const {
	return _friends;
}

QJsonArray SessionModel::directory() const {
	return _directory;
}

QJsonArray SessionModel::groups() const {
	return _groups;
}

QJsonArray SessionModel::historyFor(const QString &peerId) const {
	return _historyByPeer.value(peerId);
}

QJsonArray SessionModel::mergeUserArray(const QJsonArray &users) {
	QJsonArray result;
	for (const auto &value : users) {
		const auto incoming = value.toObject();
		const auto id = userId(incoming);
		if (id.isEmpty()) {
			result.append(incoming);
			continue;
		}
		const auto combined = mergeObjects(_usersById.value(id), incoming);
		_usersById.insert(id, combined);
		result.append(combined);
	}
	return result;
}

void SessionModel::cacheUser(const QJsonObject &user) {
	const auto id = userId(user);
	if (id.isEmpty()) return;
	_usersById.insert(id, mergeObjects(_usersById.value(id), user));
}

QString SessionModel::peerIdForMessage(const QJsonObject &message) const {
	const auto sender = message.value(QStringLiteral("sender_id")).toString();
	const auto recipient = message.value(QStringLiteral("recipient_id")).toString();
	for (const auto &value : _groups) {
		if (value.toObject().value(QStringLiteral("group_id")).toString() == recipient) {
			return recipient;
		}
	}
	if (sender == _currentUserId) return recipient;
	if (recipient == _currentUserId) return sender;
	return !sender.isEmpty() ? sender : recipient;
}

void SessionModel::updateHistoryObject(
		const QString &peerId,
		const QString &messageId,
		const QString &key,
		const QJsonValue &value) {
	auto history = _historyByPeer.value(peerId);
	bool changed = false;
	for (int i = 0; i < history.size(); ++i) {
		auto message = history.at(i).toObject();
		if (message.value(QStringLiteral("id")).toString() == messageId) {
			message.insert(key, value);
			history.replace(i, message);
			changed = true;
			break;
		}
	}
	if (changed) {
		_historyByPeer.insert(peerId, history);
		emit chatHistoryChanged(peerId, history);
	}
}

} // namespace Larptrix
