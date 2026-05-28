/*
 * =============================================================================
 * RadioHijackC - Embedded Dashboard Interface
 * =============================================================================
 *
 * Purpose:
 *   Declares access to the static HTML/CSS/JavaScript dashboard served from the
 *   Pico at / and /index.html.
 *
 * UI features:
 *   Tune, seek, step, volume, mute, scan, presets, RDS station text, RSSI,
 *   stereo status, and the current dashboard IP.
 */

/**
 * @file web_page.hpp
 * @brief Embedded HTML/CSS/JavaScript dashboard for browser radio control.
 */

#pragma once

namespace app {

/**
 * @brief Return the built-in web dashboard document.
 * @param None.
 * @return Pointer to a static null-terminated HTML string stored in flash.
 */
const char* webPageHtml();

}  // namespace app
