#ifndef MQ_READER_H
#define MQ_READER_H

#include <QString>
#include <QObject>
#include <QMqttClient>
#include <QMqttMessage>
#include <QElapsedTimer>

class MQReader : public QObject
{
   Q_OBJECT
public:
   MQReader();
   virtual ~MQReader();

   void connectToBroker(const QString &url, int port, const QString &subscriptionTopicHP,
      const QString &subscriptionTopicHPPulse, const QString &subscriptionTopicEnergy);

   /** Total number of MQTT messages received since startup. */
   quint64 messageCount() const { return messageCount_; }
   /** Seconds since the last MQTT message was received (-1 if none yet). */
   qint64 secondsSinceLastMessage() const
   {
      return lastMessageTimer_.isValid() ? lastMessageTimer_.elapsed() / 1000 : -1;
   }
   /** Human-readable current connection state of the MQTT client. */
   QString connectionStateString() const;

public slots:
   void slotMessageReceived(const QMqttMessage &msg);

signals:
   void newEnergyValues(double total_power, double phase1, double phase2, double phase3, double energyPos, double energyNeg);
   void newHeatPumpPower(double hpPower);
   void newHeatPumpPowerPulse();

private:
   void subscribeToTopics();

   QMqttClient *pMqttClient_{nullptr};
   bool isConnected_{false};

   QString subscriptionTopicHP_, subscriptionTopicHPPulse_, subscriptionTopicEnergy_;

   quint64 messageCount_{0};
   QElapsedTimer lastMessageTimer_;
};

#endif // !MQ_READER_H
