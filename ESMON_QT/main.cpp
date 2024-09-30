#include "mainwindow.h"
#include <QApplication>
#include"logindialog.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
        MainWindow mainWindow;
        mainWindow.showMaximized();
        mainWindow.show();
        return a.exec();
}
