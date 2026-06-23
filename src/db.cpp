#include <sqlite3.h>

#include <fsatutils/log/log.hpp>
#include <rad-tests-app/db.hpp>
#include <string>

namespace db {

int SqliteDb::createSqliteDB(void) {
  int32_t rq = sqlite3_open(dbPath_.c_str(), &dbHandle_);

  if (rq) {
    logs::log(ERR, "Failed to create/open database!! Error: %s\n",
              sqlite3_errmsg(dbHandle_));
    sqlite3_close(dbHandle_);
    return -1;
  }

  dbOpen_ = true;

  return 0;
}

int SqliteDb::createMeasurementsTable(void) {
  const char *createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS measurements (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sensorName TEXT NOT NULL,
            channel INTEGER NOT NULL,
            value REAL NOT NULL,
            timestamp_ms INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_meas_sensor_time
            ON measurements(sensorName, timestamp);
    )";

  char *errMsg = nullptr;

  int32_t result =
      sqlite3_exec(dbHandle_, createTableSQL, nullptr, nullptr, &errMsg);

  if (result != SQLITE_OK) {
    logs::log(ERR, "Failed to create Sensors table in DB!!\n");
    sqlite3_free(errMsg);
    return -1;
  }

  logs::log(INFO, "Sensor Table was created (if it didn't exist already)!\n");

  return 0;
}

int SqliteDb::createActuationTable(void) {
  const char *createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS actuation (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            dacName TEXT NOT NULL,
            channel INTEGER NOT NULL,
            setPoint REAL NOT NULL,
            timestamp_ms INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_meas_sensor_time
            ON measurements(dacName, timestamp);
    )";

  char *errMsg = nullptr;

  int32_t result =
      sqlite3_exec(dbHandle_, createTableSQL, nullptr, nullptr, &errMsg);

  if (result != SQLITE_OK) {
    logs::log(ERR, "Failed to create Sensors table in DB!!\n");
    sqlite3_free(errMsg);
    return -1;
  }

  logs::log(INFO, "Sensor Table was created (if it didn't exist already)!\n");

  return 0;
}

void SqliteDb::applyPragmas(void) {
  if (!dbOpen_)
    return;

  /* WAL improves write throughput and lets readers (e.g. Grafana) run during
   * writes. synchronous is left at FULL to guarantee durability across
   * crashes/power loss. */
  const char *pragmas = "PRAGMA journal_mode = WAL;"
                        "PRAGMA temp_store = MEMORY;";

  char *errMsg = nullptr;
  if (sqlite3_exec(dbHandle_, pragmas, nullptr, nullptr, &errMsg) !=
      SQLITE_OK) {
    logs::log(WARN, "Failed to apply DB pragmas: %s\n", errMsg ? errMsg : "");
    sqlite3_free(errMsg);
  }
}

int SqliteDb::prepareStmt(void) {
  if (!dbOpen_)
    return -1;

  const char *measStmt = "INSERT INTO measurements (sensorName, channel, "
                         "value, timestamp_ms) VALUES (?, ?, ?, ?);";

  const char *actuationStmt = "INSERT INTO actuation (dacName, channel, "
                              "setPoint, timestamp_ms) VALUES (?, ?, ?, ?);";

  if (sqlite3_prepare_v2(dbHandle_, measStmt, -1, &insertMeasurementStmt_,
                         nullptr) != SQLITE_OK) {
    logs::log(ERR, "Failed to prepare insert measurement stmt: %s\n",
              sqlite3_errmsg(dbHandle_));
    insertMeasurementStmt_ = nullptr;
    return -1;
  }

  if (sqlite3_prepare_v2(dbHandle_, actuationStmt, -1, &insertActuationStmt_,
                         nullptr) != SQLITE_OK) {
    logs::log(ERR, "Failed to prepare insert actuation stmt: %s\n",
              sqlite3_errmsg(dbHandle_));
    insertActuationStmt_ = nullptr;
    return -1;
  }

  return 0;
}

int SqliteDb::begin(void) {
  if (!dbOpen_)
    return -1;
  char *errMsg = nullptr;
  if (sqlite3_exec(dbHandle_, "BEGIN;", nullptr, nullptr, &errMsg) !=
      SQLITE_OK) {
    logs::log(ERR, "BEGIN failed: %s\n", errMsg ? errMsg : "");
    sqlite3_free(errMsg);
    return -1;
  }
  return 0;
}

int SqliteDb::commit(void) {
  if (!dbOpen_)
    return -1;
  char *errMsg = nullptr;
  if (sqlite3_exec(dbHandle_, "COMMIT;", nullptr, nullptr, &errMsg) !=
      SQLITE_OK) {
    logs::log(ERR, "COMMIT failed: %s\n", errMsg ? errMsg : "");
    sqlite3_free(errMsg);
    return -1;
  }
  return 0;
}

int SqliteDb::addActuation(ActuationEntry &entry) {

  if (!dbOpen_ || !insertActuationStmt_)
    return -1;

  sqlite3_stmt *stmt = insertActuationStmt_;
  sqlite3_reset(stmt);
  sqlite3_clear_bindings(stmt);

  sqlite3_bind_text(stmt, 1, entry.dacName.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 2, entry.channel);
  sqlite3_bind_double(stmt, 3, entry.setPoint);
  sqlite3_bind_int64(stmt, 4, entry.unix_ms);

  int32_t result = sqlite3_step(stmt);

  if (result != SQLITE_DONE) {
    logs::log(ERR, "Failed to execute DB statament!! Error: %s\n",
              sqlite3_errmsg(dbHandle_));
    return -1;
  }

  return 0;
}

int SqliteDb::addMeasurement(MeasurementEntry &entry) {

  if (!dbOpen_ || !insertMeasurementStmt_)
    return -1;

  sqlite3_stmt *stmt = insertMeasurementStmt_;
  sqlite3_reset(stmt);
  sqlite3_clear_bindings(stmt);

  sqlite3_bind_text(stmt, 1, entry.sensorName.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 2, entry.channel);
  sqlite3_bind_double(stmt, 3, entry.value);
  sqlite3_bind_int64(stmt, 4, entry.unix_ms);

  int32_t result = sqlite3_step(stmt);

  if (result != SQLITE_DONE) {
    logs::log(ERR, "Failed to execute DB statament!! Error: %s\n",
              sqlite3_errmsg(dbHandle_));
    return -1;
  }

  return 0;
}

} // namespace db
