#include <cstdlib>
#include <fsatutils/log/log.hpp>
#include <rad-tests-app/experiment_manager.hpp>
#include <rad-tests-app/helpers.hpp>
#include <thread>

struct gpiod_chip* ExperimentManager::gpioChip_ = nullptr;
struct gpiod_line* ExperimentManager::venableLine_ = nullptr;
struct gpiod_line* ExperimentManager::pdwnOneLine_ = nullptr;
struct gpiod_line* ExperimentManager::pdwnTwoLine_ = nullptr;
struct gpiod_line* ExperimentManager::pdwnThreeLine_ = nullptr;

ExperimentManager::ExperimentManager(std::string dbPath) : db_{dbPath} {
  try {
    dacs_.emplace_back("/dev/spidev1.0", "/dev/gpiochip0", 0U, DAC81408_PIN_UNUSED);
  } catch (std::exception& e) {
    logs::log(ERR, "Exception triggered creating DAC! e: %s\n", e.what());
    exit(1);
  }

  gpioChip_ = gpiod_chip_open("/dev/gpiochip1");
  if (!gpioChip_) {
      logs::log(ERR, "Failed to open gpiochip1!\n");
      exit(1);
  }
  
  venableLine_ = gpiod_chip_get_line(gpioChip_, 0);
  pdwnOneLine_ = gpiod_chip_get_line(gpioChip_, 1);
  pdwnTwoLine_ = gpiod_chip_get_line(gpioChip_, 4);    
  pdwnThreeLine_ = gpiod_chip_get_line(gpioChip_, 6);
  
  gpiod_line_request_output(venableLine_, "rad-tests-app", 0);
  gpiod_line_request_output(pdwnOneLine_, "rad-tests-app", 0);
  gpiod_line_request_output(pdwnTwoLine_, "rad-tests-app", 0);
  gpiod_line_request_output(pdwnThreeLine_, "rad-tests-app", 0);


  try {
    iio_ctx_ = std::make_shared<fsatutils::iio::Context>(
        fsatutils::iio::ContextType::LOCAL);
  } catch (std::exception& e) {
    logs::log(ERR, "Exception creating IIO context! e: %s\n", e.what());
    exit(1);
  }

  for (int attempt = 0; attempt < 3; ++attempt) {
      try {
          adcs_.emplace_back(iio_ctx_, "iio:device2");
          break;
      } catch (std::exception &e) {
          if (attempt == 2) {
              logs::log(ERR, "Failed to create Ads1256 iio:device2 after retries!\n");
          } else {
              logs::log(WARN, "Retrying iio:device2...\n");
              std::this_thread::sleep_for(std::chrono::seconds(1));
          }
      }
  }

  for (int attempt = 0; attempt < 3; ++attempt) {
      try {
          adcs_.emplace_back(iio_ctx_, "iio:device3");
          break;
      } catch (std::exception &e) {
          if (attempt == 2) {
              logs::log(ERR, "Failed to create Ads1256 iio:device3 after retries!\n");
          } else {
              logs::log(WARN, "Retrying iio:device3...\n");
              std::this_thread::sleep_for(std::chrono::seconds(1));
          }
      }
  }

  for (int attempt = 0; attempt < 3; ++attempt) {
      try {
          adcs_.emplace_back(iio_ctx_, "iio:device4");
          break;
      } catch (std::exception &e) {
          if (attempt == 2) {
              logs::log(ERR, "Failed to create Ads1256 iio:device4 after retries!\n");
          } else {
              logs::log(WARN, "Retrying iio:device4...\n");
              std::this_thread::sleep_for(std::chrono::seconds(1));
          }
      }
  }
}

std::vector<fsatutils::zmq::Command> ExperimentManager::getCommandDescription() {
    fsatutils::zmq::Command run;
    run.cmd = "run";

    fsatutils::zmq::Command stop;
    stop.cmd = "stop";

    fsatutils::zmq::Command set_interval;
    set_interval.cmd = "set-interval";

    return {run, stop, set_interval};
}

void ExperimentManager::commandHandler(void* manager, fsatutils::zmq::Command cmd) {
  ExperimentManager* man = static_cast<ExperimentManager*>(manager);

  if (cmd.cmd == "run") {
    man->run_ = true;
    logs::log(INFO, "Starting run at %llu...\n", getUnixMs());
  } else if (cmd.cmd == "set-interval") {
    man->interval_ = std::stoi(cmd.args[0].value);
    logs::log(INFO, "Interval between sweeps changed to %d.\n", man->interval_);
  } else {
    logs::log(WARN, "Invalid command received!\n");
  }
}

int ExperimentManager::runDacChannelSweep(DAC& dac) {
  std::vector<db::ActuationEntry> dbEntries(dacChannels_);

  currentSetPoint_ += step_;

  if (currentSetPoint_ > maxSetPoint_) currentSetPoint_ = 0.0;

  for (int i = 0; i < dacChannels_; ++i) {
    if (!dac.isChannelEnabled(i)) continue;

    int ret = dac.setChannelVoltage(i, currentSetPoint_);

    if (ret == 0)
      dbEntries[i].unix_ms = getUnixMs();
    else
      logs::log(ERR, "Failed to set voltage at channel[%i] on sweep\n", i);
  }

  int retval = 0;
  for (int i = 0; i < dacChannels_; ++i) {
    if (!dac.isChannelEnabled(i)) continue;

    if (dbEntries[i].unix_ms != 0) {
      dbEntries[i].channel = i;
      dbEntries[i].dacName = dac.spidev();
      dbEntries[i].setPoint = currentSetPoint_;

      if (db_.addActuation(dbEntries[i]) < 0) {
        logs::log(ERR, "Failed to add actuation for channel[%i] to db!\n", i);
        retval = -1;
      }
    } else {
      logs::log(ERR, "Failed to set voltage for channel[%i]!\n", i);
      retval = -1;
    }
  }

  return retval;
}

int ExperimentManager::runAdcSample() {
  if (adcs_.empty()) return 0;

  int retval = 0;

  /*
   * ADC #1 (adcs_[0]) — voltage readings, single-ended channels 0-7.
   * One reading per MOSFET gate channel.
   */
  if (adcs_.size() >= 1) {
    for (int ch = 0; ch < 8; ++ch) {
      auto val = adcs_[0].get_single_ended(ch);
      if (val) {
        db::MeasurementEntry entry{
            .sensorName = adcs_[0].name(),
            .channel = ch,
            .value = *val,
            .unix_ms = getUnixMs(),
        };

        if (db_.addMeasurement(entry) < 0) {
          logs::log(ERR, "Failed to add voltage measurement ch[%d] to db!\n",
                    ch);
          retval = -1;
        }
      } else {
        logs::log(ERR, "Failed to read voltage on ch[%d]!\n", ch);
        retval = -1;
      }
    }
  }

  /*
   * ADC #2 and #3 (adcs_[1], adcs_[2]) — current readings, differential
   * channels: 0-1, 2-3, 4-5, 6-7.
   */
  for (size_t adcIdx = 1; adcIdx < adcs_.size() && adcIdx <= 2; ++adcIdx) {
    int chOffset = (adcIdx - 1) * 4;

    for (int pair = 0; pair < 4; ++pair) {
      auto val = adcs_[adcIdx].get_differential(pair);
      if (val) {
        db::MeasurementEntry entry{
            .sensorName = adcs_[adcIdx].name(),
            .channel = chOffset + pair,
            .value = *val,
            .unix_ms = getUnixMs(),
        };

        if (db_.addMeasurement(entry) < 0) {
          logs::log(ERR, "Failed to add current measurement pair[%d] to db!\n",
                    pair);
          retval = -1;
        }
      } else {
        logs::log(ERR, "Failed to read current on pair[%d]!\n", pair);
        retval = -1;
      }
    }
  }

  return retval;
}

void ExperimentManager::runExperiment() {
  for (auto& dac : dacs_) {
    dac.setIntReference(DAC81408_REF_ON);

    dac.setChannelState(0, true);
    dac.setChannelState(1, true);
    dac.setChannelState(2, true);
    dac.setChannelState(3, true);
    dac.setChannelState(4, true);
    dac.setChannelState(5, true);
    dac.setChannelState(6, true);
    dac.setChannelState(7, true);

    logs::log(INFO, "DAC[%s], Ch[%d]: %d\n", dac.spidev(), 0,
              dac.isChannelEnabled(0));
    logs::log(INFO, "DAC[%s], Ch[%d]: %d\n", dac.spidev(), 1,
              dac.isChannelEnabled(1));
    logs::log(INFO, "DAC[%s], Ch[%d]: %d\n", dac.spidev(), 2,
              dac.isChannelEnabled(2));
    logs::log(INFO, "DAC[%s], Ch[%d]: %d\n", dac.spidev(), 3,
              dac.isChannelEnabled(3));
    logs::log(INFO, "DAC[%s], Ch[%d]: %d\n", dac.spidev(), 4,
              dac.isChannelEnabled(4));
    logs::log(INFO, "DAC[%s], Ch[%d]: %d\n", dac.spidev(), 5,
              dac.isChannelEnabled(5));
    logs::log(INFO, "DAC[%s], Ch[%d]: %d\n", dac.spidev(), 6,
              dac.isChannelEnabled(6));
    logs::log(INFO, "DAC[%s], Ch[%d]: %d\n", dac.spidev(), 7,
              dac.isChannelEnabled(7));

    dac.setChannelRange(0, DAC81408_RANGE_0_5V);
    dac.setChannelRange(1, DAC81408_RANGE_0_5V);
    dac.setChannelRange(2, DAC81408_RANGE_0_5V);
    dac.setChannelRange(3, DAC81408_RANGE_0_5V);
    dac.setChannelRange(4, DAC81408_RANGE_0_5V);
    dac.setChannelRange(5, DAC81408_RANGE_0_5V);
    dac.setChannelRange(6, DAC81408_RANGE_0_5V);
    dac.setChannelRange(7, DAC81408_RANGE_0_5V);

    dac.setChannelVoltage(0, 0.0);
    dac.setChannelVoltage(1, 0.0);
    dac.setChannelVoltage(2, 0.0);
    dac.setChannelVoltage(3, 0.0);
    dac.setChannelVoltage(4, 0.0);
    dac.setChannelVoltage(5, 0.0);
    dac.setChannelVoltage(6, 0.0);
    dac.setChannelVoltage(7, 0.0);
  }

  gpiod_line_set_value(pdwnOneLine_, 1);
  gpiod_line_set_value(pdwnTwoLine_, 1);
  gpiod_line_set_value(pdwnThreeLine_, 1);

  std::jthread timer([this]() {
      while (true) {
          std::this_thread::sleep_for(std::chrono::seconds(interval_));
          run_ = true;
      }
  });
  
  while (true) {
      if (!run_) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          continue;
      }

      gpiod_line_set_value(venableLine_, 1);
      db_.begin();
      int ret = 0;

      while (true) {
          for (auto& dac : dacs_) {
              if (runDacChannelSweep(dac) < 0) ret = -1;
          }
          if (runAdcSample() < 0) ret = -1;

          if (currentSetPoint_ <= 0.0) {
              if (ret == 0) {
                  logs::log(DEBUG, "Committed sweep to DB!\n");
                  db_.commit();
              } else {
                  logs::log(DEBUG, "Rolled back sweep to DB!\n");
                  db_.rollback();
              }
              gpiod_line_set_value(venableLine_, 0);
              break;
          }
      }

      run_ = false;
  }
} 
