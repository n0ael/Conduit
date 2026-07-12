#include "MidiDeviceProfile.h"

namespace conduit::midirig
{

namespace
{
    /** Zerlegt EINE CSV-Zeile in Felder — RFC-4180-Quoting ("…"-Felder
        dürfen Kommas enthalten, "" innerhalb ist ein escaped "). */
    juce::StringArray splitCsvLine (const juce::String& line)
    {
        juce::StringArray fields;
        juce::String current;
        bool inQuotes = false;

        for (int i = 0; i < line.length(); ++i)
        {
            const auto c = line[i];

            if (inQuotes)
            {
                if (c == '"')
                {
                    if (i + 1 < line.length() && line[i + 1] == '"')
                    {
                        current += '"';
                        ++i;
                    }
                    else
                    {
                        inQuotes = false;
                    }
                }
                else
                {
                    current += c;
                }
            }
            else if (c == '"')
            {
                inQuotes = true;
            }
            else if (c == ',')
            {
                fields.add (current);
                current.clear();
            }
            else
            {
                current += c;
            }
        }

        fields.add (current);
        return fields;
    }

    /** Feld als Zahl; leer/kein Integer = fallback. */
    int fieldAsInt (const juce::StringArray& fields, int index, int fallback)
    {
        if (index < 0 || index >= fields.size())
            return fallback;

        const auto trimmed = fields[index].trim();
        if (trimmed.isEmpty() || ! trimmed.containsOnly ("-0123456789"))
            return fallback;

        return trimmed.getIntValue();
    }

    juce::String fieldAsString (const juce::StringArray& fields, int index)
    {
        return index >= 0 && index < fields.size() ? fields[index].trim() : juce::String();
    }
}

std::vector<DeviceProfile> parseMidiGuideCsv (const juce::String& text, ParseReport* report)
{
    std::vector<DeviceProfile> profiles;

    ParseReport localReport;
    auto& out = report != nullptr ? *report : localReport;
    out = {};

    juce::StringArray lines;
    lines.addLines (text);

    if (lines.isEmpty())
        return profiles;

    // Header-Zeile: Spaltenindizes über Namen finden (case-insensitiv,
    // unbekannte Spalten ignoriert — ADR E1).
    const auto header = splitCsvLine (lines[0]);
    const auto columnIndex = [&header] (const char* columnName)
    {
        for (int i = 0; i < header.size(); ++i)
            if (header[i].trim().equalsIgnoreCase (columnName))
                return i;
        return -1;
    };

    const auto colManufacturer = columnIndex ("manufacturer");
    const auto colDevice       = columnIndex ("device");
    const auto colSection      = columnIndex ("section");
    const auto colName         = columnIndex ("parameter_name");
    const auto colCcMsb        = columnIndex ("cc_msb");
    const auto colCcMin        = columnIndex ("cc_min_value");
    const auto colCcMax        = columnIndex ("cc_max_value");
    const auto colNrpnMsb      = columnIndex ("nrpn_msb");
    const auto colNrpnLsb      = columnIndex ("nrpn_lsb");
    const auto colNrpnMin      = columnIndex ("nrpn_min_value");
    const auto colNrpnMax      = columnIndex ("nrpn_max_value");

    if (colName < 0 || (colCcMsb < 0 && colNrpnMsb < 0))
    {
        out.warnings.add ("Kopfzeile ohne parameter_name/cc_msb/nrpn_msb — keine midi.guide-CSV?");
        return profiles;
    }

    const auto profileFor = [&profiles] (const juce::String& manufacturer,
                                         const juce::String& device) -> DeviceProfile&
    {
        for (auto& profile : profiles)
            if (profile.manufacturer == manufacturer && profile.device == device)
                return profile;

        profiles.push_back ({ manufacturer, device, {} });
        return profiles.back();
    };

    for (int lineIndex = 1; lineIndex < lines.size(); ++lineIndex)
    {
        if (lines[lineIndex].trim().isEmpty())
            continue;

        const auto fields = splitCsvLine (lines[lineIndex]);

        ProfileParam param;
        param.section = fieldAsString (fields, colSection);
        param.name    = fieldAsString (fields, colName);

        if (param.name.isEmpty())
        {
            ++out.skipped;
            continue;
        }

        const auto ccMsb = fieldAsInt (fields, colCcMsb, -1);
        if (ccMsb >= 0 && ccMsb <= 127)
            param.cc = ccMsb;

        const auto nrpnMsb = fieldAsInt (fields, colNrpnMsb, -1);
        const auto nrpnLsb = fieldAsInt (fields, colNrpnLsb, -1);
        if (nrpnMsb >= 0 && nrpnMsb <= 127)
            param.nrpn = nrpnMsb * 128 + juce::jlimit (0, 127, juce::jmax (0, nrpnLsb));

        if (param.cc < 0 && param.nrpn < 0)
        {
            ++out.skipped;
            out.warnings.add ("Zeile " + juce::String (lineIndex + 1) + " ('" + param.name
                              + "'): weder CC noch NRPN — übersprungen");
            continue;
        }

        // Wertebereich: NRPN-Range hat Vorrang (14-bit), sonst CC-Range,
        // sonst 0..127-Default; verdrehte/kaputte Ranges reparieren.
        const auto isNrpn = param.nrpn >= 0;
        param.minValue = fieldAsInt (fields, isNrpn ? colNrpnMin : colCcMin, 0);
        param.maxValue = fieldAsInt (fields, isNrpn ? colNrpnMax : colCcMax,
                                     isNrpn ? 16383 : 127);
        param.minValue = juce::jmax (0, param.minValue);
        param.maxValue = juce::jmin (isNrpn ? 16383 : 127, param.maxValue);
        if (param.maxValue <= param.minValue)
        {
            param.minValue = 0;
            param.maxValue = isNrpn ? 16383 : 127;
        }

        auto& profile = profileFor (fieldAsString (fields, colManufacturer),
                                    fieldAsString (fields, colDevice));
        profile.params.push_back (param);
        ++out.accepted;
    }

    return profiles;
}

} // namespace conduit::midirig
