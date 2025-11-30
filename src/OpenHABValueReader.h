#ifndef OPENHAB_VALUE_READER_H
#define OPENHAB_VALUE_READER_H

#include <QString>
#include <QProcess>
#include <QByteArray>
#include <QDebug>
#include <QStringList>

#include "CommandRunner.h"

class Config;

class OpenHABValueReader : public CommandRunner
{
   Q_OBJECT
public:
   explicit OpenHABValueReader(QObject *parent = nullptr) : CommandRunner(parent) {}

   void runCommand(const Config &config, const QString &valueName);

   void commandFinished(int, QProcess::ExitStatus status, const QString &stdoutStr, const QString &stderrStr, const QString &payloadToFinished) override;

signals:
   void valueRead(const QString &valueName, double value);

private:
};

#endif // !OPENHAB_VALUE_READER_H
