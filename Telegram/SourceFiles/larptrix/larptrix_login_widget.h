/*
Freshtrix Larptrix sign-in screen.
This file is part of the Freshtrix experimental port.
*/
#pragma once

#include "larptrix/larptrix_api.h"

#include <QtWidgets/QWidget>

class QVBoxLayout;
class QListWidget;

class QLineEdit;
class QLabel;
class QPushButton;

namespace Larptrix {

// A self-contained sign-in surface styled to fit FreshGram's centered intro
// flow. The host application can embed it in the intro window or use it as
// the initial page while the Telegram intro is being removed.
class LoginWidget final : public QWidget {
	Q_OBJECT

public:
	explicit LoginWidget(QWidget *parent = nullptr);

signals:
	void authenticated(const QJsonObject &user);

private:
	void setAccessKeyMode(bool enabled);
	void submit();
	void showError(const QString &message);
	void showSession(const QJsonObject &user);
	void showLoginForm();

	Api _api;
	QVBoxLayout *_root = nullptr;
	QListWidget *_friendsList = nullptr;
	QWidget *_sessionPage = nullptr;
	QLineEdit *_server = nullptr;
	QLineEdit *_email = nullptr;
	QLineEdit *_password = nullptr;
	QLineEdit *_accessKey = nullptr;
	QStackedWidget *_credentials = nullptr;
	QPushButton *_passwordMode = nullptr;
	QPushButton *_accessKeyMode = nullptr;
	QPushButton *_login = nullptr;
	QLabel *_status = nullptr;
	bool _usingAccessKey = false;
};

} // namespace Larptrix
