#ifndef DB_HPP_
#define DB_HPP_

#include <cstdint>
#include <sqlite3.h>

#include <string>

namespace db {

struct ActuationEntry {
    std::string dacName;
    int channel;
    double setPoint;
    int64_t unix_ms;
};

struct MeasurementEntry {
    std::string sensorName;
    int channel;
    double value;
    int64_t unix_ms;
};

class SqliteDb {
  public:
    SqliteDb(std::string dbPath) : dbPath_{dbPath} {
        createSqliteDB();
        applyPragmas();
        createMeasurementsTable();
        createActuationTable();
        prepareStmt();
    }
    ~SqliteDb() {
        if (insertMeasurementStmt_)
            sqlite3_finalize(insertMeasurementStmt_);
        if (insertActuationStmt_)
            sqlite3_finalize(insertActuationStmt_);
        sqlite3_close(dbHandle_);
    }

    operator bool() const { return dbOpen_; };

    int begin(void);
    int commit(void);
    int rollback(void);

    int addMeasurement(MeasurementEntry &entry);
    int addActuation(ActuationEntry &entry);

  private:
    std::string dbPath_;
    sqlite3 *dbHandle_;
    bool dbOpen_{false};
    sqlite3_stmt *insertMeasurementStmt_{nullptr};
    sqlite3_stmt *insertActuationStmt_{nullptr};

    int prepareStmt(void);
    void applyPragmas(void);
    int createSqliteDB(void);
    int createMeasurementsTable(void);
    int createActuationTable(void);
};

} // namespace db

#endif
