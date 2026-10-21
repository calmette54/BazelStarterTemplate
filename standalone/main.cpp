/**
 * @file main.cpp
 * @brief Command-line front end for the greeter library.
 *
 * Usage: `hello-world [name] [en|de|es|fr]`. Prints a greeting for `name`
 * (default "world") in the given language (default English).
 *
 * @author Your Name
 * @date 2025-04-28
 */

#include <iostream>
#include <stdexcept>
#include <string>

#include "lib/greeter/include/greeter.h"

namespace {

  /**
   * @brief Maps a two-letter language code to greeter::LanguageCode.
   * @throw std::invalid_argument if the code is not supported
   */
  greeter::LanguageCode parse_language(const std::string& code) {
    if (code == "en") return greeter::LanguageCode::EN;
    if (code == "de") return greeter::LanguageCode::DE;
    if (code == "es") return greeter::LanguageCode::ES;
    if (code == "fr") return greeter::LanguageCode::FR;
    throw std::invalid_argument("unsupported language: " + code);
  }

}  // namespace

/**
 * @brief Entry point of the program.
 *
 * @return 0 on success, 1 on invalid arguments.
 *
 * @par Example
 * @code
 * $ ./hello-world Alice fr
 * Bonjour, Alice!
 * @endcode
 */
int main(int argc, char* argv[]) {
  try {
    const std::string name = argc > 1 ? argv[1] : "world";
    const auto lang = argc > 2 ? parse_language(argv[2]) : greeter::LanguageCode::EN;
    std::cout << greeter::Greeter(name).greet(lang) << std::endl;
    return 0;
  } catch (const std::invalid_argument& e) {
    std::cerr << "error: " << e.what() << "\nusage: " << argv[0] << " [name] [en|de|es|fr]"
              << std::endl;
    return 1;
  }
}
