#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace conduit
{

//==============================================================================
/**
    Kleiner VBlank-getriebener Animations-Helfer (CLAUDE.md 10: Animationen
    via juce::VBlankAttachment, kein ComponentAnimator): ein Float-Wert
    läuft zeitbasiert mit Ease-Out-Kubik auf ein Ziel — Browser-Panel-
    Slide-in und TouchKeyboard-Slide-up teilen sich diese Klasse.

    Headless-Verhalten (Tests, kein Component-Peer): der VBlank tickt nie —
    animateTo() springt dann sofort ans Ziel und feuert onUpdate einmal,
    damit Zustände nie an einem Animationsende hängen. Tests nutzen
    zusätzlich snapTo(). Nur Message Thread.
*/
class AnimatedValue final
{
public:
    explicit AnimatedValue (juce::Component& ownerToUse)
        : owner (ownerToUse),
          vblank (&ownerToUse, [this] (double) { tick(); })
    {
    }

    /** Startet die Animation zum Ziel (Ease-Out-Kubik, zeitbasiert). */
    void animateTo (float target, int durationMs)
    {
        if (owner.getPeer() == nullptr || durationMs <= 0)
        {
            snapTo (target);
            return;
        }

        startValue  = current;
        targetValue = target;
        startTimeMs = juce::Time::getMillisecondCounterHiRes();
        durationMillis = durationMs;
        animating = ! juce::exactlyEqual (current, target);

        if (! animating)
            notify();  // Ziel schon erreicht — Konsument trotzdem informieren
    }

    /** Sofort ans Ziel (Tests / Öffnen ohne Animation). */
    void snapTo (float target)
    {
        animating = false;
        current   = target;
        notify();
    }

    [[nodiscard]] float getCurrent() const noexcept { return current; }
    [[nodiscard]] bool isAnimating() const noexcept { return animating; }

    /** Einmal pro Frame während der Animation (und nach snapTo). */
    std::function<void (float)> onUpdate;

private:
    void tick()
    {
        if (! animating)
            return;

        const auto elapsed = juce::Time::getMillisecondCounterHiRes() - startTimeMs;
        const auto t = juce::jlimit (0.0, 1.0, elapsed / (double) durationMillis);

        // Ease-Out-Kubik: schnell los, weich ankommen
        const auto eased = 1.0 - std::pow (1.0 - t, 3.0);
        current = startValue + (targetValue - startValue) * (float) eased;

        if (t >= 1.0)
        {
            current   = targetValue;
            animating = false;
        }

        notify();
    }

    void notify()
    {
        if (onUpdate != nullptr)
            onUpdate (current);
    }

    juce::Component& owner;
    juce::VBlankAttachment vblank;

    float current     = 0.0f;
    float startValue  = 0.0f;
    float targetValue = 0.0f;
    double startTimeMs = 0.0;
    int durationMillis = 0;
    bool animating = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimatedValue)
};

} // namespace conduit
