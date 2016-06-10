#include <QtWidgets>
#include <QtNetwork>

#include "sendandreceive.h"


sendAndReceive::sendAndReceive(QWidget *parent, int listeningPort, int sendingPort)
    : QWidget(parent)
{

    //qStdOut() << "initializing class";

    timer = new QTimer(this);
    udpSocket = new QUdpSocket(this);
    //messageNo = 1;

    receiverSocket = new QUdpSocket(this);
    receiverSocket->bind(QHostAddress::LocalHost, listeningPort);

    connect(receiverSocket, SIGNAL(readyRead()),
            this, SLOT(processPendingMessages()));


    //startButton = new QPushButton(tr("&Start"));
    //connect(startButton, SIGNAL(clicked()), this, SLOT(broadcastToStart()));
    //connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(timer, SIGNAL(timeout()), this, SLOT(getTemp())); //use this later for timed sampling!

    statusLabel = new QLabel(tr("Ready to send and receive messages"));
    statusLabel->setWordWrap(true);


    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(statusLabel);
    //mainLayout->addWidget(startButton);
    setLayout(mainLayout);

    //setWindowTitle(tr("Sender and Receiver"));

    sendToPort = sendingPort;


}

double sendAndReceive::getTemp()
{
    int tempRange = 50;
    double randomValue = 70 + qrand() % tempRange;
    //statusLabel->setText("sending Temp");
    QString s = "Temp" + QString::number(randomValue);
    broadcastAnything(s.toLatin1(), sendToPort);
    return randomValue;
}

void sendAndReceive::broadcastAnything(QByteArray message, int portNum)
{
    udpSocket->writeDatagram(message.data(), message.size(),
                             QHostAddress::LocalHost, portNum);
}

void sendAndReceive::processPendingMessages()
{
    while (receiverSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(receiverSocket->pendingDatagramSize());
        receiverSocket->readDatagram(datagram.data(), datagram.size());
        statusLabel->setText(tr("Received datagram: \"%1\"")
                             .arg(datagram.data()));
        interpretInstructions(QString::fromStdString(datagram.data()));
    }
}

void sendAndReceive::interpretInstructions(QString instructions)
{

    if(!QString::compare(instructions, "start", Qt::CaseInsensitive)){
        timer->start(300);
        broadcastAnything("started", sendToPort);
        //statusLabel->setText(tr("sending Temp"));
        //cout << "sending temp";
    }

    if(!QString::compare(instructions, "stop", Qt::CaseInsensitive)){
        timer->stop();
        broadcastAnything("stopped", sendToPort);
        //statusLabel->setText(tr("sending Temp"));
        //cout << "sending temp";
    }

}



