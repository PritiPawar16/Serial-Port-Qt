#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "logindialog.h"

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QList>
#include<QMessageBox>
#include<QGraphicsView>
QT_BEGIN_NAMESPACE
class QLabel;

namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class SettingsDialog;
class LoginDailog;
class MainWindow : public QMainWindow

{
    Q_OBJECT

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

   QByteArray Cl_gaDatabuffer;
   QByteArray Cl_gaDatatoSend;
   int Counter = 0;
   uint8_t U8_gfFModeSelectionFlag;


   struct ErrorMessage {
       QString message;
       int priority;
   };

private slots:

    void writeData(const QByteArray &data);
    void openSerialPort();
    void closeSerialPort();
    void on_actionClear_triggered();
    void handleError(QSerialPort::SerialPortError error);
    void on_DriverIDSend_button_clicked();
    void on_TrainNoSend_button_clicked();
    void on_ShedNameSend_button_clicked();
    void on_EquiNoSend_button_clicked();
    void on_OdometerSend_button_clicked();
    void on_TrainLoadSend_button_clicked();
    void on_TimeSetSend_button_clicked();
    void on_DateSetSend_button_clicked();
    void on_LocoNoSend_buttton_clicked();
    void on_CtFactorSend_button_clicked();
    void on_PtFactorSend_button_clicked();
    void on_WheelSettingSend_button_clicked();
    void on_MaxSpeedSend_button_clicked();
    void on_ResetCounterSend_button_clicked();
    void on_LocoTypeSend_button_clicked();
    void on_actionAboutApplication_triggered();
    void on_pgnoSend_button_clicked();
    void on_ctinputSend_button_clicked();
    void on_ctoutputSend_button_clicked();
    void on_ptinputSend_button_clicked();
    void on_ptoutputSend_button_clicked();
    QString swapLSBAndMSB(QString hexValue);
    QString addHexNumbers(QString Cl_lvhexval1, QString Cl_lvhexval2);
    QString extractAlphanumeric(const QByteArray &ReceivedData);
    void readDataSerial();
    QChar hexToAscii(const QString &hexString);
    void showSuccessMessage(const QString &message);
    void hideSuccessMessage();
    void updateSpeed(int speed);
    void drawPointer(double angle,QColor& color);
    QString addHexNumbers2(QString hex1, QString hex2,QString hex3,QString hex4);
    void toggleBlink();
    void saveDataToFile(const QString ParameterName,const QByteArray &incomingData);
    void SendSoftwareQuery();
    void on_ToggleButton_clicked();
    void Case16DataToSend();
    void ConnectCase16Signals();
    void flashText();
    void openLoginDialog();
    void loginDialogShow();
    void closeApplication();
private:
    uint16_t calculateCRC16(const QByteArray &data,int length);
    void showStatusMessage(const QString &message);
    void initActionsConnections();
    int ToCreateDataBase();

    Ui::MainWindow *ui=nullptr;
    QSerialPort *mSerial;
    QList<QSerialPortInfo> mSerialPorts;
    QTimer *mSerialScanTimer;
    SettingsDialog *m_settings = nullptr;
    QLabel *m_status = nullptr;    
    QMessageBox *popupMessage;
    QString US_gaKeypadInput;
    QPixmap ledOn, ledOff;
    bool F_gvIsBlinking;
    QTimer *QueryTimer,*flashTimer,*blinkTimer,*messageTimer;
    bool F_gvisMasterMode = true;
    QString US_gaDriverID , US_gaTrainNo , US_gaTrainLoad;
    LoginDialog *loginDialog = nullptr;

};
#endif // MAINWINDOW_H
