#include "BrowserPaths.h"

namespace conduit::browser_paths
{

namespace
{
    juce::File ensured (juce::File directory)
    {
        directory.createDirectory();
        return directory;
    }
} // namespace

juce::File projectsDirectory()
{
    // Deckt sich mit dem FileChooser-Default des Preset-Systems
    return ensured (juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                        .getChildFile ("Conduit"));
}

juce::File loopsDirectory()
{
    return ensured (projectsDirectory().getChildFile ("Loops"));
}

juce::File oneShotsDirectory()
{
    return ensured (projectsDirectory().getChildFile ("One-Shots"));
}

} // namespace conduit::browser_paths
