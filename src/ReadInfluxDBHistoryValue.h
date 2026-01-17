#include <QString>
#include <QProcess>
#include <QByteArray>
#include <QDebug>
#include <QStringList>

#include "CommandRunner.h"

class Config;

class ReadInfluxDBHistoryValue : public CommandRunner
{
   Q_OBJECT
public:
   explicit ReadInfluxDBHistoryValue(QObject *parent = nullptr) : CommandRunner(parent) {}

   void runCommand(const Config &config, const QString &valueName);

   void commandFinished(int, QProcess::ExitStatus status, const QString &stdoutStr, const QString &stderrStr, const QString &payloadToFinished) override;

signals:
   void valueFromInfluxDBHistory(const QString &valueName, double value);

private:
};
