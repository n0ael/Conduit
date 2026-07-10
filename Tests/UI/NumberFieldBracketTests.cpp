#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "UI/NumberFieldBracket.h"

using conduit::NumberFieldBracket;
using Config = NumberFieldBracket::Config;

//==============================================================================
TEST_CASE ("NumberFieldBracket: clampAndStep klemmt an min/max", "[numberfieldbracket]")
{
    Config cfg;
    cfg.minValue = 0.0;
    cfg.maxValue = 100.0;
    cfg.step = 1.0;

    CHECK (NumberFieldBracket::clampAndStep (-50.0, cfg) == Catch::Approx (0.0));
    CHECK (NumberFieldBracket::clampAndStep (500.0, cfg) == Catch::Approx (100.0));
    CHECK (NumberFieldBracket::clampAndStep (42.0, cfg) == Catch::Approx (42.0));
}

TEST_CASE ("NumberFieldBracket: clampAndStep rastet auf step", "[numberfieldbracket]")
{
    Config cfg;
    cfg.minValue = 0.0;
    cfg.maxValue = 100.0;
    cfg.step = 5.0;

    CHECK (NumberFieldBracket::clampAndStep (12.0, cfg) == Catch::Approx (10.0));
    CHECK (NumberFieldBracket::clampAndStep (13.0, cfg) == Catch::Approx (15.0));
    CHECK (NumberFieldBracket::clampAndStep (2.4, cfg) == Catch::Approx (0.0));
}

TEST_CASE ("NumberFieldBracket: step=0 bleibt stufenlos", "[numberfieldbracket]")
{
    Config cfg;
    cfg.minValue = 0.0;
    cfg.maxValue = 100.0;
    cfg.step = 0.0;

    CHECK (NumberFieldBracket::clampAndStep (12.345, cfg) == Catch::Approx (12.345));
}

TEST_CASE ("NumberFieldBracket: valueFromDrag -- Swipe nach oben erhoeht", "[numberfieldbracket]")
{
    Config cfg;
    cfg.minValue = 0.0;
    cfg.maxValue = 100.0;
    cfg.step = 0.0;
    cfg.unitsPerPixel = 0.5;

    // dragDeltaY negativ = Bewegung nach oben.
    CHECK (NumberFieldBracket::valueFromDrag (50.0, -20.0f, cfg) == Catch::Approx (60.0));
    CHECK (NumberFieldBracket::valueFromDrag (50.0, 20.0f, cfg) == Catch::Approx (40.0));
}

TEST_CASE ("NumberFieldBracket: valueFromDrag skaliert linear mit unitsPerPixel", "[numberfieldbracket]")
{
    Config cfg;
    cfg.minValue = -1000.0;
    cfg.maxValue = 1000.0;
    cfg.step = 0.0;
    cfg.unitsPerPixel = 2.0;

    CHECK (NumberFieldBracket::valueFromDrag (0.0, -10.0f, cfg) == Catch::Approx (20.0));
}

TEST_CASE ("NumberFieldBracket: valueFromDrag klemmt am Bereichsende", "[numberfieldbracket]")
{
    Config cfg;
    cfg.minValue = 0.0;
    cfg.maxValue = 10.0;
    cfg.step = 0.0;
    cfg.unitsPerPixel = 1.0;

    CHECK (NumberFieldBracket::valueFromDrag (9.0, -100.0f, cfg) == Catch::Approx (10.0));
    CHECK (NumberFieldBracket::valueFromDrag (1.0, 100.0f, cfg) == Catch::Approx (0.0));
}

//==============================================================================
TEST_CASE ("NumberFieldBracket: Default-Ctor startet auf defaultValue", "[numberfieldbracket]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    NumberFieldBracket field;
    CHECK (field.getValue() == Catch::Approx (50.0));
}

TEST_CASE ("NumberFieldBracket: Ctor mit Config setzt geklemmten Default", "[numberfieldbracket]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    Config cfg;
    cfg.minValue = 0.0;
    cfg.maxValue = 10.0;
    cfg.defaultValue = 999.0;
    cfg.step = 1.0;

    NumberFieldBracket field (cfg);
    CHECK (field.getValue() == Catch::Approx (10.0));
}

TEST_CASE ("NumberFieldBracket: setValue feuert onValueChanged nur bei echter Aenderung", "[numberfieldbracket]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    Config cfg;
    cfg.minValue = 0.0;
    cfg.maxValue = 100.0;
    cfg.defaultValue = 50.0;
    cfg.step = 1.0;

    NumberFieldBracket field (cfg);
    int callCount = 0;
    double lastValue = -1.0;
    field.onValueChanged = [&] (double v) { ++callCount; lastValue = v; };

    field.setValue (50.0);
    CHECK (callCount == 0);

    field.setValue (75.0);
    CHECK (callCount == 1);
    CHECK (lastValue == Catch::Approx (75.0));

    field.setValue (75.0);
    CHECK (callCount == 1);
}

TEST_CASE ("NumberFieldBracket: setValue mit dontSendNotification feuert nicht", "[numberfieldbracket]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    NumberFieldBracket field;
    bool fired = false;
    field.onValueChanged = [&] (double) { fired = true; };

    field.setValue (10.0, juce::dontSendNotification);
    CHECK_FALSE (fired);
    CHECK (field.getValue() == Catch::Approx (10.0));
}
