#pragma once

#include <functional>

#include <juce_audio_basics/juce_audio_basics.h>

#include "HardwarePresetLibrary.h"
#include "MidiPortHub.h"

namespace conduit
{

//==============================================================================
/**
    Preset-Namen-Scan eines DSI-Geraets (MIDI-Rig M9b, ADR 007) — reiner
    Message-Thread-Zustandsautomat, getaktet ueber MidiPortHub::subscribeTick
    (60 Hz, Subsystem-Konvention statt eigenem Timer).

    Ablauf: start(deviceId) -> Port armen (setSysExCaptureEnabled) ->
    Universal Device Inquiry (Timeout kInquiryTimeoutMs, Fallback =
    fallbackDeviceIdByte) -> sequentielle Ping-Pong-Program-Dump-Requests
    (Timeout kDumpTimeoutMs, kMaxRetries Re-Requests, danach "?"-Name und
    weiter — NIE parallel, das Geraet beantwortet seriell) -> Ergebnis in
    die HardwarePresetLibrary + disarm.

    Antworten mit fremder Bank/Programm-Nummer werden ignoriert (verspaetete
    Dumps einer vorigen Runde); die Device-ID der Antwort wird NICHT
    gefiltert (DSI-Geraete antworten mit ihrer eigenen ID — massgeblich ist
    die angefragte Position). cancel() bricht ab ohne Library-Schreiben.

    Zeitbasis injizierbar (nowMs-Seam, Muster LiveRemoteBridge) — Tests
    treiben den Automaten ueber hub.drainNow() + Fake-Clock.
*/
class HardwarePresetScanner
{
public:
    HardwarePresetScanner (MidiPortHub& hubToUse, HardwarePresetLibrary& libraryToUse);
    ~HardwarePresetScanner();

    //==========================================================================
    static constexpr double kInquiryTimeoutMs = 200.0;
    static constexpr double kDumpTimeoutMs = 300.0;
    static constexpr int kMaxRetries = 2;

    /** Fallback-Device-ID, wenn die Inquiry unbeantwortet bleibt
        (Default: Mopho Desktop 0x25). */
    juce::uint8 fallbackDeviceIdByte;

    /** Fortschritt (done, total) — feuert nach jedem erledigten Programm. */
    std::function<void (int, int)> onProgress;

    /** Abschluss: true = Ergebnis in der Library, false = abgebrochen. */
    std::function<void (bool)> onFinished;

    /** Zeit-Seam fuer Tests; Default = Time::getMillisecondCounterHiRes. */
    std::function<double()> nowMs;

    //==========================================================================
    /** false, wenn schon ein Scan laeuft. [Message Thread] */
    bool start (const juce::Uuid& deviceId);

    /** Abbrechen (disarm, kein Library-Schreiben). No-op wenn idle. */
    void cancel();

    [[nodiscard]] bool isScanning() const noexcept { return state != State::idle; }
    [[nodiscard]] int totalPrograms() const noexcept;

private:
    //==========================================================================
    enum class State { idle, inquiry, scanning };

    void handleSysEx (const juce::MidiMessage& message);
    void tick();

    void beginDumpSequence (juce::uint8 deviceIdByte);
    void requestCurrent();
    void advance (juce::String name);
    void finish (bool success);

    [[nodiscard]] double now() const;

    //==========================================================================
    MidiPortHub& hub;
    HardwarePresetLibrary& library;

    State state = State::idle;
    juce::Uuid scanDeviceId;
    juce::uint8 activeDeviceIdByte = 0;

    int bank = 0, program = 0, retries = 0;
    double deadlineMs = 0.0;
    std::vector<juce::String> names;

    int sysexToken = -1, tickToken = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HardwarePresetScanner)
};

} // namespace conduit
