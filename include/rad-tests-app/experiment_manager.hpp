#ifndef EXPERIMENT_MANAGER_HPP_
#define EXPERIMENT_MANAGER_HPP_

#include <rad-tests-app/dac.hpp>
#include <rad-tests-app/db.hpp>
#include <vector>

class ExperimentManager {
  public:
    ExperimentManager(std::string dbPath);
    void runExperiment();

  private:
    std::vector<DAC> dacs_;
    db::SqliteDb db_;
};

#endif // EXPERIMENT_MANAGER_HPP_
