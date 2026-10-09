/*
Freshtrix Larptrix sign-in screen.
*/
#include "larptrix/larptrix_login_widget.h"

#include <QtCore/QJsonObject>
#include <QtCore/QSettings>
#include <QtCore/QJsonArray>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStyle>
#include <QtWidgets/QVBoxLayout>

#if defined(LARPTRIX_HAS_QT_WEBSOCKETS) && LARPTRIX_HAS_QT_WEBSOCKETS
#include "larptrix/larptrix_ws_client.h"
#endif

namespace Larptrix {
namespace {

QLineEdit *makeField(
	QWidget *parent,
	const QString &placeholder,
	QLineEdit::EchoMode echo = QLineEdit::Normal) {
	auto field = new QLineEdit(parent);
	field->setPlaceholderText(placeholder);
	field->setMinimumHeight(44);
	field->setEchoMode(echo);
	field->setClearButtonEnabled(echo == QLineEdit::Normal);
	return field;
}

} // namespace

LoginWidget::LoginWidget(QWidget *parent)
: QWidget(parent) {
	setObjectName(QStringLiteral("larptrixLogin"));
	setMinimumSize(420, 520);
	setStyleSheet(QStringLiteral(R"(
		QWidget#larptrixLogin {
			background: #1b1c17;
			color: #e3e3d9;
			font-family: "Inter", "Noto Sans", sans-serif;
			font-size: 14px;
		}
		QLabel#brand {
			color: #e3e3d9;
			font-size: 27px;
			font-weight: 700;
		}
		QLabel#subtitle, QLabel#hint {
			color: #c6c9b8;
		}
		QLineEdit {
			background: #272823;
			color: #e3e3d9;
			border: 1px solid #414238;
			border-radius: 9px;
			padding: 0 13px;
			selection-background-color: #bacf7a;
		}
		QLineEdit:focus { border: 1px solid #bacf7a; }
		QPushButton {
			min-height: 40px;
			border-radius: 9px;
			padding: 0 14px;
			background: #272823;
			color: #e3e3d9;
			border: 1px solid #414238;
		}
		QPushButton:hover { background: #34352f; }
		QPushButton#primary {
			background: #bacf7a;
			border: 1px solid #bacf7a;
			color: #ffffff;
			font-weight: 600;
		}
		QPushButton#primary:hover { background: #c6d98c; }
		QPushButton#mode[active="true"] {
			background: #35382c;
			border: 1px solid #bacf7a;
		}
		QLabel#status { color: #e06c75; }
	QLabel#liveStatus { color: #c6c9b8; font-size: 12px; }
	)"));

	auto root = new QVBoxLayout(this);
	root->setContentsMargins(36, 34, 36, 30);
	root->setSpacing(12);

	auto brand = new QLabel(QStringLiteral("Freshtrix"), this);
	brand->setObjectName(QStringLiteral("brand"));
	brand->setAlignment(Qt::AlignHCenter);
	root->addWidget(brand);

	auto subtitle = new QLabel(
		QStringLiteral("Connect to your Larptrix server"), this);
	subtitle->setObjectName(QStringLiteral("subtitle"));
	subtitle->setAlignment(Qt::AlignHCenter);
	root->addWidget(subtitle);
	root->addSpacing(14);

	auto serverLabel = new QLabel(QStringLiteral("Server address"), this);
	root->addWidget(serverLabel);
	_server = makeField(this, QStringLiteral("https://chat.example.com"));
	auto settings = QSettings();
	_server->setText(settings.value(QStringLiteral("Larptrix/serverUrl")).toString());
	root->addWidget(_server);

	auto modeRow = new QHBoxLayout;
	modeRow->setSpacing(8);
	_passwordMode = new QPushButton(QStringLiteral("Email and password"), this);
	_passwordMode->setObjectName(QStringLiteral("mode"));
	_accessKeyMode = new QPushButton(QStringLiteral("Access key"), this);
	_accessKeyMode->setObjectName(QStringLiteral("mode"));
	modeRow->addWidget(_passwordMode);
	modeRow->addWidget(_accessKeyMode);
	root->addLayout(modeRow);

	_credentials = new QStackedWidget(this);
	auto passwordPage = new QWidget(_credentials);
	auto passwordLayout = new QVBoxLayout(passwordPage);
	passwordLayout->setContentsMargins(0, 0, 0, 0);
	passwordLayout->setSpacing(10);
	_email = makeField(passwordPage, QStringLiteral("Email"));
	_email->setClearButtonEnabled(true);
	_password = makeField(
		passwordPage,
		QStringLiteral("Password"),
		QLineEdit::Password);
	passwordLayout->addWidget(_email);
	passwordLayout->addWidget(_password);
	_credentials->addWidget(passwordPage);

	auto keyPage = new QWidget(_credentials);
	auto keyLayout = new QVBoxLayout(keyPage);
	keyLayout->setContentsMargins(0, 0, 0, 0);
	_accessKey = makeField(
		keyPage,
		QStringLiteral("64-character access key"),
		QLineEdit::Password);
	keyLayout->addWidget(_accessKey);
	auto keyHint = new QLabel(
		QStringLiteral("Your access key is sent only to the server above."),
		keyPage);
	keyHint->setObjectName(QStringLiteral("hint"));
	keyHint->setWordWrap(true);
	keyLayout->addWidget(keyHint);
	_credentials->addWidget(keyPage);
	root->addWidget(_credentials);

	_login = new QPushButton(QStringLiteral("Sign in"), this);
	_login->setObjectName(QStringLiteral("primary"));
	_login->setMinimumHeight(46);
	root->addWidget(_login);

	_status = new QLabel(this);
	_status->setObjectName(QStringLiteral("status"));
	_status->setWordWrap(true);
	_status->setAlignment(Qt::AlignHCenter);
	root->addWidget(_status);

	_friendsList = new QListWidget(this);
	_friendsList->setObjectName(QStringLiteral("friendsList"));
	_friendsList->setMinimumHeight(120);
	_friendsList->setMaximumHeight(220);
	_friendsList->setStyleSheet(QStringLiteral(
		"QListWidget { background: #272823; color: #e3e3d9; border: 1px solid #414238; border-radius: 9px; padding: 5px; }"
		"QListWidget::item { padding: 8px; border-radius: 5px; }"
		"QListWidget::item:selected { background: #35382c; }"));
	_friendsList->hide();
	root->addWidget(_friendsList);

	connect(&_model, &SessionModel::friendsChanged, this,
		[this](const QJsonArray &friends) {
			_friendsList->clear();
			for (const auto &value : friends) {
				const auto friendObject = value.toObject();
				const auto name = friendObject.value(QStringLiteral("display_name")).toString();
				const auto username = friendObject.value(QStringLiteral("username")).toString();
				const auto id = friendObject.value(QStringLiteral("user_id")).toString();
				const auto label = !name.isEmpty() ? name
					: (!username.isEmpty() ? username : id);
				auto item = new QListWidgetItem(label, _friendsList);
				item->setData(Qt::UserRole, id);
				auto tooltip = username.isEmpty()
					? QString()
					: QStringLiteral("@%1").arg(username);
				const auto presence = friendObject.value(QStringLiteral("presence")).toString();
				if (!presence.isEmpty()) {
					if (!tooltip.isEmpty()) tooltip += QStringLiteral(" · ");
					tooltip += presence;
				}
				item->setToolTip(tooltip);
			}
			if (!friends.isEmpty()) _friendsList->show();
		});

	_liveStatus = new QLabel(
		QStringLiteral("Live updates connect after sign-in."), this);
	_liveStatus->setObjectName(QStringLiteral("liveStatus"));
	_liveStatus->setWordWrap(true);
	_liveStatus->setAlignment(Qt::AlignHCenter);
	root->addWidget(_liveStatus);

	root->addStretch(1);

#if defined(LARPTRIX_HAS_QT_WEBSOCKETS) && LARPTRIX_HAS_QT_WEBSOCKETS
	_webSocket = new WebSocketClient(this);
#endif

	auto footer = new QLabel(
		QStringLiteral("A server you choose. Your Larptrix account."),
		this);
	footer->setObjectName(QStringLiteral("hint"));
	footer->setAlignment(Qt::AlignHCenter);
	footer->setWordWrap(true);
	root->addWidget(footer);

	connect(_passwordMode, &QPushButton::clicked, this, [this] {
		setAccessKeyMode(false);
	});
	connect(_accessKeyMode, &QPushButton::clicked, this, [this] {
		setAccessKeyMode(true);
	});
	connect(_login, &QPushButton::clicked, this, [this] {
		submit();
	});
	connect(_password, &QLineEdit::returnPressed, this, [this] {
		submit();
	});
	connect(_accessKey, &QLineEdit::returnPressed, this, [this] {
		submit();
	});
#if defined(LARPTRIX_HAS_QT_WEBSOCKETS) && LARPTRIX_HAS_QT_WEBSOCKETS
	connect(_webSocket, &WebSocketClient::connected, this, [this] {
		_liveStatus->setProperty("liveConnected", true);
		_liveStatus->setStyleSheet(QStringLiteral("color: #bacf7a;"));
		_liveStatus->setText(QStringLiteral("Live connection established."));
	});
	connect(_webSocket, &WebSocketClient::connectionFailed, this,
		[this](const QString &message) {
			_liveStatus->setProperty("liveConnected", false);
			_liveStatus->setStyleSheet(QStringLiteral("color: #e06c75;"));
			_liveStatus->setText(QStringLiteral("Live connection failed: %1").arg(message));
		});
	connect(_webSocket, &WebSocketClient::disconnected, this, [this] {
		if (!_liveStatus->property("liveConnected").toBool()) return;
		_liveStatus->setProperty("liveConnected", false);
		_liveStatus->setStyleSheet(QString());
		_liveStatus->setText(QStringLiteral("Live connection disconnected."));
	});
	connect(_webSocket, &WebSocketClient::protocolError, this,
		[this](const QString &message) {
			_liveStatus->setStyleSheet(QStringLiteral("color: #e06c75;"));
			_liveStatus->setText(message);
		});
	connect(_webSocket, &WebSocketClient::serverEvent,
		&_model, &SessionModel::applyEvent);
	connect(&_model, &SessionModel::sessionReady, this, [this] {
		_liveStatus->setStyleSheet(QStringLiteral("color: #bacf7a;"));
		_liveStatus->setText(
			QStringLiteral("Live updates connected · %1 directory entries")
				.arg(_model.directory().size()));
	});
	connect(&_model, &SessionModel::chatHistoryChanged, this,
		[this](const QString &, const QJsonArray &history) {
			_liveStatus->setStyleSheet(QString());
			_liveStatus->setText(
				QStringLiteral("History loaded · %1 encrypted message(s). Decryption is not wired yet.")
					.arg(history.size()));
		});
	connect(&_model, &SessionModel::newMessageReceived, this,
		[this](const QString &, const QJsonObject &) {
			_liveStatus->setText(
				QStringLiteral("A new encrypted message event was received."));
		});
	connect(&_model, &SessionModel::protocolError, this,
		[this](const QString &message) {
			_liveStatus->setStyleSheet(QStringLiteral("color: #e06c75;"));
			_liveStatus->setText(message);
		});
	connect(_friendsList, &QListWidget::itemDoubleClicked, this,
		[this](QListWidgetItem *item) {
			if (!item) return;
			if (!_webSocket->isConnected()) {
				_liveStatus->setText(QStringLiteral("Live connection is not ready."));
				return;
			}
			const auto peerId = item->data(Qt::UserRole).toString();
			if (peerId.isEmpty()) return;
			_webSocket->openChat(peerId);
			_liveStatus->setStyleSheet(QString());
			_liveStatus->setText(QStringLiteral("Requesting encrypted chat history…"));
		});
#else
	// The missing-module explanation is shown after sign-in below.
#endif

	connect(&_api, &Api::requestFailed, this,
		[this](const QString &, const QString &message) {
			_login->setEnabled(true);
			_login->setText(QStringLiteral("Sign in"));
			showError(message);
		});
	connect(&_api, &Api::requestSucceeded, this,
		[this](const QString &route, const QJsonObject &payload) {
			if (route != QStringLiteral("/api/friends")) return;
			const auto friends = payload.value(QStringLiteral("friends")).toArray();
			_model.setFriends(friends);
			_friendsList->show();
			_status->setStyleSheet(QStringLiteral("color: #bacf7a;"));
			_status->setText(QStringLiteral("Connected · %1 friend(s)").arg(friends.size()));
		});
	connect(&_api, &Api::authenticationChanged, this,
		[this](bool authenticated, const QJsonObject &user) {
			_login->setEnabled(true);
			_login->setText(QStringLiteral("Sign in"));
			if (authenticated) {
				_model.setCurrentUser(user);
				QSettings().setValue(QStringLiteral("Larptrix/serverUrl"), _api.serverUrl());
				_password->clear();
				_accessKey->clear();
				_status->setStyleSheet(QStringLiteral("color: #bacf7a;"));
				_status->setText(QStringLiteral("Signed in. Loading friends…"));
#if defined(LARPTRIX_HAS_QT_WEBSOCKETS) && LARPTRIX_HAS_QT_WEBSOCKETS
				QString socketError;
				if (_webSocket->open(
						_api.serverUrl(), _api.sessionCookieHeader(), &socketError)) {
					_liveStatus->setStyleSheet(QString());
					_liveStatus->setText(QStringLiteral("Connecting to live updates…"));
				} else {
					_liveStatus->setStyleSheet(QStringLiteral("color: #e06c75;"));
					_liveStatus->setText(socketError);
				}
#else
				_liveStatus->setStyleSheet(QStringLiteral("color: #e06c75;"));
				_liveStatus->setText(QStringLiteral(
					"Live updates unavailable: this build has no Qt WebSockets module."));
#endif
				_api.fetchFriends();
				emit authenticated(user);
			} else {
				showError(QStringLiteral(
					"The server did not return a valid Larptrix session."));
			}
		});

	setAccessKeyMode(false);
}

void LoginWidget::setAccessKeyMode(bool enabled) {
	_usingAccessKey = enabled;
	_credentials->setCurrentIndex(enabled ? 1 : 0);
	_passwordMode->setProperty("active", !enabled);
	_accessKeyMode->setProperty("active", enabled);
	_passwordMode->style()->unpolish(_passwordMode);
	_passwordMode->style()->polish(_passwordMode);
	_accessKeyMode->style()->unpolish(_accessKeyMode);
	_accessKeyMode->style()->polish(_accessKeyMode);
	_status->clear();
}

void LoginWidget::submit() {
	QString error;
	if (!_api.setServerUrl(_server->text(), &error)) {
		showError(error);
		_server->setFocus();
		return;
	}

	QSettings().setValue(QStringLiteral("Larptrix/serverUrl"), _api.serverUrl());
	_login->setEnabled(false);
	_login->setText(QStringLiteral("Connecting…"));
	_status->setStyleSheet(QString());
	_status->setText(QStringLiteral("Connecting to %1…").arg(_api.serverUrl()));

	if (_usingAccessKey) {
		const auto key = _accessKey->text().trimmed();
		if (key.isEmpty()) {
			_login->setEnabled(true);
			_login->setText(QStringLiteral("Sign in"));
			showError(QStringLiteral("Enter your access key."));
			_accessKey->setFocus();
			return;
		}
		_api.loginWithAccessKey(key, QStringLiteral("Freshtrix Desktop"));
	} else {
		if (_email->text().trimmed().isEmpty()
			|| _password->text().isEmpty()) {
			_login->setEnabled(true);
			_login->setText(QStringLiteral("Sign in"));
			showError(QStringLiteral("Enter your email and password."));
			return;
		}
		_api.loginWithPassword(
			_email->text().trimmed(),
			_password->text(),
			QStringLiteral("Freshtrix Desktop"));
	}
}

void LoginWidget::showError(const QString &message) {
	_status->setStyleSheet(QString());
	_status->setText(message);
}

} // namespace Larptrix
