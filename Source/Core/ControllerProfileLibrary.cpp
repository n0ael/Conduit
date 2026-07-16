#include "ControllerProfileLibrary.h"

#include "BinaryData.h"

namespace conduit
{

ControllerProfileLibrary::ControllerProfileLibrary (const juce::File& userFolderToUse)
    : userProfileFolder (userFolderToUse)
{
    reload();
}

void ControllerProfileLibrary::reload()
{
    loadedProfiles.clear();
    sourceReports.clear();

    // Factory-Profil: direkt per BinaryData-Symbol referenziert (nicht
    // generisch ueber alle .csv-Ressourcen gescannt wie MidiProfileLibrary --
    // originalFilenames traegt keinen Ordnerpfad, ein genereller Scan
    // wuerde hier mit den Klangerzeuger-CSVs im selben Namensraum
    // kollidieren, s. Header-Kommentar).
    {
        const juce::String sourceName ("AllenHeath_XoneK1.csv");
        SourceReport sourceReport;
        sourceReport.sourceName = sourceName;
        sourceReport.fromUserFolder = false;

        auto profile = midirig::parseControllerProfileCsv (
            juce::String::fromUTF8 (BinaryData::AllenHeath_XoneK1_csv,
                                    BinaryData::AllenHeath_XoneK1_csvSize),
            &sourceReport.parse);

        if (profile.device.isEmpty())
            profile.device = "Xone:K1";

        sourceReport.controls = static_cast<int> (profile.controls.size());
        sourceReports.push_back (sourceReport);
        addProfile (std::move (profile), false);
    }

    // MIDI-Rig M8: Frontier AlphaTrack (Native Mode) -- gleiche Direkt-
    // Referenz-Begruendung wie oben.
    {
        const juce::String sourceName ("Frontier_AlphaTrack.csv");
        SourceReport sourceReport;
        sourceReport.sourceName = sourceName;
        sourceReport.fromUserFolder = false;

        auto profile = midirig::parseControllerProfileCsv (
            juce::String::fromUTF8 (BinaryData::Frontier_AlphaTrack_csv,
                                    BinaryData::Frontier_AlphaTrack_csvSize),
            &sourceReport.parse);

        if (profile.device.isEmpty())
            profile.device = "AlphaTrack";

        sourceReport.controls = static_cast<int> (profile.controls.size());
        sourceReports.push_back (sourceReport);
        addProfile (std::move (profile), false);
    }

    // User-Ordner Conduit/Controllers/*.csv (FLACH, ADR E2 -- eine Datei =
    // ein Geraet, keine {Hersteller}/-Unterordner).
    if (userProfileFolder != juce::File() && userProfileFolder.isDirectory())
    {
        for (const auto& entry : juce::RangedDirectoryIterator (
                 userProfileFolder, false, "*.csv", juce::File::findFiles))
        {
            const auto& file = entry.getFile();

            SourceReport sourceReport;
            sourceReport.sourceName = file.getFileName();
            sourceReport.fromUserFolder = true;

            auto profile = midirig::parseControllerProfileCsv (file.loadFileAsString(),
                                                               &sourceReport.parse);

            if (profile.device.isEmpty())
                profile.device = file.getFileNameWithoutExtension();

            if (profile.controls.empty())
                sourceReport.parse.warnings.add ("Keine Controls erkannt");

            sourceReport.controls = static_cast<int> (profile.controls.size());
            sourceReports.push_back (sourceReport);
            addProfile (std::move (profile), true);
        }
    }
}

void ControllerProfileLibrary::addProfile (midirig::ControllerProfile profile, bool fromUser)
{
    auto* existing = const_cast<midirig::ControllerProfile*> (find (profile.device));

    if (existing != nullptr)
    {
        if (fromUser)
            *existing = std::move (profile);   // User ersetzt Factory komplett
        // Duplikat aus derselben Quellart: erste Quelle gewinnt (deterministisch)
        return;
    }

    loadedProfiles.push_back (std::move (profile));
}

const midirig::ControllerProfile* ControllerProfileLibrary::find (const juce::String& device) const noexcept
{
    for (const auto& profile : loadedProfiles)
        if (profile.device == device)
            return &profile;

    return nullptr;
}

} // namespace conduit
