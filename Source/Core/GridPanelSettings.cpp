#include "GridPanelSettings.h"

namespace conduit
{

namespace
{
    constexpr const char* editorPanelWidthKey     = "editorPanelWidth";
    constexpr const char* editorPanelOpenKey      = "editorPanelOpen";
    constexpr const char* editorThresholdWidthKey = "editorThresholdWidth";
    constexpr const char* noteCircleFadeMsKey     = "noteCircleFadeMs";
}

//==============================================================================
juce::PropertiesFile::Options GridPanelSettings::defaultOptions()
{
    juce::PropertiesFile::Options options;
    options.applicationName     = "GridPanel";
    options.filenameSuffix      = ".settings";
    options.folderName          = "Conduit";
    options.osxLibrarySubFolder = "Application Support";
    return options;
}

//==============================================================================
GridPanelSettings::GridPanelSettings (const juce::PropertiesFile::Options& options)
{
    applicationProperties.setStorageParameters (options);
    loadFromFile();
}

GridPanelSettings::~GridPanelSettings()
{
    if (auto* file = applicationProperties.getUserSettings())
        file->saveIfNeeded();
}

//==============================================================================
void GridPanelSettings::loadFromFile()
{
    if (auto* file = applicationProperties.getUserSettings())
    {
        editorPanelWidth = file->getIntValue (editorPanelWidthKey, defaultWidth);
        editorPanelOpen  = file->getBoolValue (editorPanelOpenKey, false);

        editorThresholdWidth = juce::jlimit (minThresholdWidth, maxThresholdWidth,
            file->getIntValue (editorThresholdWidthKey, defaultThresholdWidth));
        noteCircleFadeMs = juce::jlimit (minNoteCircleFadeMs, maxNoteCircleFadeMs,
            file->getIntValue (noteCircleFadeMsKey, defaultNoteCircleFadeMs));
    }
}

void GridPanelSettings::setEditorPanelWidth (int newWidth)
{
    if (newWidth == editorPanelWidth)
        return;

    editorPanelWidth = newWidth;

    if (auto* file = applicationProperties.getUserSettings())
    {
        file->setValue (editorPanelWidthKey, editorPanelWidth);
        file->saveIfNeeded();
    }
}

void GridPanelSettings::setEditorPanelOpen (bool shouldBeOpen)
{
    if (shouldBeOpen == editorPanelOpen)
        return;

    editorPanelOpen = shouldBeOpen;

    if (auto* file = applicationProperties.getUserSettings())
    {
        file->setValue (editorPanelOpenKey, editorPanelOpen);
        file->saveIfNeeded();
    }
}

void GridPanelSettings::setEditorThresholdWidth (int newWidth)
{
    const auto clamped = juce::jlimit (minThresholdWidth, maxThresholdWidth, newWidth);

    if (clamped == editorThresholdWidth)
        return;

    editorThresholdWidth = clamped;

    if (auto* file = applicationProperties.getUserSettings())
    {
        file->setValue (editorThresholdWidthKey, editorThresholdWidth);
        file->saveIfNeeded();
    }
}

void GridPanelSettings::setNoteCircleFadeMs (int newFadeMs)
{
    const auto clamped = juce::jlimit (minNoteCircleFadeMs, maxNoteCircleFadeMs, newFadeMs);

    if (clamped == noteCircleFadeMs)
        return;

    noteCircleFadeMs = clamped;

    if (auto* file = applicationProperties.getUserSettings())
    {
        file->setValue (noteCircleFadeMsKey, noteCircleFadeMs);
        file->saveIfNeeded();
    }
}

} // namespace conduit
