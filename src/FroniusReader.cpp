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

#include "FroniusReader.h"
#include "Config.h"

FroniusReader::FroniusReader(QObject *parent)
   : CommandRunner(parent)
{
}

void FroniusReader::runCommand(const Config &config)
{
   const QStringList command = { config.getCurlFullPath(),
      QStringLiteral("-X"),
      QStringLiteral("GET"),
      QStringLiteral("--no-progress-meter"),
      config.getFroniusURL() };

   CommandRunner::runCommand(command);
}

void FroniusReader::commandFinished(int, QProcess::ExitStatus status, const QString &stdoutStr, const QString &stderrStr, const QString &payloadToFinished)
{
   if (status == QProcess::NormalExit) {

      QByteArray responseData(stdoutStr.toUtf8());

      QJsonParseError parseError;
      QJsonDocument jsonDoc(QJsonDocument::fromJson(responseData, &parseError));
      if(parseError.error == QJsonParseError::ParseError::NoError)
      {
         QJsonObject mainObject = jsonDoc.object();
         if(mainObject.contains(QStringLiteral("Body")))
         {
            QJsonObject bodyObject = mainObject[QStringLiteral("Body")].toObject();
            if(bodyObject.contains(QStringLiteral("Data")))
            {
               QJsonObject dataObject = bodyObject[QStringLiteral("Data")].toObject();

               if(dataObject.contains(QStringLiteral("DAY_ENERGY")))
               {
                  QJsonObject dayEnergyObject = dataObject[QStringLiteral("DAY_ENERGY")].toObject();
                  QString dayEnergyUnit;
                  double dayEnergy{0};

                  if(dayEnergyObject.contains(QStringLiteral("Unit")))
                     dayEnergyUnit = dayEnergyObject[QStringLiteral("Unit")].toString();
                  if(dayEnergyObject.contains(QStringLiteral("Values")))
                  {
                     QJsonObject valuesObject = dayEnergyObject[QStringLiteral("Values")].toObject();
                     for(const auto &val : valuesObject)
                     {
                        if(val.isDouble())
                           dayEnergy += val.toDouble();
                     }
                  }

                  emit newFroniusDayEnergyValue(dayEnergy);
               }

               if(dataObject.contains(QStringLiteral("PAC")))
               {
                  QJsonObject pacObject = dataObject[QStringLiteral("PAC")].toObject();
                  QString pacUnit;
                  double pac{0};

                  if(pacObject.contains(QStringLiteral("Unit")))
                     pacUnit = pacObject[QStringLiteral("Unit")].toString();
                  if(pacObject.contains(QStringLiteral("Values")))
                  {
                     QJsonObject valuesObject = pacObject[QStringLiteral("Values")].toObject();
                     for(const auto &val : valuesObject)
                     {
                        if(val.isDouble())
                           pac += val.toDouble();
                     }
                  }

                  emit newFroniusPACValue(pac);
               }
            }
         }
      }

   } else {
      // Handle abnormal termination
      qDebug() << "FroniusReader: Command terminated abnormally";
   }
}
