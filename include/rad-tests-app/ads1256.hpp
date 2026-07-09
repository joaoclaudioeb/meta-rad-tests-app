#ifndef ADS1256_HPP_
#define ADS1256_HPP_

#include <fsatutils/iio/channel.hpp>
#include <fsatutils/iio/context.hpp>
#include <fsatutils/iio/device.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Ads1256 {
 public:
  Ads1256(std::shared_ptr<fsatutils::iio::Context> ctx, std::string name);

  std::optional<double> get_single_ended(int channel) const;
  std::optional<double> get_differential(int pair) const;

  const std::string& name() const { return name_; }

 private:
  std::string name_;

  std::unique_ptr<fsatutils::iio::Device> dev_;

  std::vector<std::unique_ptr<fsatutils::iio::Channel>> single_ended_;
  std::vector<std::unique_ptr<fsatutils::iio::Channel>> differential_;
};

#endif  // ADS1256_HPP_
