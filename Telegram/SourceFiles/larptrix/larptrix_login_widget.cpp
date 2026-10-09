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
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStyle>
#include <QtWidgets/QVBoxLayout>

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
			background: #242933;
			color: #e5e9f0;
			font-family: "Inter", "Noto Sans", sans-serif;
			font-size: 14px;
		}
		QLabel#brand {
			color: #eceff4;
			font-size: 27px;
			font-weight: 700;
		}
		QLabel#subtitle, QLabel#hint {
			color: #aab3c2;
		}
		QLineEdit {
			background: #303744;
			color: #eceff4;
			border: 1px solid #454f60;
			border-radius: 9px;
			padding: 0 13px;
			selection-background-color: #5e81ac;
		}
		QLineEdit:focus { border: 1px solid #81a1c1; }
		QPushButton {
			min-height: 40px;
			border-radius: 9px;
			padding: 0 14px;
			background: #343d4b;
			color: #e5e9f0;
			border: 1px solid #454f60;
		}
		QPushButton:hover { background: #414c5d; }
		QPushButton#primary {
			background: #5e81ac;
			border: 1px solid #5e81ac;
			color: #ffffff;
			font-weight: 600;
		}
		QPushButton#primary:hover { background: #7295c0; }
		QPushButton#mode[active="true"] {
			background: #3b4656;
			border: 1px solid #81a1c1;
		}
		QLabel#status { color: #bf616a; }
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
	root->addStretch(1);

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
			_status->setStyleSheet(QStringLiteral("color: #a3be8c;"));
			_status->setText(QStringLiteral("Connected. Friends: %1").arg(friends.size()));
		});
	connect(&_api, &Api::authenticationChanged, this,
		[this](bool authenticated, const QJsonObject &user) {
			_login->setEnabled(true);
			_login->setText(QStringLiteral("Sign in"));
			if (authenticated) {
				QSettings().setValue(QStringLiteral("Larptrix/serverUrl"), _api.serverUrl());
				_password->clear();
				_accessKey->clear();
				_status->setStyleSheet(QStringLiteral("color: #a3be8c;"));
				_status->setText(QStringLiteral("Signed in. Loading friends…"));
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
