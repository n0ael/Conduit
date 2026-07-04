#include <catch2/catch_test_macros.hpp>

#include "Core/AudioDeviceController.h"

using conduit::AudioDeviceController;

//==============================================================================
// computeWarning ist ein reiner, device-freier Helfer (CLAUDE.md 3.2/9.1):
// leer = alles im Ziel, sonst der Warntext für die Toolbar.
// Buffer-Fenster 64–256 (Feld-Lektion 04.07.2026: 32 riss Callback-Deadlines).

TEST_CASE ("AudioDeviceController: Zielwerte erzeugen keine Warnung", "[audiodevice]")
{
    CHECK (AudioDeviceController::computeWarning (48000.0, 64).isEmpty());
    CHECK (AudioDeviceController::computeWarning (48000.0, 128).isEmpty());
    CHECK (AudioDeviceController::computeWarning (48000.0, 256).isEmpty());
    CHECK (AudioDeviceController::computeWarning (44100.0, 128).isEmpty());  // Fallback-Rate
}

TEST_CASE ("AudioDeviceController: abweichende Rate warnt", "[audiodevice]")
{
    const auto warning = AudioDeviceController::computeWarning (96000.0, 128);
    REQUIRE (warning.isNotEmpty());
    CHECK (warning.contains ("96000"));
    CHECK (warning.contains ("Ziel: 48000 Hz / 64-256"));
}

TEST_CASE ("AudioDeviceController: Buffer ausserhalb 64-256 warnt", "[audiodevice]")
{
    // Zu klein: Callback-Deadline-Risiko (32 = alte Zielgröße, jetzt Warnung)
    CHECK (AudioDeviceController::computeWarning (48000.0, 32).isNotEmpty());
    CHECK (AudioDeviceController::computeWarning (48000.0, 63).isNotEmpty());

    // Zu groß: spürbare Latenz
    CHECK (AudioDeviceController::computeWarning (48000.0, 257).isNotEmpty());
    CHECK (AudioDeviceController::computeWarning (48000.0, 512).isNotEmpty());

    // Grenzwerte inklusiv: 64 und 256 sind ok
    CHECK (AudioDeviceController::computeWarning (48000.0, 64).isEmpty());
    CHECK (AudioDeviceController::computeWarning (48000.0, 256).isEmpty());
}

TEST_CASE ("AudioDeviceController: Warntext enthält Rate und Buffer", "[audiodevice]")
{
    const auto warning = AudioDeviceController::computeWarning (44100.0, 512);
    REQUIRE (warning.isNotEmpty());  // Buffer > 256
    CHECK (warning.contains ("44100"));
    CHECK (warning.contains ("512"));
    CHECK (warning.contains ("Samples"));
}
