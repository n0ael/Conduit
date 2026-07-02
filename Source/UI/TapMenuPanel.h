#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/TransportSettings.h"

namespace conduit
{

//==============================================================================
/**
    Dropdown-Panel hinter dem Tap-▾ der TransportBar (User-Entwurf
    2026-07-02, Muster LinkMenuPanel): Auto-Commit nach n Taps (fürs
    MIDI/OSC-Mapping des Tap-Buttons) und die Reset-Haltedauer.

    Schreibt NUR in die TransportSettings — die TransportBar liest sie
    beim nächsten Tap bzw. im refresh()-Poll. Message Thread.
*/
class TapMenuPanel final : public juce::Component
{
public:
    explicit TapMenuPanel (TransportSettings& settingsToUse);

    void resized() override;
    void paint (juce::Graphics& g) override;

    static constexpr int panelWidth  = 260;
    static constexpr int panelHeight = 168;

    // Test-Zugriff
    [[nodiscard]] juce::ToggleButton& getAutoCommitToggle() noexcept { return autoCommitToggle; }
    [[nodiscard]] juce::Slider& getTapCountSlider() noexcept { return tapCountSlider; }
    [[nodiscard]] juce::Slider& getResetHoldSlider() noexcept { return resetHoldSlider; }

private:
    TransportSettings& settings;

    juce::ToggleButton autoCommitToggle { "Auto-Commit nach n Taps" };
    juce::Label tapCountCaption;
    juce::Slider tapCountSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Label resetHoldCaption;
    juce::Slider resetHoldSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapMenuPanel)
};

} // namespace conduit
