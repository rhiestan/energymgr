#ifndef MQ_READER_H
#define MQ_READER_H

#include <QString>
#include <QObject>
#include <QMqttClient>
#include <QMqttMessage>

class MQReader : public QObject
{
   Q_OBJECT
public:
   MQReader();
   virtual ~MQReader();

   void connectToBroker(const QString &url, int port, const QString &subscriptionTopicHP,
      const QString &subscriptionTopicHPPulse, const QString &subscriptionTopicEnergy);

public slots:
   void slotMessageReceived(const QMqttMessage &msg);

signals:
   void newEnergyValues(double total_power, double phase1, double phase2, double phase3, double energyPos, double energyNeg);
   void newHeatPumpPower(double hpPower);
   void newHeatPumpPowerPulse();

private:
   QMqttClient *pMqttClient_{nullptr};
   bool isConnected_{false};

   QString subscriptionTopicHP_, subscriptionTopicHPPulse_, subscriptionTopicEnergy_;
};

#endif // !MQ_READER_H
