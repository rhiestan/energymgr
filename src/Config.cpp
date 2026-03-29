
#include "Config.h"

// Qt
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QFile>
#include <QVariant>

Config::Config()
{
}

Config::~Config()
{
}

void Config::loadFromFile(const QString &filename)
{
   QFile loadFile(filename);
   loadFile.open(QIODeviceBase::OpenModeFlag::ReadOnly);
   QByteArray fileData = loadFile.readAll();

   QJsonDocument jsonDoc(QJsonDocument::fromJson(fileData));
   QJsonObject mainObject = jsonDoc.object();

   if(mainObject.contains(QStringLiteral("mqtt_uri")))
      mqtt_uri_ = mainObject[QStringLiteral("mqtt_uri")].toString();
   if(mainObject.contains(QStringLiteral("mqtt_port")))
      mqtt_port_ = mainObject[QStringLiteral("mqtt_port")].toInt();
   if(mainObject.contains(QStringLiteral("mqtt_topicHP")))
      mqtt_topicHP_ = mainObject[QStringLiteral("mqtt_topicHP")].toString();
   if(mainObject.contains(QStringLiteral("mqtt_topicHPPulse")))
      mqtt_topicHPPulse_ = mainObject[QStringLiteral("mqtt_topicHPPulse")].toString();
   if(mainObject.contains(QStringLiteral("mqtt_topicEnergy")))
      mqtt_topicEnergy_ = mainObject[QStringLiteral("mqtt_topicEnergy")].toString();
   if(mainObject.contains(QStringLiteral("dbFilename")))
      dbFilename_ = mainObject[QStringLiteral("dbFilename")].toString();
   if(mainObject.contains(QStringLiteral("curlFullPath")))
      curlFullPath_ = mainObject[QStringLiteral("curlFullPath")].toString();
   if(mainObject.contains(QStringLiteral("openHAB_URL")))
      openHABURL_ = mainObject[QStringLiteral("openHAB_URL")].toString();
   if(mainObject.contains(QStringLiteral("openHAB_Token")))
      openHABToken_ = mainObject[QStringLiteral("openHAB_Token")].toString();
   if(mainObject.contains(QStringLiteral("openHAB_virtualPositiveSwitch")))
      openHABVirtualPositiveSwitch_ = mainObject[QStringLiteral("openHAB_virtualPositiveSwitch")].toString();
   if(mainObject.contains(QStringLiteral("influxDB_URL")))
      influxDBURL_ = mainObject[QStringLiteral("influxDB_URL")].toString();
   if(mainObject.contains(QStringLiteral("influxDB_UserPW")))
      influxDBUserPW_ = mainObject[QStringLiteral("influxDB_UserPW")].toString();
   if(mainObject.contains(QStringLiteral("influxDB_DBName")))
      influxDBDBName_ = mainObject[QStringLiteral("influxDB_DBName")].toString();
   if(mainObject.contains(QStringLiteral("minIntervalPosSwitch")))
      minIntervalPosSwitch_ = mainObject[QStringLiteral("minIntervalPosSwitch")].toDouble();
   if(mainObject.contains(QStringLiteral("froniusURL")))
      froniusURL_ = mainObject[QStringLiteral("froniusURL")].toString();
}
