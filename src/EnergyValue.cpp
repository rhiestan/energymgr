
#include "EnergyValue.h"

#include <sstream>
#include <chrono>

struct EnergyValuePIMPL
{
   // Using Neumaier sum, c is the compensation term
   double val{0}, c{0};

   std::chrono::time_point<std::chrono::high_resolution_clock> lastSetTimePoint;
};

EnergyValue::EnergyValue()
   : pEnergyValuePIMPL_{new EnergyValuePIMPL()}
{
}

EnergyValue::EnergyValue(double val)
   : pEnergyValuePIMPL_{new EnergyValuePIMPL()}
{
   setValue(val);
}

EnergyValue::~EnergyValue()
{
   delete pEnergyValuePIMPL_;
}

void EnergyValue::setValue(double val)
{
   pEnergyValuePIMPL_->val = val;
   pEnergyValuePIMPL_->c = 0;
}

double EnergyValue::getValue() const
{
   return pEnergyValuePIMPL_->val
      + pEnergyValuePIMPL_->c;
}

std::string EnergyValue::getValueStr() const
{
   std::ostringstream ostr;
   ostr.precision(14);
   ostr << std::fixed << getValue();
   return ostr.str();
}

void EnergyValue::incrementValue(double val)
{
   double t = pEnergyValuePIMPL_->val + val;
   if(std::abs(pEnergyValuePIMPL_->val) >= std::abs(val))
      pEnergyValuePIMPL_->c += (pEnergyValuePIMPL_->val - t) + val;
   else
      pEnergyValuePIMPL_->c += (val - t) + pEnergyValuePIMPL_->val; 
   pEnergyValuePIMPL_->val = t;
}

double EnergyValue::setValueWithTime(double val)
{
   auto timeNow = std::chrono::high_resolution_clock::now();

   setValue(val);

   std::chrono::duration<double> diff = timeNow - pEnergyValuePIMPL_->lastSetTimePoint;
   pEnergyValuePIMPL_->lastSetTimePoint = timeNow;
   return diff.count();
}

double EnergyValue::incrementValueWithTime(double val)
{
   auto timeNow = std::chrono::high_resolution_clock::now();

   incrementValue(val);

   std::chrono::duration<double> diff = timeNow - pEnergyValuePIMPL_->lastSetTimePoint;
   pEnergyValuePIMPL_->lastSetTimePoint = timeNow;
   return diff.count();
}
