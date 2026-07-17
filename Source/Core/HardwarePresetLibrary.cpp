#include "HardwarePresetLibrary.h"

namespace conduit
{

namespace
{
    const juce::Identifier kRootId { "HardwarePresets" };
    const juce::Identifier kPresetId { "Preset" };
    const juce::Identifier kDeviceIdByte { "deviceIdByte" };
    const juce::Identifier kProgramsPerBank { "programsPerBank" };
    const juce::Identifier kIndex { "index" };
    const juce::Identifier kName { "name" };
}

//==============================================================================
HardwarePresetLibrary::HardwarePresetLibrary (const juce::File& presetFolderToUse)
    : presetFolder (presetFolderToUse)
{
    reload();
}

void HardwarePresetLibrary::reload()
{
    JUCE_ASSERT_MESSAGE_THREAD

    cache.clear();

    for (const auto& file : presetFolder.findChildFiles (juce::File::findFiles, false, "*.xml"))
    {
        const juce::Uuid deviceId { file.getFileNameWithoutExtension() };
        if (deviceId.isNull())
            continue;

        const auto xml = juce::parseXML (file);
        if (xml == nullptr)
            continue;   // korrupte Datei -> schlicht kein Cache (kein Crash)

        const auto tree = juce::ValueTree::fromXml (*xml);
        if (! tree.hasType (kRootId))
            continue;

        DevicePresets presets;
        presets.deviceIdByte = (juce::uint8) (int) tree.getProperty (kDeviceIdByte, 0);
        presets.programsPerBank = (int) tree.getProperty (kProgramsPerBank, 0);

        if (presets.programsPerBank <= 0)
            continue;

        for (const auto& child : tree)
        {
            if (! child.hasType (kPresetId))
                continue;

            const auto index = (int) child.getProperty (kIndex, -1);
            if (index < 0)
                continue;

            if ((int) presets.names.size() <= index)
                presets.names.resize ((size_t) index + 1);
            presets.names[(size_t) index] = child.getProperty (kName).toString();
        }

        if (! presets.names.empty())
            cache[deviceId] = std::move (presets);
    }
}

//==============================================================================
void HardwarePresetLibrary::setPresets (const juce::Uuid& deviceId, DevicePresets presets)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if (presets.programsPerBank <= 0 || presets.names.empty())
        return;

    juce::ValueTree tree { kRootId };
    tree.setProperty (kDeviceIdByte, (int) presets.deviceIdByte, nullptr);
    tree.setProperty (kProgramsPerBank, presets.programsPerBank, nullptr);

    for (size_t i = 0; i < presets.names.size(); ++i)
    {
        juce::ValueTree preset { kPresetId };
        preset.setProperty (kIndex, (int) i, nullptr);
        preset.setProperty (kName, presets.names[i], nullptr);
        tree.appendChild (preset, nullptr);
    }

    presetFolder.createDirectory();
    if (const auto xml = tree.createXml(); xml != nullptr)
        xml->writeTo (fileFor (deviceId));

    cache[deviceId] = std::move (presets);
    sendChangeMessage();
}

void HardwarePresetLibrary::clearPresets (const juce::Uuid& deviceId)
{
    JUCE_ASSERT_MESSAGE_THREAD

    fileFor (deviceId).deleteFile();
    if (cache.erase (deviceId) > 0)
        sendChangeMessage();
}

//==============================================================================
bool HardwarePresetLibrary::hasPresets (const juce::Uuid& deviceId) const
{
    return cache.find (deviceId) != cache.end();
}

juce::String HardwarePresetLibrary::presetName (const juce::Uuid& deviceId,
                                                int bank, int program) const
{
    const auto it = cache.find (deviceId);
    if (it == cache.end() || bank < 0 || program < 0
        || program >= it->second.programsPerBank)
        return {};

    const auto index = (size_t) (bank * it->second.programsPerBank + program);
    return index < it->second.names.size() ? it->second.names[index] : juce::String();
}

int HardwarePresetLibrary::bankCount (const juce::Uuid& deviceId) const
{
    const auto it = cache.find (deviceId);
    if (it == cache.end() || it->second.programsPerBank <= 0)
        return 0;

    return ((int) it->second.names.size() + it->second.programsPerBank - 1)
           / it->second.programsPerBank;
}

int HardwarePresetLibrary::programsPerBank (const juce::Uuid& deviceId) const
{
    const auto it = cache.find (deviceId);
    return it != cache.end() ? it->second.programsPerBank : 0;
}

juce::uint8 HardwarePresetLibrary::deviceIdByte (const juce::Uuid& deviceId) const
{
    const auto it = cache.find (deviceId);
    return it != cache.end() ? it->second.deviceIdByte : (juce::uint8) 0;
}

//==============================================================================
juce::File HardwarePresetLibrary::fileFor (const juce::Uuid& deviceId) const
{
    return presetFolder.getChildFile (deviceId.toString() + ".xml");
}

} // namespace conduit
