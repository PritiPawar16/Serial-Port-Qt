#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H
#include<QStackedWidget>
#include <QMainWindow>
#include "mainwindow.h"
#include<QMap>
QT_BEGIN_NAMESPACE
namespace Ui { class LoginWindow; }
QT_END_NAMESPACE
class MainWindow;
class LoginWindow : public QMainWindow
{
    Q_OBJECT

public:
    LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

private slots:
    void on_LoginButton_clicked();

    void on_checkBox_password_stateChanged(int state);

    void attemptLogin();
    bool isValidLogin(const QString &username, const QString &password) const;
private:
    Ui::LoginWindow *ui;
    MainWindow *mainwindow;
    QStackedWidget *stackedWidget;
    QMap<QString, QString> userCredentials;
};
#endif // LOGINWINDOW_H
