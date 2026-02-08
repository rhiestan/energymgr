#ifndef VALUE_DB_STORAGE_H
#define VALUE_DB_STORAGE_H

#include <QString>
#include <QList>
#include <QObject>
#include <QThread>

#include <tuple>
#include <mutex>
#include <functional>

class ValueDBStorageWorker;

/**
 * Stores values in a SQLite DB.
 * Runs in a separate thread with signal-based communication.
 */
class ValueDBStorage : public QObject
{
   Q_OBJECT

public:
   using ErrorCallback = std::function<void(const QString &errorMessage)>;

   void openDatabase(const QString &dbFilename);
   void storeValue(const QString &valueName, double value);
   bool readValue(const QString &valueName, double &value);
   void storeMultipleValues(const QList<std::tuple<QString, double>> &valueList);

   void setErrorCallback(ErrorCallback callback);

   void shutdown();

   static ValueDBStorage &getInstance() { return instance_; }

signals:
   void requestOpenDatabase(const QString dbFilename);
   void requestStoreValue(const QString valueName, double value);
   void requestReadValue(const QString valueName, double &value, bool &found);
   void requestStoreMultipleValues(const QList<std::tuple<QString, double>> valueList);

   void errorOccurred(const QString &errorMessage);

private slots:
   void onDatabaseOpened(bool success, const QString &errorMessage);
   void onOperationError(const QString &errorMessage);

private:
   ValueDBStorage();
   virtual ~ValueDBStorage();

   QThread workerThread_;
   ValueDBStorageWorker *worker_;
   ErrorCallback errorCallback_;

   // Singleton
   static ValueDBStorage instance_;
};

/**
 * Worker class that performs database operations in a separate thread.
 */
class ValueDBStorageWorker : public QObject
{
   Q_OBJECT

public:
   explicit ValueDBStorageWorker(QObject *parent = nullptr);
   virtual ~ValueDBStorageWorker();

signals:
   void databaseOpened(bool success, const QString &errorMessage);
   void operationError(const QString &errorMessage);

public slots:
   void onOpenDatabase(const QString dbFilename);
   void onStoreValue(const QString valueName, double value);
   void onReadValue(const QString valueName, double &value, bool &found);
   void onStoreMultipleValues(const QList<std::tuple<QString, double>> valueList);

private:
   bool isOpen_{false};
   std::mutex mutex_;
};

#endif // !VALUE_DB_STORAGE_H
