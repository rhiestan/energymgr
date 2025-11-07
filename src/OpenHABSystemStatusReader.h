#ifndef OPENHAB_SYSTEM_STATUS_READER_H
#define OPENHAB_SYSTEM_STATUS_READER_H

#include <QString>
#include <QProcess>
#include <QByteArray>
#include <QDebug>
#include <QStringList>

#include "CommandRunner.h"

class Config;

class OpenHABSystemStatusReader : public CommandRunner
{
   Q_OBJECT
public:
   explicit OpenHABSystemStatusReader(QObject *parent = nullptr) : CommandRunner(parent) {}

   void runCommand(const Config &config);

   virtual void commandFinished(int, QProcess::ExitStatus status, const QString &stdoutStr, const QString &stderrStr);

signals:
   void newOpenHABSystemStatus(qint64 uptime, qint64 runLevel);

private:
};

#endif // !OPENHAB_SYSTEM_STATUS_READER_H
