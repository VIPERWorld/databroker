#include "module.h"

#include <QDebug>
#include <QDateTime>

Module::Module(const QString& configPath, QObject *parent) : QObject(parent)
{
    configuration = new ModuleConfiguration();
    configuration->loadFromJsonFile(configPath);

    initLiveDataMapFromConfiguration();

    communication = new Communication(configuration);

    connect(communication, &Communication::receivedDataPublished,
            this, &Module::processDataPublishedReceived);

    connect(communication, &Communication::receivedCommand,
            this, &Module::processCommandReceived);
}

Module::~Module()
{
    delete configuration;
    if(communication) {
        delete communication;
    }
}

const ModuleConfiguration *Module::getConfiguration()
{
    return configuration;
}

QVariant Module::getData(const QString &dataId) const
{
    return currentData.value(dataId);
}

Communication *Module::getCommunication() const
{
    return communication;
}

QVariant Module::fromProtoDataTypeToVariant(Broker::Data *data)
{
    if(data->data_type() == Broker::DATA_INT32) {
        return QVariant((int)data->data_int32());
    } else if(data->data_type() == Broker::DATA_BOOLEAN) {
        return QVariant((bool)data->data_bool());
    } else if(data->data_type() == Broker::DATA_DOUBLE) {
        return QVariant((double)data->data_double());
    } else if(data->data_type() == Broker::DATA_STRING) {
        return QVariant(QString((char*)data->data_string().c_str()));
    }
    return QVariant();
}

void Module::fromVariantToProtoDataType(const QVariant &dataVariant, Broker::Data *data)
{
    if(dataVariant.type() == QVariant::Int) {
        data->set_data_type(Broker::DATA_INT32);
        data->set_data_int32(dataVariant.toInt());
    } else if(dataVariant.type() == QVariant::Bool) {
        data->set_data_type(Broker::DATA_BOOLEAN);
        data->set_data_bool(dataVariant.toBool());
    } else if(dataVariant.type() == QVariant::Double) {
        data->set_data_type(Broker::DATA_DOUBLE);
        data->set_data_double(dataVariant.toDouble());
    } else if(dataVariant.type() == QVariant::String) {
        data->set_data_type(Broker::DATA_STRING);
        data->set_data_string(dataVariant.toString().toStdString());
    }
    return;
}

bool Module::sendControlCommand(Broker::ControlCommand *command)
{
    if(communication) {
        return communication->getControlConnection()->sendControlCommand(command);
    }
    return false;
}

bool Module::sendDataPacket(Broker::DataCollection *dataCollection)
{
    if(communication) {
        return communication->getDataConnection()->provideDataConsumed(dataCollection);
    }
    return false;
}

bool Module::isMaster() const
{
    if(configuration) {
        return configuration->getMaster();
    }
    return false;
}

void Module::initLiveDataMapFromConfiguration()
{
    if(configuration) {
        const QMap<QString, DataDescriptor> *map = configuration->getDataPublishedMap();
        for(QString dataId : map->keys()) {
            currentData.insert(dataId, QVariant());
        }
    }
}

void Module::processDataPublishedReceived()
{
    Broker::DataCollection dataPacket;
    communication->getDataConnection()->receiveDataPublished(&dataPacket);

    /* avoid inconsistences due to
     * out of order messages when using
     * UdpDataConnection
     */
    if(lastTimeStamp > dataPacket.timestamp()) {
        qDebug() << __FILE__ << __LINE__ << "[lastTimeStamp > data.timestamp()]";
        return;
    }

    if(configuration) {
        if(dataPacket.provider_name() != configuration->getId().toStdString()) {
            qDebug() << __FILE__ << __LINE__ << "data.name() != module.name()"
                     << configuration->getId() << "!=" << dataPacket.provider_name().c_str();
            return;
        }
    }

    for(int i=0; i<dataPacket.data_provided_size(); i++) {
        Broker::Data dataObject = dataPacket.data_provided().Get(i);
        QString dataId = QString(dataObject.data_id().c_str());
        if(currentData.contains(dataId)) {
            currentData[dataId] = fromProtoDataTypeToVariant(&dataObject);
        }
    }

    qDebug() << "VarianMap" << currentData;
    qDebug() << "==========================================================";
}
