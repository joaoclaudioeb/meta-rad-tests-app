#include "rad-tests-app/experiment_manager.hpp"
#include <fsatutils/log/log.hpp>
#include <fsatutils/zmq/service.hpp>
#include <fsatutils/zmq/zmq_engine.hpp>
#include <fsatutils/zmq/zprotocol.hpp>
#include <memory>
#include <string>

#define DB_PATH "/experiment/rad-tests-app.sqlite3"
#define LOG_DIR "/experiment/logs/"

#include "version.hpp"

using namespace fsatutils;

int main(void) {
    if (logs::logFile.empty()) {
        if (!std::filesystem::exists(LOG_DIR))
            std::filesystem::create_directories(LOG_DIR);

        logs::logFile = std::string{LOG_DIR} + "rad-tests-app.log";
    }

    logs::log(INFO, "Radiation Tests Application - Version [%s]\n",
              PROJECT_VERSION);

    zmq::Service::ServiceDescription desc = {
        .name = "rad-tests-app",
        .version = PROJECT_VERSION,
        .compatibleProtocols =
            static_cast<std::uint8_t>(zmq::MessageProtocol::JSON),
        .preferedProtocol =
            static_cast<std::uint8_t>(zmq::MessageProtocol::JSON),
    };

    std::unique_ptr<zmq::Service> service;

    try {
        service = std::make_unique<zmq::Service>(desc);
    } catch (std::runtime_error const &e) {
        logs::log(ERR, "Failed to create ZMQ service: [%s]!\n", e.what());
    }

    if (service != nullptr) {
        service->runService();
    }

    auto exp = std::make_unique<ExperimentManager>(DB_PATH);

    exp->runExperiment();

    return 0;
}
