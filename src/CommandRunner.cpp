#include <QString>
#include <QDebug>
#include <QTimer>

#include "CommandRunner.h"

std::atomic<int> CommandRunner::runningProcessCount_{0};

void CommandRunner::runCommand(const QStringList &command, const QString &payloadToFinished)
{
   ProcessData *pProcessData = new ProcessData(this);
   pProcessData->pProcess = new QProcess(pProcessData);
   pProcessData->commandName = payloadToFinished;

   // Connect the signals to capture stdout and stderr
   connect(pProcessData->pProcess, &QProcess::readyReadStandardOutput, this, [pProcessData]() {
      pProcessData->output.append(pProcessData->pProcess->readAllStandardOutput());
      });
   connect(pProcessData->pProcess, &QProcess::readyReadStandardError, this, [pProcessData]() {
      pProcessData->errorOutput.append(pProcessData->pProcess->readAllStandardError());
      });

   // Log (and recover from) processes that fail to even start, e.g. curl missing.
   connect(pProcessData->pProcess, &QProcess::errorOccurred, this, [this, pProcessData](QProcess::ProcessError error)
      {
         // FailedToStart means finished() will never fire, so clean up here.
         if(error == QProcess::FailedToStart)
         {
            qCritical().noquote() << "CommandRunner: process failed to start:" << pProcessData->program
               << "(" << pProcessData->commandName << ") error:" << pProcessData->pProcess->errorString();
            runningProcessCount_.fetch_sub(1);
            pProcessData->deleteLater();
         }
         else
         {
            qWarning().noquote() << "CommandRunner: process error on" << pProcessData->program
               << "(" << pProcessData->commandName << "):" << pProcessData->pProcess->errorString();
         }
      });

   // Watchdog: detect commands that hang (no --max-time on curl). This is the
   // prime suspect for the service slowly leaking processes until it stops working.
   pProcessData->pWatchdog = new QTimer(pProcessData);
   pProcessData->pWatchdog->setInterval(kWatchdogWarnSeconds * 1000);
   connect(pProcessData->pWatchdog, &QTimer::timeout, this, [pProcessData]()
      {
         const qint64 secs = pProcessData->elapsed.elapsed() / 1000;
         if(secs >= kWatchdogKillSeconds)
         {
            qCritical().noquote() << "CommandRunner: KILLING hung command after" << secs << "s:"
               << pProcessData->program << "(" << pProcessData->commandName << "). In-flight processes:"
               << CommandRunner::runningProcessCount();
            pProcessData->pWatchdog->stop();
            pProcessData->pProcess->kill();   // triggers finished() -> normal cleanup below
         }
         else
         {
            qWarning().noquote() << "CommandRunner: command still running after" << secs << "s:"
               << pProcessData->program << "(" << pProcessData->commandName << "). In-flight processes:"
               << CommandRunner::runningProcessCount();
         }
      });

   // Connect the finished signal to handle what happens when the command completes
   connect(pProcessData->pProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, pProcessData, payloadToFinished](int exitCode, QProcess::ExitStatus status)
      {
         pProcessData->pWatchdog->stop();
         const int remaining = runningProcessCount_.fetch_sub(1) - 1;

         const qint64 ms = pProcessData->elapsed.elapsed();
         if(ms >= kWatchdogWarnSeconds * 1000)
            qWarning().noquote() << "CommandRunner: slow command finished after" << ms << "ms:"
               << pProcessData->program << "(" << pProcessData->commandName << ") exit:" << exitCode
               << "in-flight:" << remaining;

         commandFinished(exitCode, status, pProcessData->output, pProcessData->errorOutput, payloadToFinished);

         pProcessData->deleteLater();
      });

   // Start the process with the given command
   QStringList args = command;
   QString program = args.takeFirst();
   pProcessData->program = program;

   const int inFlight = runningProcessCount_.fetch_add(1) + 1;
   // A high in-flight count is itself a strong diagnostic signal that things are stuck.
   if(inFlight > 30)
      qWarning().noquote() << "CommandRunner: high number of in-flight processes:" << inFlight
         << "- newest:" << program << "(" << payloadToFinished << ")";

   pProcessData->elapsed.start();
   pProcessData->pWatchdog->start();
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
