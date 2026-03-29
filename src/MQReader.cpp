
#include "MQReader.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QFile>
#include <QVariant>

MQReader::MQReader()
{
}

MQReader::~MQReader()
{
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

   connect(pMqttClient_, &QMqttClient::connected, this, [this, &subscriptionTopicHP, &subscriptionTopicEnergy]()
      {
         isConnected_ = true;

         QMqttSubscription *pSubscriptionHP = pMqttClient_->subscribe(subscriptionTopicHP, 2);
         QMqttSubscription *pSubscriptionEnergy = pMqttClient_->subscribe(subscriptionTopicEnergy, 2);

         connect(pSubscriptionHP, &QMqttSubscription::messageReceived, this, &MQReader::slotMessageReceived);
         connect(pSubscriptionEnergy, &QMqttSubscription::messageReceived, this, &MQReader::slotMessageReceived);
      });

   pMqttClient_->connectToHost();
}

void MQReader::slotMessageReceived(const QMqttMessage &msg)
{
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
