/*
 * =============================================================================
 * RadioHijackC - Status LED Implementation
 * =============================================================================
 *
 * Responsibilities:
 *   Drive the Pico W wireless-chip LED with clear, repeated network status
 *   patterns while avoiding blocking delays in the main server loop.
 *
 * Timing:
 *   Booting/connecting - 100 ms on, 100 ms off.
 *   Wi-Fi connected    - 80 ms on, 2920 ms off.
 *   Fallback AP        - 100 ms on, 120 ms off, 100 ms on, 2680 ms off.
 */

/**
 * @file status_led.cpp
 * @brief Pico W onboard LED status pattern implementation.
 */

#include "ui/status_led.hpp"

#include "pico/cyw43_arch.h"

namespace app {

namespace {

/** @brief Convert milliseconds to an absolute future time. @param ms Delay in milliseconds. @return Absolute deadline. */
absolute_time_t deadline(uint32_t ms) { return make_timeout_time_ms(ms); }

}  // namespace

/** @brief Construct the LED controller in Off mode. @param None. @return Constructed controller. */
StatusLed::StatusLed() : nextStep_(deadline(0)) {}

/** @brief Select a new LED pattern and start it immediately. @param mode New mode. @return Nothing. */
void StatusLed::setMode(Mode mode) {
  mode_ = mode;
  step_ = 0;
  bool on = false;
  const uint32_t duration = stepDurationMs(step_, on);
  write(on);
  nextStep_ = deadline(duration);
}

/** @brief Advance the LED pattern when its current duration expires. @param None. @return Nothing. */
void StatusLed::poll() {
  if (!time_reached(nextStep_)) {
    return;
  }

  bool on = false;
  ++step_;
  const uint32_t duration = stepDurationMs(step_, on);
  write(on);
  nextStep_ = deadline(duration);
}

/** @brief Write the physical Pico W LED if available. @param on Desired state. @return Nothing. */
void StatusLed::write(bool on) {
#ifdef CYW43_WL_GPIO_LED_PIN
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
#else
  (void)on;
#endif
}

/**
 * @brief Calculate one pattern step.
 * @param step Pattern step index. It wraps inside this function.
 * @param on Output parameter set to the LED state for the step.
 * @return Step duration in milliseconds.
 */
uint32_t StatusLed::stepDurationMs(uint8_t step, bool& on) const {
  switch (mode_) {
    case Mode::Booting:
      on = (step % 2) == 0;
      return 100;

    case Mode::WifiConnected:
      on = (step % 2) == 0;
      return on ? 80 : 2920;

    case Mode::FallbackAccessPoint: {
      const uint8_t phase = step % 4;
      on = phase == 0 || phase == 2;
      if (phase == 0 || phase == 2) {
        return 100;
      }
      return phase == 1 ? 120 : 2680;
    }

    case Mode::Off:
    default:
      on = false;
      return 1000;
  }
}

}  // namespace app
