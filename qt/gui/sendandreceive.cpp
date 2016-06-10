#include <QtWidgets>
#include <QtNetwork>

#include "sendandreceive.h"


sendAndReceive::sendAndReceive(QWidget *parent, int listeningPort, int sendingPort)
    : QWidget(parent)
{

    //qStdOut() << "initializing class";

    //timer = new QTimer(this);
    udpSocket = new QUdpSocket(this);
    //messageNo = 1;

    receiverSocket = new QUdpSocket(this);
    receiverSocket->bind(QHostAddress::LocalHost, listeningPort);

    connect(receiverSocket, SIGNAL(readyRead()),
            this, SLOT(processPendingMessages()));


    startButton = new QPushButton(tr("&Start"));
    connect(startButton, SIGNAL(clicked()), this, SLOT(broadcastToStart()));

    stopButton = new QPushButton(tr("&Stop"));
    connect(stopButton, SIGNAL(clicked()), this, SLOT(broadcastToStop()));

    plot = new QCustomPlot();
    QCustomPlot *customPlot;
    customPlot = new QCustomPlot();

    //connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
    //connect(timer, SIGNAL(timeout()), this, SLOT(broadcastDatagram())); //use this later for timed sampling!

    statusLabel = new QLabel(tr("Ready to send and receive messages"));
    statusLabel->setWordWrap(true);


    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(startButton);
    mainLayout->addWidget(stopButton);
    mainLayout->addWidget(plot);
    mainLayout->addWidget(customPlot);

    setLayout(mainLayout);

    //setWindowTitle(tr("Sender and Receiver"));

    //attempting to create a colormap
    QCPColorMap *colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
    customPlot->addPlottable(colorMap);


    colorMap->data()->setSize(500, 500);
      colorMap->data()->setRange(QCPRange(0, 5), QCPRange(0, 5));
      for (int i=0; i<500; ++i)
        for (int j=0; j<500; ++j){

          //grid lines
          if (i % 100 == 0 || i % 100 == 1 || i % 100 == 2 || i % 100 == 98 || i % 100 == 99){
                colorMap->data()->setCell(i, j, 0);
                continue;
              }
          if (j % 100 == 0 || j % 100 == 1 || j % 100 == 2 || j % 100 == 98 || j % 100 == 99){
                colorMap->data()->setCell(i, j, 0);
                continue;
              }


          //subgrid lines
          if (i % 50 == 0 || i % 50 == 1 || i % 50 == 49)
                if(i % 100 != i % 50){
                    colorMap->data()->setCell(i, j, 0);
                    continue;
                }
          if (j % 50 == 0 || j % 50 == 1 || j % 50 == 49)
                if(j % 100 != j % 50){
                    colorMap->data()->setCell(i, j, 0);
                    continue;
                }

          colorMap->data()->setCell(i, j, qCos(i/100.0)+qSin(j/100.0));
       }


      colorMap->setGradient(QCPColorGradient::gpPolar);
      colorMap->rescaleDataRange(true);
      colorMap->setInterpolate(0);
      customPlot->rescaleAxes();
      customPlot->yAxis->grid()->setVisible(1);
      customPlot->yAxis->grid()->setSubGridVisible(1);
      customPlot->replot();


    // setting up the basic plot parameters:
    //QVector<double> x(101), y(101); // initialize with entries 0..100
    x.resize(101);
    y.resize(101);
    for (int i=0; i<101; ++i)
    {
      x[i] = -i/3; //
      y[i] = -1; //
    }
    // create graph and assign data to it:
    plot->addGraph();
    plot->graph(0)->setData(x, y);
    // give the axes some labels:
    plot->xAxis->setLabel("Time ago [seconds]");
    plot->yAxis->setLabel("Temperature");
    // set axes ranges, so we see all data:
    //plot->xAxis->setRange(-1, 1);
    //plot->yAxis->setRange(0, 1);
    plot->graph(0)->setLineStyle(QCPGraph::lsNone);
    plot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 7));
    plot->replot();
    plot->graph(0)->rescaleAxes();

    sendToPort = sendingPort;


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

    if(!QString::compare(instructions, "started", Qt::CaseInsensitive)){
        startButton->setEnabled(false);
        stopButton->setEnabled(true);
    }

    if(!QString::compare(instructions, "stopped", Qt::CaseInsensitive)){
        startButton->setEnabled(true);
        stopButton->setEnabled(false);
    }

    if(!QString::compare(instructions.left(4), "temp", Qt::CaseInsensitive)){
        double temp = instructions.right(instructions.size()-4).toDouble();
        y.remove(100);
        y.insert(0, temp);
        plot->graph(0)->setData(x, y);
        plot->replot();
        plot->graph(0)->rescaleAxes();
    }

}

void sendAndReceive::broadcastToStart()
{

   broadcastAnything("start",sendToPort);

}

void sendAndReceive::broadcastToStop()
{

   broadcastAnything("stop",sendToPort);

}


