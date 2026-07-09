#ifndef DAC_HPP_
#define DAC_HPP_

#include <libdac81408/dac81408.h>

#include <cstdint>
#include <memory>

class DAC {
 public:
  DAC(std::string spiPath, std::string gpiochipName, std::uint8_t rstPin,
      std::uint8_t ldacPin);
  int getIntReference();
  int setIntReference(dac81408_ref_state_t state);
  int getChannelRange(std::uint8_t ch);
  int setChannelRange(std::uint8_t ch, dac81408_range_t range);
  int setChannelState(std::uint8_t ch, bool enabled);
  int setChannelVoltage(std::uint8_t ch, double outVoltage);
  bool isChannelEnabled(std::uint8_t ch);
  const char* spidev() const { return spidevPath_.data(); }

 private:
  std::unique_ptr<dac81408_t> impl_;
  std::string spidevPath_;
  std::string gpiochipName_;
};

#endif
