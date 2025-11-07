#include "EnergyManager.h"
#include "ValueDBStorage.h"
#include "Config.h"

#include <QDebug>

void EnergyManager::initializeValues(const Config &config)
{
   double val{0};
   bool isOk = ValueDBStorage::getInstance().readValue("", val);

   lastTotalPowerPositive_ = std::chrono::high_resolution_clock::now();
   minIntervalPosSwitch_ = config.getMinIntervalPosSwitch();

   positiveSwitch_ = true;
}

void EnergyManager::onFroniusPACValue(double val)
{
   double timeDiff = valPowerProduced.setValueWithTime(val);
   if(timeDiff < 10.0)
      valWorkProduced.incrementValue(timeDiff * val / 3600.0);

   qDebug() << "WorkProduced: " << valWorkProduced.getValueStr();
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

      qDebug() << "WorkConsumedFromGrid: " << valWorkConsumedFromGrid.getValueStr();
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

   qDebug() << "WorkConsumed: " << valWorkConsumed.getValueStr();

   // Consumed from producers: Portion of the load that is met directly at the instant by onsite production (PV or other producers), not counting storage-mediated delivery.
   timeDiff = valPowerConsumedFromProducers.setValueWithTime( std::min(valPowerConsumed.getValue(), valPowerProduced.getValue() ));
   if(timeDiff < 2.0)
      valWorkConsumedFromProducers.incrementValue(timeDiff * valPowerConsumedFromProducers.getValue() / 3600.0);

   //EnergyValue valWorkSelfConsumed;
   // Work self consumed: Total energy consumed by the load that originates onsite, either consumed instantly from production or consumed later after being buffered in storage from producers, during Δt.
   valWorkConsumedFromProducers.incrementValue(timeDiff * valPowerConsumedFromProducers.getValue() / 3600.0);  // Plus battery out, if available
}

void EnergyManager::onHeatPumpPower(double hpPower)
{
   valWP_powerIn.setValue(hpPower);
   valWP_workIn.incrementValue(1.0);
}
