#include "HardwarePresetScanner.h"

#include "Sysex/DsiSysex.h"

namespace conduit
{

//==============================================================================
HardwarePresetScanner::HardwarePresetScanner (MidiPortHub& hubToUse,
                                              HardwarePresetLibrary& libraryToUse)
    : fallbackDeviceIdByte (midirig::dsi::kDeviceIdMophoDesktop),
      hub (hubToUse),
      library (libraryToUse)
{
}

HardwarePresetScanner::~HardwarePresetScanner()
{
    // Bei Destruktion KEINE Callbacks mehr feuern — deren Capture-Ziele
    // koennen bereits zerstoert sein (Member-Reihenfolge des Besitzers).
    onProgress = nullptr;
    onFinished = nullptr;
    cancel();
    hub.unsubscribe (tickToken);
}

int HardwarePresetScanner::totalPrograms() const noexcept
{
    return midirig::dsi::kBankCount * midirig::dsi::kProgramsPerBank;
}

double HardwarePresetScanner::now() const
{
    return nowMs != nullptr ? nowMs() : juce::Time::getMillisecondCounterHiRes();
}

//==============================================================================
bool HardwarePresetScanner::start (const juce::Uuid& deviceId)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if (state != State::idle || deviceId.isNull())
        return false;

    scanDeviceId = deviceId;
    names.clear();
    names.reserve ((size_t) totalPrograms());

    hub.setSysExCaptureEnabled (scanDeviceId, true);
    sysexToken = hub.subscribeSysEx (scanDeviceId,
        [this] (const juce::MidiMessage& message) { handleSysEx (message); });
    if (tickToken < 0)
        tickToken = hub.subscribeTick ([this] { tick(); });

    // Inquiry zuerst: die Antwort liefert die SysEx-Device-ID (DSI:
    // familyLsb). Bleibt sie aus, greift der Fallback nach dem Timeout.
    state = State::inquiry;
    deadlineMs = now() + kInquiryTimeoutMs;
    hub.send (scanDeviceId, midirig::dsi::makeDeviceInquiry());
    return true;
}

void HardwarePresetScanner::cancel()
{
    if (state == State::idle)
        return;

    finish (false);
}

//==============================================================================
void HardwarePresetScanner::handleSysEx (const juce::MidiMessage& message)
{
    const auto* raw = message.getRawData();
    const auto size = message.getRawDataSize();

    if (state == State::inquiry)
    {
        if (const auto reply = midirig::dsi::parseDeviceInquiryReply (raw, size))
            if (const auto deviceIdByte = midirig::dsi::deviceIdFromInquiry (*reply))
                beginDumpSequence (*deviceIdByte);
        return;
    }

    if (state != State::scanning)
        return;

    if (const auto dump = midirig::dsi::parseProgramDump (raw, size))
    {
        // Nur die AKTUELL angefragte Position zaehlt — verspaetete Dumps
        // einer vorigen Anfrage (Timeout-Rennen) werden ignoriert.
        if (dump->bank == bank && dump->program == program)
            advance (midirig::dsi::extractName (dump->data));
    }
}

void HardwarePresetScanner::tick()
{
    if (state == State::idle || now() < deadlineMs)
        return;

    if (state == State::inquiry)
    {
        beginDumpSequence (fallbackDeviceIdByte);   // keine Antwort -> Fallback
        return;
    }

    // Dump-Timeout: begrenzt neu anfragen, dann Platzhalter und weiter.
    if (retries < kMaxRetries)
    {
        ++retries;
        requestCurrent();
        return;
    }

    advance ("?");
}

//==============================================================================
void HardwarePresetScanner::beginDumpSequence (juce::uint8 deviceIdByte)
{
    activeDeviceIdByte = deviceIdByte;
    state = State::scanning;
    bank = 0;
    program = 0;
    retries = 0;
    requestCurrent();
}

void HardwarePresetScanner::requestCurrent()
{
    deadlineMs = now() + kDumpTimeoutMs;
    hub.send (scanDeviceId, midirig::dsi::makeProgramDumpRequest (activeDeviceIdByte,
                                                                  bank, program));
}

void HardwarePresetScanner::advance (juce::String name)
{
    names.push_back (std::move (name));

    if (onProgress != nullptr)
        onProgress ((int) names.size(), totalPrograms());

    ++program;
    if (program >= midirig::dsi::kProgramsPerBank)
    {
        program = 0;
        ++bank;
    }

    if (bank >= midirig::dsi::kBankCount)
    {
        finish (true);
        return;
    }

    retries = 0;
    requestCurrent();
}

void HardwarePresetScanner::finish (bool success)
{
    hub.setSysExCaptureEnabled (scanDeviceId, false);
    hub.unsubscribe (sysexToken);
    sysexToken = -1;
    state = State::idle;

    if (success)
    {
        HardwarePresetLibrary::DevicePresets presets;
        presets.deviceIdByte = activeDeviceIdByte;
        presets.programsPerBank = midirig::dsi::kProgramsPerBank;
        presets.names = std::move (names);
        library.setPresets (scanDeviceId, std::move (presets));
    }

    names.clear();

    if (onFinished != nullptr)
        onFinished (success);
}

} // namespace conduit
