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
#include <QMap>

#include "ReadInfluxDBHistoryValue.h"
#include "Config.h"

void ReadInfluxDBHistoryValue::runCommand(const Config &config, const QString &valueName)
{
   QUrl influxDBUrl(config.getInfluxDBURL());
   influxDBUrl.setPath(QStringLiteral("/query"));


   const QStringList command = { config.getCurlFullPath(),
      QStringLiteral("-G"),
      influxDBUrl.toString(),
      QStringLiteral("--no-progress-meter"),
      QStringLiteral("-u"),
      config.getInfluxDBUserPW(),
      QStringLiteral("--data-urlencode"),
      QStringLiteral("db=") + config.getInfluxDBDBName(),
      QStringLiteral("--data-urlencode"),
      QStringLiteral("q=SELECT MAX(\"value\") FROM \"") + valueName + QStringLiteral("\" WHERE time >= now() - 24h")
    };

   CommandRunner::runCommand(command);
}

void ReadInfluxDBHistoryValue::commandFinished(int, QProcess::ExitStatus status, const QString &stdoutStr, const QString &stderrStr, const QString &payloadToFinished)
{
   if (status == QProcess::NormalExit)
   {
      QByteArray responseData(stdoutStr.toUtf8());
      QJsonParseError parseError;
      QJsonDocument jsonDoc(QJsonDocument::fromJson(responseData, &parseError));
      if(parseError.error == QJsonParseError::ParseError::NoError)
      {
         QJsonObject mainObject = jsonDoc.object();
         if(mainObject.contains(QStringLiteral("results")))
         {
            QJsonArray resultsArray = mainObject[QStringLiteral("results")].toArray();
            for(const auto &resultIter : resultsArray)
            {
               QJsonObject resultObject = resultIter.toObject();
               if(resultObject.contains(QStringLiteral("series")))
               {
                  QJsonArray seriesArray = resultObject[QStringLiteral("series")].toArray();
                  for(const auto &seriesIter : seriesArray)
                  {
                     QJsonObject seriesObject = seriesIter.toObject();

                     QString valueName;
                     if(seriesObject.contains(QStringLiteral("name")))
                        valueName = seriesObject[QStringLiteral("name")].toString();

                     QStringList columnNames;
                     QMap<QString, QVariant> columnValueMap;

                     if(seriesObject.contains(QStringLiteral("columns")))
                     {
                        QJsonArray columnsArray = seriesObject[QStringLiteral("columns")].toArray();
                        for(const auto &columnsIter : columnsArray)
                        {
                           QString columnName = columnsIter.toString();
                           columnNames.push_back(columnName);
                        }
                     }

                     if(seriesObject.contains(QStringLiteral("values")))
                     {
                        QJsonArray valuesArray = seriesObject[QStringLiteral("values")].toArray();
                        for(const auto &valuesRowIter : valuesArray)
                        {
                           int columnIndex = 0;
                           QJsonArray valuesRowArray = valuesRowIter.toArray();
                           for(const auto &valuesCol : valuesRowArray)
                           {
                              QVariant val = valuesCol.toVariant();
                              const QString &columnName = columnNames[columnIndex];
                              columnValueMap.insert(columnName, val);

                              if(valuesCol.isDouble())
                              {
                                 double value = valuesCol.toDouble();
                              }
                              else if(valuesCol.isString())
                              {
                                 QString valuesStr = valuesCol.toString();
                              }

                              columnIndex++;
                           }
                        }
                     }

                     if(columnValueMap.contains(QStringLiteral("max")))
                     {
                        double value = columnValueMap[QStringLiteral("max")].toDouble();
                        emit valueFromInfluxDBHistory(valueName, value);
                     }
                  }
               }
            }
         }
      }

   } else {
      // Handle abnormal termination
      qDebug() << "ReadInfluxDBHistoryValue: Command terminated abnormally";
   }
}
