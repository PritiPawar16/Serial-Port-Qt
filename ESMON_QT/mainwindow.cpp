#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"
#include <QDebug>
#include <QScrollBar>
#include<QFile>
#include<QMessageBox>
#include <QLabel>
#include<QPixmap>
#include<QValidator>
#include <QSqlQuery>
#include <QtSql/QSqlDatabase>
#include<QDateTime>
#include<QPainter>
#include <QtMath>
#include<QColor>
#include<QStandardPaths>
#include<QDir>

/**************************************************************************
 * constructor
***************************************************************************/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
      m_settings(new SettingsDialog),
      m_status(new QLabel)
{

  ui->setupUi(this);
  this->setWindowTitle("ESMON Serial Applicaion");

  // QPixmap bkgnd(":/images/TRAIN_ENGINE_INDIA2.jpg");
  // bkgnd = bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

  //     QPalette palette;
  //     palette.setBrush(QPalette::Window, bkgnd);
  //     this->setPalette(palette);

  loginDialogShow();
  mSerial = new QSerialPort(this);
  ui->actionConnect->setEnabled(true);
  ui->actionDisconnect->setEnabled(false);
  ui->actionLogOut->setEnabled(true);
  ui->actionPort_settings->setEnabled(true);
  ui->statusbar->addWidget(m_status);
  messageTimer = new QTimer(this);
  blinkTimer = new QTimer(this);
  QueryTimer = new QTimer(this);
  flashTimer = new QTimer(this);
  ui->speed_offLED->setFixedSize(40, 40);
  ledOn.load(":/images/redon.png");
  ledOff.load(":/images/redoff.png");
  ui->speed_offLED->setPixmap(ledOff);

   ConnectCase16Signals();
   initActionsConnections();
   ToCreateDataBase();

  ui->tabWidget->setCurrentIndex(0);
  mSerialScanTimer = new QTimer(this);
  mSerialScanTimer->setInterval(1000);
  mSerialScanTimer->start();

  connect(mSerial, &QSerialPort::readyRead, this, &MainWindow::readDataSerial);
  connect(mSerial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
  connect(messageTimer, &QTimer::timeout, this, &MainWindow::hideSuccessMessage);
  connect(blinkTimer, &QTimer::timeout, this, &MainWindow::toggleBlink);
  connect(flashTimer, &QTimer::timeout, this, &MainWindow::flashText);
  connect(ui->actionLogOut, &QAction::triggered, this, &MainWindow::openLoginDialog);
  connect(loginDialog, &LoginDialog::rejected, this, &MainWindow::closeApplication);


      popupMessage = new QMessageBox(this);
      popupMessage->setIcon(QMessageBox::Information);

  // ui->toolBar->setStyleSheet("QToolButton { color: white; }");
  QPixmap speedometerPixmap(":/images/Backup_of_new240.png");
  ui->image_speed3->setPixmap(speedometerPixmap);

      QColor color;
      color = Qt::green;
      drawPointer(-149,color);

}

/*******************************************************************************
 * Destructor
********************************************************************************/

MainWindow::~MainWindow()

{
    delete m_settings;
    delete ui;
}


/*********************************************************************************
 *SaveDataToFile
 *it takes two arguments one is parameter name which is constant string and
 *other is const array of received data over serial port and create txt file on
 *desktop in append mode for writting data continously
**********************************************************************************/

void MainWindow::saveDataToFile(const QString ParameterName, const QByteArray &incomingData)
{
 QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    /* Check if the directory exists, and create it if not*/

    QDir desktopDir(desktopPath);

     if (!desktopDir.exists())
      {
       if (!desktopDir.mkpath(desktopPath))
         {
           qDebug() << "Error creating desktop directory.";
         }
    }
       QString filePath = desktopPath + "/ESMON_QT_DATA.txt";
       QFile file(filePath);

            /*open file in append mode*/

    if (file.open(QIODevice::Append | QIODevice::Text))
        {
            QTextStream out(&file);
            out<< ParameterName << incomingData<<"\t";
            file.close();

            qDebug() << "Data saved to" << filePath;
        }
        else
        {
         qDebug() << "Error opening the file:" << file.errorString();
        }
}
/*********************************************************************************
 *ToCreateDataBase
 *this function create an data base file to store received data by using timestamp
*********************************************************************************/

int MainWindow::ToCreateDataBase()
{
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

  db.setDatabaseName("ESMON_database.db"); /*Set the database file name*/

  if (!db.open())
  {
    qDebug() << "Error: Unable to open database";
    return 1;
  }
    QSqlQuery query;
    if (!query.exec("CREATE TABLE IF NOT EXISTS serial_data_table (id INTEGER PRIMARY KEY, "
                    "timestamp DATETIME, data TEXT)"))
    {
     qDebug() << "Error creating table:" << query.lastQuery();
    }
}

/***********************************************************************************
 *initActionsConnections
 *this function establish a connection between ui action class to  main functions
 *by connecting signal and slot
***********************************************************************************/


void MainWindow::initActionsConnections()
{
  connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
  connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
  connect(ui->actionPort_settings, &QAction::triggered, m_settings, &SettingsDialog::show);
  connect(ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
}

/*********************************************************************************
 *openSerialPort
 *this function defines a connection between two serial port by setting serial
 *parameter like port name ,baud rate,parity,stopbits , flow control
**********************************************************************************/

void MainWindow::openSerialPort()
{
    /* create a structure variable of struct settings */

  const SettingsDialog::Settings parameter = m_settings->settings();
  mSerial->setPortName(parameter.name);
  mSerial->setBaudRate(parameter.baudRate);
  mSerial->setDataBits(parameter.dataBits);
  mSerial->setParity(parameter.parity);
  mSerial->setStopBits(parameter.stopBits);
  mSerial->setFlowControl(parameter.flowControl);

  if (mSerial->open(QIODevice::ReadWrite))
   {

    ui->actionConnect->setEnabled(false);
    ui->actionDisconnect->setEnabled(true);
    ui->actionPort_settings->setEnabled(false);
    showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
              .arg(parameter.name)
              .arg(parameter.stringBaudRate)
              .arg(parameter.stringDataBits)
              .arg(parameter.stringParity)
              .arg(parameter.stringStopBits)
              .arg(parameter.stringFlowControl));

    ui->actionConnect->setToolTip(tr("Connected to %1 : %2, %3, %4, %5, %6")
              .arg(parameter.name)
              .arg(parameter.stringBaudRate)
              .arg(parameter.stringDataBits)
              .arg(parameter.stringParity)
              .arg(parameter.stringStopBits)
              .arg(parameter.stringFlowControl));

    showSuccessMessage("  Connected  ");

     /*send queries to master per second */
    if(F_gvisMasterMode)
    {
        QueryTimer->start(1000);
        // connect(QueryTimer, &QTimer::timeout, this, &MainWindow::SendSoftwareQuery);
    }
    else
    {
        ;
    }
  }
   else
   {
     QMessageBox::critical(this, tr("Error"), mSerial->errorString());
     showStatusMessage(tr("Open error"));
   }
}

/*********************************************************************************
 *closeSerialPort
 *this function is use to close serial port connection by triggering actions
*********************************************************************************/

void MainWindow::closeSerialPort()
{
 if (mSerial->isOpen())
 {
     mSerial->close();

   ui->actionConnect->setEnabled(true);
   ui->actionDisconnect->setEnabled(false);
   ui->actionPort_settings->setEnabled(true);
   showStatusMessage(tr("Port Disconnected"));
   showSuccessMessage(" Port Disconnected  ");
   QueryTimer->stop();
 }
    else
    {
     qDebug()<<"already disconnected";
    }
}


void MainWindow::showStatusMessage(const QString &message)
{
  m_status->setText(message);
  m_status->setStyleSheet("QLabel {font: 600 12pt Segoe UI Semibold ; color : white; }");
}

/***********************************************************************************
 *readDataSerial
 *this is the main function to read data from serial port and diffrentiate it using
 *many functions and variables
**********************************************************************************/

void MainWindow:: readDataSerial()
{
  bool ok;
  QByteArray Cl_lalaSerialData = mSerial->readAll();
  Cl_gaDatabuffer.append(Cl_lalaSerialData);

  qDebug()<<"data length :" <<Cl_gaDatabuffer.size()<<"value Cl_gaDatabuffer : "<<Cl_gaDatabuffer;

  if(Cl_gaDatabuffer.size()>4)
   {
     QByteArray Cl_laReceivedData =Cl_gaDatabuffer;
     QByteArray Cl_laReceivedDataHex =Cl_laReceivedData.toHex();

     qDebug()<<"received data :"<<Cl_laReceivedData;

     QByteArray startBitAscii = Cl_laReceivedDataHex.mid(2,2);
     uint16_t U16_lvstartBitIdentifier = startBitAscii.toInt(&ok,16);

     qDebug()<<"hex val "<<startBitAscii;
     qDebug()<<"startBit"<<U16_lvstartBitIdentifier;

     switch(U16_lvstartBitIdentifier)
     {
       case 1:
       {
         if(Cl_laReceivedData.size()>20)
          {
            qDebug()<<"DRIVER_ID";

            QString Cl_lvParameterName = "DRIVER ID : ";
            QByteArray Cl_laExtractedData = Cl_laReceivedData.mid(0,20);

            qDebug()<< Cl_laExtractedData;
            qDebug()<<"case 1 data"<< Cl_laExtractedData.toHex();

            QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

            qDebug()<<"case 1 ReceivedCrc"<<Cl_lvReceivedcrcHex;

            /*Remove the last 2 bytes for CRC calculation*/

            Cl_laExtractedData.chop(2);

            qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

            uint32_t U32_lvDataLength = Cl_laExtractedData.size();
            QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
            QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

            qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

           if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
           {
             QByteArray Cl_laDriverID   =  Cl_laExtractedData.mid(2,17);
             QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laDriverID);

             US_gaDriverID += Cl_lvAlphanumericData;

             qDebug()<<"Cl_lvAlphanumericData"<<Cl_lvAlphanumericData<<US_gaDriverID;
             saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());

             ui->textEdit_driverId->setText(Cl_lvAlphanumericData);
             ui->textEdit_driverId->setAlignment(Qt::AlignCenter);
             ui->textEdit_driverId_tx->setText(Cl_lvAlphanumericData);
             ui->textEdit_driverId_tx->setAlignment(Qt::AlignCenter);
            }
            else
            {
             qDebug()<< "Invalid data";
            }

           Cl_gaDatabuffer.clear();
         }
         else
         {
               qDebug()<< "Incomplete Data ";
               Cl_gaDatabuffer.clear();
         }

              break;
        }

         case 2:
       {
          if(Cl_laReceivedData.size()>20)
          {
            qDebug()<<"TRAIN_NO";
            QString Cl_lvParameterName = "TRAIN_NO : ";
            QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,20);

            qDebug()<< Cl_laExtractedData;
            qDebug()<<"case 2 data"<< Cl_laExtractedData.toHex();

            QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

            qDebug()<<"case 2 ReceivedCrc"<<Cl_lvReceivedcrcHex;

            Cl_laExtractedData.chop(2);

            qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

            uint32_t U32_lvDataLength = Cl_laExtractedData.size();
            QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
            QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

            qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

            if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
            {
              QByteArray Cl_laTrainNo   =  Cl_laExtractedData.mid(2,17);
              QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laTrainNo);
              US_gaTrainNo =Cl_lvAlphanumericData;
              saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());

              ui->textEdit_trainNo->setText(Cl_lvAlphanumericData);
              ui->textEdit_trainNo->setAlignment(Qt::AlignCenter);
              ui->textEdit_trainNo_tx->setText(Cl_lvAlphanumericData);
              ui->textEdit_trainNo_tx->setAlignment(Qt::AlignCenter);
            }
            else
            {
              qDebug()<< "Invalid data";
            }

              Cl_gaDatabuffer.clear();
         }
          else
          {
                qDebug()<< "Incomplete Data ";
                Cl_gaDatabuffer.clear();
          }

              break;
        }
          case 3:
       {
          if(Cl_laReceivedData.size()>20)
         {
           qDebug()<<"SHED_NAME";

           QString Cl_lvParameterName = "SHED_NAME : ";
           QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,20);

           qDebug()<< Cl_laExtractedData;

           qDebug()<<"case 3 data"<< Cl_laExtractedData.toHex();

           QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

           qDebug()<<"case 3 ReceivedCrc"<<Cl_lvReceivedcrcHex;

           Cl_laExtractedData.chop(2);

           qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

           uint32_t U32_lvDataLength = Cl_laExtractedData.size();
           QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
           QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

           qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

           if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
          {
            QByteArray Cl_laShedName   =  Cl_laExtractedData.mid(2,17);
            QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laShedName);

            saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());

            ui->textEdit_shedName->setText(Cl_lvAlphanumericData);
            ui->textEdit_shedName->setAlignment(Qt::AlignCenter);
            ui->textEdit_shedName_tx->setText(Cl_lvAlphanumericData);
            ui->textEdit_shedName_tx->setAlignment(Qt::AlignCenter);
          }
           else
          {
            qDebug()<< "Invalid data";
          }

          Cl_gaDatabuffer.clear();
         }
          else
          {
                qDebug()<< "Incomplete Data ";
                Cl_gaDatabuffer.clear();
          }

          break;
        }
          case 4:
       {
          if(Cl_laReceivedData.size() > 20)
         {
           qDebug()<<"EQUIPMENT_NO";
           QString Cl_lvParameterName = "EQUIPMENT_NO : ";
           QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,20);
           qDebug()<< Cl_laExtractedData;

           qDebug()<<"case 4 data"<< Cl_laExtractedData.toHex();

           QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

           qDebug()<<"case 4 ReceivedCrc"<<Cl_lvReceivedcrcHex;
           Cl_laExtractedData.chop(2);
           qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

            uint32_t U32_lvDataLength = Cl_laExtractedData.size();
            QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
            QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

            qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

            if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
           {
             QByteArray Cl_laEquipmentNo  =  Cl_laExtractedData.mid(2,17);
             QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laEquipmentNo);

             saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());
             ui->textEdit_EQUno->setText(Cl_lvAlphanumericData);
             ui->textEdit_EQUno->setAlignment(Qt::AlignCenter);
             ui->textEdit_EQUno_tx->setText(Cl_lvAlphanumericData);
             ui->textEdit_EQUno_tx->setAlignment(Qt::AlignCenter);

           }
            else
           {
             qDebug()<< "Invalid data";
           }

           Cl_gaDatabuffer.clear();
         }
          else
          {
                qDebug()<< "Incomplete Data ";
                Cl_gaDatabuffer.clear();
          }
          break;
        }
         case 5:
       {
          if(Cl_laReceivedData.size()>11)
         {
           qDebug()<<"SET_ODOMETER";
           QString Cl_lvParameterName = "SET_ODOMETER : ";
           QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,11);
           qDebug()<< Cl_laExtractedData;

           qDebug()<<"case 5 data"<< Cl_laExtractedData.toHex();

           QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

           qDebug()<<"case 5 ReceivedCrc"<<Cl_lvReceivedcrcHex;
           Cl_laExtractedData.chop(2);
           qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

           uint32_t U32_lvDataLength = Cl_laExtractedData.size();
           QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
           QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

           qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

           if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
          {
            QByteArray Cl_laSetOdometer =  Cl_laExtractedData.mid(2,8);
            QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laSetOdometer);

            saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());

            ui->textEdit_odometer->setText(Cl_lvAlphanumericData);
            ui->textEdit_odometer->setAlignment(Qt::AlignCenter);
            ui->textEdit_odometer_tx->setText(Cl_lvAlphanumericData);
            ui->textEdit_odometer_tx->setAlignment(Qt::AlignCenter);
           }
           else
          {
            qDebug()<< "Invalid data";
          }

          Cl_gaDatabuffer.clear();
         }
          else
          {
                qDebug()<< "Incomplete Data ";
                Cl_gaDatabuffer.clear();
          }

           break;
        }
          case 6:
       {
           if(Cl_laReceivedData.size()>10)
          {
              qDebug()<<"TRAIN_LOAD";
              QString Cl_lvParameterName = "TRAIN_LOAD : ";
              QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,10);
              qDebug()<< Cl_laExtractedData;

              qDebug()<<"case 6 data"<< Cl_laExtractedData.toHex();

              QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

              qDebug()<<"case 6 ReceivedCrc"<<Cl_lvReceivedcrcHex;
              Cl_laExtractedData.chop(2);
              qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

              uint32_t U32_lvDataLength = Cl_laExtractedData.size();
              QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
              QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

              qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

              if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
              {
              QByteArray Cl_laTrainLoad   =  Cl_laExtractedData.mid(2,7);
              QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laTrainLoad);

              US_gaTrainLoad = Cl_lvAlphanumericData;
              saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());

              ui->textEdit_trainLoad->setText(Cl_lvAlphanumericData);
              ui->textEdit_trainLoad->setAlignment(Qt::AlignCenter);
              ui->textEdit_trainLoad_tx->setText(Cl_lvAlphanumericData);
              ui->textEdit_trainLoad_tx->setAlignment(Qt::AlignCenter);
              }
              else
              {
                      qDebug()<< "Invalid data";

              }
              Cl_gaDatabuffer.clear();
          }
           else
           {
                 qDebug()<< "Incomplete Data ";
                 Cl_gaDatabuffer.clear();
           }

          break;
        }
          case 7:
       {
         if(Cl_laReceivedData.size()>10)
        {
          qDebug()<<"TIME_SETTING";
          QString Cl_lvParameterName = "TIME_SETTING : ";
          QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,10);
          qDebug()<< Cl_laExtractedData;

          qDebug()<<"case 7 data"<< Cl_laExtractedData.toHex();

          QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

          qDebug()<<"case 7 ReceivedCrc"<<Cl_lvReceivedcrcHex;
          Cl_laExtractedData.chop(2);
          qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

          uint32_t U32_lvDataLength = Cl_laExtractedData.size();
          QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
          QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

          qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

          if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
         {
           QByteArray Cl_laTimeSetting   =  Cl_laExtractedData.mid(2,7);
           QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laTimeSetting);
           QString Cl_lvSeparatedString;

            /* Format the alphanumeric data with hyphens for display */

           for (int i = 0; i < Cl_laTimeSetting.size(); i += 2)
           {
             Cl_lvSeparatedString += Cl_laTimeSetting.mid(i, 2);
             if (i + 2 < Cl_laTimeSetting.size())
             Cl_lvSeparatedString += " : ";

             qDebug()<<"time "<<Cl_lvSeparatedString <<Cl_laTimeSetting<<Cl_lvAlphanumericData;
             saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());

             ui->textEdit_timeSet->setText(Cl_lvSeparatedString);
             ui->textEdit_timeSet->setAlignment(Qt::AlignCenter);
             ui->textEdit_timeSet_tx->setText(Cl_lvSeparatedString);
             ui->textEdit_timeSet_tx->setAlignment(Qt::AlignCenter);
            }
           }
            else
           {
             qDebug()<< "Invalid data";
           }

            Cl_gaDatabuffer.clear();
         }
         else
         {
               qDebug()<< "Incomplete Data ";
               Cl_gaDatabuffer.clear();
         }
           break;
        }
         case 8:
       {
          if(Cl_laReceivedData.size()>10)
         {
            qDebug()<<"DATE_SETTING";
            QString Cl_lvParameterName = "DATE_SETTING : ";
            QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,10);
            qDebug()<< Cl_laExtractedData;

            qDebug()<<"case 8 data"<< Cl_laExtractedData.toHex();

            QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

            qDebug()<<"case 8 ReceivedCrc"<<Cl_lvReceivedcrcHex;
            Cl_laExtractedData.chop(2);
            qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

            uint32_t U32_lvDataLength = Cl_laExtractedData.size();
            QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
            QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

            qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

            if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
           {
             QByteArray Cl_laDateSetting   =  Cl_laExtractedData.mid(2,7);
             QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laDateSetting);
             QString Cl_lvSeparatedString;

            for (int i = 0; i < Cl_lvAlphanumericData.size(); i += 2)
           {
             Cl_lvSeparatedString += Cl_lvAlphanumericData.midRef(i, 2);
             if (i + 2 < Cl_lvAlphanumericData.size())
             Cl_lvSeparatedString += "-";

             saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());

             ui->textEdit_dateSet->setText(Cl_lvSeparatedString);
             ui->textEdit_dateSet->setAlignment(Qt::AlignCenter);
             ui->textEdit_dateSet_tx->setText(Cl_lvSeparatedString);
             ui->textEdit_dateSet_tx->setAlignment(Qt::AlignCenter);
           }
          }
           else
          {
            qDebug()<< "Invalid data";
          }

          Cl_gaDatabuffer.clear();
         }
          else
          {
                qDebug()<< "Incomplete Data ";
                Cl_gaDatabuffer.clear();
          }

         break;
        }
          case 9:
        {
           if(Cl_laReceivedData.size()>9)
          {
            qDebug()<<"LOCO_NO";
            QString Cl_lvParameterName = "LOCO_NO : ";
            QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,9);
            qDebug()<< Cl_laExtractedData;

            qDebug()<<"case 9 data"<< Cl_laExtractedData.toHex();

            QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

            qDebug()<<"case 9 ReceivedCrc"<<Cl_lvReceivedcrcHex;
            Cl_laExtractedData.chop(2);
            qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

            uint32_t U32_lvDataLength = Cl_laExtractedData.size();
            QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
            QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

            qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

            if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
           {
             QByteArray Cl_laLocoNo   =  Cl_laExtractedData.mid(2,6);
             QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laLocoNo);
             saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());


             ui->textEdit_locoNo->setText(Cl_lvAlphanumericData);
             ui->textEdit_locoNo->setAlignment(Qt::AlignCenter);
             ui->textEdit_locoNo_tx->setText(Cl_lvAlphanumericData);
             ui->textEdit_locoNo_tx->setAlignment(Qt::AlignCenter);
            }
             else
            {
             qDebug()<< "Invalid data";
            }

            Cl_gaDatabuffer.clear();
         }
           else
           {
                 qDebug()<< "Incomplete Data ";
                 Cl_gaDatabuffer.clear();
           }
          break;
        }

          case 10:
       {
          if(Cl_laReceivedData.size()>8)
         {
          qDebug()<<"CT_FACTOR";
          QString Cl_lvParameterName = "CT_FACTOR : ";
          QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,8);
          qDebug()<< Cl_laExtractedData;

          qDebug()<<"case 10 data"<< Cl_laExtractedData.toHex();

          QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

          qDebug()<<"case 10 ReceivedCrc"<<Cl_lvReceivedcrcHex;
          Cl_laExtractedData.chop(2);
          qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

          uint32_t U32_lvDataLength = Cl_laExtractedData.size();
          QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
          QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

          qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

          if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
         {
           QByteArray Cl_laCtFactor   =  Cl_laExtractedData.mid(2,5);
           QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laCtFactor);
           saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());

           ui->textEdit_ctFactor->setText(Cl_lvAlphanumericData);
           ui->textEdit_ctFactor->setAlignment(Qt::AlignCenter);
           ui->textEdit_ctFactor_tx->setText(Cl_lvAlphanumericData);
           ui->textEdit_ctFactor_tx->setAlignment(Qt::AlignCenter);

          }
           else
          {
           qDebug()<< "Invalid data";
          }
          Cl_gaDatabuffer.clear();
         }
          else
          {
                qDebug()<< "Incomplete Data ";
                Cl_gaDatabuffer.clear();
          }

          break;
        }
         case 11:
        {
           if(Cl_laReceivedData.size()>8)
          {
            qDebug()<<"PT_FACTOR";
            QString Cl_lvParameterName = "PT_FACTOR : ";
            QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,8);
            qDebug()<< Cl_laExtractedData;

            qDebug()<<"case 11 data"<< Cl_laExtractedData.toHex();

            QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

            qDebug()<<"case 11 ReceivedCrc"<<Cl_lvReceivedcrcHex;
            Cl_laExtractedData.chop(2);
            qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

            uint32_t U32_lvDataLength = Cl_laExtractedData.size();
            QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
            QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

            qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

            if(Cl_lvcalculatedSwappedCrc == Cl_lvReceivedcrcHex)
           {
             QByteArray Cl_laPtFactor   =  Cl_laExtractedData.mid(2,5);
             QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laPtFactor);
             saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());

             ui->textEdit_ptFactor->setText(Cl_lvAlphanumericData);
             ui->textEdit_ptFactor->setAlignment(Qt::AlignCenter);
             ui->textEdit_ptFactor_tx->setText(Cl_lvAlphanumericData);
             ui->textEdit_ptFactor_tx->setAlignment(Qt::AlignCenter);
           }
            else
           {
             qDebug()<< "Invalid data";
           }

           Cl_gaDatabuffer.clear();
         }
           else
           {
                 qDebug()<< "Incomplete Data ";
                 Cl_gaDatabuffer.clear();
           }
           break;
        }
          case 12:
       {
           if(Cl_laReceivedData.size()>8)
          {
            qDebug()<<"WHEELDIA_SETTING";
            QString Cl_lvParameterName = "WHEELDIA_SETTING : ";
            QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,8);
            qDebug()<< Cl_laExtractedData;

            qDebug()<<"case 12 data"<< Cl_laExtractedData.toHex();

            QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

            qDebug()<<"case 12 ReceivedCrc"<<Cl_lvReceivedcrcHex;
                    Cl_laExtractedData.chop(2);
            qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

            uint32_t U32_lvDataLength = Cl_laExtractedData.size();
            QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
            QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

            qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

            if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
           {
             QByteArray Cl_laWheelSetting   =  Cl_laExtractedData.mid(2,5);
             QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laWheelSetting);
             saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());

             ui->textEdit_wheelSetting->setText(Cl_lvAlphanumericData);
             ui->textEdit_wheelSetting->setAlignment(Qt::AlignCenter);
             ui->textEdit_wheelSetting_tx->setText(Cl_lvAlphanumericData);
             ui->textEdit_wheelSetting_tx->setAlignment(Qt::AlignCenter);
            }
            else
           {
             qDebug()<< "Invalid data";
           }

            Cl_gaDatabuffer.clear();
         }
           else
           {
                 qDebug()<< "Incomplete Data ";
                 Cl_gaDatabuffer.clear();
           }
            break;
        }
         case 13:
       {
          if(Cl_laReceivedData.size()>7)
         {
           qDebug()<<"MAX_SPEED";
           QString Cl_lvParameterName = "MAX_SPEED : ";
           QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,7);
           qDebug()<< Cl_laExtractedData;

           qDebug()<<"case 13 data"<< Cl_laExtractedData.toHex();

           QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

           qDebug()<<"case 13 ReceivedCrc"<<Cl_lvReceivedcrcHex;
           Cl_laReceivedData.chop(2);
           qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

           uint32_t U32_lvDataLength = Cl_laExtractedData.size();
           QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
           QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

           qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

           if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
          {
            QByteArray Cl_laMaxSpped   =  Cl_laExtractedData.mid(2,3);
            QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laMaxSpped);
            saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());

            ui->textEdit_maxSpeed->setText(Cl_lvAlphanumericData);
            ui->textEdit_maxSpeed->setAlignment(Qt::AlignCenter);
            ui->textEdit_maxSpeed_tx->setText(Cl_lvAlphanumericData);
            ui->textEdit_maxSpeed_tx->setAlignment(Qt::AlignCenter);
          }
           else
          {
           qDebug()<<"invalid data";
          }
           Cl_gaDatabuffer.clear();
          }
          else
          {
                qDebug()<< "Incomplete Data ";
                Cl_gaDatabuffer.clear();
          }
            break;
        }
         case 14:
       {
           if(Cl_laReceivedData.size()>5)
          {
            flashTimer->start(400);
            qDebug()<<"RESET_COUNTERS";
            QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,5);
            qDebug()<< Cl_laExtractedData;

            flashText();

            Cl_gaDatabuffer.clear();
          }
           else
           {
                 qDebug()<< "Incomplete Data ";
                 Cl_gaDatabuffer.clear();
           }
             break;
        }
          case 15:
       {
           if(Cl_laReceivedData.size()>5)
          {
            qDebug()<<"TYPE_OF_LOCO";
            QString Cl_lvParameterName = "TYPE_OF_LOCO : ";
            QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,5);
            qDebug()<< Cl_laExtractedData;

            qDebug()<<"case 15 data"<< Cl_laExtractedData.toHex();

            QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

            qDebug()<<"case 15 ReceivedCrc"<<Cl_lvReceivedcrcHex;
                    Cl_laExtractedData.chop(2);
            qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

            uint32_t U32_lvDataLength = Cl_laExtractedData.size();
            QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
            QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

            qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

            if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
           {
             QByteArray Cl_laTypeOfLoco  =  Cl_laExtractedData.mid(2,1);
             QString HexString = Cl_laTypeOfLoco.toHex();
             saveDataToFile(Cl_lvParameterName,HexString.toUtf8());

             ui->textEdit_typeOfLoco->setText(HexString);
             ui->textEdit_typeOfLoco->setAlignment(Qt::AlignCenter);
             ui->textEdit_typeOfLoco_tx->setText(HexString);
             ui->textEdit_typeOfLoco_tx->setAlignment(Qt::AlignCenter);
           }
            else
           {
             qDebug()<< "Invalid data";
           }

            Cl_gaDatabuffer.clear();
         }
           else
           {
                 qDebug()<< "Incomplete Data ";
                 Cl_gaDatabuffer.clear();
           }
           break;
        }
         case 16:
       {
          if(Cl_laReceivedData.size()>66)
         {
           QByteArray LastString   = Cl_laReceivedData.mid(0,67);
           qDebug()<<"case 16 data"<<LastString.toHex();

           QString Cl_lvReceivedcrcHex =LastString.toHex().right(4);

           qDebug()<<"case 16 ReceivedCrc"<<Cl_lvReceivedcrcHex;
           LastString.chop(2);
           qDebug()<<"crc byte for calculation "<<LastString.toHex();

           uint32_t U32_lvDataLength =LastString.size();
           QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(LastString,U32_lvDataLength),16);
           QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

           qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

           if(LastString.size()>64)
          {
             QByteArray Cl_laDataString =LastString.toHex().mid(4,126);
             QByteArray Cl_laDataString2=LastString.mid(2,63);

             qDebug()<<"Cl_laDataString2 if "<<Cl_laDataString2.mid(0,2);

             QString Cl_lvspeed_LSB = Cl_laDataString.mid(0,2);
             QString Cl_lvspeed_MSB = Cl_laDataString.mid(2,2);
             QString Cl_lvSpeed = addHexNumbers(Cl_lvspeed_LSB,Cl_lvspeed_MSB);
             int16_t S16_lvSpeedInt = Cl_lvSpeed.toInt(&ok,16);

             if(S16_lvSpeedInt >=180)
            {
              if (! F_gvIsBlinking)
              {
                F_gvIsBlinking = true;
                blinkTimer->start(400);  /* Blink every 400 milliseconds*/
              }

            }
             else
           {
              if ( F_gvIsBlinking)
              {
                 F_gvIsBlinking = false;
                 blinkTimer->stop();
                 ui->speed_offLED->setPixmap(ledOff);  /* Set the LED to the off state*/
              }
            }
            updateSpeed(S16_lvSpeedInt);

//          qDebug()<<"speed"<<Speed<<"LSB"<<speed_LSB<<"MSB"<<speed_MSB;

            qDebug()<<"speed int"<<S16_lvSpeedInt;
            ui->textEdit_speed->setText(QString::number(S16_lvSpeedInt));
            ui->textEdit_speed->setAlignment(Qt::AlignCenter);
            ui->lineEdit_speed->setText(QString::number(S16_lvSpeedInt));
            ui->lineEdit_speed->setAlignment(Qt::AlignCenter);
            ui->textEdit_speed_tx->setText(QString::number(S16_lvSpeedInt));
            ui->textEdit_speed_tx->setAlignment(Qt::AlignCenter);

            QString Cl_lvvoltage_LSB = Cl_laDataString.mid(4,2);
            QString Cl_lvvoltage_MSB = Cl_laDataString.mid(6,2);
            QString Cl_lvvoltage = addHexNumbers(Cl_lvvoltage_LSB,Cl_lvvoltage_MSB);
            QString Cl_lvvalue = addHexNumbers2(Cl_lvspeed_LSB,Cl_lvspeed_MSB,Cl_lvvoltage_LSB,Cl_lvvoltage_MSB);

            uint16_t U16_lvvoltageInt = Cl_lvvoltage.toInt(&ok,16);
            uint16_t U16_lvvalueint = Cl_lvvalue.toInt(&ok,16);

            qDebug()<<"value int"<<Cl_lvvalue<<U16_lvvalueint;

            ui->textEdit_voltage->setText(QString::number(U16_lvvoltageInt));
            ui->textEdit_voltage->setAlignment(Qt::AlignCenter);
            ui->textEdit_voltage_tx->setText(QString::number(U16_lvvoltageInt));
            ui->textEdit_voltage_tx->setAlignment(Qt::AlignCenter);

            QString Cl_lvcurrent_LSB = Cl_laDataString.mid(8,2);
            QString Cl_lvcurrent_MSB = Cl_laDataString.mid(10,2);
            QString Cl_lvCurrent = addHexNumbers(Cl_lvcurrent_LSB,Cl_lvcurrent_MSB);
            uint16_t U16_lvCurrentInt = Cl_lvCurrent.toInt(&ok,16);

            ui->textEdit_current->setText(QString::number(U16_lvCurrentInt));
            ui->textEdit_current->setAlignment(Qt::AlignCenter);
            ui->textEdit_current_tx->setText(QString::number(U16_lvCurrentInt));
            ui->textEdit_current_tx->setAlignment(Qt::AlignCenter);

            uint8_t  U8_lvCoustingstatus = Cl_laDataString.mid(12,2).toInt(&ok,10);
            uint8_t  U8_lvBreakingStatus = Cl_laDataString.mid(14,2).toInt(&ok,10);

            if(U8_lvCoustingstatus>0)
           {
             ui->textEdit_coustingStatus->setStyleSheet("color : green  ;");
             ui->textEdit_coustingStatus->setFont(QFont( "Consolas", 12));
             ui->textEdit_coustingStatus_tx->setStyleSheet("color : green  ;");
             ui->textEdit_coustingStatus_tx->setFont(QFont( "Consolas", 12));
             ui->textEdit_coustingStatus->setText("ON");
             ui->textEdit_coustingStatus->setAlignment(Qt::AlignCenter);
             ui->textEdit_coustingStatus_tx->setText("ON");
             ui->textEdit_coustingStatus_tx->setAlignment(Qt::AlignCenter);
           }
           else
           {
             ui->textEdit_coustingStatus->setStyleSheet("color : red  ;");
             ui->textEdit_coustingStatus->setFont(QFont( "Consolas", 12));
             ui->textEdit_coustingStatus_tx->setStyleSheet("color : red  ;");
             ui->textEdit_coustingStatus_tx->setFont(QFont( "Consolas", 12));
             ui->textEdit_coustingStatus->setText("OFF");
             ui->textEdit_coustingStatus->setAlignment(Qt::AlignCenter);
             ui->textEdit_coustingStatus_tx->setText("OFF");
             ui->textEdit_coustingStatus_tx->setAlignment(Qt::AlignCenter);
            }
            if(U8_lvBreakingStatus>0)
           {
             ui->textEdit_breakingStatus->setStyleSheet("color : green  ;");
             ui->textEdit_breakingStatus->setFont(QFont( "Consolas", 12));
             ui->textEdit_breakingStatus_tx->setStyleSheet("color : green  ;");
             ui->textEdit_breakingStatus_tx->setFont(QFont( "Consolas", 12));
             ui->textEdit_breakingStatus->setText("ON");
             ui->textEdit_breakingStatus->setAlignment(Qt::AlignCenter);
             ui->textEdit_breakingStatus_tx->setText("ON");
             ui->textEdit_breakingStatus_tx->setAlignment(Qt::AlignCenter);
           }
            else
           {
             ui->textEdit_breakingStatus->setStyleSheet("color : red  ;");
             ui->textEdit_breakingStatus->setFont(QFont( "Consolas", 12));
             ui->textEdit_breakingStatus_tx->setStyleSheet("color : red  ;");
             ui->textEdit_breakingStatus_tx->setFont(QFont( "Consolas", 12));
             ui->textEdit_breakingStatus->setText("OFF");
             ui->textEdit_breakingStatus->setAlignment(Qt::AlignCenter);
             ui->textEdit_breakingStatus_tx->setText("OFF");
             ui->textEdit_breakingStatus_tx->setAlignment(Qt::AlignCenter);
           }

            QString Cl_lvTotalEnergyConsumed_0 =  Cl_laDataString.mid(16,2);
            QString Cl_lvTotalEnergyConsumed_8 =  Cl_laDataString.mid(18,2);
            QString Cl_lvTotalEnergyConsumed_16 = Cl_laDataString.mid(20,2);
            QString Cl_lvTotalEnergyConsumed_24 = Cl_laDataString.mid(22,2);
            QString TotalEnergyConsumed = addHexNumbers2(Cl_lvTotalEnergyConsumed_0,Cl_lvTotalEnergyConsumed_8,
                                                         Cl_lvTotalEnergyConsumed_16,Cl_lvTotalEnergyConsumed_24);
            uint32_t U32_lvTotalEnergyConsumedInt = TotalEnergyConsumed.toInt(&ok,16);

             qDebug()<<"TotalEnergyConsumed int"<<U32_lvTotalEnergyConsumedInt;

             ui->textEdit_totalEnergyConsumed->setText(QString::number(U32_lvTotalEnergyConsumedInt));
             ui->textEdit_totalEnergyConsumed->setAlignment(Qt::AlignCenter);
             ui->textEdit_totalEnergyConsumed_tx->setText(QString::number(U32_lvTotalEnergyConsumedInt));
             ui->textEdit_totalEnergyConsumed_tx->setAlignment(Qt::AlignCenter);


             QString Cl_lvDisTravelledOdometer_0 =  Cl_laDataString.mid(24,2);
             QString Cl_lvDisTravelledOdometer_8 =  Cl_laDataString.mid(26,2);
             QString Cl_lvDisTravelledOdometer_16 = Cl_laDataString.mid(28,2);
             QString Cl_lvDisTravelledOdometer_24= Cl_laDataString.mid(30,2);
             QString Cl_lvDisTravelledOdometer = addHexNumbers2(Cl_lvDisTravelledOdometer_0,Cl_lvDisTravelledOdometer_8,
                                                           Cl_lvDisTravelledOdometer_16,Cl_lvDisTravelledOdometer_24);
             uint U32_lvDisTravelledOdometerInt = Cl_lvDisTravelledOdometer.toInt(&ok,16);

             qDebug()<<"DisTravelledOdometer int"<<U32_lvDisTravelledOdometerInt;

             ui->textEdit_disTravelledOdometer->setText(QString::number(U32_lvDisTravelledOdometerInt));
             ui->textEdit_disTravelledOdometer->setAlignment(Qt::AlignCenter);
             ui->textEdit_disTravelledOdometer_tx->setText(QString::number(U32_lvDisTravelledOdometerInt));
             ui->textEdit_disTravelledOdometer_tx->setAlignment(Qt::AlignCenter);

             QString Cl_lvEnergyConsumedRun_0 =  Cl_laDataString.mid(32,2);
             QString Cl_lvEnergyConsumedRun_8 =  Cl_laDataString.mid(34,2);
             QString Cl_lvEnergyConsumedRun_16 = Cl_laDataString.mid(36,2);
             QString Cl_lvEnergyConsumedRun_24=  Cl_laDataString.mid(38,2);
             QString Cl_lvEnergyConsumedRun = addHexNumbers2(Cl_lvEnergyConsumedRun_0,Cl_lvEnergyConsumedRun_8,
                                                        Cl_lvEnergyConsumedRun_16,Cl_lvEnergyConsumedRun_24);
             uint32_t U32_lvEnergyConsumedRunInt = Cl_lvEnergyConsumedRun.toInt(&ok,16);

             qDebug()<<"EnergyConsumedRun int"<<U32_lvEnergyConsumedRunInt;

             ui->textEdit_energyConsumedRunning->setText(QString::number(U32_lvEnergyConsumedRunInt));
             ui->textEdit_energyConsumedRunning->setAlignment(Qt::AlignCenter);
             ui->textEdit_energyConsumedRunning_tx->setText(QString::number(U32_lvEnergyConsumedRunInt));
             ui->textEdit_energyConsumedRunning_tx->setAlignment(Qt::AlignCenter);

             QString Cl_lvEnergyconsumedHalt_0 =  Cl_laDataString.mid(40,2);
             QString Cl_lvEnergyconsumedHalt_8 =  Cl_laDataString.mid(42,2);
             QString Cl_lvEnergyconsumedHalt_16 = Cl_laDataString.mid(44,2);
             QString Cl_lvEnergyconsumedHalt_24=  Cl_laDataString.mid(46,2);
             QString Cl_lvEnergyconsumedHalt = addHexNumbers2(Cl_lvEnergyconsumedHalt_0,Cl_lvEnergyconsumedHalt_8,
                                                         Cl_lvEnergyconsumedHalt_16,Cl_lvEnergyconsumedHalt_24);
             uint U32_lvEnergyconsumedHaltInt = Cl_lvEnergyconsumedHalt.toInt(&ok,16);

             qDebug()<<"EnergyconsumedHalt int"<<U32_lvEnergyconsumedHaltInt;

            ui->textEdit_energyConsumedHalt->setText(QString::number(U32_lvEnergyconsumedHaltInt));
            ui->textEdit_energyConsumedHalt->setAlignment(Qt::AlignCenter);
            ui->textEdit_energyConsumedHalt_tx->setText(QString::number(U32_lvEnergyconsumedHaltInt));
            ui->textEdit_energyConsumedHalt_tx->setAlignment(Qt::AlignCenter);

            QString Cl_lvDriverSpecificEnergy_0 =  Cl_laDataString.mid(48,2);
            QString Cl_lvDriverSpecificEnergy_8 =  Cl_laDataString.mid(50,2);
            QString Cl_lvDriverSpecificEnergy_16 = Cl_laDataString.mid(52,2);
            QString Cl_lvDriverSpecificEnergy_24=  Cl_laDataString.mid(54,2);
            QString Cl_lvDriverSpecificEnergy = addHexNumbers2(Cl_lvDriverSpecificEnergy_0,Cl_lvDriverSpecificEnergy_8,Cl_lvDriverSpecificEnergy_16,Cl_lvDriverSpecificEnergy_24);
            uint U32_lvDriverSpecificEnergyInt = Cl_lvDriverSpecificEnergy.toInt(&ok,16);

            qDebug()<<"DriverSpecificEnergy int"<<U32_lvDriverSpecificEnergyInt;

            ui->textEdit_driverSpecificEnergy->setText(QString::number(U32_lvDriverSpecificEnergyInt));
            ui->textEdit_driverSpecificEnergy->setAlignment(Qt::AlignCenter);
            ui->textEdit_driverSpecificEnergy_tx->setText(QString::number(U32_lvDriverSpecificEnergyInt));
            ui->textEdit_driverSpecificEnergy_tx->setAlignment(Qt::AlignCenter);

            QString Cl_lvDriverSpecificDistance_0 =  Cl_laDataString.mid(56,2);
            QString Cl_lvDriverSpecificDistance_8 =  Cl_laDataString.mid(58,2);
            QString Cl_lvDriverSpecificDistance_16 = Cl_laDataString.mid(60,2);
            QString Cl_lvDriverSpecificDistance_24=  Cl_laDataString.mid(62,2);
            QString Cl_lvDriverSpecificDistance = addHexNumbers2(Cl_lvDriverSpecificDistance_0,Cl_lvDriverSpecificDistance_8,
                                                            Cl_lvDriverSpecificDistance_16,Cl_lvDriverSpecificDistance_24);
            uint U32_lvDriverSpecificDistanceInt = Cl_lvDriverSpecificDistance.toInt(&ok,16);

            qDebug()<<"DriverSpecificDistance int"<<U32_lvDriverSpecificDistanceInt;

            ui->textEdit_driverSpecificDistance->setText(QString::number(U32_lvDriverSpecificDistanceInt));
            ui->textEdit_driverSpecificDistance->setAlignment(Qt::AlignCenter);
            ui->textEdit_driverSpecificDistance_tx->setText(QString::number(U32_lvDriverSpecificDistanceInt));
            ui->textEdit_driverSpecificDistance_tx->setAlignment(Qt::AlignCenter);

            QString Cl_lvDynamicBreakingDuration =  Cl_laDataString2.mid(32,8);

            qDebug()<<"DynamicBreakingDuration "<<Cl_lvDynamicBreakingDuration;

            ui->textEdit_breakingDuration->setText(Cl_lvDynamicBreakingDuration);
            ui->textEdit_breakingDuration->setAlignment(Qt::AlignCenter);
            ui->textEdit_breakingDuration_tx->setText(Cl_lvDynamicBreakingDuration);
            ui->textEdit_breakingDuration_tx->setAlignment(Qt::AlignCenter);

            QString Cl_lvCoastingDistance_0 =  Cl_laDataString.mid(80,2);
            QString Cl_lvCoastingDistance_8 =  Cl_laDataString.mid(82,2);
            QString Cl_lvCoastingDistance_16 = Cl_laDataString.mid(84,2);
            QString Cl_lvCoastingDistance_24=  Cl_laDataString.mid(86,2);
            QString Cl_lvCoastingDistance = addHexNumbers2(Cl_lvCoastingDistance_0,Cl_lvCoastingDistance_8,
                                                      Cl_lvCoastingDistance_16,Cl_lvCoastingDistance_24);
            uint U32_lvCoastingDistanceInt = Cl_lvCoastingDistance.toInt(&ok,16);

            qDebug()<<"CoastingDistance int"<<U32_lvCoastingDistanceInt;

            ui->textEdit_coastingDistance->setText(QString::number(U32_lvCoastingDistanceInt));
            ui->textEdit_coastingDistance->setAlignment(Qt::AlignCenter);
            ui->textEdit_coastingDistance_tx->setText(QString::number(U32_lvCoastingDistanceInt));
            ui->textEdit_coastingDistance_tx->setAlignment(Qt::AlignCenter);


            QString CoastingDuration =  Cl_laDataString2.mid(44,8);

            qDebug()<<"CoastingDuration "<<CoastingDuration;

            ui->textEdit_coastingDuration->setText(CoastingDuration);
            ui->textEdit_coastingDuration->setAlignment(Qt::AlignCenter);
            ui->textEdit_coastingDuration_tx->setText(CoastingDuration);
            ui->textEdit_coastingDuration_tx->setAlignment(Qt::AlignCenter);

            QString OverSpeedDistance_0 =  Cl_laDataString.mid(104,2);
            QString OverSpeedDistance_8 =  Cl_laDataString.mid(106,2);
            QString OverSpeedDistance_16=  Cl_laDataString.mid(108,2);
            QString OverSpeedDistance_24=  Cl_laDataString.mid(110,2);
            QString OverSpeedDistance = addHexNumbers2(OverSpeedDistance_0,OverSpeedDistance_8,
                                                       OverSpeedDistance_16,OverSpeedDistance_24);
            uint OverSpeedDistanceInt = OverSpeedDistance.toInt(&ok,16);

            qDebug()<<"OverSpeedDistance int"<<OverSpeedDistanceInt;

            ui->textEdit_overSpeedDistance->setText(QString::number(OverSpeedDistanceInt));
            ui->textEdit_overSpeedDistance->setAlignment(Qt::AlignCenter);
            ui->textEdit_overSpeedDistance_tx->setText(QString::number(OverSpeedDistanceInt));
            ui->textEdit_overSpeedDistance_tx->setAlignment(Qt::AlignCenter);

            QString Cl_lvOverSpeedDuration =  Cl_laDataString2.mid(56,6).toHex();

            QString Cl_lvOverSpeedDH1 = Cl_lvOverSpeedDuration.at(1);
            QString Cl_lvOverSpeedDH2 = Cl_lvOverSpeedDuration.at(3);
            QString Cl_lvOverSpeedDM1 = Cl_lvOverSpeedDuration.at(5);
            QString Cl_lvOverSpeedDM2=  Cl_lvOverSpeedDuration.at(7);
            QString Cl_lvOverSpeedDS1=  Cl_lvOverSpeedDuration.at(9);
            QString Cl_lvOverSpeedDS2=  Cl_lvOverSpeedDuration.at(11);

            QString Cl_lvResult = Cl_lvOverSpeedDH1+Cl_lvOverSpeedDH2+Cl_lvOverSpeedDM1+Cl_lvOverSpeedDM2+Cl_lvOverSpeedDS1+Cl_lvOverSpeedDS2;

            QString Cl_lvSeparatedString;

            for (int i = 0; i < Cl_lvResult.size(); i += 2)
           {
             Cl_lvSeparatedString += Cl_lvResult.midRef(i, 2);
             if (i + 2 < Cl_lvResult.size())
              Cl_lvSeparatedString += ":";

             qDebug()<<"OverSpeedDuration "<<Cl_lvSeparatedString;

             ui->textEdit_overSpeedDuration_tx->setText(Cl_lvSeparatedString);
             ui->textEdit_overSpeedDuration_tx->setAlignment(Qt::AlignCenter);
             ui->textEdit_overSpeedDuration->setText(Cl_lvSeparatedString);
             ui->textEdit_overSpeedDuration->setAlignment(Qt::AlignCenter);

            }

            QString Cl_lvFModeSelection =  Cl_laDataString2.mid(62,1).toHex();
            U8_gfFModeSelectionFlag =  Cl_lvFModeSelection.toUInt(&ok,16);

            qDebug()<<"FModeSelection "<<U8_gfFModeSelectionFlag<<Cl_lvFModeSelection;

            ui->textEdit_fModeSelection->setText(QString::number(U8_gfFModeSelectionFlag));
            ui->textEdit_fModeSelection->setAlignment(Qt::AlignCenter);
            ui->textEdit_fModeSelection_tx->setText(QString::number(U8_gfFModeSelectionFlag));
            ui->textEdit_fModeSelection_tx->setAlignment(Qt::AlignCenter);
          }
           else
          {
            qDebug()<< "Invalid data";
          }

          Cl_gaDatabuffer.clear();
         }
          else
          {
                qDebug()<< "Incomplete Data ";
                Cl_gaDatabuffer.clear();
          }
          break;
        }
           case 17:
       {
          if(Cl_laReceivedData.size()>20)
         {
            qDebug()<<"Set PG No";
            QString Cl_lvParameterName = "PG No : ";
            QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,20);
            qDebug()<< Cl_laExtractedData;

            qDebug()<<"PG No data"<< Cl_laExtractedData.toHex();
            QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);
            qDebug()<<"PG No ReceivedCrc"<<Cl_lvReceivedcrcHex;
            Cl_laExtractedData.chop(2);
            qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

            uint32_t U32_lvDataLength = Cl_laExtractedData.size();
            QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
            QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

            qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

           if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
          {
             QByteArray Cl_laPGNO   =  Cl_laExtractedData.mid(2,17);
             QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laPGNO);

             saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());

             ui->textEdit_PGno->setText(Cl_lvAlphanumericData);
             ui->textEdit_PGno->setAlignment(Qt::AlignCenter);
             ui->textEdit_PGno_tx->setText(Cl_lvAlphanumericData);
             ui->textEdit_PGno_tx->setAlignment(Qt::AlignCenter);
          }
           else
          {
            qDebug()<< "Invalid data";
          }

           Cl_gaDatabuffer.clear();

         }
          else
          {
                qDebug()<< "Incomplete Data ";
                Cl_gaDatabuffer.clear();
          }
           break;
        }
          case 18:
       {
          if(Cl_laReceivedData.size()>8)
         {
           qDebug()<<"Set ct Input";
           QString Cl_lvParameterName = "ct Input : ";
           QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,8);
           qDebug()<< Cl_laExtractedData;

           qDebug()<<"ct Input data"<<Cl_laReceivedData.toHex();

           QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

           qDebug()<<"ct Input ReceivedCrc"<<Cl_lvReceivedcrcHex;
           Cl_laExtractedData.chop(2);
           qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

           uint32_t U32_lvDataLength = Cl_laExtractedData.size();
           QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
           QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

           qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

           if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
          {
            QByteArray ctInput   =  Cl_laExtractedData.mid(2,5);
            QString Cl_lvAlphanumericData = extractAlphanumeric(ctInput);
            saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());
            ui->textEdit_CTinput->setText(Cl_lvAlphanumericData);
            ui->textEdit_CTinput->setAlignment(Qt::AlignCenter);
            ui->textEdit_CTinput_tx->setText(Cl_lvAlphanumericData);
            ui->textEdit_CTinput_tx->setAlignment(Qt::AlignCenter);
          }
            else
          {
            qDebug()<< "Invalid data";
          }
          Cl_gaDatabuffer.clear();
         }
          else
          {
                qDebug()<< "Incomplete Data ";
                Cl_gaDatabuffer.clear();
          }
         break;
        }
          case 19:
       {
          if(Cl_laReceivedData.size()>8)
         {
           qDebug()<<"ct Output";
           QString Cl_lvParameterName = "ct Output : ";
           QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,8);
           qDebug()<< Cl_laExtractedData;

           qDebug()<<" ct data"<< Cl_laExtractedData.toHex();
           QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

           qDebug()<<"ct  ReceivedCrc"<<Cl_lvReceivedcrcHex;
           Cl_laExtractedData.chop(2);
           qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

           uint32_t U32_lvDataLength = Cl_laExtractedData.size();
           QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
           QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

           qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;
           if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
          {
            QByteArray Cl_laCtOutput   =  Cl_laExtractedData.mid(2,5);
            QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laCtOutput);

            saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());

            ui->textEdit_CToutput->setText(Cl_lvAlphanumericData);
            ui->textEdit_CToutput->setAlignment(Qt::AlignCenter);
            ui->textEdit_CToutput_tx->setText(Cl_lvAlphanumericData);
            ui->textEdit_CToutput_tx->setAlignment(Qt::AlignCenter);
           }
            else
          {
           qDebug()<< "Invalid data";
          }
          Cl_gaDatabuffer.clear();
         }
          else
          {
              qDebug()<< "Incomplete Data ";
              Cl_gaDatabuffer.clear();
          }

          break;
        }
         case 20:

       {
          if(Cl_laReceivedData.size()>8)
         {
           qDebug()<<"Pt input";
           QString Cl_lvParameterName = "Pt input : ";
           QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,8);
           qDebug()<< Cl_laExtractedData;


           qDebug()<<"Pt input data"<<Cl_laReceivedData.toHex();

           QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

           qDebug()<<"Pt input  ReceivedCrc"<<Cl_lvReceivedcrcHex;
           Cl_laExtractedData.chop(2);
           qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

           uint32_t U32_lvDataLength = Cl_laExtractedData.size();
           QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
           QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

           qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

           if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
          {
           QByteArray Cl_laPtInput   =  Cl_laExtractedData.mid(2,5);
           QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laPtInput);

           saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());

           ui->textEdit_PTinput->setText(Cl_lvAlphanumericData);
           ui->textEdit_PTinput->setAlignment(Qt::AlignCenter);
           ui->textEdit_PTinput_tx->setText(Cl_lvAlphanumericData);
           ui->textEdit_PTinput_tx->setAlignment(Qt::AlignCenter);
          }
           else
          {
            qDebug()<< "Invalid data";
          }
          Cl_gaDatabuffer.clear();
        }
          else
          {
              qDebug()<< "Incomplete Data ";
              Cl_gaDatabuffer.clear();
          }

          break;
        }
          case 21:
       {
          if(Cl_laReceivedData.size()>8)
         {
           qDebug()<<"Pt Output";
           QString Cl_lvParameterName = "Pt Output : ";
           QByteArray  Cl_laExtractedData = Cl_laReceivedData.mid(0,8);
           qDebug()<< Cl_laExtractedData;

           qDebug()<<"case 21 data"<< Cl_laExtractedData.toHex();

           QString Cl_lvReceivedcrcHex = Cl_laExtractedData.toHex().right(4);

           qDebug()<<"21 ReceivedCrc"<<Cl_lvReceivedcrcHex;
           Cl_laExtractedData.chop(2);
           qDebug()<<"crc byte for calculation "<< Cl_laExtractedData.toHex();

           uint32_t U32_lvDataLength = Cl_laExtractedData.size();
           QString Cl_gvCalculatedCrc = QString::number(calculateCRC16( Cl_laExtractedData,U32_lvDataLength),16);
           QString Cl_lvcalculatedSwappedCrc =swapLSBAndMSB(Cl_gvCalculatedCrc);

           qDebug()<<"calculated Crc"<<Cl_lvcalculatedSwappedCrc;

           if(Cl_lvcalculatedSwappedCrc==Cl_lvReceivedcrcHex)
          {
            QByteArray Cl_laPtOutput   =  Cl_laExtractedData.mid(2,5);
            QString Cl_lvAlphanumericData = extractAlphanumeric(Cl_laPtOutput);

            saveDataToFile(Cl_lvParameterName,Cl_lvAlphanumericData.toUtf8());

            ui->textEdit_PToutput->setText(Cl_lvAlphanumericData);
            ui->textEdit_PToutput->setAlignment(Qt::AlignCenter);
            ui->textEdit_PToutput_tx->setText(Cl_lvAlphanumericData);
            ui->textEdit_PToutput_tx->setAlignment(Qt::AlignCenter);
          }
           else
          {
            qDebug()<< "Invalid data";
          }
          Cl_gaDatabuffer.clear();
         }
          else
          {
              qDebug()<< "Incomplete Data ";
              Cl_gaDatabuffer.clear();
          }
           break;
        }

      default:
             qDebug()<<"String is unrecognized";
             Cl_gaDatabuffer.clear();

       break;
     }

    }
     else
   {
    qDebug()<<"received string does not contain valid size 0R character";
   }
}

/*********************************************************************************
 *calculateCRC16
 *this function gives crc calculation using polynomial A001
*********************************************************************************/

uint16_t MainWindow:: calculateCRC16(const QByteArray &data, int length)
{
   uint16_t U16_lvCrc = 0xFFFF;            /* Initial CRC value*/

   for (int i = 0; i < length; i++)
  {
    U16_lvCrc ^= static_cast<uint16_t>(data[i]);  /* XOR with the next byte */

    for (int j = 0; j < 8; j++)
    {
      if (U16_lvCrc & 0x0001)
      {
        U16_lvCrc = (U16_lvCrc >> 1) ^ 0xA001;  /* Bitwise right shift and XOR with polynomial */
      }
      else
      {
         U16_lvCrc = U16_lvCrc >> 1;       /* For little-endian:  Bitwise right shift */
      }
    }
  }
   return U16_lvCrc;
}

/*********************************************************************************
 *handleError
 *this function handlles all serial port errors
*********************************************************************************/

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
   if (error == QSerialPort::ResourceError)
  {
    QMessageBox::critical(this, tr("Critical Error"), mSerial->errorString());
    closeSerialPort();
  }
}

/*********************************************************************************
 *on_actionClear_triggered
 *this is ui slot for action clear on menu bar
*********************************************************************************/

void MainWindow::on_actionClear_triggered()
{
    ui->textEdit_driverId->clear();
    ui->textEdit_trainNo->clear();
    ui->textEdit_shedName->clear();
    ui->textEdit_EQUno->clear();
    ui->textEdit_odometer->clear();
    ui->textEdit_trainLoad->clear();
    ui->textEdit_timeSet->clear();
    ui->textEdit_dateSet->clear();
    ui->textEdit_locoNo->clear();
    ui->textEdit_ctFactor->clear();
    ui->textEdit_ptFactor->clear();
    ui->textEdit_wheelSetting->clear();
    ui->textEdit_maxSpeed->clear();
    ui->textEdit_resetCounter->clear();
    ui->textEdit_typeOfLoco->clear();
    ui->textEdit_speed->clear();
    ui->textEdit_voltage->clear();
    ui->textEdit_current->clear();
    ui->textEdit_coustingStatus->clear();
    ui->textEdit_breakingStatus->clear();
    ui->textEdit_PGno->clear();
    ui->textEdit_PTinput->clear();
    ui->textEdit_PToutput->clear();
    ui->textEdit_CTinput->clear();
    ui->textEdit_CToutput->clear();
    ui->textEdit_energyConsumedHalt->clear();
    ui->textEdit_driverSpecificDistance->clear();
    ui->textEdit_driverSpecificEnergy->clear();
    ui->textEdit_disTravelledOdometer->clear();
    ui->textEdit_totalEnergyConsumed->clear();
    ui->textEdit_energyConsumedRunning->clear();
    ui->textEdit_breakingDuration->clear();
    ui->textEdit_coastingDuration->clear();
    ui->textEdit_overSpeedDistance->clear();
    ui->textEdit_overSpeedDuration->clear();
    ui->textEdit_fModeSelection->clear();
}

/*********************************************************************************
 *hexToAscii
 *this function convert hex string to ascii
*********************************************************************************/

QChar MainWindow:: hexToAscii(const QString& hexString)
{
    bool ok;
    int S32_lvasciiValue = hexString.toInt(&ok, 16);

    if (ok)
    {
      return QChar(S32_lvasciiValue);
    }
    else
    {
       /* Handle conversion error, e.g., return a placeholder character*/
       return QChar('?');
    }
}

void MainWindow:: showSuccessMessage(const QString &message)
{
  popupMessage->setText(message);
  popupMessage->show();
  messageTimer->start(700);
}
void MainWindow:: hideSuccessMessage()
{
   popupMessage->hide();
   messageTimer->stop();
}

void MainWindow::writeData(const QByteArray &data)
{
  mSerial->write(data);
}

/*********************************************************************************
 *Next 21 function is slots for push button click event to send queries to master
 *for receiving data response
*********************************************************************************/

void MainWindow::on_DriverIDSend_button_clicked()
{
     QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected ");
    }
    else
    {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
      QString Cl_lvCommonByte ="0x16";
      QString Cl_lvhexValue = "0x01";
      QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
      QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

      QString UC_laPacket =   QString("@")
                    + Cl_lvCommonByteChar
                    + UC8_lvstartBitIdentifier;

       Cl_gaDatatoSend = UC_laPacket.toUtf8();
       int U32_lvDataLength = Cl_gaDatatoSend.size();

       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/

       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");
       UC_laPacket.append("\r");
       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");

       QueryTimer->start(1000);
     }
}

}

void MainWindow::on_TrainNoSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
     QString Cl_lvCommonByte ="0x16";
     QString Cl_lvhexValue = "0x02";
     QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
     QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

     QString UC_laPacket = QString("@")
                        + Cl_lvCommonByteChar
                        + UC8_lvstartBitIdentifier;

      Cl_gaDatatoSend = UC_laPacket.toUtf8();
      int U32_lvDataLength = Cl_gaDatatoSend.size();
      uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

      uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
      uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

      qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

      QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

       qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

      QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
      QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

      QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
      QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

      /* Display the character*/
      qDebug() << LSB;
      qDebug() << MSB;

      UC_laPacket.append(UC_lvextendedAsciiChar1);
      UC_laPacket.append(UC_lvextendedAsciiChar2);
      UC_laPacket.append("\r");

      qDebug()<<"final data" <<UC_laPacket;

      writeData(UC_laPacket.toLatin1());
      showSuccessMessage(" query is Send sucessfully ");

      QueryTimer->start(1000);

        }
    }
}

void MainWindow::on_ShedNameSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
        QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
             QString Cl_lvCommonByte ="0x16";
             QString Cl_lvhexValue = "0x03";
             QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
             QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

            QString UC_laPacket = QString("@")
                       + Cl_lvCommonByteChar
                       + UC8_lvstartBitIdentifier;

             Cl_gaDatatoSend = UC_laPacket.toUtf8();
             int U32_lvDataLength = Cl_gaDatatoSend.size();
             uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

             uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
             uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

             qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

             QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

              qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

             QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
             QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

             QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
             QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

             /* Display the character*/
             qDebug() << LSB;
             qDebug() << MSB;

             UC_laPacket.append(UC_lvextendedAsciiChar1);
             UC_laPacket.append(UC_lvextendedAsciiChar2);
             UC_laPacket.append("\r");

             qDebug()<<"final data" <<UC_laPacket;

             writeData(UC_laPacket.toLatin1());
             showSuccessMessage(" query is Send sucessfully ");

             QueryTimer->start(1000);

        }

    }
}

void MainWindow::on_EquiNoSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {
      if(!F_gvisMasterMode)
     {
      QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
     }
      else
     {
      QString Cl_lvCommonByte ="0x16";
      QString Cl_lvhexValue = "0x04";
      QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
      QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

      QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;

       Cl_gaDatatoSend = UC_laPacket.toUtf8();
       int U32_lvDataLength = Cl_gaDatatoSend.size();
       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/
       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");

       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");
       QueryTimer->start(1000);
    }
  }
}

void MainWindow::on_OdometerSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
      QString Cl_lvCommonByte ="0x16";
      QString Cl_lvhexValue = "0x05";
      QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
      QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

      QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;

       Cl_gaDatatoSend = UC_laPacket.toUtf8();
       int U32_lvDataLength = Cl_gaDatatoSend.size();
       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/
       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");

       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");
       QueryTimer->start(1000);

        }
      }
}

void MainWindow::on_TrainLoadSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
      QString Cl_lvCommonByte ="0x16";
      QString Cl_lvhexValue = "0x06";
      QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
      QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

      QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;

       Cl_gaDatatoSend = UC_laPacket.toUtf8();
       int U32_lvDataLength = Cl_gaDatatoSend.size();
       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/
       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");

       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");
       QueryTimer->start(1000);

        }
     }
}

void MainWindow::on_TimeSetSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
      QString Cl_lvCommonByte ="0x16";
      QString Cl_lvhexValue = "0x07";
      QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
      QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

      QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;


       Cl_gaDatatoSend = UC_laPacket.toUtf8();
       int U32_lvDataLength = Cl_gaDatatoSend.size();
       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/
       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");

       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");
       QueryTimer->start(1000);

        }
     }
}

void MainWindow::on_DateSetSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
{
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
      QString Cl_lvhexValue = "0x08";
      QString Cl_lvCommonByte ="0x16";
      QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
      QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

      QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;

       Cl_gaDatatoSend = UC_laPacket.toUtf8();
       int U32_lvDataLength = Cl_gaDatatoSend.size();
       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/
       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");

       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");
       QueryTimer->start(1000);

        }

}
}

void MainWindow::on_LocoNoSend_buttton_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {

        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
      QString Cl_lvCommonByte ="0x16";
      QString Cl_lvhexValue = "0x09";
      QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
      QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

      QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;


       Cl_gaDatatoSend = UC_laPacket.toUtf8();
       int U32_lvDataLength = Cl_gaDatatoSend.size();
       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/
       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");

       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");
       QueryTimer->start(1000);
        }
     }
}

void MainWindow::on_CtFactorSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {
        if(!F_gvisMasterMode)
        {
          QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
      QString Cl_lvCommonByte ="0x16";
      QString Cl_lvhexValue = "0x0A";
      QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
      QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

      QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;

       Cl_gaDatatoSend = UC_laPacket.toUtf8();
       int U32_lvDataLength = Cl_gaDatatoSend.size();
       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/
       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");

       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");
       QueryTimer->start(1000);

        }
      }
}

void MainWindow::on_PtFactorSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
      QString Cl_lvCommonByte ="0x16";
      QString Cl_lvhexValue = "0x0B";
      QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
      QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

      QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;

       Cl_gaDatatoSend = UC_laPacket.toUtf8();
       int U32_lvDataLength = Cl_gaDatatoSend.size();
       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/
       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");

       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");
       QueryTimer->start(1000);

        }
}
}

void MainWindow::on_WheelSettingSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
      QString Cl_lvCommonByte ="0x16";
      QString Cl_lvhexValue = "0x0C";
      QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
      QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

      QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;

       Cl_gaDatatoSend = UC_laPacket.toUtf8();
       int U32_lvDataLength = Cl_gaDatatoSend.size();
       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/
       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");

       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");
       QueryTimer->start(1000);

        }
      }
}

void MainWindow::on_MaxSpeedSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else{
      QString Cl_lvCommonByte ="0x16";
      QString Cl_lvhexValue = "0x0D";
      QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
      QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

      QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;

       Cl_gaDatatoSend = UC_laPacket.toUtf8();
       int U32_lvDataLength = Cl_gaDatatoSend.size();
       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/
       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");

       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");
       QueryTimer->start(1000);
        }
     }
}

void MainWindow::on_ResetCounterSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
      QString Cl_lvCommonByte ="0x16";
      QString Cl_lvhexValue = "0x0E";
      QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
      QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

      QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;

       Cl_gaDatatoSend = UC_laPacket.toUtf8();
       int U32_lvDataLength = Cl_gaDatatoSend.size();
       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/
       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");

       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");
       QueryTimer->start(1000);

        }
      }
}

void MainWindow::on_LocoTypeSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
      QString Cl_lvCommonByte ="0x16";
      QString Cl_lvhexValue = "0x0F";
      QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
      QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

      QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;


       Cl_gaDatatoSend = UC_laPacket.toUtf8();
       int U32_lvDataLength = Cl_gaDatatoSend.size();
       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/
       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");

       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");
       QueryTimer->start(1000);

        }
      }
}

void MainWindow::on_pgnoSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
   else
    {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
      QString Cl_lvCommonByte ="0x16";
      QString Cl_lvhexValue = "0x11";
      QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
      QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

      QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;

       Cl_gaDatatoSend = UC_laPacket.toUtf8();
       int U32_lvDataLength = Cl_gaDatatoSend.size();
       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/
       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");

       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");
       QueryTimer->start(1000);

        }
  }
}

void MainWindow::on_ctinputSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
        QString Cl_lvCommonByte ="0x16";
        QString Cl_lvhexValue = "0x12";
        QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
        QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

       QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;

       Cl_gaDatatoSend = UC_laPacket.toUtf8();
       int U32_lvDataLength = Cl_gaDatatoSend.size();
       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/
       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");

       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");
       QueryTimer->start(1000);

        }
    }
}


void MainWindow::on_ctoutputSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
       QString Cl_lvCommonByte ="0x16";
       QString Cl_lvhexValue = "0x13";
       QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
       QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

      QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;

       Cl_gaDatatoSend = UC_laPacket.toUtf8();
       int U32_lvDataLength = Cl_gaDatatoSend.size();
       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/

       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");

       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");
       QueryTimer->start(1000);

        }
    }
}
void MainWindow::on_ptinputSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
      QString Cl_lvCommonByte ="0x16";
      QString Cl_lvhexValue = "0x14";
      QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
      QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

      QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;


       Cl_gaDatatoSend = UC_laPacket.toLatin1();
       int U32_lvDataLength = Cl_gaDatatoSend.size();
       uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

       uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
       uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

       qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

       QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

        qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

       QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
       QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

       QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
       QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

       /* Display the character*/
       qDebug() << LSB;
       qDebug() << MSB;

       UC_laPacket.append(UC_lvextendedAsciiChar1);
       UC_laPacket.append(UC_lvextendedAsciiChar2);
       UC_laPacket.append("\r");


       qDebug()<<"final data" <<UC_laPacket;

       writeData(UC_laPacket.toLatin1());
       showSuccessMessage(" query is Send sucessfully ");
       QueryTimer->start(1000);

        }
    }
}


void MainWindow::on_ptoutputSend_button_clicked()
{
    QueryTimer->stop();

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
    else
    {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {

         QString Cl_lvCommonByte ="0x16";
         QString Cl_lvhexValue = "0x15";
         QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
         QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

         QString UC_laPacket = QString("@")
              + Cl_lvCommonByteChar
              + UC8_lvstartBitIdentifier;

         Cl_gaDatatoSend = UC_laPacket.toLatin1();
         int U32_lvDataLength = Cl_gaDatatoSend.size();
         uint16_t U16_lvCrcvalue = calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength);

         uint8_t U8_lvLSBvalue = (U16_lvCrcvalue & 0x00ff);
         uint8_t U8_lvMSBvalue = (U16_lvCrcvalue & 0Xff00)>>8;

         qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue<<"MSB"<<U8_lvMSBvalue;

         QString Cl_gvCalculatedCrc = QString::number(calculateCRC16(Cl_gaDatatoSend,U32_lvDataLength),16);

         qDebug()<<"crc hex " <<Cl_gvCalculatedCrc;

         QChar UC_lvextendedAsciiChar1(U8_lvLSBvalue);
         QChar UC_lvextendedAsciiChar2(U8_lvMSBvalue);

         QString LSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar1);
         QString MSB = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar2);

         /* Display the character*/
         qDebug() << LSB;
         qDebug() << MSB;

         UC_laPacket.append(UC_lvextendedAsciiChar1);
         UC_laPacket.append(UC_lvextendedAsciiChar2);
         UC_laPacket.append("\r");

         qDebug()<<"final data" <<UC_laPacket;

         writeData(UC_laPacket.toLatin1());
         showSuccessMessage(" query is Send sucessfully ");
         QueryTimer->start(1000);

        }
    }
}
void MainWindow::Case16DataToSend()
{

    if(!mSerial->isOpen())
    {
      QMessageBox::critical(this, tr("Error"), "Device is not connected");
    }
   else
   {
        if(!F_gvisMasterMode)
        {
            QMessageBox::critical(this, tr("Error"), "Switch to Master MODE to send Query ");
        }
        else
        {
         QByteArray U64_laDatatoSend;
         QString Cl_lvCommonByte ="0x16";
         QString Cl_lvhexValue = "0x10";
         QChar Cl_lvCommonByteChar = hexToAscii(Cl_lvCommonByte);

         QChar UC8_lvstartBitIdentifier = hexToAscii(Cl_lvhexValue);
         QString UC_laPacket = QString("@")
                              + Cl_lvCommonByteChar
                              + UC8_lvstartBitIdentifier;

         U64_laDatatoSend = UC_laPacket.toUtf8();
         int U32_lvDataLength = U64_laDatatoSend.size();
         uint16_t U16_lvCrcvalue = calculateCRC16(U64_laDatatoSend,U32_lvDataLength);

         uint8_t U8_lvLSBvalue16 = (U16_lvCrcvalue & 0x00ff);
         uint8_t U8_lvMSBvalue16 = (U16_lvCrcvalue & 0Xff00)>>8;

         qDebug()<<"crc value " <<U16_lvCrcvalue<<"lsb"<<U8_lvLSBvalue16<<"MSB"<<U8_lvMSBvalue16;

         QString Cl_gvCalculatedCrc16 = QString::number(calculateCRC16(U64_laDatatoSend,U32_lvDataLength),16);

         qDebug()<<"crc hex " <<Cl_gvCalculatedCrc16;

         QChar UC_lvextendedAsciiCharL(U8_lvLSBvalue16);
         QChar UC_lvextendedAsciiCharM(U8_lvMSBvalue16);

         QString Cl_lvLSB16 = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiCharL);
         QString Cl_lvMSB16 = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiCharM);

         qDebug() << Cl_lvLSB16;
         qDebug() << Cl_lvMSB16;

         UC_laPacket.append(UC_lvextendedAsciiCharL);
         UC_laPacket.append(UC_lvextendedAsciiCharM);
         UC_laPacket.append("\r");


         qDebug()<<"final data" <<UC_laPacket;

         writeData(UC_laPacket.toLatin1());
         showSuccessMessage(" query is Send sucessfully ");
    }

  }
}

void MainWindow::openLoginDialog()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Logout Warning", "Are you sure you want to logout?",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        qDebug() << "Logging out...";
        // Check if the login dialog already exists
        if (!loginDialog) {
            loginDialog = new LoginDialog(this);
            // Connect the login success signal to reopen the main window
            connect(loginDialog, &LoginDialog::loginSuccess, this, &MainWindow::show);
        }
        if (loginDialog)
        {
            loginDialog->show();
        } else {
            qDebug() << "Login dialog pointer is null.";
        }
    }
    else
    {
        qDebug() << "Logging cancel...";
    }
}
void MainWindow:: loginDialogShow()
{
    // Check if the login dialog already exists
    if (!loginDialog) {
        loginDialog = new LoginDialog(this);
        // Connect the login success signal to reopen the main window
        connect(loginDialog, &LoginDialog::loginSuccess, this, &MainWindow::show);
    }
    if (loginDialog)
    {
        loginDialog->show();
    } else {
        qDebug() << "Login dialog pointer is null.";
    }
}

void MainWindow::on_actionAboutApplication_triggered()
{
  QMessageBox::about(this, tr("About This application"),
                    tr("This is <b>Simple Serial Application </b> to Recive data serially "
                      "using Qt Serial Port module and also transmitt data to the serial "
                      "using Qt, with a menu bar, toolbars, and a status bar."));
}

/*********************************************************************************
 *QString MainWindow::swapLSBAndMSB(QString Cl_lvhexValue)

 *this function use to swap lab and msb bit of hex value string
*********************************************************************************/

QString MainWindow::swapLSBAndMSB(QString Cl_lvhexValue)
{
    bool ok;
    uint32_t U32_lvintValue = Cl_lvhexValue.toUInt(&ok, 16);

    if (ok)
    {
      uint32_t U32_lvswappedValue = ((U32_lvintValue << 8U) & 0xFF00U) | ((U32_lvintValue >> 8U) & 0x00FFU);
      return QString("%1").arg(U32_lvswappedValue, 4, 16, QChar('0')).toLower();
    }
    else
    {
      qDebug() << "Invalid hexadecimal value: ";
    }
}

/*********************************************************************************
 *QString MainWindow ::addHexNumbers(QString Cl_lvHexval1, QString Cl_lvHexval2)

 *this function returns an adition of two hex numbers encoded in Qstring
*********************************************************************************/

QString MainWindow ::addHexNumbers(QString Cl_lvHexval1, QString Cl_lvHexval2)
{
    bool ok1, ok2;
    uint8_t U16_lvIntValue1 = Cl_lvHexval1.toUInt(&ok1, 16);
    uint8_t U16_lvIntValue2 = Cl_lvHexval2.toUInt(&ok2, 16);

    if (ok1 && ok2)
    {

        uint16_t U16_lvResult = U16_lvIntValue1 + U16_lvIntValue2;
        U16_lvResult &= 0xFFFF;
        return QString("%1").arg(U16_lvResult, 4, 16, QChar('0')).toUpper();
    }
    else
    {
        qDebug()<<"invalid hex value";
    }
}

/*********************************************************************************
 *QString MainWindow ::addHexNumbers2(QString hex1, QString hex2,QString hex3,QString hex4)

 *this function returns an adition of four hex numbers encoded in Qstring
*********************************************************************************/

QString MainWindow ::addHexNumbers2(QString hex1, QString hex2,QString hex3,QString hex4)
{

    bool ok1, ok2,ok3,ok4;

    uint8_t U8_lvIntValue1 = hex1.toUInt(&ok1, 16);
    uint8_t U8_lvIntValue2 = hex2.toUInt(&ok2, 16);
    uint8_t U8_lvIntValue3 = hex3.toUInt(&ok3, 16);
    uint8_t U8_lvIntValue4 = hex4.toUInt(&ok4, 16);

    if (((ok1 && ok2) && ok3) && ok4)
    {
      uint32_t U32_lvResult = U8_lvIntValue1 + U8_lvIntValue2 + U8_lvIntValue3 + U8_lvIntValue4;
      U32_lvResult &= 0xFFFFFFFF;
      return QString("%1").arg(U32_lvResult, 8, 16, QChar('0')).toUpper();
    }
    else
    {
     qDebug()<<"invalid hex value";
    }
}

/*********************************************************************************
 *QString MainWindow:: extractAlphanumeric(const QByteArray &ReceivedData)

 *this function extract aplha numeric characters from input array
*********************************************************************************/

QString MainWindow:: extractAlphanumeric(const QByteArray &ReceivedData)
{
   /* Define a regular expression pattern to match alphanumeric characters.*/

   QRegularExpression pattern("[a-zA-Z0-9]+");

    /* Create a match iterator to iterate over all matches in the Received Data.*/

   QRegularExpressionMatchIterator matchIterator = pattern.globalMatch(ReceivedData);
   QByteArray Cl_laAlphanumericData;

   while (matchIterator.hasNext())
  {
    /* Iterate through all matches and append the captured alphanumeric characters to the result.*/

     QRegularExpressionMatch match = matchIterator.next();
     Cl_laAlphanumericData += match.captured(0).toUtf8();
   }

    return Cl_laAlphanumericData;
}

/*********************************************************************************
 *void MainWindow:: logout()

 *this function is for logging out
*********************************************************************************/


/*********************************************************************************
 *void MainWindow::drawPointer(double angle,QColor &color)

 *this functions purpose is to create pointer to speedometer image
*********************************************************************************/


void MainWindow::drawPointer(double angle,QColor &color)
{
   QPixmap speedometerPixmap(":/images/Backup_of_new240.png");
   QPainter painter(&speedometerPixmap);
   painter.setRenderHint(QPainter::Antialiasing);

   painter.setPen(QPen(color, 5));   /* Increase the pen width as needed */
   painter.setBrush(color);

   double centerX =speedometerPixmap.width() / 2U;
   double centerY = speedometerPixmap.height() / 2U;

   double pointerLength = 280.0;    /* Adjust the length of the pointer as needed*/

   double x = centerX + pointerLength * qCos(qDegreesToRadians((angle)));
   double y = centerY - pointerLength * qSin(qDegreesToRadians((angle)));

   painter.drawLine(centerX, centerY, x, y);
   ui->image_speed3->setPixmap(speedometerPixmap);
}

/*********************************************************************************
 *updateSpeed
 *this function updates the speed value by type casting and mapping
 *it update pointer color according to received speed
*********************************************************************************/

 void MainWindow:: updateSpeed(int speed)
{
   double maxSpeed = 240.0;
   double rotationAngle = static_cast<double>(speed)* (maxSpeed / 180);

   qDebug()<<"angle"<<rotationAngle;

   QTransform transform;
   transform.rotate(rotationAngle);
   QColor color;

   if (speed >= 0 && speed < 90)
  {
    ui->speed_offLED->setPixmap(QPixmap(":/images/redoff.png"));
     color = Qt::green;
   }
   else if (speed >= 90 && speed < 110)
  {
     color = Qt::yellow;
  }
   else if (speed >= 110 && speed < 150)
  {
    color = QColor(QColorConstants::Svg::orange);
  }
   else if (speed >= 150 && speed < 180)
  {
    color = Qt::red;
  }
   else if (speed >= 180 && speed < 270)
  {
    color = Qt::darkRed;
  }
   else
  {
    color = Qt::blue;
  }

 drawPointer(-149+(-rotationAngle),color);       /*call function*/

}

void MainWindow:: toggleBlink()
{    
   if (*ui->speed_offLED->pixmap() == ledOn)
   {
     ui->speed_offLED->setPixmap(ledOff);
   }
   else
   {
     ui->speed_offLED->setPixmap(ledOn);
   }
}
/*********************************************************************************
 *void MainWindow:: flashText()

 *use to flash text of reset counter
*********************************************************************************/
void MainWindow:: flashText()
{
  static int8_t S8_lcflashCount = 0;

  if (S8_lcflashCount < 14)
    {
        if (S8_lcflashCount % 2 == 0)
        {
          ui->textEdit_resetCounter->setStyleSheet(" color : red  ;");
          ui->textEdit_resetCounter->setFont(QFont( "Consolas", 12));
          ui->textEdit_resetCounter->setText("Counter Reset");
          ui->textEdit_resetCounter->setAlignment(Qt::AlignCenter);
          ui->textEdit_resetCounter_tx->setStyleSheet("color : red  ;");
          ui->textEdit_resetCounter_tx->setFont(QFont( "Consolas", 12));
          ui->textEdit_resetCounter_tx->setText("Counter Reset");
          ui->textEdit_resetCounter_tx->setAlignment(Qt::AlignCenter);
        }
        else
        {
          ui->textEdit_resetCounter->clear();
          ui->textEdit_resetCounter_tx->clear();
        }

        S8_lcflashCount++;
    }
    else
    {
       flashTimer->stop();        /* Stop the timer after 3 seconds */
       S8_lcflashCount = 0;      /* Reset the flash count for the next time */
    }
}

/*********************************************************************************
 *void MainWindow:: ConnectSignalsButton()

 *function to conncect slots of all push buttons for letters and digits
*********************************************************************************/

// void MainWindow:: ConnectSignalsButton()
// {
//      connect(ui->digit_0, &QPushButton::clicked, this, &MainWindow::digitButton_clicked);
//      connect(ui->digit_1, &QPushButton::clicked, this, &MainWindow::digitButton_clicked);
//      connect(ui->digit_2, &QPushButton::clicked, this, &MainWindow::digitButton_clicked);
//      connect(ui->digit_3, &QPushButton::clicked, this, &MainWindow::digitButton_clicked);
//      connect(ui->digit_4, &QPushButton::clicked, this, &MainWindow::digitButton_clicked);
//      connect(ui->digit_5, &QPushButton::clicked, this, &MainWindow::digitButton_clicked);
//      connect(ui->digit_6, &QPushButton::clicked, this, &MainWindow::digitButton_clicked);
//      connect(ui->digit_7, &QPushButton::clicked, this, &MainWindow::digitButton_clicked);
//      connect(ui->digit_8, &QPushButton::clicked, this, &MainWindow::digitButton_clicked);
//      connect(ui->digit_9, &QPushButton::clicked, this, &MainWindow::digitButton_clicked);

//      connect(ui->letter_B, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_A, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_C, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_D, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_E, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_F, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_G, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_H, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_I, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_J, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_K, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_L, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_M, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_N, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_O, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_P, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_Q, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_R, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_S, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_T, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_U, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_V, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_W, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_X, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_Y, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);
//      connect(ui->letter_Z, &QPushButton::clicked, this, &MainWindow::letterButton_clicked);

// }
void MainWindow::ConnectCase16Signals()
{
    connect(ui->SpeedSend_button, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
    connect(ui->CurrentSend_button, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
    connect(ui->VoltageSend_button, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
    connect(ui->CoustingStatusSend_button, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
    connect(ui->BreakingStatusSend_button, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
    connect(ui->totalEnergyConsumedSend_button, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
    connect(ui->disTravelledOdometerSend_button, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
    connect(ui->energyconsumedrunningSend_button, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
    connect(ui->energyConsumedHaltSend_button, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
    connect(ui->driverSpeEnergySend_button, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
    connect(ui->driverSpeDistanceSend, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
    connect(ui->breakingDurationSend, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
    connect(ui->coastingDistanceSend, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
    connect(ui->coastingdurationSend, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
    connect(ui->overspeedDistanceSend, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
    connect(ui->overSpeeddurationSend, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
    connect(ui->fmodeSelectionSend, &QPushButton::clicked, this, &MainWindow::Case16DataToSend);
}

/*********************************************************************************
 *void MainWindow::SendSoftwareQuery()

 *this function sends software query to master for receiving data response
*********************************************************************************/

void MainWindow::SendSoftwareQuery()
{
   QByteArray U64_laDatatoSend;
   QString Cl_lvCommonByte16 ="0x16";
   QString Cl_lvhexValue16 = "0x10";
   QChar Cl_lvCommonByteChar16 = hexToAscii(Cl_lvCommonByte16);

   QChar UC8_lvstartBitIdentifier16 = hexToAscii(Cl_lvhexValue16);
   QString UC_laPacket16 = QString("@")
                 + Cl_lvCommonByteChar16
                 + UC8_lvstartBitIdentifier16;

    U64_laDatatoSend = UC_laPacket16.toUtf8();
    int U32_lvDataLength16 = U64_laDatatoSend.size();
    uint16_t U16_lvCrcvalue16 = calculateCRC16(U64_laDatatoSend,U32_lvDataLength16);

    uint8_t U8_lvLSBvalue16 = (U16_lvCrcvalue16 & 0x00ff);
    uint8_t U8_lvMSBvalue16 = (U16_lvCrcvalue16 & 0Xff00)>>8;

    qDebug()<<"crc value " <<U16_lvCrcvalue16<<"lsb"<<U8_lvLSBvalue16<<"MSB"<<U8_lvMSBvalue16;

    QString Cl_gvCalculatedCrc16 = QString::number(calculateCRC16(U64_laDatatoSend,U32_lvDataLength16),16);

     qDebug()<<"crc hex " <<Cl_gvCalculatedCrc16;

    QChar UC_lvextendedAsciiChar16L(U8_lvLSBvalue16);
    QChar UC_lvextendedAsciiChar16M(U8_lvMSBvalue16);

    QString Cl_lvLSB16 = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar16L);
    QString Cl_lvMSB16 = QString("Extended ASCII character: %1").arg(UC_lvextendedAsciiChar16M);

    qDebug() << Cl_lvLSB16;
    qDebug() << Cl_lvMSB16;

    UC_laPacket16.append(UC_lvextendedAsciiChar16L);
    UC_laPacket16.append(UC_lvextendedAsciiChar16M);
    UC_laPacket16.append("\r");

    qDebug()<<"final data" <<UC_laPacket16;

    writeData(UC_laPacket16.toLatin1());

}
/*********************************************************************************
 * void MainWindow::on_ToggleButton_clicked()

 *this function is a slot for toggle button
 *for switch between Master and slave mode
*********************************************************************************/

void MainWindow::on_ToggleButton_clicked()
{
   if(mSerial->isOpen())
    {
     F_gvisMasterMode = ! F_gvisMasterMode;

    if( F_gvisMasterMode)
    {
      QueryTimer->start(1000);
      ui->ToggleButton->setText("Switched to Master Mode ");
      ui->ToggleButton->setStyleSheet("background-color:rgb(181, 255, 239); font: 600 12pt Segoe UI Semibold");
    }
    else
    {
        QueryTimer->stop();
        ui->ToggleButton->setText("Switched to Slave Mode  ");
        ui->ToggleButton->setStyleSheet("background-color:rgb(148, 255, 129); font: 600 12pt Segoe UI Semibold;");
    }
}
    else
    {
         QMessageBox::critical(this, tr("Error"), "Unable to switch Mode Device is not connected");
    }
}
void MainWindow::closeApplication()
{
    qDebug() << "Closing application...";
    QApplication::quit(); // Close the application
}















// void MainWindow::digitButton_clicked()
// {
//     QPushButton *senderButton = qobject_cast<QPushButton *>(sender());

//     if (senderButton)
//     {
//         int S32_lvdigit = senderButton->text().toInt();
//         US_gaKeypadInput += QString::number(S32_lvdigit);
//         ui->textToDispay->setText(US_gaKeypadInput);
//         ui->textToDispay->setAlignment(Qt::AlignHCenter);
//     }
// }
// void MainWindow::letterButton_clicked()
// {
//     QPushButton *senderButton = qobject_cast<QPushButton *>(sender());

//     if (senderButton)
//     {
//         QString letter = senderButton->text();
//         US_gaKeypadInput += QString(letter);
//         ui->textToDispay->setText(US_gaKeypadInput);
//         ui->textToDispay->setAlignment(Qt::AlignHCenter);
//     }
// }

// void MainWindow::on_Letter_space_clicked()
// {
//     US_gaKeypadInput = ui->textToDispay->toPlainText();
//     US_gaKeypadInput += " ";
//     ui->textToDispay->setText(US_gaKeypadInput);
//     ui->textToDispay->setAlignment(Qt::AlignHCenter);
// }
// void MainWindow::on_backspace_clicked()
// {
//     US_gaKeypadInput = ui->textToDispay->toPlainText();

//     if (!US_gaKeypadInput.isEmpty())
//     {
//          US_gaKeypadInput.chop(1);

//          ui->textToDispay->setText(US_gaKeypadInput);
//          ui->textToDispay->setAlignment(Qt::AlignHCenter);

//        }
//     else
//     {
//         /**/
//     }
// }
// void MainWindow::on_trainNo_clicked()
// {
//     ui->textToDispay->setText(US_gaTrainNo);
//     ui->textToDispay->setAlignment(Qt::AlignHCenter);

//     US_gaTrainNo.clear();
// }

// void MainWindow::on_driverId_clicked()
// {
//     ui->textToDispay->setText(US_gaDriverID);
//     ui->textToDispay->setAlignment(Qt::AlignHCenter);

//     US_gaDriverID.clear();
// }
// void MainWindow::on_trainLoad_clicked()
// {

//     ui->textToDispay->setText(US_gaTrainLoad);
//     ui->textToDispay->setAlignment(Qt::AlignHCenter);

//     US_gaTrainLoad.clear();

// }
// void MainWindow::on_enter_clicked()
// {
//     QString datatoWrite = ui->textToDispay->toPlainText();
//     writeData(datatoWrite.toUtf8());
// }
