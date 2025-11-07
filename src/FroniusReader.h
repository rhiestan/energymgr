#include <QString>
#include <QProcess>
#include <QByteArray>
#include <QDebug>
#include <QStringList>

#include "CommandRunner.h"

class Config;

class FroniusReader : public CommandRunner
{
   Q_OBJECT
public:
   explicit FroniusReader(QObject *parent = nullptr);

   void runCommand(const Config &config);

   virtual void commandFinished(int, QProcess::ExitStatus status, const QString &stdoutStr, const QString &stderrStr);

signals:
   void newFroniusDayEnergyValue(double value);
   void newFroniusPACValue(double value);

private:
};
