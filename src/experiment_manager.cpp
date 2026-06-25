#include "rad-tests-app/db.hpp"
#include "rad-tests-app/helpers.hpp"
#include <cstdlib>
#include <fsatutils/log/log.hpp>
#include <rad-tests-app/dac.hpp>
#include <rad-tests-app/experiment_manager.hpp>
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

void ExperimentManager::runExperiment() {
    for (auto &dac : dacs_) {
        dac.setIntReference(DAC81408_REF_ON);

        dac.setChannelState(0, true);
        dac.setChannelState(3, true);
        dac.setChannelState(5, true);
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
        dac.setChannelRange(3, DAC81408_RANGE_0_5V);
        dac.setChannelRange(5, DAC81408_RANGE_0_5V);
        dac.setChannelRange(7, DAC81408_RANGE_0_5V);

        dac.setChannelVoltage(7, 1.2);
        dac.setChannelVoltage(3, 3.7);
        dac.setChannelVoltage(5, 4.9);
    }

    double setPoint = 0.0;

    while (true) {
        auto next = std::chrono::steady_clock::now();

        for (auto &dac : dacs_) {
            if (dac.setChannelVoltage(0, setPoint) == 0) {
                db::ActuationEntry entry = {
                    .dacName = dac.spidev(),
                    .channel = 0,
                    .setPoint = setPoint,
                    .unix_ms = getUnixMs(),
                };

                db_.addActuation(entry);
            } else {
                logs::log(ERR,
                          "Failed to set DAC voltage! dac[%s], voltage[%.2f]\n",
                          dac.spidev(), setPoint);
            }
        }

        setPoint += 0.5;

        if (setPoint > 5.0)
            setPoint = 0.0;

        std::this_thread::sleep_until(next + std::chrono::seconds(5));
    }
}
