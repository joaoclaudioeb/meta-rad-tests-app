#include <cstdint>
#include <fsatutils/errors.hpp>
#include <fsatutils/log/log.hpp>
#include <libdac81408/dac81408.h>
#include <memory>

#include <rad-tests-app/dac.hpp>

DAC::DAC(std::string spiPath, std::string gpiochipName, std::uint8_t rstPin,
         std::uint8_t ldacPin)
    : spidevPath_{std::move(spiPath)}, gpiochipName_{std::move(gpiochipName)} {
    impl_ = std::make_unique<dac81408_t>();

    dac81408_init(impl_.get(), spidevPath_.data(), gpiochipName.data(), rstPin,
                  ldacPin);

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

int DAC::setChannelVoltage(std::uint8_t ch, double outVoltage) {
    int range = getChannelRange(ch);
    double max = 0.0;
    bool isBipolar = false;

    switch (range) {
    case DAC81408_RANGE_0_5V:
        max = 5.0;
        break;
    case DAC81408_RANGE_0_10V:
        max = 10.0;
        break;
    case DAC81408_RANGE_0_20V:
        max = 20.0;
        break;
    case DAC81408_RANGE_0_40V:
        max = 40.0;
        break;
    case DAC81408_RANGE_BIPOLAR_5V:
        max = 5.0;
        isBipolar = true;
        break;
    case DAC81408_RANGE_BIPOLAR_10V:
        max = 10.0;
        isBipolar = true;
        break;
    case DAC81408_RANGE_BIPOLAR_20V:
        max = 20.0;
        isBipolar = true;
        break;
    case DAC81408_RANGE_BIPOLAR_2V5:
        max = 20.0;
        isBipolar = true;
        break;
    default:
        logs::log(ERR, "Invalid DAC range! channel[%u], range[%i]\n", ch,
                  range);
        return -1;
    }

    if (isBipolar) {
        logs::log(ERR, "Bipolar is not implemented! range[%i]\n", ch, range);
        return -1;
    }

    if (outVoltage > max) {
        logs::log(ERR,
                  "Invalid set point for range! set_point[%.2f], max[%.2f]\n",
                  outVoltage, max);
        return -1;
    }

    std::uint16_t val =
        static_cast<std::uint16_t>(outVoltage / max * UINT16_MAX);

    dac81408_set_out(impl_.get(), ch, val);

    return 0;
}

int DAC::setIntReference(dac81408_ref_state_t state) {
    dac81408_set_int_reference(impl_.get(), state);

    return 0;
}

int DAC::getChannelRange(std::uint8_t ch) {
    return dac81408_get_range(impl_.get(), ch);
}

int DAC::setChannelRange(std::uint8_t ch, dac81408_range_t range) {
    dac81408_set_range(impl_.get(), ch, range);

    return 0;
}

int DAC::setChannelState(std::uint8_t ch, bool enabled) {
    dac81408_set_ch_enabled(impl_.get(), ch, enabled);
    return 0;
}

bool DAC::isChannelEnabled(std::uint8_t ch) {
    return dac81408_get_ch_enabled(impl_.get(), ch);
}

int DAC::getIntReference() { return dac81408_get_int_reference(impl_.get()); }
