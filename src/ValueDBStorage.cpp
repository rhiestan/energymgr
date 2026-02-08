#include "ValueDBStorage.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSet>
#include <QThread>

ValueDBStorage ValueDBStorage::instance_;

// ValueDBStorage implementation

ValueDBStorage::ValueDBStorage()
   : QObject(nullptr)
   , worker_(nullptr)
   , errorCallback_(nullptr)
{
   worker_ = new ValueDBStorageWorker();
   worker_->moveToThread(&workerThread_);

   connect(this, &ValueDBStorage::requestOpenDatabase,
      worker_, &ValueDBStorageWorker::onOpenDatabase,
      Qt::QueuedConnection);

   connect(this, &ValueDBStorage::requestStoreValue,
      worker_, &ValueDBStorageWorker::onStoreValue,
      Qt::QueuedConnection);

   connect(this, &ValueDBStorage::requestReadValue,
      worker_, &ValueDBStorageWorker::onReadValue,
      Qt::BlockingQueuedConnection);

   connect(this, &ValueDBStorage::requestStoreMultipleValues,
      worker_, &ValueDBStorageWorker::onStoreMultipleValues,
      Qt::QueuedConnection);

   connect(worker_, &ValueDBStorageWorker::databaseOpened,
      this, &ValueDBStorage::onDatabaseOpened,
      Qt::QueuedConnection);

   connect(worker_, &ValueDBStorageWorker::operationError,
      this, &ValueDBStorage::onOperationError,
      Qt::QueuedConnection);

   workerThread_.start();
}

ValueDBStorage::~ValueDBStorage()
{
   shutdown();
}

void ValueDBStorage::openDatabase(const QString &dbFilename)
{
   emit requestOpenDatabase(dbFilename);
}

void ValueDBStorage::storeValue(const QString &valueName, double value)
{
   emit requestStoreValue(valueName, value);
}

bool ValueDBStorage::readValue(const QString &valueName, double &value)
{
   bool found = false;
   emit requestReadValue(valueName, value, found);
   return found;
}

void ValueDBStorage::storeMultipleValues(const QList<std::tuple<QString, double>> &valueList)
{
   emit requestStoreMultipleValues(valueList);
}

void ValueDBStorage::setErrorCallback(ErrorCallback callback)
{
   errorCallback_ = callback;
}

void ValueDBStorage::shutdown()
{
   if(workerThread_.isRunning())
   {
      workerThread_.quit();
      workerThread_.wait();
   }

   if(worker_)
   {
      delete worker_;
      worker_ = nullptr;
   }
}

void ValueDBStorage::onDatabaseOpened(bool success, const QString &errorMessage)
{
   if(!success)
   {
      QString error = QString("Could not open database: ") + errorMessage;
      emit errorOccurred(error);
      
      if(errorCallback_)
         errorCallback_(error);
   }
}

void ValueDBStorage::onOperationError(const QString &errorMessage)
{
   emit errorOccurred(errorMessage);
   
   if(errorCallback_)
      errorCallback_(errorMessage);
}

// ValueDBStorageWorker implementation

ValueDBStorageWorker::ValueDBStorageWorker(QObject *parent)
   : QObject(parent)
{
}

ValueDBStorageWorker::~ValueDBStorageWorker()
{
   isOpen_ = false;
}

void ValueDBStorageWorker::onOpenDatabase(const QString dbFilename)
{
   std::scoped_lock<std::mutex> lock(mutex_);

   QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("ValueDB"));
   db.setDatabaseName(dbFilename);
   isOpen_ = db.open();

   if(!isOpen_)
   {
      emit databaseOpened(false, db.lastError().text());
      return;
   }

   QSqlQuery query("CREATE TABLE IF NOT EXISTS ValueDB (name TEXT PRIMARY KEY, value REAL);", db);
   if(!query.exec())
   {
      emit databaseOpened(false, query.lastError().text());
      return;
   }

   emit databaseOpened(true, QString());
}

void ValueDBStorageWorker::onStoreValue(const QString valueName, double value)
{
   std::scoped_lock<std::mutex> lock(mutex_);

   try
   {
      if(!isOpen_)
      {
         emit operationError("Database is not open");
         return;
      }

      QSqlDatabase db = QSqlDatabase::database(QStringLiteral("ValueDB"));

      bool valueNameExists{false};
      {
         QSqlQuery query(db);
         query.prepare("SELECT value FROM ValueDB WHERE name = ?");
         query.bindValue(0, valueName);
         if(!query.exec())
         {
            emit operationError(QString("Database query failed: ") + db.lastError().text());
            return;
         }
         if(query.next())
         {
            valueNameExists = true;
         }
      }

      bool isOk = db.transaction();
      if(!isOk)
      {
         emit operationError("Transaction creation failed");
         return;
      }

      QSqlQuery query(db);
      if(valueNameExists)
         query.prepare("UPDATE ValueDB SET value=:value WHERE name=:name");
      else
         query.prepare("INSERT INTO ValueDB (name, value) VALUES (:name, :value)");
      query.bindValue(":name", valueName);
      query.bindValue(":value", value);
      isOk = query.exec();
      if(!isOk)
      {
         db.rollback();
         emit operationError(QString("Could not execute query: ") + query.lastError().text());
         return;
      }

      db.commit();
   }
   catch(const std::exception &ex)
   {
      emit operationError(QString("Exception in storeValue: ") + ex.what());
   }
}

void ValueDBStorageWorker::onReadValue(const QString valueName, double &value, bool &found)
{
   std::scoped_lock<std::mutex> lock(mutex_);

   found = false;

   try
   {
      if(!isOpen_)
         throw std::runtime_error("Database is not open");

      QSqlDatabase db = QSqlDatabase::database(QStringLiteral("ValueDB"));

      QSqlQuery query(db);
      query.prepare("SELECT value FROM ValueDB WHERE name = ?");
      query.bindValue(0, valueName);
      if(!query.exec())
         throw std::runtime_error(std::string("Database query failed: ") + std::string(query.lastError().text().toUtf8()));

      if(query.next())
      {
         value = query.value(0).toDouble();
         found = true;
      }
   }
   catch(const std::exception &ex)
   {
      emit operationError(QString("Exception in readValue: ") + ex.what());
      throw;
   }
}

void ValueDBStorageWorker::onStoreMultipleValues(const QList<std::tuple<QString, double>> valueList)
{
   std::scoped_lock<std::mutex> lock(mutex_);

   try
   {
      if(!isOpen_)
      {
         emit operationError("Database is not open");
         return;
      }

      QSqlDatabase db = QSqlDatabase::database(QStringLiteral("ValueDB"));

      QSet<QString> existingNames;
      {
         QSqlQuery query(db);
         query.prepare("SELECT name FROM ValueDB");
         if(!query.exec())
         {
            emit operationError(QString("Database query failed: ") + db.lastError().text());
            return;
         }
         while(query.next())
         {
            QString name = query.value(0).toString();
            existingNames.insert(name);
         }
      }

      bool isOk = db.transaction();
      if(!isOk)
      {
         emit operationError("Transaction creation failed");
         return;
      }

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
         {
            db.rollback();
            emit operationError(QString("Could not execute query: ") + query.lastError().text());
            return;
         }
      }
      db.commit();
   }
   catch(const std::exception &ex)
   {
      emit operationError(QString("Exception in storeMultipleValues: ") + ex.what());
   }
}