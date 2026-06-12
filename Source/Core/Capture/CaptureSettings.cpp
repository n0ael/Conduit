#include "CaptureSettings.h"

namespace conduit
{

namespace
{
    constexpr const char* keyBufferMinutes   = "capture.bufferMinutes";
    constexpr const char* keyPreRollSeconds  = "capture.preRollSeconds";
    constexpr const char* keyThresholdDb     = "capture.thresholdDb";
    constexpr const char* keyHoldMinutes     = "capture.holdMinutes";
    constexpr const char* keyAutoCalibrate   = "capture.autoCalibrate";
    constexpr const char* keyRamLimitGb      = "capture.ramLimitGb";
    constexpr const char* keyExportDirectory = "capture.exportDirectory";
    constexpr const char* keyExportBitDepth  = "capture.exportBitDepth";

    /** Nur 16/24/32 sind gültige Export-Bittiefen — alles andere fällt
        defensiv auf den Default zurück (auch bei editierter Settings-Datei). */
    int snapBitDepth (int requested) noexcept
    {
        return (requested == 16 || requested == 24 || requested == 32)
             ? requested
             : CaptureSettings::defaultExportBitDepth;
    }
}

//==============================================================================
juce::PropertiesFile::Options CaptureSettings::defaultOptions()
{
    juce::PropertiesFile::Options options;
    options.applicationName     = "Conduit";
    options.filenameSuffix      = ".settings";
    options.osxLibrarySubFolder = "Application Support";
    return options;
}

juce::File CaptureSettings::defaultExportDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userMusicDirectory)
               .getChildFile ("Conduit Captures");
}

//==============================================================================
CaptureSettings::CaptureSettings (const juce::PropertiesFile::Options& options)
{
    applicationProperties.setStorageParameters (options);
    loadFromFile();
}

CaptureSettings::~CaptureSettings()
{
    flush();
}

void CaptureSettings::loadFromFile()
{
    auto& file = *applicationProperties.getUserSettings();

    // Defensiv clampen — die Datei kann von Hand editiert worden sein
    bufferMinutes.store (juce::jlimit (minBufferMinutes, maxBufferMinutes,
                                       file.getIntValue (keyBufferMinutes, defaultBufferMinutes)),
                         std::memory_order_relaxed);
    preRollSeconds.store (juce::jlimit (minPreRollSeconds, maxPreRollSeconds,
                                        file.getIntValue (keyPreRollSeconds, defaultPreRollSeconds)),
                          std::memory_order_relaxed);
    thresholdDb.store (juce::jlimit (minThresholdDb, maxThresholdDb,
                                     static_cast<float> (file.getDoubleValue (keyThresholdDb, defaultThresholdDb))),
                       std::memory_order_relaxed);
    holdMinutes.store (juce::jlimit (minHoldMinutes, maxHoldMinutes,
                                     file.getIntValue (keyHoldMinutes, defaultHoldMinutes)),
                       std::memory_order_relaxed);
    autoCalibrate.store (file.getBoolValue (keyAutoCalibrate, defaultAutoCalibrate),
                         std::memory_order_relaxed);
    ramLimitGb.store (juce::jlimit (minRamLimitGb, maxRamLimitGb,
                                    file.getIntValue (keyRamLimitGb, defaultRamLimitGb)),
                      std::memory_order_relaxed);
    exportBitDepth.store (snapBitDepth (file.getIntValue (keyExportBitDepth, defaultExportBitDepth)),
                          std::memory_order_relaxed);

    exportDirectoryPath = file.getValue (keyExportDirectory,
                                         defaultExportDirectory().getFullPathName());
    if (exportDirectoryPath.isEmpty())
        exportDirectoryPath = defaultExportDirectory().getFullPathName();
}

void CaptureSettings::writeValue (const char* key, const juce::var& value)
{
    applicationProperties.getUserSettings()->setValue (key, value);
}

void CaptureSettings::flush()
{
    if (auto* file = applicationProperties.getUserSettings())
        file->saveIfNeeded();
}

//==============================================================================
int   CaptureSettings::getBufferMinutes() const noexcept  { return bufferMinutes.load (std::memory_order_relaxed); }
int   CaptureSettings::getPreRollSeconds() const noexcept { return preRollSeconds.load (std::memory_order_relaxed); }
float CaptureSettings::getThresholdDb() const noexcept    { return thresholdDb.load (std::memory_order_relaxed); }
int   CaptureSettings::getHoldMinutes() const noexcept    { return holdMinutes.load (std::memory_order_relaxed); }
bool  CaptureSettings::getAutoCalibrate() const noexcept  { return autoCalibrate.load (std::memory_order_relaxed); }
int   CaptureSettings::getRamLimitGb() const noexcept     { return ramLimitGb.load (std::memory_order_relaxed); }
int   CaptureSettings::getExportBitDepth() const noexcept { return exportBitDepth.load (std::memory_order_relaxed); }

juce::File CaptureSettings::getExportDirectory() const    { return juce::File (exportDirectoryPath); }

//==============================================================================
void CaptureSettings::setThresholdDb (float db)
{
    const auto clamped = juce::jlimit (minThresholdDb, maxThresholdDb, db);

    if (juce::exactlyEqual (clamped, getThresholdDb()))
        return;

    thresholdDb.store (clamped, std::memory_order_relaxed);
    writeValue (keyThresholdDb, clamped);
    sendChangeMessage();
}

void CaptureSettings::setHoldMinutes (int minutes)
{
    const auto clamped = juce::jlimit (minHoldMinutes, maxHoldMinutes, minutes);

    if (clamped == getHoldMinutes())
        return;

    holdMinutes.store (clamped, std::memory_order_relaxed);
    writeValue (keyHoldMinutes, clamped);
    sendChangeMessage();
}

void CaptureSettings::setAutoCalibrate (bool enabled)
{
    if (enabled == getAutoCalibrate())
        return;

    autoCalibrate.store (enabled, std::memory_order_relaxed);
    writeValue (keyAutoCalibrate, enabled);
    sendChangeMessage();
}

void CaptureSettings::setRamLimitGb (int gigabytes)
{
    const auto clamped = juce::jlimit (minRamLimitGb, maxRamLimitGb, gigabytes);

    if (clamped == getRamLimitGb())
        return;

    ramLimitGb.store (clamped, std::memory_order_relaxed);
    writeValue (keyRamLimitGb, clamped);
    sendChangeMessage();
}

void CaptureSettings::setExportDirectory (const juce::File& directory)
{
    const auto path = directory == juce::File() ? defaultExportDirectory().getFullPathName()
                                                : directory.getFullPathName();
    if (path == exportDirectoryPath)
        return;

    exportDirectoryPath = path;
    writeValue (keyExportDirectory, path);
    sendChangeMessage();
}

void CaptureSettings::setExportBitDepth (int bits)
{
    const auto snapped = snapBitDepth (bits);

    if (snapped == getExportBitDepth())
        return;

    exportBitDepth.store (snapped, std::memory_order_relaxed);
    writeValue (keyExportBitDepth, snapped);
    sendChangeMessage();
}

//==============================================================================
CaptureSettings::ResizeOutcome CaptureSettings::setBufferMinutes (int minutes)
{
    return requestRingResize (PendingResizeRequest::Field::bufferMinutes,
                              juce::jlimit (minBufferMinutes, maxBufferMinutes, minutes));
}

CaptureSettings::ResizeOutcome CaptureSettings::setPreRollSeconds (int seconds)
{
    return requestRingResize (PendingResizeRequest::Field::preRollSeconds,
                              juce::jlimit (minPreRollSeconds, maxPreRollSeconds, seconds));
}

CaptureSettings::ResizeOutcome CaptureSettings::requestRingResize (PendingResizeRequest::Field field,
                                                                   int requestedValue)
{
    const auto current = field == PendingResizeRequest::Field::bufferMinutes
                       ? getBufferMinutes()
                       : getPreRollSeconds();

    if (requestedValue == current)
        return ResizeOutcome::applied;  // nichts zu tun, keine Anfrage nötig

    // Kanal aktiv → Wert NICHT übernehmen; die UI bestätigt async
    // (Klassendoku Punkt 2). Ohne Host gilt "inaktiv".
    if (bufferHost != nullptr && bufferHost->isAnyChannelActive())
    {
        pendingResize = PendingResizeRequest { field, current, requestedValue };  // latest wins

        if (onPendingResize)
            onPendingResize (*pendingResize);

        return ResizeOutcome::pendingConfirmation;
    }

    // Kein Kanal aktiv → stille Reallokation
    applyRingValue (field, requestedValue);

    if (bufferHost != nullptr)
        bufferHost->reallocateBuffers();

    return ResizeOutcome::applied;
}

void CaptureSettings::confirmPendingResize()
{
    if (! pendingResize.has_value())
        return;

    const auto request = *pendingResize;
    pendingResize.reset();

    // Der User hat den Verlust explizit bestätigt: Gates schließen,
    // Puffer verwerfen, kein Auto-Export — dann erst der neue Wert
    if (bufferHost != nullptr)
        bufferHost->invalidateAllBuffers();

    applyRingValue (request.field, request.requestedValue);

    if (bufferHost != nullptr)
        bufferHost->reallocateBuffers();
}

void CaptureSettings::cancelPendingResize()
{
    pendingResize.reset();
}

bool CaptureSettings::hasPendingResize() const noexcept
{
    return pendingResize.has_value();
}

std::optional<CaptureSettings::PendingResizeRequest> CaptureSettings::getPendingResize() const
{
    return pendingResize;
}

void CaptureSettings::setBufferHost (ICaptureBufferHost* host) noexcept
{
    bufferHost = host;
}

void CaptureSettings::applyRingValue (PendingResizeRequest::Field field, int value)
{
    if (field == PendingResizeRequest::Field::bufferMinutes)
    {
        bufferMinutes.store (value, std::memory_order_relaxed);
        writeValue (keyBufferMinutes, value);
    }
    else
    {
        preRollSeconds.store (value, std::memory_order_relaxed);
        writeValue (keyPreRollSeconds, value);
    }

    sendChangeMessage();
}

} // namespace conduit
