/*
 * =============================================================================
 * RadioHijackC - Preset Store Implementation
 * =============================================================================
 *
 * Responsibilities:
 *   Load presets from flash, save presets to flash, provide default station
 *   presets, and keep preset updates bounded by kMaxPresets.
 *
 * Flash layout:
 *   Last flash sector contains a magic number, version, count, and fixed-size
 *   preset entries. Frequencies are stored as MHz * 10 integers.
 *
 * Safety note:
 *   Interrupts are disabled during erase/program because code executes from
 *   external flash on Pico boards.
 */

/**
 * @file preset_store.cpp
 * @brief Flash persistence implementation for station presets.
 */

#include "storage/preset_store.hpp"

#include "config/app_config.hpp"
#include "util/logger.hpp"

#include <algorithm>
#include <array>
#include <cstring>

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/platform.h"

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif

namespace app {
namespace {

constexpr uint32_t kMagic = 0x5248444A;  // RHDJ
constexpr uint32_t kVersion = 1;
constexpr uint32_t kStorageOffset = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;

struct FlashPresetEntry {
  char name[config::kPresetNameLength];
  uint32_t frequencyTimes10;
};

struct FlashPresetImage {
  uint32_t magic;
  uint32_t version;
  uint32_t count;
  FlashPresetEntry entries[config::kMaxPresets];
};

static_assert(sizeof(FlashPresetImage) <= FLASH_SECTOR_SIZE, "Preset image must fit in one flash sector");

/**
 * @brief Convert flash integer frequency format to MHz.
 * @param value Frequency stored as MHz multiplied by 10.
 * @return Frequency in MHz.
 */
float fromStoredFrequency(uint32_t value) { return static_cast<float>(value) / 10.0f; }

/**
 * @brief Convert MHz frequency to compact flash integer format.
 * @param value Frequency in MHz.
 * @return Frequency multiplied by 10 and rounded to nearest integer.
 */
uint32_t toStoredFrequency(float value) { return static_cast<uint32_t>(value * 10.0f + 0.5f); }

}  // namespace

/**
 * @brief Load presets from the last flash sector.
 * @param None.
 * @return Nothing. Defaults are loaded if the flash image is invalid or empty.
 */
void PresetStore::load() {
  presets_.clear();
  const auto* image = reinterpret_cast<const FlashPresetImage*>(XIP_BASE + kStorageOffset);
  if (image->magic == kMagic && image->version == kVersion && image->count <= config::kMaxPresets) {
    for (uint32_t i = 0; i < image->count; ++i) {
      const auto& entry = image->entries[i];
      if (entry.name[0] == '\0' || entry.name[0] == static_cast<char>(0xFF)) {
        continue;
      }
      std::string name(entry.name, strnlen(entry.name, sizeof(entry.name)));
      presets_.push_back({name, fromStoredFrequency(entry.frequencyTimes10)});
    }
  }

  if (presets_.empty()) {
    loadDefaults();
  }
}

/**
 * @brief Erase and rewrite the preset flash sector.
 * @param None.
 * @return true after erase/program completes.
 */
bool PresetStore::save() {
  std::array<uint8_t, FLASH_SECTOR_SIZE> sector{};
  sector.fill(0xFF);

  auto* image = reinterpret_cast<FlashPresetImage*>(sector.data());
  image->magic = kMagic;
  image->version = kVersion;
  image->count = std::min<size_t>(presets_.size(), config::kMaxPresets);

  for (uint32_t i = 0; i < image->count; ++i) {
    auto& entry = image->entries[i];
    std::memset(entry.name, 0, sizeof(entry.name));
    std::strncpy(entry.name, presets_[i].name.c_str(), sizeof(entry.name) - 1);
    entry.frequencyTimes10 = toStoredFrequency(presets_[i].frequency);
  }

  const uint32_t interrupts = save_and_disable_interrupts();
  flash_range_erase(kStorageOffset, FLASH_SECTOR_SIZE);
  flash_range_program(kStorageOffset, sector.data(), sector.size());
  restore_interrupts(interrupts);

  Logger::info("Saved %u presets", static_cast<unsigned>(image->count));
  return true;
}

/**
 * @brief Insert or replace one preset in RAM.
 * @param name Preset name used as the unique key.
 * @param frequency Frequency in MHz.
 * @return Nothing.
 */
void PresetStore::set(const std::string& name, float frequency) {
  auto existing = std::find_if(presets_.begin(), presets_.end(), [&](const Preset& preset) {
    return preset.name == name;
  });
  if (existing != presets_.end()) {
    existing->frequency = frequency;
    return;
  }
  if (presets_.size() >= config::kMaxPresets) {
    presets_.erase(presets_.begin());
  }
  presets_.push_back({name, frequency});
}

/**
 * @brief Remove a preset by exact name.
 * @param name Preset name to delete.
 * @return true if a preset was removed.
 */
bool PresetStore::remove(const std::string& name) {
  const auto before = presets_.size();
  presets_.erase(std::remove_if(presets_.begin(), presets_.end(), [&](const Preset& preset) {
                   return preset.name == name;
                 }),
                 presets_.end());
  return presets_.size() != before;
}

/**
 * @brief Load the built-in station list used by the original Python version.
 * @param None.
 * @return Nothing.
 */
void PresetStore::loadDefaults() {
  presets_ = {{"Hot 89.9", 89.9f},       {"CBC Radio One 91.5", 91.5f},
              {"Boom 99.7", 99.7f},      {"Majic 100.3", 100.3f},
              {"KISS 105.3", 105.3f}};
}

}  // namespace app
