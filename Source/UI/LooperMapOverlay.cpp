#include "LooperMapOverlay.h"

#include "PushLookAndFeel.h"

namespace conduit
{

LooperMapOverlay::LooperMapOverlay()
{
    setName ("looperMapOverlay");
    setInterceptsMouseClicks (true, false);   // schluckt alles unter sich
}

void LooperMapOverlay::setTargets (std::vector<TargetSpot> newTargets)
{
    targets = std::move (newTargets);
    repaint();
}

void LooperMapOverlay::setArmedKey (bool hasArmed, const grid::MacroControlKey& key)
{
    armed = hasArmed;
    armedKey = key;
    repaint();
}

void LooperMapOverlay::paint (juce::Graphics& g)
{
    // Leichte Abdunkelung signalisiert den Modus; die Ziele bleiben klar
    g.fillAll (push::colours::background.withAlpha (0.25f));

    g.setFont (push::scaledFont (9.0f));
    for (const auto& target : targets)
    {
        const auto isArmed = armed && target.key == armedKey;
        const auto frame = isArmed ? push::colours::ledOrange : push::colours::ledCyan;

        g.setColour (frame.withAlpha (isArmed ? 1.0f : 0.85f));
        g.drawRoundedRectangle (target.bounds.toFloat().reduced (1.0f), 4.0f,
                                isArmed ? 2.0f : 1.2f);

        if (target.badge.isNotEmpty())
        {
            const auto badgeArea = target.bounds.toFloat()
                                       .removeFromTop (12.0f)
                                       .removeFromRight (juce::jmin (
                                           46.0f, (float) target.bounds.getWidth()));
            g.setColour (push::colours::background.withAlpha (0.8f));
            g.fillRoundedRectangle (badgeArea, 3.0f);
            g.setColour (frame);
            g.drawText (target.badge, badgeArea, juce::Justification::centred, false);
        }
    }
}

void LooperMapOverlay::mouseUp (const juce::MouseEvent& event)
{
    if (onTargetTapped == nullptr)
        return;

    // Oberstes (zuletzt registriertes) Ziel unter dem Finger gewinnt
    for (auto it = targets.rbegin(); it != targets.rend(); ++it)
        if (it->bounds.contains (event.getPosition()))
        {
            onTargetTapped (it->key);
            return;
        }
}

} // namespace conduit
