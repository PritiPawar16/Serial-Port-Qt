#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class LoginDialog; }
QT_END_NAMESPACE

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

signals:
    void loginSuccess();
    void goToHomePage();
private slots:
    void attemptLogin();  
    void onRejected();
private:
    Ui::LoginDialog *ui;
};

#endif // LOGINDIALOG_H
