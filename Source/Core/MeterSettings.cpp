#include "MeterSettings.h"

namespace conduit
{

namespace
{
    constexpr const char* clipResetModeKey = "clipResetMode";
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
    }
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
