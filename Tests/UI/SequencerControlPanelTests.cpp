#include <catch2/catch_test_macros.hpp>

#include "Modules/StepSequencerModule.h"
#include "UI/SequencerControlPanel.h"

namespace
{

// Headless: Panel bindet nur an den Node-Subtree — der Tree→Atomic-Sync
// dahinter ist GraphManager-Sache und separat getestet.
struct PanelRig
{
    [[nodiscard]] juce::ValueTree parameterFor (const juce::String& parameterId) const
    {
        return state.getChildWithName (conduit::id::parameters)
                   .getChildWithProperty (conduit::id::paramId, parameterId);
    }

    [[nodiscard]] float valueOf (const juce::String& parameterId) const
    {
        return static_cast<float> ((double) parameterFor (parameterId)
                                       .getProperty (conduit::id::paramValue));
    }

    juce::ScopedJuceInitialiser_GUI juceRuntime;
    juce::ValueTree state = conduit::StepSequencerModule().createState();
    conduit::SequencerControlPanel panel { state };
};

} // namespace

//==============================================================================
TEST_CASE ("SequencerControlPanel: Initial-Sync zeigt die Tree-Defaults", "[ui]")
{
    PanelRig rig;

    REQUIRE (rig.panel.modeBox.getSelectedId() == 1);        // 4×16
    REQUIRE (rig.panel.directionBox.getSelectedId() == 1);   // Vorwärts
    REQUIRE (juce::exactlyEqual (rig.panel.lengthSlider.getValue(), 16.0));
    REQUIRE_FALSE (rig.panel.quantizeToggle.getToggleState());

    REQUIRE (juce::exactlyEqual (rig.panel.rateKnob.getValue(),  2.0));
    REQUIRE (juce::exactlyEqual (rig.panel.gateKnob.getValue(),  0.5));
    REQUIRE (juce::exactlyEqual (rig.panel.swingKnob.getValue(), 0.0));
    REQUIRE (juce::exactlyEqual (rig.panel.probKnob.getValue(),  1.0));

    // Knob-Ranges kommen aus dem Tree (min/max), nicht aus UI-Konstanten
    REQUIRE (juce::exactlyEqual (rig.panel.rateKnob.getMinimum(), 0.25));
    REQUIRE (juce::exactlyEqual (rig.panel.rateKnob.getMaximum(), 8.0));
    REQUIRE (juce::exactlyEqual (rig.panel.swingKnob.getMaximum(), 0.75));
}

//==============================================================================
TEST_CASE ("SequencerControlPanel: Eingaben schreiben paramValue (ohne Undo)", "[ui]")
{
    PanelRig rig;

    rig.panel.modeBox.setSelectedId (2, juce::sendNotificationSync);
    REQUIRE (juce::exactlyEqual (rig.valueOf ("mode"), 1.0f));        // 2×32

    rig.panel.directionBox.setSelectedId (3, juce::sendNotificationSync);
    REQUIRE (juce::exactlyEqual (rig.valueOf ("direction"), 2.0f));   // Pendel

    rig.panel.lengthSlider.setValue (8.0, juce::sendNotificationSync);
    REQUIRE (juce::exactlyEqual (rig.valueOf ("length"), 8.0f));

    rig.panel.quantizeToggle.setToggleState (true, juce::sendNotificationSync);
    REQUIRE (juce::exactlyEqual (rig.valueOf ("quantize"), 1.0f));

    rig.panel.swingKnob.setValue (0.5, juce::sendNotificationSync);
    REQUIRE (juce::exactlyEqual (rig.valueOf ("swing"), 0.5f));

    rig.panel.rateKnob.setValue (4.0, juce::sendNotificationSync);
    REQUIRE (juce::exactlyEqual (rig.valueOf ("rate"), 4.0f));
}

//==============================================================================
TEST_CASE ("SequencerControlPanel: Controls folgen externen Tree-Änderungen (OSC/Undo)", "[ui]")
{
    PanelRig rig;

    rig.parameterFor ("mode").setProperty (conduit::id::paramValue, 2.0, nullptr);
    REQUIRE (rig.panel.modeBox.getSelectedId() == 3);                 // 1×64

    rig.parameterFor ("direction").setProperty (conduit::id::paramValue, 3.0, nullptr);
    REQUIRE (rig.panel.directionBox.getSelectedId() == 4);            // Zufall

    rig.parameterFor ("quantize").setProperty (conduit::id::paramValue, 1.0, nullptr);
    REQUIRE (rig.panel.quantizeToggle.getToggleState());

    rig.parameterFor ("prob").setProperty (conduit::id::paramValue, 0.25, nullptr);
    REQUIRE (juce::exactlyEqual (rig.panel.probKnob.getValue(), 0.25));

    rig.parameterFor ("length").setProperty (conduit::id::paramValue, 12.0, nullptr);
    REQUIRE (juce::exactlyEqual (rig.panel.lengthSlider.getValue(), 12.0));
}
