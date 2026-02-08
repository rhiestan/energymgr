#include <QString>
#include <QDebug>

#include "CommandRunner.h"

void CommandRunner::runCommand(const QStringList &command, const QString &payloadToFinished)
{
   ProcessData *pProcessData = new ProcessData(this);
   pProcessData->pProcess = new QProcess(pProcessData);

   // Connect the signals to capture stdout and stderr
   connect(pProcessData->pProcess, &QProcess::readyReadStandardOutput, this, [pProcessData]() {
      pProcessData->output.append(pProcessData->pProcess->readAllStandardOutput());
      });
   connect(pProcessData->pProcess, &QProcess::readyReadStandardError, this, [pProcessData]() {
      pProcessData->errorOutput.append(pProcessData->pProcess->readAllStandardError());
      });

   // Connect the finished signal to handle what happens when the command completes
   connect(pProcessData->pProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, pProcessData, payloadToFinished](int exitCode, QProcess::ExitStatus status)
      {
         commandFinished(exitCode, status, pProcessData->output, pProcessData->errorOutput, payloadToFinished);

         pProcessData->deleteLater();
      });

   // Start the process with the given command
   QStringList args = command;
   QString program = args.takeFirst();
   pProcessData->pProcess->start(program, args);
}

void CommandRunner::commandFinished(int exitCode, QProcess::ExitStatus status, const QString &stdoutStr, const QString &stderrStr, const QString &payloadToFinished)
{
   if(exitCode != 0)
   {
      qCritical() << "CommandRunner: Command finished with non-zero exit code:" << exitCode;
      qCritical() << "CommandRunner: Standard error output:" << stderrStr;
   }
}
