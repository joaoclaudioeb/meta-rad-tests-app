#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <fsatutils/errors.hpp>
#include <fsatutils/log/log.hpp>
#include <rad-tests-app/adc.hpp>

namespace fs = std::filesystem;

static constexpr const char *IIO_DEVICES_PATH = "/sys/bus/iio/devices";

/* ------------------------------------------------------------------ */
/*  Sysfs helpers                                                      */
/* ------------------------------------------------------------------ */

std::optional<int> ADC::readSysfsInt(const std::string &path) const {
    std::ifstream f(path);
    if (!f.is_open()) {
        logs::log(ERR, "ADC[%s]: failed to open %s\n", label_.c_str(),
                  path.c_str());
        return std::nullopt;
    }

    int val{};
    if (!(f >> val)) {
        logs::log(ERR, "ADC[%s]: failed to parse int from %s\n",
                  label_.c_str(), path.c_str());
        return std::nullopt;
    }

    return val;
}

std::optional<double> ADC::readSysfsDouble(const std::string &path) const {
    std::ifstream f(path);
    if (!f.is_open()) {
        logs::log(ERR, "ADC[%s]: failed to open %s\n", label_.c_str(),
                  path.c_str());
        return std::nullopt;
    }

    double val{};
    if (!(f >> val)) {
        logs::log(ERR, "ADC[%s]: failed to parse double from %s\n",
                  label_.c_str(), path.c_str());
        return std::nullopt;
    }

    return val;
}

/* ------------------------------------------------------------------ */
/*  IIO device discovery                                               */
/* ------------------------------------------------------------------ */

/**
 * Scan /sys/bus/iio/devices/iio:device* looking for an ADS1256 whose
 * parent SPI device has the requested chip-select number.
 *
 * For each candidate the function:
 *   1. Reads the "name" attribute (must be "ads1256").
 *   2. Resolves the "device" symlink to find the parent SPI device
 *      name (e.g. "spi0.1").
 *   3. Compares the chip-select suffix against @p spiCs.
 */
std::string ADC::discoverIioPath(const std::string &spiCs) {
    if (!fs::exists(IIO_DEVICES_PATH))
        throw_runtime_error("IIO devices path does not exist: %s");

    for (const auto &entry : fs::directory_iterator(IIO_DEVICES_PATH)) {
        std::string dirName = entry.path().filename().string();
        if (dirName.rfind("iio:device", 0) != 0)
            continue;

        /* 1. Check device name */
        std::string nameFile = entry.path().string() + "/name";
        std::ifstream nf(nameFile);
        if (!nf.is_open())
            continue;

        std::string devName;
        std::getline(nf, devName);
        if (devName != "ads1256")
            continue;

        /* 2. Resolve 'device' symlink to get parent SPI device name */
        fs::path deviceLink = entry.path() / "device";
        if (!fs::is_symlink(deviceLink))
            continue;

        std::string parentName =
            fs::read_symlink(deviceLink).filename().string();

        /* 3. Match chip-select: last component after '.' */
        auto dotPos = parentName.rfind('.');
        if (dotPos == std::string::npos)
            continue;

        std::string cs = parentName.substr(dotPos + 1);
        if (cs == spiCs) {
            logs::log(INFO, "ADC: discovered IIO device [%s] for CS %s\n",
                      entry.path().c_str(), spiCs.c_str());
            return entry.path().string();
        }
    }

    throw_runtime_error("ADC: no ADS1256 IIO device found for SPI CS %s");
    return {}; /* unreachable */
}

/* ------------------------------------------------------------------ */
/*  Construction                                                       */
/* ------------------------------------------------------------------ */

ADC::ADC(const std::string &spiCs, const std::string &label)
    : label_{label}, iioPath_{discoverIioPath(spiCs)} {
    logs::log(DEBUG, "Initialized ADC [%s] at %s\n", label_.c_str(),
              iioPath_.c_str());
}

/* ------------------------------------------------------------------ */
/*  Channel reads                                                      */
/* ------------------------------------------------------------------ */

std::optional<double> ADC::readSingleEnded(int channel) const {
    if (channel < 0 || channel > 7) {
        logs::log(ERR, "ADC[%s]: invalid single-ended channel %d\n",
                  label_.c_str(), channel);
        return std::nullopt;
    }

    /* e.g. in_voltage3_raw, in_voltage3_scale */
    std::string chStr = std::to_string(channel);
    std::string rawPath = iioPath_ + "/in_voltage" + chStr + "_raw";
    std::string scalePath = iioPath_ + "/in_voltage" + chStr + "_scale";

    auto raw = readSysfsInt(rawPath);
    auto scale = readSysfsDouble(scalePath);
    if (!raw || !scale)
        return std::nullopt;

    /* raw * scale gives millivolts (IIO voltage convention) → convert to V */
    double volts = static_cast<double>(*raw) * (*scale) / 1000.0;
    return volts;
}

std::optional<double> ADC::readDifferential(int chPos, int chNeg) const {
    if (chPos < 0 || chPos > 7 || chNeg < 0 || chNeg > 7) {
        logs::log(ERR, "ADC[%s]: invalid differential pair %d-%d\n",
                  label_.c_str(), chPos, chNeg);
        return std::nullopt;
    }

    /* e.g. in_voltage0-voltage1_raw, in_voltage0-voltage1_scale */
    std::string rawPath = iioPath_ + "/in_voltage" + std::to_string(chPos) +
                          "-voltage" + std::to_string(chNeg) + "_raw";
    std::string scalePath = iioPath_ + "/in_voltage" + std::to_string(chPos) +
                            "-voltage" + std::to_string(chNeg) + "_scale";

    auto raw = readSysfsInt(rawPath);
    auto scale = readSysfsDouble(scalePath);
    if (!raw || !scale)
        return std::nullopt;

    double volts = static_cast<double>(*raw) * (*scale) / 1000.0;
    return volts;
}
