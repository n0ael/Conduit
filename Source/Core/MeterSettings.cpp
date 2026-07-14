#include "MeterSettings.h"

namespace conduit
{

namespace
{
    constexpr const char* clipResetModeKey   = "clipResetMode";
    constexpr const char* rmsWindowKey       = "rmsWindowSeconds";
    constexpr const char* peakReleaseKey     = "peakReleaseSeconds";
    constexpr const char* peakHoldKey        = "peakHoldSeconds";
}

//==============================================================================
juce::PropertiesFile::Options MeterSettings::defaultOptions()
{
    juce::PropertiesFile::Options options;
    options.applicationName     = "Meter";
    options.filenameSuffix      = ".settings";
    options.folderName          = "Conduit";
    options.osxLibrarySubFolder = "Application Support";
    return options;
}

//==============================================================================
MeterSettings::MeterSettings (const juce::PropertiesFile::Options& options)
{
    applicationProperties.setStorageParameters (options);
    loadFromFile();
}

MeterSettings::~MeterSettings()
{
    if (auto* file = applicationProperties.getUserSettings())
        file->saveIfNeeded();
}

//==============================================================================
void MeterSettings::loadFromFile()
{
    if (auto* file = applicationProperties.getUserSettings())
    {
        const auto stored = file->getIntValue (clipResetModeKey,
                                               static_cast<int> (defaultMode));
        clipResetMode = stored == static_cast<int> (ClipResetMode::automatic)
                      ? ClipResetMode::automatic
                      : ClipResetMode::manual;

        // Ballistik: auch beim Laden clampen (handeditierte Datei)
        rmsWindowSeconds = juce::jlimit (minRmsWindowSeconds, maxRmsWindowSeconds,
            static_cast<float> (file->getDoubleValue (rmsWindowKey,
                                                      LevelMeter::defaultRmsWindowSeconds)));
        peakReleaseSeconds = juce::jlimit (minPeakReleaseSeconds, maxPeakReleaseSeconds,
            static_cast<float> (file->getDoubleValue (peakReleaseKey,
                                                      LevelMeter::defaultPeakReleaseSeconds)));
        peakHoldSeconds = juce::jlimit (minPeakHoldSeconds, maxPeakHoldSeconds,
            static_cast<float> (file->getDoubleValue (peakHoldKey,
                                                      LevelMeter::defaultPeakHoldSeconds)));
    }
}

void MeterSettings::storeFloat (const char* key, float value)
{
    if (auto* file = applicationProperties.getUserSettings())
    {
        file->setValue (key, static_cast<double> (value));
        file->saveIfNeeded();
    }
}

void MeterSettings::setRmsWindowSeconds (float seconds)
{
    const auto clamped = juce::jlimit (minRmsWindowSeconds, maxRmsWindowSeconds, seconds);
    if (juce::exactlyEqual (clamped, rmsWindowSeconds))
        return;

    rmsWindowSeconds = clamped;
    storeFloat (rmsWindowKey, clamped);
    sendChangeMessage();
}

void MeterSettings::setPeakReleaseSeconds (float seconds)
{
    const auto clamped = juce::jlimit (minPeakReleaseSeconds, maxPeakReleaseSeconds, seconds);
    if (juce::exactlyEqual (clamped, peakReleaseSeconds))
        return;

    peakReleaseSeconds = clamped;
    storeFloat (peakReleaseKey, clamped);
    sendChangeMessage();
}

void MeterSettings::setPeakHoldSeconds (float seconds)
{
    const auto clamped = juce::jlimit (minPeakHoldSeconds, maxPeakHoldSeconds, seconds);
    if (juce::exactlyEqual (clamped, peakHoldSeconds))
        return;

    peakHoldSeconds = clamped;
    storeFloat (peakHoldKey, clamped);
    sendChangeMessage();
}

void MeterSettings::setClipResetMode (ClipResetMode mode)
{
    if (mode == clipResetMode)
        return;

    clipResetMode = mode;

    if (auto* file = applicationProperties.getUserSettings())
    {
        file->setValue (clipResetModeKey, static_cast<int> (mode));
        file->saveIfNeeded();
    }

    sendChangeMessage();
}

} // namespace conduit
