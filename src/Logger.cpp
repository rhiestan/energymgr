
#include "Logger.h"
#include "Config.h"

#include <iostream>

Logger Logger::instance_;

void Logger::logMessageHandler(const QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
   instance_.logMessageHandler_nonstatic(type, context, msg);
}

Logger::Logger()
{
}

Logger::~Logger()
{
   logFile_.close();
}

void Logger::logMessageHandler_nonstatic(const QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
   std::unique_lock<std::mutex> lock(mutex_);

   if(!logFile_.isOpen())
   {
      logFile_.setFileName("energymgr.log");
      if(!logFile_.open(QIODevice::WriteOnly | QIODevice::Append))
         std::cerr << "Could not open log file!";
   }

   if(logFile_.isOpen())
   {
      logFile_.write(qUtf8Printable(qFormatLogMessage(type, context, msg) + "\n"));
   }
}
