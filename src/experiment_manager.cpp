#include <cstdlib>
#include <fsatutils/log/log.hpp>
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

        std::this_thread::sleep_until(next + std::chrono::microseconds(500));
    }
}
