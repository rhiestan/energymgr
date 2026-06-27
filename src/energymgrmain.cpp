
#include <iostream>
#include <string>
#include <atomic>
#include <csignal>

#include <QCoreApplication>
#include <QString>
#include <QTimer>
#include <QCommandLineParser>
#include <QFileInfo>

#include "CommandRunner.h"
#include "Config.h"
#include "MQReader.h"
#include "ValueDBStorage.h"
#include "FroniusReader.h"
#include "OpenHABSystemStatusReader.h"
#include "Logger.h"
#include "ReadInfluxDBHistoryValue.h"
#include "EnergyManager.h"
#include "EnergyValue.h"
#include "SendValueToOpenHAB.h"

// Set from the (async-signal-safe) signal handler on SIGTERM/SIGINT and polled
// from the Qt event loop, so we can shut down gracefully without calling any
// non-async-signal-safe Qt function directly from the handler.
static std::atomic_bool g_shutdownRequested{false};

static void requestShutdownHandler(int /*signal*/)
{
   g_shutdownRequested.store(true);
}

int main(int argc, char **argv)
{
   QCoreApplication app(argc, argv);

   QCoreApplication::setApplicationName(QStringLiteral("EnergyManager"));
   QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));

   qInstallMessageHandler(Logger::logMessageHandler);
   qSetMessagePattern("%{time yyyy-MM-dd hh:mm:ss,zzz} [%{type}] %{category}: %{message}");

   QCommandLineParser parser;
   parser.setApplicationDescription(QCoreApplication::translate("main", "Energy manager"));
   parser.addHelpOption();
   parser.addVersionOption();

   QCommandLineOption configFileOption(QStringList() << "c" << "config",
      QCoreApplication::translate("main", "Config file"),
      QStringLiteral("config"));
   parser.addOption(configFileOption);
   parser.process(app);

   QString configFile = parser.value(configFileOption);
   if(!QFileInfo::exists(configFile))
   {
      std::cerr << "Config file not found" << std::endl;
      return 2;
   }

   try
   {
      qInfo() << "energymgr: starting up, version" << QCoreApplication::applicationVersion();

      // Handle service stop (systemd sends SIGTERM) and Ctrl-C (SIGINT) so we can
      // shut down cleanly: flush the latest values to the DB and close it.
      std::signal(SIGTERM, requestShutdownHandler);
      std::signal(SIGINT, requestShutdownHandler);

      Config config;
      config.loadFromFile(configFile);
      qInfo().noquote() << "energymgr: config loaded from" << configFile;

#if 0
      {
         EnergyValue v{1.0};
         v.incrementValue(1.0);
         double a = v.getValue();
         std::string s = v.getValueStr();

         v.setValue(1.0);
         v.incrementValue(1.0e100);
         v.incrementValue(1.0);
         v.incrementValue(-1.0e100);

         a = v.getValue();
         s = v.getValueStr();
      }
#endif

      ValueDBStorage::getInstance().openDatabase(config.getDBFileName());
      ValueDBStorage::getInstance().setErrorCallback([](const QString &errorMessage)
         {
            qCritical() << "Database error: " << errorMessage;
         });

      EnergyManager energyMgr;
      energyMgr.initializeValues(config);

      SendValueToOpenHAB sendValueToOpenHAB;

      QObject::connect(&energyMgr, &EnergyManager::sendPositiveSwitchToOH, &app, [&sendValueToOpenHAB, &config](bool positiveSwitch)
         {
            sendValueToOpenHAB.runCommand(config, QStringLiteral("virtualPositiveSwitch"), positiveSwitch ? QStringLiteral("ON") : QStringLiteral("OFF"));
         });

      MQReader mqReader;
      mqReader.connectToBroker(config.getMQTTUri(), config.getMQTTPort(),
         config.getMQTTTopicHP(), config.getMQTTTopicHPPulse(), config.getMQTTTopicEnergy());

      QObject::connect(&mqReader, &MQReader::newEnergyValues, &energyMgr, &EnergyManager::onEnergyValues, Qt::ConnectionType::QueuedConnection);
      QObject::connect(&mqReader, &MQReader::newHeatPumpPower, &energyMgr, &EnergyManager::onHeatPumpPower, Qt::ConnectionType::QueuedConnection);
      QObject::connect(&mqReader, &MQReader::newHeatPumpPowerPulse, &energyMgr, &EnergyManager::onHeatPumpPowerPulse, Qt::ConnectionType::QueuedConnection);

      FroniusReader froniusReader;
      froniusReader.runCommand(config);
      QObject::connect(&froniusReader, &FroniusReader::newFroniusPACValue, &energyMgr, &EnergyManager::onFroniusPACValue, Qt::ConnectionType::QueuedConnection);

      OpenHABSystemStatusReader openHABSystemStatusReader;
      openHABSystemStatusReader.runCommand(config);
      QObject::connect(&openHABSystemStatusReader, &OpenHABSystemStatusReader::newOpenHABSystemStatus, &app, [](qint64 uptime, qint64 runLevel)
         {
            qDebug() << "OpenHAB uptime: " << uptime << ", runlevel: " << runLevel;
         }, Qt::ConnectionType::QueuedConnection);

      QTimer tmr1, timerFronius, timerStoreValuesDB, timerSendValuesToOpenHAB;
      QObject::connect(&tmr1, &QTimer::timeout, &app, [&app, &openHABSystemStatusReader, &sendValueToOpenHAB, &config]()
         {
            static int counter{0};

            openHABSystemStatusReader.runCommand(config);

            //sendValueToOpenHAB.runCommand(config, QStringLiteral("virtualPositiveSwitch"), QStringLiteral("false"));

            counter++;

            //if(counter >= 50)
            //   app.quit();
         });
      tmr1.start(5000);

      QObject::connect(&timerStoreValuesDB, &QTimer::timeout, &energyMgr, &EnergyManager::onStoreEnergyValuesInDB);
      timerStoreValuesDB.start(1000);

      QObject::connect(&timerFronius, &QTimer::timeout, &app, [&froniusReader, &config]()
         {
            froniusReader.runCommand(config);
         });
      timerFronius.start(2000);

      QObject::connect(&timerSendValuesToOpenHAB, &QTimer::timeout, &energyMgr, [&energyMgr, &sendValueToOpenHAB]()
         {
            energyMgr.writeValuesToOpenHAB(sendValueToOpenHAB);
         });
      timerSendValuesToOpenHAB.start(10000);

      // Periodic heartbeat: a single line that shows whether the event loop is still
      // alive, whether MQTT messages are still arriving, and whether curl processes
      // are piling up. This is the primary signal for diagnosing where the service
      // gets stuck. If this line keeps appearing but "MQmsgAge" grows without bound,
      // the MQTT input has died; if "curlInFlight" keeps climbing, curl is hanging.
      QTimer timerHeartbeat;
      QObject::connect(&timerHeartbeat, &QTimer::timeout, &app, [&mqReader]()
         {
            qInfo().noquote() << "HEARTBEAT: mqttState=" << mqReader.connectionStateString()
               << "MQmsgCount=" << mqReader.messageCount()
               << "MQmsgAge(s)=" << mqReader.secondsSinceLastMessage()
               << "curlInFlight=" << CommandRunner::runningProcessCount();
         });
      timerHeartbeat.start(30000);

      // Poll the shutdown flag set by the signal handler and quit the event loop
      // gracefully when a SIGTERM/SIGINT has been received.
      QTimer timerSignalCheck;
      QObject::connect(&timerSignalCheck, &QTimer::timeout, &app, [&app]()
         {
            if(g_shutdownRequested.load())
            {
               qInfo() << "energymgr: shutdown signal received, stopping event loop";
               app.quit();
            }
         });
      timerSignalCheck.start(200);

      const int exitCode = app.exec();

      // Persist the latest values before the worker thread is stopped below.
      qInfo() << "energymgr: storing final values before shutdown";
      energyMgr.onStoreEnergyValuesInDB();

      ValueDBStorage::getInstance().shutdown();
      qInfo() << "energymgr: shutdown complete";
      return exitCode;
   }
   catch(std::exception &e)
   {
      std::cerr << "energymgr: Exception caught\n"
         << e.what()
         << std::endl;
   }
   catch(...)
   {
      std::cerr << "energymgr: Unknown exception" << std::endl;
   }

   ValueDBStorage::getInstance().shutdown();
}
