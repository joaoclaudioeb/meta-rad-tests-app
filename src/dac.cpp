#include <fsatutils/errors.hpp>
#include <fsatutils/log/log.hpp>
#include <libdac81408/dac81408.h>
#include <memory>

#include <rad-tests-app/dac.hpp>

DAC::DAC(std::string spi_path, std::string gpiochip_name, std::uint8_t rst_pin,
         std::uint8_t ldac_pin)
    : spidev_path_{std::move(spi_path)}, gpiochip_name_{std::move(gpio_ctrl)} {
    impl_ = std::make_unique<dac81408_t>();

    dac81408_init(impl_.get(), spidev_path_.data(), gpiochip_name.data(),
                  rst_pin, ldac_pin);

    int res = dac81408_config(impl_.get());
    if (res < 0) {
        logs::log(ERR,
                  "Failed to configure dac81408: spidev[%s], gpiochip[%s]\n",
                  impl_->spidev_path, impl_->gpiochip_name);
        throw_runtime_error(
            "failed to configure dac81408: spidev[%s], gpiochip[%s]");
    }

    std::uint16_t dev_id =
        dac81408_read_register(impl_.get(), DAC81408_REG_DEVICEID);

    logs::log(DEBUG, "Initialized dac81408: spidev[%s], gpiochip[%s], id[%u]\n",
              impl_->spidev_path, impl_->gpiochip_name, dev_id);
}
