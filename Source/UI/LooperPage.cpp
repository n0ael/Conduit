#include "LooperPage.h"

#include <cmath>

namespace conduit
{

namespace
{
    constexpr int footerHeight = 44;   // Status + Papierkorb + Stop All (Touch-Ziel)
    constexpr int panelGap = 6;
}

//==============================================================================
LooperTrashTile::LooperTrashTile()
    : juce::Button ("looperTrashTile")
{
    setVisible (false);   // sichtbar erst mit Papierkorb-Bestand
}

void LooperTrashTile::setTrashState (double secondsRemaining, bool hasEntries)
{
    remaining = secondsRemaining;
    entries = hasEntries;

    setVisible (entries || flashTicks > 0);

    // Repaint nur bei sichtbarem Schritt (Sekunde wechselt — deckt auch
    // den Rot-Fade ab, der pro Sekunde weiterrückt)
    const auto seconds = (int) std::ceil (remaining);
    if (seconds != shownSeconds)
    {
        shownSeconds = seconds;
        repaint();
    }
}

void LooperTrashTile::flashEmptied()
{
    flashTicks = 6;   // 3× aus/an über ~420 ms
    flashOn = false;
    setVisible (true);
    repaint();
    scheduleFlashTick();
}

void LooperTrashTile::scheduleFlashTick()
{
    // Member-Timer bleibt frei — Muster NodeEditor-Lektion
    juce::Timer::callAfterDelay (70,
        [safe = juce::Component::SafePointer<LooperTrashTile> (this)]
        {
            if (safe == nullptr)
                return;

            safe->flashOn = ! safe->flashOn;
            if (--safe->flashTicks > 0)
                safe->scheduleFlashTick();
            else
                safe->setVisible (safe->entries);

            safe->repaint();
        });
}

void LooperTrashTile::paintButton (juce::Graphics& g, bool isHighlighted, bool)
{
    if (flashTicks > 0 && ! flashOn)
        return;   // Flacker-Aus-Phase

    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (push::colours::tile.brighter (isHighlighted ? 0.15f : 0.0f));
    g.fillRoundedRectangle (bounds, 4.0f);

    // Letzte warnSeconds: weiß → rot faden (User-Spezifikation 19.07.2026)
    const auto danger = (float) juce::jlimit (0.0, 1.0, 1.0 - remaining / warnSeconds);
    const auto accent = push::colours::ledWhite.interpolatedWith (push::colours::ledRed,
                                                                  danger);

    const auto seconds = juce::jmax (0, (int) std::ceil (remaining));
    const auto label = juce::String::fromUTF8 ("\xe2\x86\xba ")   // ↺
                     + juce::String (seconds / 60) + ":"
                     + juce::String (seconds % 60).paddedLeft ('0', 2);

    g.setColour (accent);
    g.setFont (push::scaledFont (12.0f, true));
    g.drawText (label, getLocalBounds(), juce::Justification::centred, false);
}

//==============================================================================
LooperPage::LooperPage()
{
    setName ("looperPage");

    trashTile.onClick = [this] { if (onRestoreTrash) onRestoreTrash(); };
    stopAllTile.onClick = [this] { if (onStop) onStop(); };

    statusLabel.setColour (juce::Label::textColourId, push::colours::textDim);
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    statusLabel.setInterceptsMouseClicks (false, false);

    addChildComponent (trashTile);   // sichtbar erst mit Papierkorb-Bestand
    addAndMakeVisible (stopAllTile);
    addAndMakeVisible (statusLabel);

    setLooperCount (1);
}

void LooperPage::setLooperCount (int count)
{
    count = juce::jlimit (1, 4, count);
    if ((int) panels.size() == count)
        return;

    while ((int) panels.size() > count)
        panels.pop_back();

    while ((int) panels.size() < count)
    {
        auto panel = std::make_unique<LooperPanel> ((int) panels.size() + 1);
        addAndMakeVisible (*panel);
        panels.push_back (std::move (panel));
    }

    resized();

    if (onPanelsChanged != nullptr)
        onPanelsChanged();
}

LooperPanel& LooperPage::getPanel (int looperIndex)
{
    jassert (looperIndex >= 0 && looperIndex < (int) panels.size());
    return *panels[(size_t) juce::jlimit (0, (int) panels.size() - 1, looperIndex)];
}

void LooperPage::setSpectrumView (int looperIndex, bool spectrum)
{
    if (looperIndex >= 0 && looperIndex < (int) panels.size())
        panels[(size_t) looperIndex]->setSpectrumView (spectrum);
}

void LooperPage::setShowStopAll (bool show)
{
    if (showStopAll == show)
        return;

    showStopAll = show;
    stopAllTile.setVisible (show);
    resized();
}

void LooperPage::setStatus (const juce::String& statusText)
{
    statusLabel.setText (statusText, juce::dontSendNotification);
}

void LooperPage::setPulsePhase (float phase01)
{
    for (auto& panel : panels)
        panel->setPulsePhase (phase01);
}

void LooperPage::setTrashState (double secondsRemaining, bool hasEntries)
{
    trashTile.setTrashState (secondsRemaining, hasEntries);
}

void LooperPage::paint (juce::Graphics& g)
{
    g.fillAll (push::colours::background);
}

void LooperPage::resized()
{
    auto bounds = getLocalBounds();

    // Untere Zeile: Status links, Papierkorb + Stop All rechts (die
    // frühere Kopfzeile ist komplett ins Seitenpanel gewandert)
    auto footer = bounds.removeFromBottom (footerHeight).reduced (4, 2);
    if (showStopAll)
        stopAllTile.setBounds (footer.removeFromRight (92).reduced (2));
    trashTile.setBounds (footer.removeFromRight (72).reduced (2));
    statusLabel.setBounds (footer.reduced (6, 0));

    auto panelArea = bounds.reduced (4);
    const auto count = (int) panels.size();
    if (count > 0)
    {
        const auto panelWidth = (panelArea.getWidth() - panelGap * (count - 1)) / count;
        for (int i = 0; i < count; ++i)
        {
            panels[(size_t) i]->setBounds (panelArea.removeFromLeft (panelWidth));
            panelArea.removeFromLeft (panelGap);
        }
    }
}

} // namespace conduit
