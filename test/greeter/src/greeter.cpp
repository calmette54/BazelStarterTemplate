#include "lib/greeter/include/greeter.h"

#include <stdexcept>
#include <string>

#include "doctest/doctest.h"

TEST_CASE("Greeter greets in every supported language") {
  using namespace greeter;

  const Greeter greeter("Tests");

  CHECK(greeter.greet(LanguageCode::EN) == "Hello, Tests!");
  CHECK(greeter.greet(LanguageCode::DE) == "Hallo, Tests!");
  CHECK(greeter.greet(LanguageCode::ES) == "¡Hola, Tests!");
  CHECK(greeter.greet(LanguageCode::FR) == "Bonjour, Tests!");
}

TEST_CASE("Greeter defaults to English") {
  using namespace greeter;

  CHECK(Greeter("Tests").greet() == "Hello, Tests!");
}

TEST_CASE("Greeter rejects an empty name") {
  using namespace greeter;

  CHECK_THROWS_AS(Greeter(""), std::invalid_argument);
}
