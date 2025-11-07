#ifndef LOGGER_H
#define LOGGER_H

#include <QCoreApplication>
#include <QString>
#include <QFile>

#include <mutex>

/**
 * Responsible for logging.
 */
class Logger
{
public:

   static Logger &getInstance() { return instance_; }

   static void logMessageHandler(const QtMsgType type, const QMessageLogContext& context, const QString& msg);

protected:
   Logger();
   virtual ~Logger();

   void logMessageHandler_nonstatic(const QtMsgType type, const QMessageLogContext& context, const QString& msg);

private:
   static Logger instance_;

   std::mutex mutex_;
   QFile logFile_;
};

#endif // !LOGGER_H
