#include <catch2/catch_test_macros.hpp>

#include "Core/EngineProcessor.h"
#include "TestSettingsFolder.h"

//==============================================================================
TEST_CASE ("Looper-Quelle (B3): Schlüssel-Auflösung, Arming und Persistenz", "[looper]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    conduit::test::ScopedSettingsFolder settingsFolder;
    conduit::EngineProcessor engine { settingsFolder.folder };
    auto& capture = engine.getCaptureService();

    engine.setPlayConfigDetails (2, 2, 48000.0, 480);
    engine.prepareToPlay (48000.0, 480);

    SECTION ("Default 'master': prepareToPlay armt die Master-Tap-Kanäle")
    {
        REQUIRE (engine.getTransportSettings().getLooperSource() == "master");
        REQUIRE (engine.getLooperLeftIndex() == 2);   // hinter 2 Hardware-Kanälen
        REQUIRE (engine.getLooperRightIndex() == 3);
        REQUIRE (capture.isChannelArmed (2));
        REQUIRE (capture.isChannelArmed (3));
    }

    SECTION ("Quellwechsel auf 'hw:0' entwaffnet den Vorgänger")
    {
        engine.setLooperSource ("hw:0");

        REQUIRE (engine.getLooperLeftIndex() == 0);
        REQUIRE (engine.getLooperRightIndex() == 1);
        REQUIRE (capture.isChannelArmed (0));
        REQUIRE (capture.isChannelArmed (1));
        REQUIRE_FALSE (capture.isChannelArmed (2));
        REQUIRE_FALSE (capture.isChannelArmed (3));

        // Persistenz: der Schlüssel liegt in den TransportSettings
        REQUIRE (engine.getTransportSettings().getLooperSource() == "hw:0");
    }

    SECTION ("Hardware-Paar außerhalb der Kanalzahl bleibt unaufgelöst")
    {
        engine.setLooperSource ("hw:7");  // Kanäle 14/15 — Interface hat 2

        REQUIRE (engine.getLooperLeftIndex() == -1);
        REQUIRE (engine.getLooperRightIndex() == -1);
        REQUIRE_FALSE (capture.isChannelArmed (2));
    }

    SECTION ("Stereo-Tap: Basisname löst das _l/_r-Paar auf")
    {
        // Modul-Tap simulieren (Muster CaptureTapModule: _l/_r pro Seite);
        // nach prepare sind alle Kanäle idle → der Satz erweitert verlustfrei
        auto left  = capture.registerVirtualChannel ("delay_1_l");
        auto right = capture.registerVirtualChannel ("delay_1_r");
        REQUIRE (left.isValid());
        REQUIRE (right.isValid());

        engine.setLooperSource ("tap:delay_1");

        const auto expectedLeft  = capture.getVirtualChannelUiInfo (left.slot).captureIndex;
        const auto expectedRight = capture.getVirtualChannelUiInfo (right.slot).captureIndex;
        REQUIRE (expectedLeft >= 0);
        REQUIRE (engine.getLooperLeftIndex() == expectedLeft);
        REQUIRE (engine.getLooperRightIndex() == expectedRight);
        REQUIRE (capture.isChannelArmed (expectedLeft));
        REQUIRE (capture.isChannelArmed (expectedRight));

        capture.unregisterVirtualChannel (left);
        capture.unregisterVirtualChannel (right);
    }

    SECTION ("Mono-Tap: rechts folgt links")
    {
        auto mono = capture.registerVirtualChannel ("cv_solo");
        REQUIRE (mono.isValid());

        engine.setLooperSource ("tap:cv_solo");

        const auto expected = capture.getVirtualChannelUiInfo (mono.slot).captureIndex;
        REQUIRE (expected >= 0);
        REQUIRE (engine.getLooperLeftIndex() == expected);
        REQUIRE (engine.getLooperRightIndex() == expected);
        REQUIRE (capture.isChannelArmed (expected));

        capture.unregisterVirtualChannel (mono);
    }

    SECTION ("Unbekannter Tap-Name: nichts gearmt, Indizes -1")
    {
        engine.setLooperSource ("tap:gibts_nicht");

        REQUIRE (engine.getLooperLeftIndex() == -1);
        REQUIRE (engine.getLooperRightIndex() == -1);
    }
}

//==============================================================================
TEST_CASE ("Looper-Quellen (M4): Arming-Refcount über mehrere Looper", "[looper]")
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    conduit::test::ScopedSettingsFolder settingsFolder;
    conduit::EngineProcessor engine { settingsFolder.folder };
    auto& capture = engine.getCaptureService();

    engine.setPlayConfigDetails (2, 2, 48000.0, 480);
    engine.prepareToPlay (48000.0, 480);

    SECTION ("Geteilte Quelle bleibt gearmt, bis der LETZTE Looper sie verlässt")
    {
        engine.setLooperSource (0, "hw:0");
        engine.setLooperSource (1, "hw:0");
        REQUIRE (engine.getLooperLeftIndex (0) == 0);
        REQUIRE (engine.getLooperLeftIndex (1) == 0);
        REQUIRE (capture.isChannelArmed (0));
        REQUIRE (capture.isChannelArmed (1));

        // Looper 1 wechselt weg — Looper 0 nutzt hw:0 weiter: Gate offen!
        engine.setLooperSource (1, "master");
        REQUIRE (capture.isChannelArmed (0));
        REQUIRE (capture.isChannelArmed (1));
        REQUIRE (capture.isChannelArmed (2));   // master_l dazu
        REQUIRE (capture.isChannelArmed (3));

        // Jetzt verlässt auch Looper 0 → hw:0 wird entwaffnet
        engine.setLooperSource (0, "master");
        REQUIRE_FALSE (capture.isChannelArmed (0));
        REQUIRE_FALSE (capture.isChannelArmed (1));
        REQUIRE (capture.isChannelArmed (2));
        REQUIRE (capture.isChannelArmed (3));
    }

    SECTION ("Leerer Schlüssel = keine Quelle; unabhängige Auflösung pro Looper")
    {
        REQUIRE (engine.getLooperLeftIndex (1) == -1);
        REQUIRE (engine.getLooperLeftIndex (3) == -1);

        engine.setLooperSource (2, "hw:0");
        REQUIRE (engine.getLooperLeftIndex (2) == 0);
        REQUIRE (engine.getLooperRightIndex (2) == 1);

        // Looper 0 (Default master) blieb unberührt
        REQUIRE (engine.getLooperLeftIndex (0) == 2);
        REQUIRE (capture.isChannelArmed (2));

        engine.setLooperSource (2, "");
        REQUIRE (engine.getLooperLeftIndex (2) == -1);
        REQUIRE_FALSE (capture.isChannelArmed (0));
    }

    SECTION ("Vier Taps: jeder Looper hat seinen eigenen Waveform-Binner")
    {
        // Verschiedene Instanzen — Strip-Kontrakt (ein Strip pro Tap)
        REQUIRE (&engine.getLooperWaveformTap (0) != &engine.getLooperWaveformTap (1));
        REQUIRE (&engine.getLooperWaveformTap (1) != &engine.getLooperWaveformTap (2));
        REQUIRE (&engine.getLooperWaveformTap (2) != &engine.getLooperWaveformTap (3));
    }
}
