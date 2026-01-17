#ifndef SEND_VALUE_TO_OPENHAB_H
#define SEND_VALUE_TO_OPENHAB_H

#include <QString>
#include <QProcess>
#include <QByteArray>
#include <QDebug>
#include <QStringList>

#include "CommandRunner.h"

class Config;


class SendValueToOpenHAB : public CommandRunner {
   Q_OBJECT
public:
   explicit SendValueToOpenHAB(QObject *parent = nullptr) : CommandRunner(parent) {}

   void runCommand(const Config &config, const QString &valueName, const QString &valueStr);

   void commandFinished(int, QProcess::ExitStatus status, const QString &stdoutStr, const QString &stderrStr, const QString &payloadToFinished) override;

private:
};

#endif // !SEND_VALUE_TO_OPENHAB_H
