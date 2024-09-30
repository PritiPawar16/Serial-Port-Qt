#include "loginwindow.h"
#include "ui_loginwindow.h"
#include<QMessageBox>
#include <QComboBox>
#include<QLineEdit>

/*********************************************************************************
 *Constructor
*********************************************************************************/

LoginWindow::LoginWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
    this->setWindowIcon(QIcon(":/images/appIcon.ico"));
    this->setWindowTitle("Serial Applicaion");
    QPixmap bkgnd(":/images/TRAIN_ENGINE_INDIA2.jpg");
        bkgnd = bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        QPalette palette;
        palette.setBrush(QPalette::Window, bkgnd);
        this->setPalette(palette);
        ui->lineEdit_password->setEchoMode(QLineEdit::Password);

        ui->comboBox_USERNAME->addItem("User1");
        ui->comboBox_USERNAME->addItem("User2");
}

/*********************************************************************************
 *Destructor
*********************************************************************************/
LoginWindow::~LoginWindow()
{
    delete ui;
}
void LoginWindow::on_LoginButton_clicked()
{
    attemptLogin();

}
/*********************************************************************************
 *void LoginWindow:: attemptLogin()

 *this is an user login function which takes username and password
*********************************************************************************/
void LoginWindow:: attemptLogin()
{

    QString username = ui->comboBox_USERNAME->currentText();
    QString password = ui->lineEdit_password->text();

    if (isValidLogin(username, password))
    {
       QMessageBox::information(this, "Login Successful", "Welcome, " + username + "!");
       mainwindow = new MainWindow(this);
       mainwindow->setParent(nullptr);

       mainwindow->show();
       mainwindow->setFixedHeight(850);
       mainwindow->setFixedWidth(1600);
       mainwindow->showMaximized();
//     mainwindow->showFullScreen();
       hide();

    }
    else
    {
      QMessageBox::warning(this, "Login Failed", "Invalid username or password.");
    }

 }

/*********************************************************************************
 *bool LoginWindow::isValidLogin(const QString &username, const QString &password) const

 *this function validate user entered details of passwords
*********************************************************************************/

bool LoginWindow::isValidLogin(const QString &username, const QString &password) const
{
       if (username == "User1" && password == "")
       {
         return true;
       }
       else if (username == "User2" && password == "")
       {
         return true;
       }

       return false;
}

void LoginWindow::on_checkBox_password_stateChanged(int state)
{
   ui->lineEdit_password->setEchoMode(state == Qt::Checked ? QLineEdit::Normal : QLineEdit::Password);
}

