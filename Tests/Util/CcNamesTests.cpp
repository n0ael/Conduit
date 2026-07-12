#include <catch2/catch_test_macros.hpp>

#include "Util/CcNames.h"

//==============================================================================
TEST_CASE ("CcNames: bekannte CCs liefern ihren Funktionsnamen", "[ccnames]")
{
    CHECK (conduit::CcNames::displayName (1) == "Mod Wheel");
    CHECK (conduit::CcNames::displayName (11) == "Expression");
    CHECK (conduit::CcNames::displayName (74) == "Brightness");
    CHECK (conduit::CcNames::displayName (123) == "All Notes Off");
}

TEST_CASE ("CcNames: unbekannte CCs fallen auf 'CC {n}' zurueck", "[ccnames]")
{
    CHECK (conduit::CcNames::displayName (3) == "CC 3");
    CHECK (conduit::CcNames::displayName (0) == "CC 0");
    CHECK (conduit::CcNames::displayName (127) == "CC 127");
}

TEST_CASE ("CcNames: Tabelle ist frei von Dopplungen", "[ccnames]")
{
    for (size_t i = 0; i < std::size (conduit::CcNames::kTable); ++i)
        for (size_t j = i + 1; j < std::size (conduit::CcNames::kTable); ++j)
            CHECK (conduit::CcNames::kTable[i].cc != conduit::CcNames::kTable[j].cc);
}
