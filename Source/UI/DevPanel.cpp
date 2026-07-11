#include "DevPanel.h"

#include "NumberFieldBracket.h"
#include "UI/PushLookAndFeel.h"
#include "UI/UiSettingsComponent.h"

namespace conduit
{

namespace
{
    // Inhalt des Dev-Panels: UiSettingsComponent (Oberflaeche) oben, darunter
    // die Grid-Dev-Werte Schwellbreite/Fade-Zeit (Block A4 -- Umzug aus
    // MpeShapingView, dort war es dieselbe Slider-Optik; hier NumberFieldBracket).
    class DevPanelContent final : public juce::Component
    {
    public:
        DevPanelContent (UiSettings& uiSettingsToUse, GridPanelSettings& gridPanelSettingsToUse)
            : uiSettingsComponent (uiSettingsToUse), gridPanelSettings (gridPanelSettingsToUse)
        {
            addAndMakeVisible (uiSettingsComponent);
            addAndMakeVisible (gridHeading);
            addAndMakeVisible (thresholdField);
            addAndMakeVisible (fadeField);
            addAndMakeVisible (tabMinWidthField);
            addAndMakeVisible (physicsHeading);
            addAndMakeVisible (forceField);
            addAndMakeVisible (massField);
            addAndMakeVisible (inertiaField);
            addAndMakeVisible (gravityDelayField);
            addAndMakeVisible (gravityThresholdField);
            addAndMakeVisible (gravityFadeField);

            for (auto* heading : { &gridHeading, &physicsHeading })
            {
                heading->setColour (juce::Label::textColourId, push::colours::textDim);
                heading->setFont (push::scaledFont (12.0f, true));
            }

            thresholdField.setValue (gridPanelSettings.getEditorThresholdWidth(), juce::dontSendNotification);
            thresholdField.onValueChanged = [this] (double v)
            { gridPanelSettings.setEditorThresholdWidth ((int) v); };

            fadeField.setValue (gridPanelSettings.getNoteCircleFadeMs(), juce::dontSendNotification);
            fadeField.onValueChanged = [this] (double v)
            { gridPanelSettings.setNoteCircleFadeMs ((int) v); };

            // Block H3 Runde 3: Mindestbreite der Track-Tabs (der Strip
            // pollt den Wert live und wird ggf. horizontal scrollbar).
            tabMinWidthField.setValue (gridPanelSettings.getTrackTabMinWidthPx(),
                                       juce::dontSendNotification);
            tabMinWidthField.onValueChanged = [this] (double v)
            { gridPanelSettings.setTrackTabMinWidthPx ((int) v); };

            // Block J (Physics): Feder-Tuning — Keyboard/Control-Layer
            // pollen die Settings live, jede Änderung wirkt sofort.
            forceField.setValue (gridPanelSettings.getPhysicsForce(), juce::dontSendNotification);
            forceField.onValueChanged = [this] (double v)
            { gridPanelSettings.setPhysicsForce (v); };

            massField.setValue (gridPanelSettings.getPhysicsMass(), juce::dontSendNotification);
            massField.onValueChanged = [this] (double v)
            { gridPanelSettings.setPhysicsMass (v); };

            inertiaField.setValue (gridPanelSettings.getPhysicsInertia(), juce::dontSendNotification);
            inertiaField.onValueChanged = [this] (double v)
            { gridPanelSettings.setPhysicsInertia ((int) v); };

            gravityDelayField.setValue (gridPanelSettings.getGravityDelayMs(), juce::dontSendNotification);
            gravityDelayField.onValueChanged = [this] (double v)
            { gridPanelSettings.setGravityDelayMs ((int) v); };

            gravityThresholdField.setValue (gridPanelSettings.getGravityThreshold(), juce::dontSendNotification);
            gravityThresholdField.onValueChanged = [this] (double v)
            { gridPanelSettings.setGravityThreshold (v); };

            gravityFadeField.setValue (gridPanelSettings.getGravityFadeMs(), juce::dontSendNotification);
            gravityFadeField.onValueChanged = [this] (double v)
            { gridPanelSettings.setGravityFadeMs ((int) v); };
        }

        void resized() override
        {
            auto area = getLocalBounds();
            uiSettingsComponent.setBounds (area.removeFromTop (UiSettingsComponent::preferredHeight()));

            gridHeading.setBounds (area.removeFromTop (24).reduced (8, 0));
            thresholdField.setBounds (area.removeFromTop (NumberFieldBracket::kRowHeight).reduced (8, 0));
            fadeField.setBounds (area.removeFromTop (NumberFieldBracket::kRowHeight).reduced (8, 0));
            tabMinWidthField.setBounds (area.removeFromTop (NumberFieldBracket::kRowHeight).reduced (8, 0));

            physicsHeading.setBounds (area.removeFromTop (24).reduced (8, 0));
            forceField.setBounds (area.removeFromTop (NumberFieldBracket::kRowHeight).reduced (8, 0));
            massField.setBounds (area.removeFromTop (NumberFieldBracket::kRowHeight).reduced (8, 0));
            inertiaField.setBounds (area.removeFromTop (NumberFieldBracket::kRowHeight).reduced (8, 0));
            gravityDelayField.setBounds (area.removeFromTop (NumberFieldBracket::kRowHeight).reduced (8, 0));
            gravityThresholdField.setBounds (area.removeFromTop (NumberFieldBracket::kRowHeight).reduced (8, 0));
            gravityFadeField.setBounds (area.removeFromTop (NumberFieldBracket::kRowHeight).reduced (8, 0));
        }

        [[nodiscard]] static int preferredHeight() noexcept
        {
            return UiSettingsComponent::preferredHeight() + 24 * 2 + NumberFieldBracket::kRowHeight * 9;
        }

    private:
        UiSettingsComponent uiSettingsComponent;
        GridPanelSettings& gridPanelSettings;
        juce::Label gridHeading { {}, "Grid" };
        NumberFieldBracket thresholdField { NumberFieldBracket::Config {
            (double) GridPanelSettings::minThresholdWidth, (double) GridPanelSettings::maxThresholdWidth,
            (double) GridPanelSettings::defaultThresholdWidth, 1.0, 0, 1.0, "Thr" } };
        NumberFieldBracket fadeField { NumberFieldBracket::Config {
            (double) GridPanelSettings::minNoteCircleFadeMs, (double) GridPanelSettings::maxNoteCircleFadeMs,
            (double) GridPanelSettings::defaultNoteCircleFadeMs, 1.0, 0, 1.0, "Fade" } };
        NumberFieldBracket tabMinWidthField { NumberFieldBracket::Config {
            (double) GridPanelSettings::minTrackTabMinWidthPx, (double) GridPanelSettings::maxTrackTabMinWidthPx,
            (double) GridPanelSettings::defaultTrackTabMinWidthPx, 1.0, 0, 1.0, "TabW" } };

        // Block J (Physics): gemeinsamer Feder-Parametersatz (Force/Mass/
        // Inert) + Gravity-Timing (Delay/Thresh in Pad-Breiten/s/FadeF).
        juce::Label physicsHeading { {}, "Physics" };
        NumberFieldBracket forceField { NumberFieldBracket::Config {
            GridPanelSettings::minPhysicsForce, GridPanelSettings::maxPhysicsForce,
            GridPanelSettings::defaultPhysicsForce, 1.0, 0, 5.0, "Force" } };
        NumberFieldBracket massField { NumberFieldBracket::Config {
            GridPanelSettings::minPhysicsMass, GridPanelSettings::maxPhysicsMass,
            GridPanelSettings::defaultPhysicsMass, 0.1, 1, 0.02, "Mass" } };
        NumberFieldBracket inertiaField { NumberFieldBracket::Config {
            (double) GridPanelSettings::minPhysicsInertia, (double) GridPanelSettings::maxPhysicsInertia,
            (double) GridPanelSettings::defaultPhysicsInertia, 1.0, 0, 0.5, "Inert" } };
        NumberFieldBracket gravityDelayField { NumberFieldBracket::Config {
            (double) GridPanelSettings::minGravityDelayMs, (double) GridPanelSettings::maxGravityDelayMs,
            (double) GridPanelSettings::defaultGravityDelayMs, 1.0, 0, 2.0, "Delay" } };
        NumberFieldBracket gravityThresholdField { NumberFieldBracket::Config {
            GridPanelSettings::minGravityThreshold, GridPanelSettings::maxGravityThreshold,
            GridPanelSettings::defaultGravityThreshold, 0.05, 2, 0.02, "Thresh" } };
        NumberFieldBracket gravityFadeField { NumberFieldBracket::Config {
            (double) GridPanelSettings::minGravityFadeMs, (double) GridPanelSettings::maxGravityFadeMs,
            (double) GridPanelSettings::defaultGravityFadeMs, 1.0, 0, 3.0, "FadeF" } };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DevPanelContent)
    };
} // namespace

DevPanel::DevPanel (UiSettings& uiSettingsToUse, GridPanelSettings& gridPanelSettingsToUse)
    : juce::DocumentWindow ("Dev", push::colours::panel,
                            juce::DocumentWindow::closeButton)
{
    setUsingNativeTitleBar (true);
    setAlwaysOnTop (true);

    auto content = std::make_unique<DevPanelContent> (uiSettingsToUse, gridPanelSettingsToUse);
    content->setSize (380, DevPanelContent::preferredHeight());
    setContentOwned (content.release(), true);

    setResizable (false, false);
    setVisible (true);
}

void DevPanel::closeButtonPressed()
{
    if (onClose != nullptr)
        onClose();
}

} // namespace conduit
