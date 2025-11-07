#ifndef VALUE_DB_STORAGE_H
#define VALUE_DB_STORAGE_H

#include <QString>

/**
 * Stores values in a SQLite DB.
 */
class ValueDBStorage
{
public:

   void openDatabase(const QString &dbFilename);
   void storeValue(const QString &valueName, double value);
   bool readValue(const QString &valueName, double &value);

   static ValueDBStorage &getInstance() { return instance_; }

private:
   ValueDBStorage();
   virtual ~ValueDBStorage();

   bool isOpen_{false};

   // Singleton
   static ValueDBStorage instance_;
};

#endif // !VALUE_DB_STORAGE_H
