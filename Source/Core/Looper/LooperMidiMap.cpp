#include "LooperMidiMap.h"

namespace conduit
{

namespace
{
    const juce::String keyMap = "looperMidiMap";
    const juce::Identifier xmlRoot ("LooperMidiMap");
    const juce::Identifier xmlBinding ("Binding");

    /** Modifier-Set ↔ "ch:note;ch:note" (kanonische Ordnung via bind()). */
    juce::String modifiersToString (const grid::ModifierSet& modifiers)
    {
        juce::String text;
        for (const auto& modifier : modifiers)
        {
            if (text.isNotEmpty())
                text << ";";
            text << modifier.channel << ":" << modifier.note;
        }
        return text;
    }

    grid::ModifierSet modifiersFromString (const juce::String& text)
    {
        grid::ModifierSet modifiers;
        for (const auto& token : juce::StringArray::fromTokens (text, ";", {}))
        {
            const auto channel = token.upToFirstOccurrenceOf (":", false, false);
            const auto note = token.fromFirstOccurrenceOf (":", false, false);
            if (channel.isNotEmpty() && note.isNotEmpty())
                modifiers.push_back ({ channel.getIntValue(), note.getIntValue() });
        }
        return modifiers;
    }
}

//==============================================================================
juce::PropertiesFile::Options LooperMidiMap::defaultOptions()
{
    juce::PropertiesFile::Options options;
    options.applicationName     = "LooperMidi";
    options.filenameSuffix      = ".settings";
    options.folderName          = "Conduit";
    options.osxLibrarySubFolder = "Application Support";
    return options;
}

LooperMidiMap::LooperMidiMap (const juce::PropertiesFile::Options& options)
{
    applicationProperties.setStorageParameters (options);
    loadFromFile();

    bindings.onBindingsChanged = [this]
    {
        if (loading)
            return;
        saveToFile();
        if (onChanged != nullptr)
            onChanged();
    };
}

LooperMidiMap::~LooperMidiMap()
{
    flush();
}

void LooperMidiMap::flush()
{
    if (auto* file = applicationProperties.getUserSettings())
        file->saveIfNeeded();
}

//==============================================================================
void LooperMidiMap::loadFromFile()
{
    const auto xml = applicationProperties.getUserSettings()->getXmlValue (keyMap);
    if (xml == nullptr || ! xml->hasTagName (xmlRoot.toString()))
        return;

    loading = true;
    for (const auto* bindingXml : xml->getChildWithTagNameIterator (xmlBinding.toString()))
    {
        grid::MacroControlKey key;
        key.layer = loopermidi::kLooperLayer;
        key.controlId = bindingXml->getIntAttribute ("control");
        key.axis = bindingXml->getIntAttribute ("axis", 0);

        bindings.bind (key,
                       juce::jlimit (1, 16, bindingXml->getIntAttribute ("channel", 1)),
                       bindingXml->getIntAttribute ("cc", 1),
                       bindingXml->getBoolAttribute ("isNote", false),
                       modifiersFromString (bindingXml->getStringAttribute ("modifiers")));
    }
    loading = false;
}

void LooperMidiMap::saveToFile()
{
    juce::XmlElement xml (xmlRoot.toString());

    for (const auto& binding : bindings.all())
    {
        if (! loopermidi::isLooperKey (binding.key))
            continue;

        auto* bindingXml = xml.createNewChildElement (xmlBinding.toString());
        bindingXml->setAttribute ("control", binding.key.controlId);
        bindingXml->setAttribute ("axis", binding.key.axis);
        bindingXml->setAttribute ("channel", binding.channel);
        bindingXml->setAttribute ("cc", binding.cc);
        bindingXml->setAttribute ("isNote", binding.isNote);
        if (! binding.modifiers.empty())
            bindingXml->setAttribute ("modifiers", modifiersToString (binding.modifiers));
    }

    applicationProperties.getUserSettings()->setValue (keyMap, &xml);
}

} // namespace conduit
