/*
 * =============================================================================
 * RadioHijackC - RDS Parser Interface
 * =============================================================================
 *
 * Purpose:
 *   Declares the incremental parser for RDS station names and RadioText.
 *
 * Supported groups:
 *   Group 0 - Program Service station name.
 *   Group 2 - RadioText, including song/artist style messages.
 *
 * Main class:
 *   RdsParser - Maintains RDS character buffers and publishes clean strings.
 */

/**
 * @file rds_parser.hpp
 * @brief Parser for RDS station name and RadioText groups from the RDA5807M.
 */

#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace app {

/**
 * @brief Incrementally decodes the two RDS groups used by the dashboard.
 *
 * Group 0 supplies the Program Service station name. Group 2 supplies RadioText
 * such as song/artist strings. Unsupported groups are ignored.
 */
class RdsParser {
 public:
  /**
   * @brief Construct a parser with empty RDS buffers.
   * @param None.
   * @return Constructed parser.
   */
  RdsParser();

  /**
   * @brief Reset station/text buffers after a tune, seek, or PI change.
   * @param None.
   * @return Nothing.
   */
  void clear();

  /**
   * @brief Process one complete four-block RDS group.
   * @param block1 RDS block A, usually the Program Identification code.
   * @param block2 RDS block B, containing group type, version, and segment index.
   * @param block3 RDS block C, used by RadioText group 2A.
   * @param block4 RDS block D, used by station name and RadioText characters.
   * @return true if the group was supported and consumed, otherwise false.
   */
  bool process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4);

  /**
   * @brief Return the latest decoded station name.
   * @param None.
   * @return Constant reference to the station string.
   */
  const std::string& station() const { return station_; }

  /**
   * @brief Return the latest decoded RadioText string.
   * @param None.
   * @return Constant reference to the RadioText string.
   */
  const std::string& text() const { return text_; }

 private:
  /**
   * @brief Convert one RDS byte to a printable ASCII character.
   * @param value Raw RDS byte.
   * @return Printable ASCII character or space for unsupported bytes.
   */
  static char printable(uint8_t value);

  /**
   * @brief Extract up to four characters from one or two 16-bit RDS words.
   * @param first First RDS word.
   * @param second Optional second RDS word. Defaults to zero for two-character groups.
   * @return Array containing high-byte then low-byte characters from each word.
   */
  static std::array<char, 4> charsFromWords(uint16_t first, uint16_t second = 0);

  /**
   * @brief Publish buffered Program Service characters if they form a non-empty name.
   * @param None.
   * @return Nothing.
   */
  void publishStation();

  /**
   * @brief Publish buffered RadioText characters after trimming blanks and CR terminators.
   * @param None.
   * @return Nothing.
   */
  void publishText();

  std::string station_;
  std::string text_;
  std::array<char, 8> ps_{};
  std::array<char, 64> rt_{};
  bool hasRtFlag_ = false;
  bool rtFlag_ = false;
  bool hasLastPi_ = false;
  uint16_t lastPi_ = 0;
};

}  // namespace app
