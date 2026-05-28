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
