// Qt
#include <QString>
#include <QProcess>
#include <QByteArray>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QFile>
#include <QVariant>
#include <QUrl>

#include "OpenHABSystemStatusReader.h"
#include "Config.h"

void OpenHABSystemStatusReader::runCommand(const Config &config)
{
   QUrl openHABUrl(config.getOpenHABURL());
   openHABUrl.setPath(QStringLiteral("/rest/systeminfo"));

   const QStringList command = { config.getCurlFullPath(),
      QStringLiteral("-X"),
      QStringLiteral("GET"),
      QStringLiteral("--no-progress-meter"),
      QStringLiteral("-H"),
      QStringLiteral("accept: application/json"),
      QStringLiteral("-H"),
      QStringLiteral("Authorization: Bearer ") + config.getOpenHABToken() + QStringLiteral(""),
      openHABUrl.toString() };

   CommandRunner::runCommand(command);
}

void OpenHABSystemStatusReader::commandFinished(int, QProcess::ExitStatus status, const QString &stdoutStr, const QString &stderrStr, const QString &payloadToFinished)
{
   if (status == QProcess::NormalExit)
   {
      QByteArray responseData(stdoutStr.toUtf8());

      QJsonParseError parseError;
      QJsonDocument jsonDoc(QJsonDocument::fromJson(responseData, &parseError));
      if(parseError.error == QJsonParseError::ParseError::NoError)
      {
         qint64 uptime{0}, startLevel{0};

         QJsonObject mainObject = jsonDoc.object();
         if(mainObject.contains(QStringLiteral("systemInfo")))
         {
            QJsonObject systemInfoObject = mainObject[QStringLiteral("systemInfo")].toObject();
            if(systemInfoObject.contains(QStringLiteral("uptime")))
            {
               uptime = systemInfoObject[QStringLiteral("uptime")].toInteger();
            }
            if(systemInfoObject.contains(QStringLiteral("startLevel")))
            {
               startLevel = systemInfoObject[QStringLiteral("startLevel")].toInteger();
            }
         }

         if(uptime != 0 && startLevel != 0)
            emit newOpenHABSystemStatus(uptime, startLevel);
      }

   } else {
      // Handle abnormal termination
      qDebug() << "OpenHABSystemStatusReader: Command terminated abnormally";
   }
}
