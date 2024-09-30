#include "logindialog.h"
#include "ui_logindialog.h"
#include <QDebug>
#include<QMessageBox>
LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    setWindowModality(Qt::ApplicationModal); // Set the dialog to be application modal
    connect(ui->pushButtonLogin, &QPushButton::clicked, this, &LoginDialog::attemptLogin);
    connect(this, &QDialog::rejected, this, &LoginDialog::onRejected);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}
void LoginDialog::attemptLogin()
{
    QString username = ui->lineEditUsername->text();
    QString password = ui->lineEditPassword->text();
    if (username == "" && password == "")
    {
        emit loginSuccess();
        accept();
    }
    else
    {
        QMessageBox::critical(this, "Invalid Credentials", "Invalid username or password. Please try again.");
        qDebug() << "Invalid credentials";
    }
}
void LoginDialog::onRejected()
{
      // qDebug() << "Rejected signal received, switching to home page";


}
