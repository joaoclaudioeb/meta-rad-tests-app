#ifndef EXPERIMENT_MANAGER_HPP_
#define EXPERIMENT_MANAGER_HPP_

#include <gpiod.h>

#include <atomic>
#include <fsatutils/iio/context.hpp>
#include <fsatutils/zmq/service.hpp>
#include <rad-tests-app/ads1256.hpp>
#include <rad-tests-app/dac.hpp>
#include <rad-tests-app/db.hpp>
#include <vector>

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

  std::atomic<int> interval_{1800};
  std::atomic<bool> run_{false};

  static struct gpiod_chip* gpioChip_;
  static struct gpiod_line* venableLine_;
  static struct gpiod_line* pdwnOneLine_;
  static struct gpiod_line* pdwnTwoLine_;
  static struct gpiod_line* pdwnThreeLine_;
};

#endif  // EXPERIMENT_MANAGER_HPP_
