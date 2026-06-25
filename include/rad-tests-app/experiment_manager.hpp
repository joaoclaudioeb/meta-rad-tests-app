#ifndef EXPERIMENT_MANAGER_HPP_
#define EXPERIMENT_MANAGER_HPP_

#include <rad-tests-app/dac.hpp>
#include <vector>

class ExperimentManager {
  public:
    ExperimentManager();
    void runExperiment();

  private:
    std::vector<DAC> dacs_;
};

#endif // EXPERIMENT_MANAGER_HPP_
