

#include "ValueDBStorage.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSet>

ValueDBStorage ValueDBStorage::instance_;

void ValueDBStorage::openDatabase(const QString &dbFilename)
{
   QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("ValueDB"));
   db.setDatabaseName(dbFilename);
   isOpen_ = db.open();
   if(!isOpen_)
      throw std::runtime_error(std::string("Could not open database: ") + std::string(db.lastError().text().toUtf8()));

   QSqlQuery query("CREATE TABLE IF NOT EXISTS ValueDB (name TEXT PRIMARY KEY, value REAL);", db);
   query.exec();
}

void ValueDBStorage::storeValue(const QString &valueName, double value)
{
   if(!isOpen_)
      throw std::runtime_error("Database is not open");

   QSqlDatabase db = QSqlDatabase::database(QStringLiteral("ValueDB"));

   bool valueNameExists{false};
   {
      QSqlQuery query(db);
      query.prepare("SELECT value FROM ValueDB WHERE name = ?");
      query.bindValue(0, valueName);
      if(!query.exec())
         throw std::runtime_error(std::string("Database query failed: ") + std::string(db.lastError().text().toUtf8()));
      if(query.next())
      {
         valueNameExists = true;
      }
   }

   bool isOk = db.transaction();
   if(!isOk)
      throw std::runtime_error("Transaction creation failed");

   QSqlQuery query(db);
   if(valueNameExists)
      query.prepare("UPDATE ValueDB SET value=:value WHERE name=:name");
   else
      query.prepare("INSERT INTO ValueDB (name, value) VALUES (:name, :value)");
   query.bindValue(":name", valueName);
   query.bindValue(":value", value);
   isOk = query.exec();
   if(!isOk)
      throw std::runtime_error(std::string("Could not execute query: ") + std::string(query.lastError().text().toUtf8()));

   db.commit();
}

bool ValueDBStorage::readValue(const QString &valueName, double &value)
{
   if(!isOpen_)
      throw std::runtime_error("Database is not open");

   QSqlDatabase db = QSqlDatabase::database(QStringLiteral("ValueDB"));

   QSqlQuery query(db);
   query.prepare("SELECT value FROM ValueDB WHERE name = ?");
   query.bindValue(0, valueName);
   if(!query.exec())
      throw std::runtime_error(std::string("Database query failed: ") + std::string(query.lastError().text().toUtf8()));
   while(query.next())
   {
      value = query.value(0).toDouble();
      return true;
   }

   return false;
}

void ValueDBStorage::storeMultipleValues(const QList<std::tuple<QString, double>> &valueList)
{
   if(!isOpen_)
      throw std::runtime_error("Database is not open");

   QSqlDatabase db = QSqlDatabase::database(QStringLiteral("ValueDB"));

   QSet<QString> existingNames;
   {
      QSqlQuery query(db);
      query.prepare("SELECT name FROM ValueDB");
      if(!query.exec())
         throw std::runtime_error(std::string("Database query failed: ") + std::string(db.lastError().text().toUtf8()));
      while(query.next())
      {
         QString name = query.value(0).toString();
         existingNames.insert(name);
      }
   }

   bool isOk = db.transaction();
   if(!isOk)
      throw std::runtime_error("Transaction creation failed");

   for(const auto &nameValuePair : valueList)
   {
      const QString &valueName = std::get<0>(nameValuePair);
      const double value = std::get<1>(nameValuePair);

      QSqlQuery query(db);
      if(existingNames.contains(valueName))
         query.prepare("UPDATE ValueDB SET value=:value WHERE name=:name");
      else
         query.prepare("INSERT INTO ValueDB (name, value) VALUES (:name, :value)");
      query.bindValue(":name", valueName);
      query.bindValue(":value", value);
      isOk = query.exec();
      if(!isOk)
         throw std::runtime_error(std::string("Could not execute query: ") + std::string(query.lastError().text().toUtf8()));
   }
   db.commit();
}

ValueDBStorage::ValueDBStorage()
{
}

ValueDBStorage::~ValueDBStorage()
{
   isOpen_ = false;
}
