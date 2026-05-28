/**
 * @file preset_store.hpp
 * @brief Flash-backed preset station storage.
 *
 * MicroPython used a JSON file. Pico SDK firmware does not have a filesystem by
 * default, so this module stores a compact preset image in the last flash sector.
 */

#pragma once

#include <string>
#include <vector>

namespace app {

/**
 * @brief One named FM station preset.
 */
struct Preset {
  /** @brief Human-readable preset name displayed on the dashboard. */
  std::string name;
  /** @brief Frequency in MHz, rounded to one decimal place. */
  float frequency = 0.0f;
};

/**
 * @brief Manages station presets persisted in onboard flash.
 */
class PresetStore {
 public:
  /**
   * @brief Load presets from flash, or defaults if flash is empty/invalid.
   * @param None.
   * @return Nothing.
   */
  void load();

  /**
   * @brief Save the current preset list to flash.
   * @param None.
   * @return true if the save sequence completed.
   */
  bool save();

  /**
   * @brief Return all presets currently held in RAM.
   * @param None.
   * @return Constant reference to the preset list.
   */
  const std::vector<Preset>& all() const { return presets_; }

  /**
   * @brief Add or update a preset.
   * @param name Preset display name. Existing presets with the same name are updated.
   * @param frequency Station frequency in MHz.
   * @return Nothing.
   */
  void set(const std::string& name, float frequency);

  /**
   * @brief Delete a preset by name.
   * @param name Exact preset name to remove.
   * @return true if a preset was removed, false if no matching name existed.
   */
  bool remove(const std::string& name);

 private:
  /**
   * @brief Populate RAM with the original MicroPython default presets.
   * @param None.
   * @return Nothing.
   */
  void loadDefaults();

  std::vector<Preset> presets_;
};

}  // namespace app
