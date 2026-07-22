#ifndef EXPERIMENT_MANAGER_HPP_
#define EXPERIMENT_MANAGER_HPP_

#include <fsatutils/iio/context.hpp>
#include <fsatutils/zmq/service.hpp>
#include <rad-tests-app/ads1256.hpp>
#include <rad-tests-app/dac.hpp>
#include <rad-tests-app/db.hpp>
#include <vector>
#include <gpiod.h>

struct gpiod_chip *gpioChip_;
struct gpiod_line *venableLine_;
struct gpiod_line *pdwnOneLine_;
struct gpiod_line *pdwnTwoLine_;
struct gpiod_line *pdwnThreeLine_; 

class ExperimentManager {
 public:
  ExperimentManager(std::string dbPath);
  void runExperiment();

  static void commandHandler(void* manager, fsatutils::zmq::Command cmd);
  static std::vector<fsatutils::zmq::Command> getCommandDescription();

 private:
  int runDacChannelSweep(DAC& dac);
  int runAdcSample();

  std::shared_ptr<fsatutils::iio::Context> iio_ctx_;

  std::vector<DAC> dacs_;
  std::vector<Ads1256> adcs_;
  db::SqliteDb db_;
  double step_{0.050};
  double currentSetPoint_{0.0};
  double maxSetPoint_{5.0};
  int dacChannels_{8};
  bool run_{false};
};

#endif  // EXPERIMENT_MANAGER_HPP_
