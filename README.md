Serial Communication Manager

A robust application for managing serial communication via RS232 and RS485 protocols. This tool allows users to read and send data through serial ports, featuring a secure login window for 

authorized access. Built with C++ and Qt Framework, it provides an intuitive interface for seamless data handling and monitoring.


Table of Contents

Features

Prerequisites

Installation

Configuration

Usage


Contributing

License

Contact

Features

Dual Protocol Support: Communicate using RS232 and RS485 serial protocols.

Data Logging: Automatically log incoming and outgoing data for monitoring and analysis.

Secure Login: Access the application through a secure login window to ensure data integrity.

User-Friendly Interface: Intuitive Qt-based GUI for easy navigation and operation.

Real-Time Data Transmission: Send and receive data in real-time with minimal latency.

Configuration Management: Save and load serial port configurations for different setups.

Prerequisites

Before you begin, ensure you have met the following requirements:


Operating System: Windows, macOS, or Linux

Qt Framework: Version 5.15.2 or higher

C++ Compiler: Compatible with your Qt version (e.g., GCC, MSVC)

Serial Port Hardware: RS232 or RS485 compatible devices

Git: For cloning the repository

Installation

1. Clone the Repository
2. 
Open your terminal or command prompt and run:


bash

Copy code

git clone https://github.com/PritiPawar16/Serial-Port-Qt.git

2. Navigate to the Project Directory
3. 
bash

Copy code

cd serial-communication-manager

5. Install Dependencies
   
Ensure you have the necessary Qt components installed. If you haven't installed Qt yet, download it from the official website.


7. Open the Project in Qt Creator

Launch Qt Creator.

Click on File > Open File or Project....

Select the SerialCommunicationManager.pro file from the cloned repository.

9. Configure the Project
10. 
Choose the appropriate Qt kit for your environment.

Configure build settings as needed.

12. Build the Project
13. 
Click the Build button or press Ctrl + B to compile the project.


Configuration

Setting Up Serial Ports

Open the Application: After building, run the application.

Navigate to Settings: Click on the Settings tab.

Configure Ports:

Select the desired serial port (RS232 or RS485).

Set parameters such as baud rate, parity, data bits, and stop bits.

Save Configuration: Click Save to store your settings for future use.

Usage

Logging In

Launch the Application.

Login Window: Enter your username and password.

Authenticate: Click Login to access the main interface.

Note: Ensure you have valid credentials. Contact the administrator if you encounter issues.

Reading Data from Serial Port

Select Serial Port: Choose between RS232 or RS485 from the dropdown menu.

Start Listening: Click the Start button to begin reading data.

View Data: Incoming data will appear in the Data Log section in real-time.

Sending Data to Serial Port

Input Data: Enter the data you wish to send in the Send Data field.

Send Data: Click the Send button to transmit data through the selected serial port.

Confirmation: Sent data will appear in the Data Log with a timestamp.

Data Logging

All incoming and outgoing data is automatically logged.

Access logs through the Logs tab for review and analysis.

Screenshots

Login Window


Main Application Interface


Serial Port Configuration Settings


Contributing

Contributions are welcome! To contribute to this project, follow these steps:


Fork the Repository


Click the Fork button at the top right of this page.


Clone Your Fork


bash

Copy code

git clone https://github.com/PritiPawar16/Serial-Port-Qt.git

Create a New Branch


bash

Copy code







Implement your feature or bug fix.

Commit Your Changes
bash
Copy code

Open a Pull Request

Go to the original repository and click Compare & pull request.
