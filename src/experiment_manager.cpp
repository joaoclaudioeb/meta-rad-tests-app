#include <cstdlib>
#include <fsatutils/log/log.hpp>
#include <rad-tests-app/adc.hpp>
#include <rad-tests-app/dac.hpp>
#include <rad-tests-app/db.hpp>
#include <rad-tests-app/experiment_manager.hpp>
#include <rad-tests-app/helpers.hpp>
#include <thread>

ExperimentManager::ExperimentManager(std::string dbPath) : db_{dbPath} {
    try {
        dacs_.emplace_back("/dev/spidev1.0", "gpiochip0", 0U,
                           DAC81408_PIN_UNUSED);
    } catch (std::exception &e) {
        logs::log(ERR, "Exception triggered creating DAC! e: %s\n", e.what());
        exit(1);
    }

    /* ADS1256 #1 (CS 1) — voltage, single-ended, all 8 channels */
    try {
        adcs_.emplace_back("1", "ads1256-voltage");
    } catch (std::exception &e) {
        logs::log(ERR, "Exception creating ADC voltage (CS1)! e: %s\n",
                  e.what());
        exit(1);
    }

    /* ADS1256 #2 (CS 2) — current, differential, 4 pairs */
    try {
        adcs_.emplace_back("2", "ads1256-current-1");
    } catch (std::exception &e) {
        logs::log(ERR, "Exception creating ADC current-1 (CS2)! e: %s\n",
                  e.what());
        exit(1);
    }

    /* ADS1256 #3 (CS 3) — current, differential, 4 pairs */
    try {
        adcs_.emplace_back("3", "ads1256-current-2");
    } catch (std::exception &e) {
        logs::log(ERR, "Exception creating ADC current-2 (CS3)! e: %s\n",
                  e.what());
        exit(1);
    }
}

std::vector<fsatutils::zmq::Command>
ExperimentManager::getCommandDescription() {
    fsatutils::zmq::Command run;
    run.cmd = "run";

    return {run};
}

void ExperimentManager::commandHandler(void *manager,
                                       fsatutils::zmq::Command cmd) {
    ExperimentManager *man = static_cast<ExperimentManager *>(manager);

    if (cmd.cmd == "run") {
        man->run_ = true;
        logs::log(INFO, "Starting run at %llu...\n", getUnixMs());
    } else {
        logs::log(WARN, "Invalid command received!\n");
    }
}

int ExperimentManager::runDacChannelSweep(DAC &dac) {
    std::vector<db::ActuationEntry> dbEntries(dacChannels_);

    currentSetPoint_ += step_;

    if (currentSetPoint_ > maxSetPoint_)
        currentSetPoint_ = 0.0;

    for (int i = 0; i < dacChannels_; ++i) {
        if (!dac.isChannelEnabled(i))
            continue;
        
        int ret = dac.setChannelVoltage(i, currentSetPoint_);

        if (ret == 0)
            dbEntries[i].unix_ms = getUnixMs();
        else
            logs::log(ERR, "Failed to set voltage at channel[%i] on sweep\n",
                      i);
    }

    if (db_.begin() < 0)
        return -1;

    int retval = 0;
    for (int i = 0; i < dacChannels_; ++i) {
        if (!dac.isChannelEnabled(i))
            continue;

        if (dbEntries[i].unix_ms != 0) {
            dbEntries[i].channel = i;
            dbEntries[i].dacName = dac.spidev();
            dbEntries[i].setPoint = currentSetPoint_;

            if (db_.addActuation(dbEntries[i]) < 0) {
                logs::log(
                    ERR, "Failed to add actuation for channel[%i] to db!\n", i);
                retval = -1;
            }
        } else {
            logs::log(ERR, "Failed to set voltage for channel[%i]!\n", i);
            retval = -1;
        }
    }

    if (retval == 0) {
        logs::log(DEBUG, "Commited a channel sweep to DB!\n");
        db_.commit();
    } else {
        logs::log(DEBUG, "Rollbacked a channel sweep to DB!\n");
        db_.rollback();
    }

    return retval;
}

int ExperimentManager::runAdcSample() {
    if (db_.begin() < 0)
        return -1;

    int retval = 0;

    /*
     * ADC #1 (adcs_[0]) — voltage readings, single-ended channels 0-7.
     * One reading per MOSFET gate channel.
     */
    for (int ch = 0; ch < 8; ++ch) {
        auto val = adcs_[0].readSingleEnded(ch);
        if (val) {
            db::MeasurementEntry entry{
                .sensorName = adcs_[0].label(),
                .channel = ch,
                .value = *val,
                .unix_ms = getUnixMs(),
            };

            if (db_.addMeasurement(entry) < 0) {
                logs::log(ERR,
                          "Failed to add voltage measurement ch[%d] to db!\n",
                          ch);
                retval = -1;
            }
        } else {
            logs::log(ERR, "Failed to read voltage on ch[%d]!\n", ch);
            retval = -1;
        }
    }

    /*
     * ADC #2 and #3 (adcs_[1], adcs_[2]) — current readings, differential
     * channels: 0-1, 2-3, 4-5, 6-7.
     *
     * The channel number stored in the DB is the differential pair index
     * (0..3) offset by 4 for the second ADC, so we get a contiguous
     * 0..7 range across both current ADCs.
     */
    for (int adcIdx = 1; adcIdx <= 2; ++adcIdx) {
        int chOffset = (adcIdx - 1) * 4;

        for (int pair = 0; pair < 4; ++pair) {
            int chPos = pair * 2;
            int chNeg = pair * 2 + 1;

            auto val = adcs_[adcIdx].readDifferential(chPos, chNeg);
            if (val) {
                db::MeasurementEntry entry{
                    .sensorName = adcs_[adcIdx].label(),
                    .channel = chOffset + pair,
                    .value = *val,
                    .unix_ms = getUnixMs(),
                };

                if (db_.addMeasurement(entry) < 0) {
                    logs::log(
                        ERR,
                        "Failed to add current measurement ch[%d-%d] to db!\n",
                        chPos, chNeg);
                    retval = -1;
                }
            } else {
                logs::log(ERR, "Failed to read current on ch[%d-%d]!\n", chPos,
                          chNeg);
                retval = -1;
            }
        }
    }

    if (retval == 0) {
        logs::log(DEBUG, "Committed ADC sample to DB!\n");
        db_.commit();
    } else {
        logs::log(DEBUG, "Rolled back ADC sample to DB!\n");
        db_.rollback();
    }

    return retval;
}

void ExperimentManager::runExperiment() {
    for (auto &dac : dacs_) {
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

    while (true) {
        auto next = std::chrono::steady_clock::now();

        for (auto &dac : dacs_) {
            runDacChannelSweep(dac);
        }

        runAdcSample();

        std::this_thread::sleep_until(next + std::chrono::microseconds(500));
    }
}
