#include "EnergyManager.h"
#include "ValueDBStorage.h"
#include "ReadInfluxDBHistoryValue.h"
#include "OpenHABValueReader.h"
#include "SendValueToOpenHAB.h"

#include <QDebug>
#include <QEventLoop>

void EnergyManager::initializeValues(const Config &config)
{
   configCopy_ = config;

   ValueDBStorage &instance = ValueDBStorage::getInstance();

   double val{0};
   if(instance.readValue(QStringLiteral("powerIn"), val))
      valPowerIn.setValue(val);
   if(instance.readValue(QStringLiteral("PowerOut"), val))
      valPowerOut.setValue(val);
   if(instance.readValue(QStringLiteral("PowerProduced"), val))
      valPowerProduced.setValue(val);
   if(instance.readValue(QStringLiteral("PowerConsumed"), val))
      valPowerConsumed.setValue(val);
   if(instance.readValue(QStringLiteral("PowerConsumedFromProducers"), val))
      valPowerConsumedFromProducers.setValue(val);
   if(instance.readValue(QStringLiteral("WorkIn"), val))
      valWorkIn.setValue(val);
   if(instance.readValue(QStringLiteral("WorkOut"), val))
      valWorkOut.setValue(val);
   if(instance.readValue(QStringLiteral("WorkConsumedFromGrid"), val))
      valWorkConsumedFromGrid.setValue(val);
   if(instance.readValue(QStringLiteral("WorkConsumed"), val))
      valWorkConsumed.setValue(val);
   if(instance.readValue(QStringLiteral("WorkProduced"), val))
      valWorkProduced.setValue(val);
   if(instance.readValue(QStringLiteral("WorkConsumedFromProducers"), val))
      valWorkConsumedFromProducers.setValue(val);
   if(instance.readValue(QStringLiteral("WorkSelfConsumed"), val))
      valWorkSelfConsumed.setValue(val);
   if(instance.readValue(QStringLiteral("WP_powerIn"), val))
      valWP_powerIn.setValue(val);
   if(instance.readValue(QStringLiteral("WP_workIn"), val))
      valWP_workIn.setValue(val);

   // Get accumulated values from OpenHAB, use the higher value
   int outstandingResults = 2*8;
   OpenHABValueReader rdOpenHABValue;

   auto readValueFromOHOrInflux = [this, &outstandingResults](const QString &valueName, double value) 
   {
      qDebug() << "Variable: " << valueName << ", value: " << value;

      --outstandingResults;
      if(valueName == QStringLiteral("http_einfachSolar2_workOut"))
      {
         if(valWorkOut.getValue() < value)
            valWorkOut.setValue(value);
      }
      if(valueName == QStringLiteral("http_einfachSolar2_workIn"))
      {
         if(valWorkIn.getValue() < value)
            valWorkIn.setValue(value);
      }
      if(valueName == QStringLiteral("http_einfachSolar2_WorkProduced"))
      {
         if(valWorkProduced.getValue() < value)
            valWorkProduced.setValue(value);
      }
      if(valueName == QStringLiteral("http_einfachSolar2_WorkConsumed"))
      {
         if(valWorkConsumed.getValue() < value)
            valWorkConsumed.setValue(value);
      }
      if(valueName == QStringLiteral("http_einfachSolar2_WP_workIn"))
      {
         if(valWP_workIn.getValue() < value)
            valWP_workIn.setValue(value);
      }
      if(valueName == QStringLiteral("http_einfachSolar2_WorkSelfConsumed"))
      {
         if(valWorkSelfConsumed.getValue() < value)
            valWorkSelfConsumed.setValue(value);
      }
      if(valueName == QStringLiteral("http_einfachSolar2_WorkConsumedFromGrid"))
      {
         if(valWorkConsumedFromGrid.getValue() < value)
            valWorkConsumedFromGrid.setValue(value);
      }
      if(valueName == QStringLiteral("http_einfachSolar2_WorkConsumedFromProducers"))
      {
         if(valWorkConsumedFromProducers.getValue() < value)
            valWorkConsumedFromProducers.setValue(value);
      }
   };

   QObject::connect(&rdOpenHABValue, &OpenHABValueReader::valueRead, this, readValueFromOHOrInflux);

   // Get accumulated values from InfluxDB, use the higher value
   ReadInfluxDBHistoryValue rdInfluxDBHistoryValue;
   QObject::connect(&rdInfluxDBHistoryValue, &ReadInfluxDBHistoryValue::valueFromInfluxDBHistory, this, readValueFromOHOrInflux);

   // Run event loop until all values have been fetched, or timeout occurred
   QEventLoop eventLoop;

   rdOpenHABValue.runCommand(config, QStringLiteral("http_einfachSolar2_workOut"));
   rdOpenHABValue.runCommand(config, QStringLiteral("http_einfachSolar2_workIn"));
   rdOpenHABValue.runCommand(config, QStringLiteral("http_einfachSolar2_WorkProduced"));
   rdOpenHABValue.runCommand(config, QStringLiteral("http_einfachSolar2_WorkConsumed"));
   rdOpenHABValue.runCommand(config, QStringLiteral("http_einfachSolar2_WP_workIn"));
   rdOpenHABValue.runCommand(config, QStringLiteral("http_einfachSolar2_WorkSelfConsumed"));
   rdOpenHABValue.runCommand(config, QStringLiteral("http_einfachSolar2_WorkConsumedFromGrid"));
   rdOpenHABValue.runCommand(config, QStringLiteral("http_einfachSolar2_WorkConsumedFromProducers"));

   rdInfluxDBHistoryValue.runCommand(config, QStringLiteral("http_einfachSolar2_workOut"));
   rdInfluxDBHistoryValue.runCommand(config, QStringLiteral("http_einfachSolar2_workIn"));
   rdInfluxDBHistoryValue.runCommand(config, QStringLiteral("http_einfachSolar2_WorkProduced"));
   rdInfluxDBHistoryValue.runCommand(config, QStringLiteral("http_einfachSolar2_WorkConsumed"));
   rdInfluxDBHistoryValue.runCommand(config, QStringLiteral("http_einfachSolar2_WP_workIn"));
   rdInfluxDBHistoryValue.runCommand(config, QStringLiteral("http_einfachSolar2_WorkSelfConsumed"));
   rdInfluxDBHistoryValue.runCommand(config, QStringLiteral("http_einfachSolar2_WorkConsumedFromGrid"));
   rdInfluxDBHistoryValue.runCommand(config, QStringLiteral("http_einfachSolar2_WorkConsumedFromProducers"));

   bool timeout = false;
   const float timeoutTime = 2.0;  // seconds
   std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

   while(outstandingResults > 0 && !timeout)
   {
      eventLoop.processEvents();
      std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
      float elapsedSeconds = std::chrono::duration<float>(now - startTime).count();
      if(elapsedSeconds > timeoutTime)
         timeout = true;
   }

   lastTotalPowerPositive_ = std::chrono::high_resolution_clock::now();
   minIntervalPosSwitch_ = config.getMinIntervalPosSwitch();

   positiveSwitch_ = true;
}

void EnergyManager::onFroniusPACValue(double val)
{
   double timeDiff = valPowerProduced.setValueWithTime(val);
   if(timeDiff < 10.0)
      valWorkProduced.incrementValue(timeDiff * val / 3600.0);

   //qDebug() << "WorkProduced: " << valWorkProduced.getValueStr();
}

void EnergyManager::onEnergyValues(double total_power, double phase1, double phase2, double phase3, double energyPos, double energyNeg)
{
   bool newPositiveSwitch = positiveSwitch_;

   if(total_power > 0)
   {
      // Power drawn from power grid
      newPositiveSwitch = false;
      lastTotalPowerPositive_ = std::chrono::high_resolution_clock::now();
      valPowerIn.setValue(total_power);
      valPowerOut.setValue(0);
      valWorkIn.incrementValue(energyPos);

      valWorkConsumedFromGrid.incrementValue(energyPos); // minus energy to battery

      //qDebug() << "WorkConsumedFromGrid: " << valWorkConsumedFromGrid.getValueStr();
   }
   else
   {
      valPowerIn.setValue(0);
      valPowerOut.setValue(std::abs(total_power));
      valWorkOut.incrementValue(energyNeg);

      std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
      float intervalSeconds = std::chrono::duration<float>(now - lastTotalPowerPositive_).count();
      if(intervalSeconds > minIntervalPosSwitch_)
         newPositiveSwitch = true;
   }

   if(newPositiveSwitch != positiveSwitch_)
   {
      emit sendPositiveSwitchToOH(newPositiveSwitch);
      positiveSwitch_ = newPositiveSwitch;
   }

   double timeDiff = valPowerConsumed.setValueWithTime(valPowerProduced.getValue() + total_power);
   if(timeDiff < 2.0)
      valWorkConsumed.incrementValue(timeDiff * valPowerProduced.getValue() / 3600.0 + energyPos - energyNeg );

   //qDebug() << "WorkConsumed: " << valWorkConsumed.getValueStr();

   // Consumed from producers: Portion of the load that is met directly at the instant by onsite production (PV or other producers), not counting storage-mediated delivery.
   timeDiff = valPowerConsumedFromProducers.setValueWithTime( std::min(valPowerConsumed.getValue(), valPowerProduced.getValue() ));
   if(timeDiff < 2.0)
      valWorkConsumedFromProducers.incrementValue(timeDiff * valPowerConsumedFromProducers.getValue() / 3600.0);

   // EnergyValue valWorkSelfConsumed;
   // Work self consumed: Total energy consumed by the load that originates onsite, either consumed instantly from production or consumed later after being buffered in storage from producers, during Δt.
   valWorkConsumedFromProducers.incrementValue(timeDiff * valPowerConsumedFromProducers.getValue() / 3600.0);  // Plus battery out, if available
}

void EnergyManager::onHeatPumpPower(double hpPower)
{
   valWP_powerIn.setValue(hpPower);
   valWP_workIn.incrementValue(1.0);
}

void EnergyManager::onStoreEnergyValuesInDB()
{
   ValueDBStorage &instance = ValueDBStorage::getInstance();

   instance.storeValue(QStringLiteral("powerIn"), valPowerIn.getValue());
   instance.storeValue(QStringLiteral("PowerOut"), valPowerOut.getValue());
   instance.storeValue(QStringLiteral("PowerProduced"), valPowerProduced.getValue());
   instance.storeValue(QStringLiteral("PowerConsumed"), valPowerConsumed.getValue());
   instance.storeValue(QStringLiteral("PowerConsumedFromProducers"), valPowerConsumedFromProducers.getValue());
   instance.storeValue(QStringLiteral("WorkIn"), valWorkIn.getValue());
   instance.storeValue(QStringLiteral("WorkOut"), valWorkOut.getValue());
   instance.storeValue(QStringLiteral("WorkConsumedFromGrid"), valWorkConsumedFromGrid.getValue());
   instance.storeValue(QStringLiteral("WorkConsumed"), valWorkConsumed.getValue());
   instance.storeValue(QStringLiteral("WorkProduced"), valWorkProduced.getValue());
   instance.storeValue(QStringLiteral("WorkConsumedFromProducers"), valWorkConsumedFromProducers.getValue());
   instance.storeValue(QStringLiteral("WorkSelfConsumed"), valWorkSelfConsumed.getValue());
   instance.storeValue(QStringLiteral("WP_powerIn"), valWP_powerIn.getValue());
   instance.storeValue(QStringLiteral("WP_workIn"), valWP_workIn.getValue());
}

void EnergyManager::writeValuesToOpenHAB(SendValueToOpenHAB &sendValueToOpenHAB)
{
   sendValueToOpenHAB.runCommand(configCopy_, QStringLiteral("http_einfachSolar2_WorkConsumedFromGrid_cmp"), QString::fromUtf8(valWorkConsumedFromGrid.getValueStr()));
   sendValueToOpenHAB.runCommand(configCopy_, QStringLiteral("http_einfachSolar2_workOut_cmp"), QString::fromUtf8(valWorkConsumedFromGrid.getValueStr()));
}
