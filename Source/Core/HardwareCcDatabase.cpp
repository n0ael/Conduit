#include "HardwareCcDatabase.h"

#include <algorithm>

#include "BinaryData.h"

namespace conduit::grid
{

std::vector<HardwareDevice> HardwareCcDatabase::parse (const juce::String& text)
{
    std::vector<HardwareDevice> result;
    HardwareDevice* current = nullptr;

    for (const auto& rawLine : juce::StringArray::fromLines (text))
    {
        const auto line = rawLine.trim();

        if (line.isEmpty() || line.startsWith ("#"))
            continue;

        if (line.startsWith ("[") && line.endsWith ("]"))
        {
            const auto name = line.substring (1, line.length() - 1).trim();
            if (name.isEmpty())
                continue;

            result.push_back ({ name, {} });
            current = &result.back();
            continue;
        }

        if (current == nullptr)
            continue;   // Parameterzeile ohne vorausgehenden [Gerät]-Header

        const auto separator = line.indexOfChar ('=');
        if (separator < 0)
            continue;

        const auto cc = line.substring (0, separator).trim().getIntValue();
        const auto name = line.substring (separator + 1).trim();

        if (name.isEmpty() || cc < 0 || cc > 127)
            continue;

        current->params.push_back ({ cc, name });
    }

    return result;
}

std::vector<HardwareDevice> HardwareCcDatabase::merge (
    std::vector<HardwareDevice> base, const std::vector<HardwareDevice>& overlay)
{
    for (const auto& overlayDevice : overlay)
    {
        auto deviceIt = std::find_if (base.begin(), base.end(), [&] (const HardwareDevice& d)
                                      { return d.name == overlayDevice.name; });

        if (deviceIt == base.end())
        {
            base.push_back (overlayDevice);
            continue;
        }

        for (const auto& overlayParam : overlayDevice.params)
        {
            auto paramIt = std::find_if (deviceIt->params.begin(), deviceIt->params.end(),
                [&] (const HardwareCcParam& p) { return p.cc == overlayParam.cc; });

            if (paramIt != deviceIt->params.end())
                paramIt->name = overlayParam.name;
            else
                deviceIt->params.push_back (overlayParam);
        }
    }

    return base;
}

void HardwareCcDatabase::load (const juce::File& userFile)
{
    const auto factoryText = juce::String::fromUTF8 (
        BinaryData::HardwareDevices_txt, BinaryData::HardwareDevices_txtSize);

    merged = parse (factoryText);

    if (userFile != juce::File() && userFile.existsAsFile())
        merged = merge (std::move (merged), parse (userFile.loadFileAsString()));
}

const HardwareDevice* HardwareCcDatabase::findDevice (const juce::String& name) const noexcept
{
    for (const auto& device : merged)
        if (device.name == name)
            return &device;

    return nullptr;
}

} // namespace conduit::grid
