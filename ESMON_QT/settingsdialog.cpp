
#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QIntValidator>
#include <QLineEdit>
#include <QSerialPortInfo>

/*********************************************************************************
 *Constructor
*********************************************************************************/

static const char blankString[] = QT_TRANSLATE_NOOP("SettingsDialog", "N/A");

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog),
    m_intValidator(new QIntValidator(0, 4000000, this))
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
    ui->baudRateBox_3->setInsertPolicy(QComboBox::NoInsert);

    connect(ui->applyButton_3, &QPushButton::clicked,
            this, &SettingsDialog::apply);
    connect(ui->serialPortInfoListBox_3, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::showPortInfo);
    connect(ui->baudRateBox_3,  QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::checkCustomBaudRatePolicy);
    connect(ui->serialPortInfoListBox_3, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::checkCustomDevicePathPolicy);

    fillPortsParameters();
    fillPortsInfo();

    updateSettings();
}

/*********************************************************************************
 *Destructor
*********************************************************************************/

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

SettingsDialog::Settings SettingsDialog::settings() const
{
    return m_currentSettings;
}
/**********************************************************************************
 *void SettingsDialog::showPortInfo(int idx)

 *this function show information of all avaliable ports
***********************************************************************************/

void SettingsDialog::showPortInfo(int idx)
{
    if (idx == -1)
        return;

    const QStringList list = ui->serialPortInfoListBox_3->itemData(idx).toStringList();
    ui->descriptionLabel_3->setText(tr("Description: %1").arg(list.count() > 1 ? list.at(1) : tr(blankString)));
    ui->manufacturerLabel_3->setText(tr("Manufacturer: %1").arg(list.count() > 2 ? list.at(2) : tr(blankString)));
    ui->serialNumberLabel_3->setText(tr("Serial number: %1").arg(list.count() > 3 ? list.at(3) : tr(blankString)));
    ui->locationLabel_3->setText(tr("Location: %1").arg(list.count() > 4 ? list.at(4) : tr(blankString)));
    ui->vidLabel_3->setText(tr("Vendor Identifier: %1").arg(list.count() > 5 ? list.at(5) : tr(blankString)));
    ui->pidLabel_3->setText(tr("Product Identifier: %1").arg(list.count() > 6 ? list.at(6) : tr(blankString)));
}

void SettingsDialog::apply()
{
    updateSettings();
    hide();
}

/*********************************************************************************
 *void SettingsDialog::checkCustomBaudRatePolicy(int idx)

 *his function is responsible for handling the custom baud rate behavior in
 *the SettingsDialog.It determines whether the selected baud rate is a custom
 *value or a predefined one.
*********************************************************************************/

void SettingsDialog::checkCustomBaudRatePolicy(int idx)
{
    const bool isCustomBaudRate = !ui->baudRateBox_3->itemData(idx).isValid();
    ui->baudRateBox_3->setEditable(isCustomBaudRate);

    /* If it's a custom baud rate, clear the edit text and set a validator for integers */

    if (isCustomBaudRate)
    {
        ui->baudRateBox_3->clearEditText();
        QLineEdit *edit = ui->baudRateBox_3->lineEdit();
        edit->setValidator(m_intValidator);
    }
    else
    {
        ;
    }
}

/*********************************************************************************
 *void SettingsDialog::checkCustomDevicePathPolicy(int idx)

 * This function is responsible for handling the custom device path behavior in
 * the SettingsDialog. It determines whether the selected device path is a custom
 *  value or a predefined one.
*********************************************************************************/

void SettingsDialog::checkCustomDevicePathPolicy(int idx)
{
    /* Check if the selected device path is a custom value */

    const bool isCustomPath = !ui->serialPortInfoListBox_3->itemData(idx).isValid();
    ui->serialPortInfoListBox_3->setEditable(isCustomPath);
    if (isCustomPath)
        ui->serialPortInfoListBox_3->clearEditText();
}
/*********************************************************************************
 *void SettingsDialog::fillPortsParameters()

* Populates the baud rate combo box with standard baud rates and an option for
* custom baud rate.
*********************************************************************************/

void SettingsDialog::fillPortsParameters()
{
    ui->baudRateBox_3->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);
    ui->baudRateBox_3->addItem(QStringLiteral("19200"), QSerialPort::Baud19200);
    ui->baudRateBox_3->addItem(QStringLiteral("38400"), QSerialPort::Baud38400);
    ui->baudRateBox_3->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);
    ui->baudRateBox_3->addItem(tr("Custom"));

    ui->dataBitsBox_3->addItem(QStringLiteral("5"), QSerialPort::Data5);
    ui->dataBitsBox_3->addItem(QStringLiteral("6"), QSerialPort::Data6);
    ui->dataBitsBox_3->addItem(QStringLiteral("7"), QSerialPort::Data7);
    ui->dataBitsBox_3->addItem(QStringLiteral("8"), QSerialPort::Data8);
    ui->dataBitsBox_3->setCurrentIndex(3);

    ui->parityBox_3->addItem(tr("None"), QSerialPort::NoParity);
    ui->parityBox_3->addItem(tr("Even"), QSerialPort::EvenParity);
    ui->parityBox_3->addItem(tr("Odd"), QSerialPort::OddParity);
    ui->parityBox_3->addItem(tr("Mark"), QSerialPort::MarkParity);
    ui->parityBox_3->addItem(tr("Space"), QSerialPort::SpaceParity);

    ui->stopBitsBox_3->addItem(QStringLiteral("1"), QSerialPort::OneStop);
#ifdef Q_OS_WIN
    ui->stopBitsBox_3->addItem(tr("1.5"), QSerialPort::OneAndHalfStop);
#endif
    ui->stopBitsBox_3->addItem(QStringLiteral("2"), QSerialPort::TwoStop);

    ui->flowControlBox_3->addItem(tr("None"), QSerialPort::NoFlowControl);
    ui->flowControlBox_3->addItem(tr("RTS/CTS"), QSerialPort::HardwareControl);
    ui->flowControlBox_3->addItem(tr("XON/XOFF"), QSerialPort::SoftwareControl);
}

/************************************************************************************
 *void SettingsDialog::fillPortsInfo()

 *This function retrieves information about the available serial ports using
 *QSerialPortInfo,and populates the UI serial port information list box with
 *details such as port name, description,manufacturer, serial number,system location,
 *vendor identifier, and product identifier.
*************************************************************************************/

void SettingsDialog::fillPortsInfo()
{
    ui->serialPortInfoListBox_3->clear();
    QString description;
    QString manufacturer;
    QString serialNumber;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        QStringList list;
        description = info.description();
        manufacturer = info.manufacturer();
        serialNumber = info.serialNumber();
        list << info.portName()
             << (!description.isEmpty() ? description : blankString)
             << (!manufacturer.isEmpty() ? manufacturer : blankString)
             << (!serialNumber.isEmpty() ? serialNumber : blankString)
             << info.systemLocation()
             << (info.vendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : blankString)
             << (info.productIdentifier() ? QString::number(info.productIdentifier(), 16) : blankString);

        ui->serialPortInfoListBox_3->addItem(list.first(), list);
    }

    ui->serialPortInfoListBox_3->addItem(tr("Custom"));
}

/*********************************************************************************
 *void SettingsDialog::updateSettings()

 * Update the settings based on the user input in the settings dialog.
*********************************************************************************/

void SettingsDialog::updateSettings()
{
    m_currentSettings.name = ui->serialPortInfoListBox_3->currentText();

    if (ui->baudRateBox_3->currentIndex() == 4) {
        m_currentSettings.baudRate =ui->baudRateBox_3->currentText().toInt();
    } else {
        m_currentSettings.baudRate = static_cast<QSerialPort::BaudRate>(
                    ui->baudRateBox_3->itemData(ui->baudRateBox_3->currentIndex()).toInt());
    }
    m_currentSettings.stringBaudRate = QString::number(m_currentSettings.baudRate);

    m_currentSettings.dataBits = static_cast<QSerialPort::DataBits>(
                ui->dataBitsBox_3->itemData(ui->dataBitsBox_3->currentIndex()).toInt());
    m_currentSettings.stringDataBits = ui->dataBitsBox_3->currentText();

    m_currentSettings.parity = static_cast<QSerialPort::Parity>(
                ui->parityBox_3->itemData(ui->parityBox_3->currentIndex()).toInt());
    m_currentSettings.stringParity = ui->parityBox_3->currentText();

    m_currentSettings.stopBits = static_cast<QSerialPort::StopBits>(
                ui->stopBitsBox_3->itemData(ui->stopBitsBox_3->currentIndex()).toInt());
    m_currentSettings.stringStopBits = ui->stopBitsBox_3->currentText();

    m_currentSettings.flowControl = static_cast<QSerialPort::FlowControl>(
                ui->flowControlBox_3->itemData(ui->flowControlBox_3->currentIndex()).toInt());
    m_currentSettings.stringFlowControl = ui->flowControlBox_3->currentText();

    m_currentSettings.localEchoEnabled = ui->localEchoCheckBox_3->isChecked();
}
