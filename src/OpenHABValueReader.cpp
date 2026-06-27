// Qt
#include <QString>
#include <QProcess>
#include <QByteArray>
#include <QUrl>
#include <QDebug>

#include "OpenHABValueReader.h"
#include "Config.h"

void OpenHABValueReader::runCommand(const Config &config, const QString &valueName)
{
   QUrl openHABUrl(config.getOpenHABURL());
   openHABUrl.setPath(QStringLiteral("/rest/items/") + valueName + QStringLiteral("/state"));

   const QStringList command = { config.getCurlFullPath(),
      QStringLiteral("-X"),
      QStringLiteral("GET"),
      QStringLiteral("--no-progress-meter"),
      QStringLiteral("--connect-timeout"),
      QStringLiteral("5"),
      QStringLiteral("--max-time"),
      QStringLiteral("10"),
      QStringLiteral("-H"),
      QStringLiteral("accept: text/plain"),
      QStringLiteral("-H"),
      QStringLiteral("Authorization: Bearer ") + config.getOpenHABToken() + QStringLiteral(""),
      openHABUrl.toString() };

   CommandRunner::runCommand(command, valueName);
}

void OpenHABValueReader::commandFinished(int, QProcess::ExitStatus status, const QString &stdoutStr, const QString &stderrStr, const QString &payloadToFinished)
{
   if (status == QProcess::NormalExit)
   {
      bool isOk{true};
      double val = stdoutStr.toDouble(&isOk);
      if(isOk)
         emit valueRead(payloadToFinished, val);
      else
         qDebug() << "OpenHABValueReader: received non-numerical value";
   } else {
      // Handle abnormal termination
      qDebug() << "OpenHABValueReader: Command terminated abnormally";
   }
}
