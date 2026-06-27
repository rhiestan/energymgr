
#include "MQReader.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QFile>
#include <QVariant>
#include <QDebug>
#include <QTimer>

MQReader::MQReader()
{
}

MQReader::~MQReader()
{
}

static QString clientStateToString(QMqttClient::ClientState state)
{
   switch(state)
   {
   case QMqttClient::Disconnected: return QStringLiteral("Disconnected");
   case QMqttClient::Connecting:   return QStringLiteral("Connecting");
   case QMqttClient::Connected:    return QStringLiteral("Connected");
   }
   return QStringLiteral("Unknown");
}

static QString clientErrorToString(QMqttClient::ClientError error)
{
   switch(error)
   {
   case QMqttClient::NoError:                    return QStringLiteral("NoError");
   case QMqttClient::InvalidProtocolVersion:     return QStringLiteral("InvalidProtocolVersion");
   case QMqttClient::IdRejected:                 return QStringLiteral("IdRejected");
   case QMqttClient::ServerUnavailable:          return QStringLiteral("ServerUnavailable");
   case QMqttClient::BadUsernameOrPassword:      return QStringLiteral("BadUsernameOrPassword");
   case QMqttClient::NotAuthorized:              return QStringLiteral("NotAuthorized");
   case QMqttClient::TransportInvalid:           return QStringLiteral("TransportInvalid");
   case QMqttClient::ProtocolViolation:          return QStringLiteral("ProtocolViolation");
   case QMqttClient::UnknownError:               return QStringLiteral("UnknownError");
   case QMqttClient::Mqtt5SpecificError:         return QStringLiteral("Mqtt5SpecificError");
   }
   return QStringLiteral("Unknown");
}

QString MQReader::connectionStateString() const
{
   if(!pMqttClient_)
      return QStringLiteral("NoClient");
   return clientStateToString(pMqttClient_->state());
}

void MQReader::connectToBroker(const QString &url, int port, const QString &subscriptionTopicHP,
   const QString &subscriptionTopicHPPulse, const QString &subscriptionTopicEnergy)
{
   subscriptionTopicHP_ = subscriptionTopicHP;
   subscriptionTopicHPPulse_ = subscriptionTopicHPPulse;
   subscriptionTopicEnergy_ = subscriptionTopicEnergy;

   pMqttClient_ = new QMqttClient(this);
   pMqttClient_->setHostname(url);
   pMqttClient_->setPort(port);

   qInfo().noquote() << "MQReader: connecting to broker" << url << "port" << port
      << "topics:" << subscriptionTopicHP_ << "," << subscriptionTopicHPPulse_ << "," << subscriptionTopicEnergy_;

   // NOTE: capture members (not the function parameters) so nothing dangles when
   // the connected signal fires asynchronously.
   connect(pMqttClient_, &QMqttClient::connected, this, [this]()
      {
         isConnected_ = true;
         qInfo() << "MQReader: connected to broker, subscribing to topics";
         subscribeToTopics();
      });

   // These are the key diagnostics for "stops updating after weeks": if the broker
   // connection drops or errors, all energy input silently stops unless we reconnect.
   connect(pMqttClient_, &QMqttClient::disconnected, this, [this]()
      {
         isConnected_ = false;
         qWarning().noquote() << "MQReader: DISCONNECTED from broker (state:" << connectionStateString()
            << ", error:" << clientErrorToString(pMqttClient_->error())
            << ", messages received so far:" << messageCount_ << ")";
         // Schedule a reconnect; without this the service would never recover.
         QTimer::singleShot(5000, this, [this]()
            {
               if(pMqttClient_->state() == QMqttClient::Disconnected)
               {
                  qInfo() << "MQReader: attempting to reconnect to broker...";
                  pMqttClient_->connectToHost();
               }
            });
      });

   connect(pMqttClient_, &QMqttClient::errorChanged, this, [this](QMqttClient::ClientError error)
      {
         if(error != QMqttClient::NoError)
            qWarning().noquote() << "MQReader: client error:" << clientErrorToString(error);
      });

   connect(pMqttClient_, &QMqttClient::stateChanged, this, [this](QMqttClient::ClientState state)
      {
         qInfo().noquote() << "MQReader: connection state changed to" << clientStateToString(state);
      });

   pMqttClient_->connectToHost();
}

void MQReader::subscribeToTopics()
{
   QMqttSubscription *pSubscriptionHP = pMqttClient_->subscribe(subscriptionTopicHP_, 2);
   QMqttSubscription *pSubscriptionHPPulse = pMqttClient_->subscribe(subscriptionTopicHPPulse_, 2);
   QMqttSubscription *pSubscriptionEnergy = pMqttClient_->subscribe(subscriptionTopicEnergy_, 2);

   if(!pSubscriptionHP || !pSubscriptionHPPulse || !pSubscriptionEnergy)
   {
      qCritical().noquote() << "MQReader: subscription FAILED - HP:" << (pSubscriptionHP != nullptr)
         << "HPPulse:" << (pSubscriptionHPPulse != nullptr) << "Energy:" << (pSubscriptionEnergy != nullptr);
   }

   if(pSubscriptionHP)
      connect(pSubscriptionHP, &QMqttSubscription::messageReceived, this, &MQReader::slotMessageReceived);
   if(pSubscriptionHPPulse)
      connect(pSubscriptionHPPulse, &QMqttSubscription::messageReceived, this, &MQReader::slotMessageReceived);
   if(pSubscriptionEnergy)
      connect(pSubscriptionEnergy, &QMqttSubscription::messageReceived, this, &MQReader::slotMessageReceived);
}

void MQReader::slotMessageReceived(const QMqttMessage &msg)
{
   ++messageCount_;
   lastMessageTimer_.restart();

   QByteArray payload = msg.payload();
   if(!payload.isEmpty())
   {
      if(msg.topic().name() == subscriptionTopicEnergy_)
      {
         QJsonDocument jsonDoc(QJsonDocument::fromJson(payload));
         QJsonObject mainObject = jsonDoc.object();

         double total_power{0}, phase1{0}, phase2{0}, phase3{0};
         double energyPos{0}, energyNeg{0};

         if(mainObject.contains(QStringLiteral("total_power")))
            total_power = mainObject[QStringLiteral("total_power")].toDouble();
         if(mainObject.contains(QStringLiteral("phase1")))
            phase1 = mainObject[QStringLiteral("phase1")].toDouble();
         if(mainObject.contains(QStringLiteral("phase2")))
            phase2 = mainObject[QStringLiteral("phase2")].toDouble();
         if(mainObject.contains(QStringLiteral("phase3")))
            phase3 = mainObject[QStringLiteral("phase3")].toDouble();
         if(mainObject.contains(QStringLiteral("energyPos")))
            energyPos = mainObject[QStringLiteral("energyPos")].toDouble();
         if(mainObject.contains(QStringLiteral("energyNeg")))
            energyNeg = mainObject[QStringLiteral("energyNeg")].toDouble();

         emit newEnergyValues(total_power, phase1, phase2, phase3,
            energyPos, energyNeg);
      }
      else if(msg.topic().name() == subscriptionTopicHP_)
      {
         bool isOk{true};
         QString hpValueStr(payload);
         double hpPower = hpValueStr.toDouble(&isOk);
         if(isOk && hpPower > 0)
            emit newHeatPumpPower(hpPower);
      }
      else if(msg.topic().name() == subscriptionTopicHPPulse_)
      {
         emit newHeatPumpPowerPulse();
      }
   }
}
