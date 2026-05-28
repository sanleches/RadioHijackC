/*
 * =============================================================================
 * RadioHijackC - Status LED Interface
 * =============================================================================
 *
 * Purpose:
 *   Declares the Pico W onboard LED pattern controller used to show network
 *   state without opening USB serial or the web page.
 *
 * Patterns:
 *   Booting/connecting - Fast continuous blink.
 *   Wi-Fi connected    - One short blink followed by a long pause.
 *   Fallback AP        - Double blink followed by a long pause.
 *
 * Main class:
 *   StatusLed - Poll-driven, non-blocking LED state machine.
 */

/**
 * @file status_led.hpp
 * @brief Non-blocking Pico W onboard LED status patterns.
 */

#pragma once

#include "pico/time.h"

namespace app {

/**
 * @brief Network/status LED pattern controller.
 */
class StatusLed {
 public:
  /**
   * @brief Available LED display modes.
   */
  enum class Mode {
    /** @brief LED disabled/off. */
    Off,
    /** @brief Fast blink used while booting or connecting. */
    Booting,
    /** @brief One short blink and long pause for normal Wi-Fi station mode. */
    WifiConnected,
    /** @brief Double blink and long pause for fallback access point mode. */
    FallbackAccessPoint,
  };

  /**
   * @brief Construct the LED controller in Off mode.
   * @param None.
   * @return Constructed controller.
   */
  StatusLed();

  /**
   * @brief Change the active LED pattern.
   * @param mode New LED mode.
   * @return Nothing.
   */
  void setMode(Mode mode);

  /**
   * @brief Advance the LED state machine if the current step expired.
   * @param None.
   * @return Nothing.
   */
  void poll();

  /**
   * @brief Return the active LED mode.
   * @param None.
   * @return Current mode.
   */
  Mode mode() const { return mode_; }

 private:
  /**
   * @brief Apply physical LED output.
   * @param on true to turn LED on, false to turn it off.
   * @return Nothing.
   */
  void write(bool on);

  /**
   * @brief Return the duration of the current step for the active mode.
   * @param step Pattern step index.
   * @param on Output parameter receiving whether the LED should be on for this step.
   * @return Step duration in milliseconds.
   */
  uint32_t stepDurationMs(uint8_t step, bool& on) const;

  Mode mode_ = Mode::Off;
  uint8_t step_ = 0;
  absolute_time_t nextStep_;
};

}  // namespace app
