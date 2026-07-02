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
    int runDacChannelSweep(DAC &dac);
    std::vector<DAC> dacs_;
    db::SqliteDb db_;
    double step_{0.10};
    double currentSetPoint_{0.0};
    double maxSetPoint_{5.0};
    int dacChannels_{8};
    bool run_{false};
};

#endif // EXPERIMENT_MANAGER_HPP_
