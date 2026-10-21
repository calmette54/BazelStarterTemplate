#include "lib/greeter/include/greeter.h"

#include <fmt/format.h>

#include <stdexcept>
#include <utility>

namespace greeter {

  Greeter::Greeter(std::string name) : name(std::move(name)) {
    if (this->name.empty()) {
      throw std::invalid_argument("Greeter: name must not be empty");
    }
  }

  std::string Greeter::greet(LanguageCode lang) const {
    switch (lang) {
      default:
      case LanguageCode::EN:
        return fmt::format("Hello, {}!", name);
      case LanguageCode::DE:
        return fmt::format("Hallo, {}!", name);
      case LanguageCode::ES:
        return fmt::format("¡Hola, {}!", name);
      case LanguageCode::FR:
        return fmt::format("Bonjour, {}!", name);
    }
  }

}  // namespace greeter
