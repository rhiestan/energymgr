#ifndef COMMAND_RUNNER_H
#define COMMAND_RUNNER_H

#include <QString>
#include <QProcess>
#include <QStringList>

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
   QString output, errorOutput;
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

private:
};

#endif // !COMMAND_RUNNER_H
