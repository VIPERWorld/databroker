#ifndef UDPDATACONNECTION_H
#define UDPDATACONNECTION_H

#include "abstractdataconnection.h"

class UdpDataConnection : public AbstractDataConnection
{
    Q_OBJECT
public:

    UdpDataConnection(QObject *parent = 0);
    UdpDataConnection(const QString& ipAddress, quint16 portNum, QObject *parent = 0);

    virtual ~UdpDataConnection();

    virtual bool initConnection();
    virtual void deInitConnection();
    virtual bool initConnection(QString ipAddress, quint16 portNum);
    virtual bool receiveDataPublished(Broker::DataCollection *dataCollection);
    virtual bool provideDataConsumed(Broker::DataCollection *dataCollection);


private slots:

    void handleDatagram();

private:

    static const int MAX_DATAGRAM_SIZE = 1000;

    bool sendFirstDatagram();
};

#endif // UDPDATACONNECTION_H
