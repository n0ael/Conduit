#include "OscSendSettings.h"

namespace conduit
{

namespace
{
    constexpr const char* hostKey    = "host";
    constexpr const char* portKey    = "port";
    constexpr const char* enabledKey = "enabled";
}

//==============================================================================
juce::PropertiesFile::Options OscSendSettings::defaultOptions()
{
    juce::PropertiesFile::Options options;
    options.applicationName     = "OscSend";
    options.filenameSuffix      = ".settings";
    options.folderName          = "Conduit";
    options.osxLibrarySubFolder = "Application Support";
    return options;
}

//==============================================================================
OscSendSettings::OscSendSettings (const juce::PropertiesFile::Options& options)
{
    applicationProperties.setStorageParameters (options);
    loadFromFile();
}

OscSendSettings::~OscSendSettings()
{
    if (auto* file = applicationProperties.getUserSettings())
        file->saveIfNeeded();
}

//==============================================================================
void OscSendSettings::loadFromFile()
{
    if (auto* file = applicationProperties.getUserSettings())
    {
        host    = file->getValue (hostKey, defaultHost);
        port    = file->getIntValue (portKey, defaultPort);
        enabled = file->getBoolValue (enabledKey, false);
    }
}

void OscSendSettings::store (const char* key, const juce::var& value)
{
    if (auto* file = applicationProperties.getUserSettings())
    {
        file->setValue (key, value);
        file->saveIfNeeded();
    }
}

void OscSendSettings::setHost (const juce::String& newHost)
{
    const auto trimmed = newHost.trim();

    if (trimmed.isEmpty() || trimmed == host)
        return;

    host = trimmed;
    store (hostKey, host);
    sendChangeMessage();
}

void OscSendSettings::setPort (int newPort)
{
    if (newPort == port || newPort < 1 || newPort > 65535)
        return;

    port = newPort;
    store (portKey, port);
    sendChangeMessage();
}

void OscSendSettings::setEnabled (bool shouldSend)
{
    if (shouldSend == enabled)
        return;

    enabled = shouldSend;
    store (enabledKey, enabled);
    sendChangeMessage();
}

} // namespace conduit
