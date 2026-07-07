#ifndef ADC_HPP_
#define ADC_HPP_

#include <cstdint>
#include <optional>
#include <string>

class ADC {
  public:
    /**
     * @brief Construct an ADC backed by an ADS1256 IIO device.
     *
     * @param spiCs  SPI chip-select number as a string ("1", "2", "3").
     *               Used to discover which iio:deviceN corresponds to
     *               this particular ADS1256 by inspecting the device
     *               symlink under /sys/bus/iio/devices/.
     * @param label  Human-readable name stored alongside measurements
     *               in the database (e.g. "ads1256-voltage").
     */
    ADC(const std::string &spiCs, const std::string &label);

    /**
     * @brief Read a single-ended channel (0-7).
     * @return Voltage in Volts, or std::nullopt on failure.
     */
    std::optional<double> readSingleEnded(int channel) const;

    /**
     * @brief Read a differential channel pair (e.g. 0-1, 2-3, 4-5, 6-7).
     * @return Voltage in Volts, or std::nullopt on failure.
     */
    std::optional<double> readDifferential(int chPos, int chNeg) const;

    const std::string &label() const { return label_; }
    const std::string &iioPath() const { return iioPath_; }

  private:
    std::string discoverIioPath(const std::string &spiCs);
    std::optional<int> readSysfsInt(const std::string &path) const;
    std::optional<double> readSysfsDouble(const std::string &path) const;

    std::string label_;
    std::string iioPath_;
};

#endif
