#ifndef ENERGY_VALUE_H
#define ENERGY_VALUE_H

#include <string>

struct EnergyValuePIMPL;

class EnergyValue
{
public:
   EnergyValue();
   EnergyValue(double val);
   virtual ~EnergyValue();

   double getValue() const;
   std::string getValueStr() const;
   void setValue(double val);
   void incrementValue(double val);

   double setValueWithTime(double val);
   double incrementValueWithTime(double val);

private:
   EnergyValuePIMPL *pEnergyValuePIMPL_{nullptr};
};

#endif // !ENERGY_VALUE_H
