#include <catch2/catch_test_macros.hpp>

#include "Core/EngineProcessor.h"

namespace
{

// Ein Layout mit genau einem Ein- und einem Ausgangs-Bus (wie der
// EngineProcessor sie deklariert), diskret in der gewünschten Kanalzahl —
// analog zu AudioProcessorPlayer::NumChannels::toLayout().
juce::AudioProcessor::BusesLayout ioLayout (int ins, int outs)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add  (juce::AudioChannelSet::discreteChannels (ins));
    layout.outputBuses.add (juce::AudioChannelSet::discreteChannels (outs));
    return layout;
}

} // namespace

//==============================================================================
// Schritt A: Der EngineProcessor akzeptiert die echte Device-I/O-Kanalzahl,
// damit der AudioProcessorPlayer sie vor dem Stereo-Default annimmt
// (findMostSuitableLayout) und bis in den Graph durchreicht.
//==============================================================================
TEST_CASE ("EngineProcessor akzeptiert Multichannel-I/O-Layouts", "[engine][io]")
{
    conduit::EngineProcessor engine;

    SECTION ("echte Hardware-Kanalzahl (8/8) wird angenommen")
        REQUIRE (engine.checkBusesLayoutSupported (ioLayout (8, 8)));

    SECTION ("asymmetrisches Interface (2 In / 8 Out) wird angenommen")
        REQUIRE (engine.checkBusesLayoutSupported (ioLayout (2, 8)));

    SECTION ("ungerade Kanalzahl (6/6) wird angenommen")
        REQUIRE (engine.checkBusesLayoutSupported (ioLayout (6, 6)));

    SECTION ("Ausgabe-only-Interface (0 Eingänge) ist zulässig")
        REQUIRE (engine.checkBusesLayoutSupported (ioLayout (0, 2)));

    SECTION ("kein Ausgang wird abgelehnt")
        REQUIRE_FALSE (engine.checkBusesLayoutSupported (ioLayout (2, 0)));
}

TEST_CASE ("EngineProcessor reicht die Device-Kanalzahl bis zum Graph durch", "[engine][io]")
{
    conduit::EngineProcessor engine;

    // Genau der Aufruf des AudioProcessorPlayer nach findMostSuitableLayout:
    // setPlayConfigDetails wählt das nächstbeste unterstützte Layout. Da wir
    // die echte Kanalzahl akzeptieren, tragen Prozessor und (via prepareToPlay)
    // Graph danach genau diese Zahl.
    engine.setPlayConfigDetails (8, 8, 48000.0, 32);
    REQUIRE (engine.getTotalNumInputChannels()  == 8);
    REQUIRE (engine.getTotalNumOutputChannels() == 8);

    SECTION ("Ausgabe-only: 0 Eingänge, Ausgänge stehen")
    {
        engine.setPlayConfigDetails (0, 2, 48000.0, 32);
        REQUIRE (engine.getTotalNumInputChannels()  == 0);
        REQUIRE (engine.getTotalNumOutputChannels() == 2);
    }
}
