/*
 * =============================================================================
 * RadioHijackC - RDS Parser Implementation
 * =============================================================================
 *
 * Responsibilities:
 *   Decode Program Service station names and RadioText strings from RDS blocks
 *   read by the RDA5807M driver.
 *
 * Buffering:
 *   Station name uses 8 characters. RadioText uses 64 characters. Buffers are
 *   cleared when PI changes or when RadioText A/B flag toggles.
 *
 * Unsupported groups:
 *   Ignored cleanly so polling can continue without errors.
 */

/**
 * @file rds_parser.cpp
 * @brief RDS group parsing implementation for station name and RadioText.
 */

#include "rds_parser.hpp"

#include <algorithm>

namespace app {

/** @brief Construct and clear parser buffers. @param None. @return Constructed parser. */
RdsParser::RdsParser() { clear(); }

/** @brief Reset parser state for a new station. @param None. @return Nothing. */
void RdsParser::clear() {
  station_ = "Loading...";
  text_.clear();
  ps_.fill(' ');
  rt_.fill(' ');
  hasRtFlag_ = false;
  rtFlag_ = false;
  hasLastPi_ = false;
  lastPi_ = 0;
}

/**
 * @brief Process a complete four-word RDS group.
 * @param block1 RDS block A.
 * @param block2 RDS block B.
 * @param block3 RDS block C.
 * @param block4 RDS block D.
 * @return true when group 0 or group 2 was decoded.
 */
bool RdsParser::process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  const uint16_t pi = block1;
  if (hasLastPi_ && pi != lastPi_) {
    clear();
  }
  hasLastPi_ = true;
  lastPi_ = pi;

  const uint8_t groupType = (block2 >> 12) & 0x0F;
  const bool versionB = (block2 & 0x0800) != 0;

  if (groupType == 0) {
    const uint8_t segment = block2 & 0x03;
    const auto chars = charsFromWords(block4);
    const size_t index = segment * 2;
    ps_[index] = chars[0];
    ps_[index + 1] = chars[1];
    publishStation();
    return true;
  }

  if (groupType == 2) {
    const bool textFlag = (block2 & 0x0010) != 0;
    if (hasRtFlag_ && rtFlag_ != textFlag) {
      rt_.fill(' ');
    }
    hasRtFlag_ = true;
    rtFlag_ = textFlag;

    const uint8_t segment = block2 & 0x0F;
    const auto chars = versionB ? charsFromWords(block4) : charsFromWords(block3, block4);
    const size_t index = versionB ? segment * 2 : segment * 4;
    const size_t count = versionB ? 2 : 4;
    for (size_t i = 0; i < count && index + i < rt_.size(); ++i) {
      rt_[index + i] = chars[i];
    }
    publishText();
    return true;
  }

  return false;
}

/** @brief Sanitize an RDS byte for display. @param value Raw byte. @return Printable character or space. */
char RdsParser::printable(uint8_t value) {
  return (value >= 32 && value <= 126) ? static_cast<char>(value) : ' ';
}

/**
 * @brief Convert RDS words into display characters.
 * @param first First 16-bit RDS word.
 * @param second Second 16-bit RDS word, or zero when unused.
 * @return Four-character array in transmission order.
 */
std::array<char, 4> RdsParser::charsFromWords(uint16_t first, uint16_t second) {
  return {printable((first >> 8) & 0xFF), printable(first & 0xFF),
          printable((second >> 8) & 0xFF), printable(second & 0xFF)};
}

/** @brief Publish buffered station name after right-trimming spaces. @param None. @return Nothing. */
void RdsParser::publishStation() {
  std::string value(ps_.begin(), ps_.end());
  while (!value.empty() && value.back() == ' ') {
    value.pop_back();
  }
  if (!value.empty()) {
    station_ = value;
  }
}

/** @brief Publish buffered RadioText after trimming and CR termination. @param None. @return Nothing. */
void RdsParser::publishText() {
  std::string value(rt_.begin(), rt_.end());
  const auto carriage = value.find('\r');
  if (carriage != std::string::npos) {
    value.resize(carriage);
  }
  while (!value.empty() && value.back() == ' ') {
    value.pop_back();
  }
  const auto first = value.find_first_not_of(' ');
  if (first != std::string::npos) {
    text_ = value.substr(first);
  }
}

}  // namespace app
