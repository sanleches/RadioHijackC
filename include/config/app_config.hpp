/*
 * =============================================================================
 * RadioHijackC - Application Configuration
 * =============================================================================
 *
 * Board:  Raspberry Pi Pico 2 W (RP2350)
 * Stack:  Pico SDK 2.2.0 + CYW43 Wi-Fi + lwIP raw API
 *
 * Purpose:
 *   Central index of compile-time settings used by the whole firmware.
 *
 * Contains:
 *   Wi-Fi station credentials, fallback AP credentials, RDA5807M I2C pinout,
 *   HTTP server constants, default radio settings, and preset limits.
 *
 * Edit here when changing:
 *   Network names/passwords, SDA/SCL pins, default station/volume, HTTP port,
 *   or maximum preset capacity.
 */

/**
 * @file app_config.hpp
 * @brief Compile-time configuration for the RadioHijack firmware.
 *
 * This file centralizes Wi-Fi credentials, fallback AP settings, I2C pinout,
 * HTTP server settings, and radio defaults so hardware or network changes only
 * require editing one place.
 */

#pragma once

#include <cstdint>

namespace app {
namespace config {

// Network used first. If STA connection fails, the firmware starts the fallback AP.
inline constexpr const char* kWifiSsid = "YourApName";
inline constexpr const char* kWifiPassword = "YourWifiPassword";

inline constexpr const char* kApSsid = "RadioHijack-AP";
inline constexpr const char* kApPassword = "radio1234";

// RDA5807M is wired to I2C0 on the original MicroPython project.
inline constexpr uint8_t kI2cId = 0;
inline constexpr uint8_t kSdaPin = 4;
inline constexpr uint8_t kSclPin = 5;
inline constexpr uint32_t kI2cFrequencyHz = 100000;

inline constexpr float kDefaultFrequencyMhz = 89.9f;
inline constexpr uint8_t kDefaultVolume = 5;

inline constexpr uint16_t kHttpPort = 80;
inline constexpr int kHttpListenBacklog = 4;
inline constexpr int kHttpClientTimeoutMs = 3000;
inline constexpr int kHttpAcceptPollMs = 250;

inline constexpr uint8_t kMaxPresets = 16;
inline constexpr uint8_t kPresetNameLength = 32;

}  // namespace config
}  // namespace app
