// Qt
#include <QString>
#include <QProcess>
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QVariant>
#include <QUrl>

#include "SendValueToOpenHAB.h"
#include "Config.h"


void SendValueToOpenHAB::runCommand(const Config &config, const QString &valueName, const QString &valueStr)
{
   QUrl openHABUrl(config.getOpenHABURL());
   openHABUrl.setPath(QStringLiteral("/rest/items/") + valueName);

   const QStringList command = { config.getCurlFullPath(),
      QStringLiteral("-X"),
      QStringLiteral("POST"),
      QStringLiteral("--no-progress-meter"),
      QStringLiteral("-H"),
      QStringLiteral("Content-Type: text/plain"),
      QStringLiteral("-H"),
      QStringLiteral("accept: application/json"),
      QStringLiteral("-d"),
      valueStr,
      QStringLiteral("-H"),
      QStringLiteral("Authorization: Bearer ") + config.getOpenHABToken() + QStringLiteral(""),
      openHABUrl.toString() };

   CommandRunner::runCommand(command, valueName);
}

void SendValueToOpenHAB::commandFinished(int, QProcess::ExitStatus status, const QString &stdoutStr, const QString &stderrStr, const QString &payloadToFinished)
{
   if (status != QProcess::NormalExit)
   {
      // Handle abnormal termination
      qDebug() << "SendValueToOpenHAB: Command terminated abnormally";
   }
}
