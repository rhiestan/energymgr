#ifndef ENERGY_MANAGER_H
#define ENERGY_MANAGER_H

class Config;

#include "EnergyValue.h"
#include "Config.h"

#include <QObject>
#include <chrono>

class SendValueToOpenHAB;

class EnergyManager : public QObject
{
   Q_OBJECT
public:
   explicit EnergyManager(QObject *parent = nullptr) : QObject(parent) {}

   void initializeValues(const Config &config);

   void writeValuesToOpenHAB(SendValueToOpenHAB &sendValueToOpenHAB);

signals:
   void sendPositiveSwitchToOH(bool positiveSwitch);

public slots:
   void onFroniusPACValue(double val);
   void onEnergyValues(double total_power, double phase1, double phase2, double phase3, double energyPos, double energyNeg);
   void onHeatPumpPower(double hpPower);
   void onHeatPumpPowerPulse();
   void onStoreEnergyValuesInDB();

private:
   EnergyValue valPowerIn, valPowerOut, valPowerProduced, valPowerConsumed;
   EnergyValue valPowerConsumedFromProducers, valPowerSelfConsumed, valPowerSelfSupplied;
   EnergyValue valWorkIn, valWorkOut;
   EnergyValue valWorkConsumedFromGrid, valWorkConsumed;
   EnergyValue valWorkProduced, valWorkConsumedFromProducers;
   EnergyValue valWorkSelfConsumed, valWorkSelfSupplied;
   EnergyValue valWP_powerIn;
   EnergyValue valWP_workIn;

   std::chrono::steady_clock::time_point lastTotalPowerPositive_;
   double minIntervalPosSwitch_{0.5};
   bool positiveSwitch_{false};
   Config configCopy_;
};

#endif // !ENERGY_MANAGER_H
