#ifndef DAC_HPP_
#define DAC_HPP_

#include <cstdint>
#include <libdac81408/dac81408.h>
#include <memory>

class DAC {
  public:
    DAC(std::string spi_path, std::string gpio_ctrl, std::uint8_t rst_pin,
        std::uint8_t ldac_pin);
    int get_int_reference();
    int set_int_reference(dac81408_ref_state_t state);
    int get_channel_range(std::uint8_t ch);
    int set_channel_range(std::uint8_t ch, dac81408_range_t range);
    int set_channel_state(std::uint8_t ch, bool enabled);
    int set_channel_voltage(std::uint8_t ch, double out_voltage);
    bool is_channel_enabled(std::uint8_t ch);

  private:
    std::unique_ptr<dac81408_t> impl_;
    std::string spidev_path_;
    std::string gpiochip_name_;
};

#endif
