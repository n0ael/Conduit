#include "MidiProfileLibrary.h"

#include "BinaryData.h"

namespace conduit
{

MidiProfileLibrary::MidiProfileLibrary (const juce::File& userFolderToUse)
    : userProfileFolder (userFolderToUse)
{
    reload();
}

void MidiProfileLibrary::reload()
{
    loadedProfiles.clear();
    sourceReports.clear();

    // Factory-Profile aus BinaryData: alle eingebetteten .csv-Ressourcen
    // (originalFilenames hält die echten Dateinamen).
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const juce::String originalName (BinaryData::originalFilenames[i]);
        if (! originalName.endsWithIgnoreCase (".csv"))
            continue;

        int dataSize = 0;
        const auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], dataSize);
        if (data == nullptr || dataSize <= 0)
            continue;

        SourceReport sourceReport;
        sourceReport.sourceName = originalName;
        sourceReport.fromUserFolder = false;

        auto parsed = midirig::parseMidiGuideCsv (juce::String::fromUTF8 (data, dataSize),
                                                  &sourceReport.parse);
        sourceReport.devices = static_cast<int> (parsed.size());
        sourceReports.push_back (sourceReport);

        addProfiles (std::move (parsed), false);
    }

    // User-Ordner Conduit/Devices/**/*.csv (rekursiv, {Hersteller}/{Gerät}.csv).
    if (userProfileFolder != juce::File() && userProfileFolder.isDirectory())
    {
        for (const auto& entry : juce::RangedDirectoryIterator (
                 userProfileFolder, true, "*.csv", juce::File::findFiles))
        {
            SourceReport sourceReport;
            sourceReport.sourceName = entry.getFile().getRelativePathFrom (userProfileFolder);
            sourceReport.fromUserFolder = true;

            auto parsed = midirig::parseMidiGuideCsv (entry.getFile().loadFileAsString(),
                                                      &sourceReport.parse);
            sourceReport.devices = static_cast<int> (parsed.size());

            if (parsed.empty())
                sourceReport.parse.warnings.add ("Keine Geräte erkannt");

            sourceReports.push_back (sourceReport);
            addProfiles (std::move (parsed), true);
        }
    }
}

void MidiProfileLibrary::addProfiles (std::vector<midirig::DeviceProfile> incoming, bool fromUser)
{
    for (auto& profile : incoming)
    {
        auto* existing = const_cast<midirig::DeviceProfile*> (
            find (profile.manufacturer, profile.device));

        if (existing != nullptr)
        {
            if (fromUser)
                *existing = std::move (profile);   // User ersetzt Factory komplett
            // Factory-Duplikat: erste Quelle gewinnt (deterministisch)
            continue;
        }

        loadedProfiles.push_back (std::move (profile));
    }
}

const midirig::DeviceProfile* MidiProfileLibrary::find (const juce::String& manufacturer,
                                                        const juce::String& device) const noexcept
{
    for (const auto& profile : loadedProfiles)
        if (profile.manufacturer == manufacturer && profile.device == device)
            return &profile;

    return nullptr;
}

} // namespace conduit
