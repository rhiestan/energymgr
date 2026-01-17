
#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

class Config
{
public:
   Config();
   ~Config();

   void loadFromFile(const QString &filename);

   const QString &getMQTTUri() const { return mqtt_uri_; }
   int getMQTTPort() const { return mqtt_port_; }
   const QString &getMQTTTopicHP() const { return mqtt_topicHP_; }
   const QString &getMQTTTopicEnergy() const { return mqtt_topicEnergy_; }
   const QString &getDBFileName() const { return dbFilename_; }
   const QString &getCurlFullPath() const { return curlFullPath_; }
   const QString &getOpenHABURL() const { return openHABURL_; }
   const QString &getOpenHABToken() const { return openHABToken_; }
   const QString &getOpenHABVirtualPositiveSwitch() const { return openHABVirtualPositiveSwitch_; }
   const QString &getInfluxDBURL() const { return influxDBURL_; }
   const QString &getInfluxDBUserPW() const { return influxDBUserPW_; }
   const QString &getInfluxDBDBName() const { return influxDBDBName_; }
   const double getMinIntervalPosSwitch() const { return minIntervalPosSwitch_; }
   const QString &getFroniusURL() const { return froniusURL_; }

private:
   QString mqtt_uri_;
   int mqtt_port_;
   QString mqtt_topicHP_;
   QString mqtt_topicEnergy_;
   QString dbFilename_;
   QString curlFullPath_;
   QString openHABURL_;
   QString openHABToken_;
   QString openHABVirtualPositiveSwitch_;
   QString influxDBURL_;
   QString influxDBUserPW_;
   QString influxDBDBName_;
   double minIntervalPosSwitch_{0.5};
   QString froniusURL_;
};

#endif /* CONFIG_H */
