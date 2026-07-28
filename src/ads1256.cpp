#include <iio.h>

#include <fsatutils/log/log.hpp>
#include <rad-tests-app/ads1256.hpp>
#include <stdexcept>
#include <string>

Ads1256::Ads1256(std::shared_ptr<fsatutils::iio::Context> ctx,
                 std::string name) {
  name_ = name;

  dev_ = std::make_unique<fsatutils::iio::Device>(ctx, name);

  for (int i = 0; i < 8; ++i) {
    std::string ch_id = "voltage" + std::to_string(i);
    single_ended_.push_back(
        std::make_unique<fsatutils::iio::Channel>(ch_id, *dev_, false));
  }

  for (int i = 0; i < 4; ++i) {
    int pos = i * 2;
    int neg = i * 2 + 1;
    std::string ch_id =
        "voltage" + std::to_string(pos) + "-voltage" + std::to_string(neg);
    differential_.push_back(
        std::make_unique<fsatutils::iio::Channel>(ch_id, *dev_, false));
  }

  logs::log(INFO, "Initialized Ads1256 [%s]\n", name_.c_str());
}

std::optional<double> Ads1256::get_single_ended(int channel) const {
  long long raw;
  double scale;

  try {
    raw = single_ended_[channel]->read_attr<long long>("raw");
    scale = single_ended_[channel]->read_attr<double>("scale");
  } catch (std::runtime_error const& e) {
    logs::log(ERR, "Ads1256[%s]: failed to read single-ended ch[%d]\n",
              name_.c_str(), channel);
    return std::nullopt;
  }

  double volts = static_cast<double>(raw) * scale / 1000.0;
  return volts;
}

std::optional<double> Ads1256::get_differential(int pair) const {
  long long raw;
  double scale;

  try {
    raw = differential_[pair]->read_attr<long long>("raw");
    scale = differential_[pair]->read_attr<double>("scale");
  } catch (std::runtime_error const& e) {
    logs::log(ERR, "Ads1256[%s]: failed to read differential pair[%d]\n",
              name_.c_str(), pair);
    return std::nullopt;
  }

  double volts = static_cast<double>(raw) * scale / 1000.0;
  double amps = volts / SHUNT_OHMS;
  return amps;
}
