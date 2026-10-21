/**
 * @file greeter.h
 * @brief Header file for the greeter library.
 *
 * The Greeter library allows you to create personalized greeting messages
 * in different languages.
 *
 * @author Your Name
 * @date 2025-04-28
 */
#pragma once

#include <cstdint>
#include <string>

namespace greeter {

  /**
   * @enum LanguageCode
   * @brief Language codes to be used with the Greeter class
   *
   * This enum defines the supported languages for greeting messages.
   */
  enum class LanguageCode : std::uint8_t {
    EN, /**< English */
    DE, /**< German (Deutsch) */
    ES, /**< Spanish (Español) */
    FR  /**< French (Français) */
  };

  /**
   * @class Greeter
   * @brief A class for saying hello in multiple languages
   *
   * The Greeter class allows you to create personalized greeting messages
   * in different languages. It stores a name and provides a method to
   * generate a greeting string localized to the specified language.
   *
   * @note Currently supported languages are English, German, Spanish, and French.
   *
   * @par Example
   * @code
   * greeter::Greeter g("Alice");
   * std::cout << g.greet(greeter::LanguageCode::FR) << std::endl; // Bonjour, Alice!
   * @endcode
   */
  class Greeter {
    std::string name; /**< The name of the person to greet */
  public:
    /**
     * @brief Constructs a new Greeter object
     * @param name The name to greet
     *
     * Initializes the Greeter with the given name. This name will be used
     * in the greeting messages.
     *
     * @throw std::invalid_argument if the name is empty
     */
    explicit Greeter(std::string name);

    /**
     * @brief Creates a localized greeting string
     * @param lang The language code to use for the greeting (default is English)
     * @return A std::string containing the greeting message
     *
     * Generates a greeting message in the specified language. If the language
     * is not recognized, it defaults to English.
     *
     * Supported greetings:
     * - English: "Hello, <name>!"
     * - German: "Hallo, <name>!"
     * - Spanish: "¡Hola, <name>!"
     * - French: "Bonjour, <name>!"
     *
     * @note The returned string includes punctuation and capitalization appropriate
     *       for the language.
     */
    [[nodiscard]] std::string greet(LanguageCode lang = LanguageCode::EN) const;
  };

}  // namespace greeter
