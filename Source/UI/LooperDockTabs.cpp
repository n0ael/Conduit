#include "LooperDockTabs.h"

#include "Core/LaunchQuantization.h"
#include "PushSection.h"
#include "PushTiles.h"
#include "TransportBar.h"

namespace conduit
{

namespace
{
    constexpr int rowHeight = 34;
    constexpr int rowPadding = 8;
    constexpr int tileWidth = 34;
}

//==============================================================================
class LooperDockTabs::PlaceholderView final : public juce::Component
{
public:
    explicit PlaceholderView (const juce::String& hintToUse) : hint (hintToUse) {}

    void paint (juce::Graphics& g) override
    {
        g.fillAll (push::colours::panel);
        g.setColour (push::colours::textDim);
        g.setFont (push::scaledFont (13.0f));
        g.drawFittedText (hint, getLocalBounds().reduced (16),
                          juce::Justification::centredTop, 6, 1.0f);
    }

private:
    juce::String hint;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlaceholderView)
};

//==============================================================================
class LooperDockTabs::LooperTabView final : public juce::Component
{
public:
    LooperTabView (LooperDockTabs& ownerToUse, LooperSettings& settingsToUse)
        : owner (ownerToUse), settings (settingsToUse),
          layoutSection ("Layout", layoutRows),
          globalSection (juce::String::fromUTF8 ("Looper \xc2\xb7 Global"), globalRows)
    {
        setName ("looperDockTab");

        buildGlobalRows();

        column.addAndMakeVisible (layoutSection);
        column.addAndMakeVisible (globalSection);
        viewport.setViewedComponent (&column, false);
        viewport.setScrollBarsShown (true, false);
        addAndMakeVisible (viewport);

        // Collapse-Zustand aus den Settings (VIEW-Zustand, Bitmaske)
        const auto mask = settings.getPanelCollapsedMask();
        layoutSection.setCollapsed ((mask & 0b01) != 0, false);
        globalSection.setCollapsed ((mask & 0b10) != 0, false);
        layoutSection.onToggled = [this] (bool) { storeCollapsed(); relayout(); };
        globalSection.onToggled = [this] (bool) { storeCollapsed(); relayout(); };

        refreshLayoutRows();
        refreshFromSettings();
    }

    void resized() override
    {
        viewport.setBounds (getLocalBounds());
        relayout();
    }

    void paint (juce::Graphics& g) override { g.fillAll (push::colours::panel); }

    //==========================================================================
    /** LAYOUT-Zeilen an die Settings-Struktur anpassen (Zähler + Limits). */
    void refreshLayoutRows()
    {
        const auto numLoopers = settings.getNumLoopers();
        const auto wanted = 1 + numLoopers;   // Zeile 0 = Looper-Anzahl

        while ((int) layoutEntries.size() > wanted)
            layoutEntries.pop_back();
        while ((int) layoutEntries.size() < wanted)
        {
            auto entry = std::make_unique<LayoutRow>();
            const auto rowIndex = (int) layoutEntries.size();

            entry->minus.onClick = [this, rowIndex]
            {
                if (rowIndex == 0) { if (owner.onRemoveLooper) owner.onRemoveLooper(); }
                else if (owner.onRemoveTrack) owner.onRemoveTrack (rowIndex - 1);
            };
            entry->plus.onClick = [this, rowIndex]
            {
                if (rowIndex == 0) { if (owner.onAddLooper) owner.onAddLooper(); }
                else if (owner.onAddTrack) owner.onAddTrack (rowIndex - 1);
            };

            entry->caption.setColour (juce::Label::textColourId, push::colours::textDim);
            entry->caption.setJustificationType (juce::Justification::centredLeft);
            entry->count.setJustificationType (juce::Justification::centred);
            entry->count.setColour (juce::Label::textColourId, push::colours::text);

            layoutRows.addAndMakeVisible (entry->caption);
            layoutRows.addAndMakeVisible (entry->minus);
            layoutRows.addAndMakeVisible (entry->count);
            layoutRows.addAndMakeVisible (entry->plus);
            layoutEntries.push_back (std::move (entry));
        }

        for (int rowIndex = 0; rowIndex < wanted; ++rowIndex)
        {
            auto& entry = *layoutEntries[(std::size_t) rowIndex];
            if (rowIndex == 0)
            {
                entry.caption.setText ("Looper", juce::dontSendNotification);
                entry.count.setText (juce::String (numLoopers), juce::dontSendNotification);
                entry.minus.setAlpha (numLoopers > 1 ? 1.0f : 0.4f);
                entry.plus.setAlpha (numLoopers < LooperSettings::maxLoopers ? 1.0f : 0.4f);
            }
            else
            {
                const auto tracks = settings.getNumTracks (rowIndex - 1);
                entry.caption.setText ("LOOPER " + juce::String (rowIndex)
                                           + juce::String::fromUTF8 (" \xc2\xb7 Tracks"),
                                       juce::dontSendNotification);
                entry.count.setText (juce::String (tracks), juce::dontSendNotification);
                entry.minus.setAlpha (tracks > 1 ? 1.0f : 0.4f);
                entry.plus.setAlpha (tracks < LooperSettings::maxTracks ? 1.0f : 0.4f);
            }
        }

        layoutSection.setContentHeight (rowPadding + rowHeight * wanted);
        layoutRowsResized();
        relayout();
    }

    /** Controls dontSendNotification aus den Settings spiegeln. */
    void refreshFromSettings()
    {
        quantCombo.setSelectedId ((int) settings.getLaunchQuant() + 1,
                                  juce::dontSendNotification);
        tapModeCombo.setSelectedId (1 + (int) settings.getTapMode(),
                                    juce::dontSendNotification);
        reverseCombo.setSelectedId (1 + (int) settings.getReverseMode(),
                                    juce::dontSendNotification);
        variGridCombo.setSelectedId (1 + (int) settings.getVariRaster(),
                                     juce::dontSendNotification);
        variSwitchCombo.setSelectedId (1 + (int) settings.getVariScope(),
                                       juce::dontSendNotification);
        variDisplayCombo.setSelectedId (1 + (int) settings.getVariDisplay(),
                                        juce::dontSendNotification);
        soloCombo.setSelectedId (1 + (int) settings.getSoloScope(),
                                 juce::dontSendNotification);
        slotsCombo.setSelectedId (settings.getVisibleSlots(), juce::dontSendNotification);
        deleteLatchToggle.setToggleState (settings.isDeleteLatchEnabled(),
                                          juce::dontSendNotification);
        autoAdvanceToggle.setToggleState (settings.isAutoAdvanceEnabled(),
                                          juce::dontSendNotification);
        showStopAllToggle.setToggleState (settings.isShowStopAll(),
                                          juce::dontSendNotification);
    }

private:
    struct LayoutRow
    {
        juce::Label caption, count;
        push::TextTile minus { juce::String::fromUTF8 ("\xe2\x88\x92") },
                       plus { "+" };
    };

    struct GlobalRow
    {
        juce::Label caption;
        juce::Component* control = nullptr;
    };

    void addGlobalRow (const juce::String& caption, juce::Component& control,
                       const juce::String& componentId)
    {
        control.setComponentID (componentId);

        auto row = std::make_unique<GlobalRow>();
        row->caption.setText (caption, juce::dontSendNotification);
        row->caption.setColour (juce::Label::textColourId, push::colours::textDim);
        row->caption.setJustificationType (juce::Justification::centredLeft);
        row->control = &control;

        globalRows.addAndMakeVisible (row->caption);
        globalRows.addAndMakeVisible (control);
        globalEntries.push_back (std::move (row));
    }

    void buildGlobalRows()
    {
        // Alle Labels ENGLISCH (Handoff-Vorgabe 20.07.2026). Quantize
        // behält bewusst die volle App-Liste (None…1/32), nicht nur die
        // vier Mock-Werte.
        for (int i = 0; i < numLaunchQuants; ++i)
            quantCombo.addItem (launchQuantName (static_cast<LaunchQuant> (i)), i + 1);
        quantCombo.onChange = [this]
        {
            settings.setLaunchQuant (static_cast<LaunchQuant> (
                quantCombo.getSelectedItemIndex()));
        };

        // Item-Ids == Enum-Wert + 1 (Reihenfolge = Enum-Reihenfolge)
        tapModeCombo.addItem ("Retrigger", 1);
        tapModeCombo.addItem ("Stop", 2);
        tapModeCombo.addItem ("Legato", 3);
        tapModeCombo.onChange = [this]
        {
            settings.setTapMode (static_cast<LooperSettings::TapMode> (
                tapModeCombo.getSelectedId() - 1));
        };

        reverseCombo.addItem ("Instant", 1);
        reverseCombo.addItem ("At Loop End", 2);
        reverseCombo.addItem ("Quantized", 3);
        reverseCombo.onChange = [this]
        {
            settings.setReverseMode (static_cast<LooperSettings::ReverseMode> (
                reverseCombo.getSelectedId() - 1));
        };

        variGridCombo.addItem ("Semitones", 1);
        variGridCombo.addItem ("Session Scale", 2);
        variGridCombo.onChange = [this]
        {
            settings.setVariRaster (static_cast<LooperSettings::VariRaster> (
                variGridCombo.getSelectedId() - 1));
        };

        variSwitchCombo.addItem ("Per Track", 1);
        variSwitchCombo.addItem ("Per Looper", 2);
        variSwitchCombo.addItem ("Global", 3);
        variSwitchCombo.onChange = [this]
        {
            settings.setVariScope (static_cast<LooperSettings::VariScope> (
                variSwitchCombo.getSelectedId() - 1));
        };

        variDisplayCombo.addItem ("Semitones", 1);
        variDisplayCombo.addItem ("Scale Degrees", 2);
        variDisplayCombo.onChange = [this]
        {
            settings.setVariDisplay (static_cast<LooperSettings::VariDisplay> (
                variDisplayCombo.getSelectedId() - 1));
        };

        soloCombo.addItem ("Per Looper", 1);
        soloCombo.addItem ("Global", 2);
        soloCombo.onChange = [this]
        {
            settings.setSoloScope (static_cast<LooperSettings::SoloScope> (
                soloCombo.getSelectedId() - 1));
        };

        for (int slots = LooperSettings::minVisibleSlots;
             slots <= LooperSettings::maxVisibleSlots; ++slots)
            slotsCombo.addItem (juce::String (slots), slots);
        slotsCombo.onChange = [this]
        { settings.setVisibleSlots (slotsCombo.getSelectedId()); };

        deleteLatchToggle.onClick = [this]
        { settings.setDeleteLatchEnabled (deleteLatchToggle.getToggleState()); };
        autoAdvanceToggle.onClick = [this]
        { settings.setAutoAdvanceEnabled (autoAdvanceToggle.getToggleState()); };
        showStopAllToggle.onClick = [this]
        { settings.setShowStopAll (showStopAllToggle.getToggleState()); };

        addGlobalRow ("Quantization", quantCombo, "quant");
        addGlobalRow ("Tap on Clip", tapModeCombo, "tapMode");
        addGlobalRow ("Reverse", reverseCombo, "reverse");
        addGlobalRow ("VARI Grid", variGridCombo, "variGrid");
        addGlobalRow ("VARI Switch", variSwitchCombo, "variSwitch");
        addGlobalRow ("VARI Display (Quant)", variDisplayCombo, "variDisplay");
        addGlobalRow ("Solo", soloCombo, "solo");
        addGlobalRow ("Visible Slots", slotsCombo, "slots");
        addGlobalRow ("Delete as Toggle", deleteLatchToggle, "deleteLatch");
        addGlobalRow ("Auto-Advance", autoAdvanceToggle, "autoAdvance");
        addGlobalRow ("Show Stop All", showStopAllToggle, "showStopAll");

        globalSection.setContentHeight (rowPadding
                                        + rowHeight * (int) globalEntries.size());
    }

    void storeCollapsed()
    {
        settings.setPanelCollapsedMask ((layoutSection.isCollapsed() ? 0b01 : 0)
                                        | (globalSection.isCollapsed() ? 0b10 : 0));
    }

    void layoutRowsResized()
    {
        auto bounds = layoutRows.getLocalBounds().reduced (rowPadding, 0);
        bounds.removeFromTop (rowPadding / 2);
        for (auto& entry : layoutEntries)
        {
            auto line = bounds.removeFromTop (rowHeight);
            entry->plus.setBounds (line.removeFromRight (tileWidth).reduced (2));
            entry->count.setBounds (line.removeFromRight (tileWidth));
            entry->minus.setBounds (line.removeFromRight (tileWidth).reduced (2));
            entry->caption.setBounds (line);
        }
    }

    void globalRowsResized()
    {
        auto bounds = globalRows.getLocalBounds().reduced (rowPadding, 0);
        bounds.removeFromTop (rowPadding / 2);
        const auto captionWidth = juce::jmax (110, bounds.getWidth() * 45 / 100);
        for (auto& entry : globalEntries)
        {
            auto line = bounds.removeFromTop (rowHeight);
            entry->caption.setBounds (line.removeFromLeft (captionWidth));
            entry->control->setBounds (line.reduced (0, 4));
        }
    }

    void relayout()
    {
        const auto width = juce::jmax (0, viewport.getWidth()
                                              - viewport.getScrollBarThickness());
        auto y = 0;

        layoutSection.setBounds (0, y, width, layoutSection.getPreferredHeight());
        y += layoutSection.getPreferredHeight();
        globalSection.setBounds (0, y, width, globalSection.getPreferredHeight());
        y += globalSection.getPreferredHeight();

        column.setSize (width, y);
        layoutRowsResized();
        globalRowsResized();
    }

    LooperDockTabs& owner;
    LooperSettings& settings;

    juce::Viewport viewport;
    juce::Component column, layoutRows, globalRows;
    push::CollapsibleSection layoutSection, globalSection;

    std::vector<std::unique_ptr<LayoutRow>> layoutEntries;
    std::vector<std::unique_ptr<GlobalRow>> globalEntries;

    juce::ComboBox quantCombo, tapModeCombo, reverseCombo, variGridCombo,
                   variSwitchCombo, variDisplayCombo, soloCombo, slotsCombo;
    juce::ToggleButton deleteLatchToggle, autoAdvanceToggle, showStopAllToggle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperTabView)
};

//==============================================================================
LooperDockTabs::LooperDockTabs (EditorDockPanel& dockToUse, LooperSettings& settingsToUse)
    : dock (dockToUse), settings (settingsToUse)
{
    const auto looperOnly = 1 << TransportBar::pageLooper;

    auto looper = std::make_unique<LooperTabView> (*this, settings);
    looperView = looper.get();
    dock.addTab ("looper", "LOOPER", std::move (looper), looperOnly);

    dock.addTab ("looperMixer", "MIXER",
                 std::make_unique<PlaceholderView> (
                     juce::String::fromUTF8 ("MASTER \xc2\xb7 SENDS \xc2\xb7 DISTANCE "
                                             "\xc2\xb7 DISPLAY folgen mit dem Mixer-"
                                             "Meilenstein.")),
                 looperOnly);
    dock.addTab ("looperMidi", "MIDI",
                 std::make_unique<PlaceholderView> (
                     juce::String::fromUTF8 ("MAP MODE folgt mit dem MIDI-Map-"
                                             "Meilenstein.")),
                 looperOnly);

    settings.addChangeListener (this);
}

LooperDockTabs::~LooperDockTabs()
{
    settings.removeChangeListener (this);

    // Muster GridPage: Inhalte referenzieren Members dieses Lieferanten —
    // vor der eigenen Zerstörung aus dem (länger lebenden) Dock entfernen
    dock.removeTab ("looper");
    dock.removeTab ("looperMixer");
    dock.removeTab ("looperMidi");
}

void LooperDockTabs::refreshLayout()
{
    if (looperView != nullptr)
        looperView->refreshLayoutRows();
}

juce::Component* LooperDockTabs::getLooperTabContent() noexcept
{
    return looperView;
}

void LooperDockTabs::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (looperView != nullptr)
    {
        looperView->refreshFromSettings();
        looperView->refreshLayoutRows();
    }
}

} // namespace conduit
