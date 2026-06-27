#ifndef COMMAND_RUNNER_H
#define COMMAND_RUNNER_H

#include <QString>
#include <QProcess>
#include <QStringList>
#include <QElapsedTimer>

#include <atomic>

class QTimer;

/**
 * Helper class that contains the process, stdout and stderr strings.
 */
class ProcessData: public QObject
{
   Q_OBJECT
public:
   ProcessData(QObject *parent) : QObject(parent) {}
   virtual ~ProcessData() {}

   QProcess *pProcess{nullptr};
   QTimer *pWatchdog{nullptr};
   QString output, errorOutput;
   QString commandName;          // human-readable name for logging (e.g. item name)
   QString program;              // the executable being run
   QElapsedTimer elapsed;        // measures how long the process runs
};

/**
 * The base class for all classes who run external commands.
 * 
 * Overwrite the commandFinished method to receive the result.
 */
class CommandRunner : public QObject {
   Q_OBJECT
public:
   explicit CommandRunner(QObject *parent = nullptr) : QObject(parent) {}

   void runCommand(const QStringList &command, const QString &payloadToFinished = QStringLiteral(""));

   virtual void commandFinished(int, QProcess::ExitStatus status, const QString &stdoutStr, const QString &stderrStr, const QString &payloadToFinished);

   /** Number of external (curl) processes currently in flight across all CommandRunners.
    *  A steadily growing value indicates hung/leaking processes. */
   static int runningProcessCount() { return runningProcessCount_.load(); }

private:
   // Number of seconds after which a running command is considered slow (logged as a warning).
   static constexpr int kWatchdogWarnSeconds = 15;
   // Number of seconds after which a running command is considered hung and gets killed.
   static constexpr int kWatchdogKillSeconds = 60;

   static std::atomic<int> runningProcessCount_;
};

#endif // !COMMAND_RUNNER_H
