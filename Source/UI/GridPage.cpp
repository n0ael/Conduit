#include "GridPage.h"

#include "CcPanel.h"
#include "GridSettingsView.h"
#include "MacroPanel.h"
#include "Modules/ConduitModule.h"
#include "MpeShapingView.h"

namespace conduit
{

GridPage::GridPage (juce::ValueTree rootStateToUse,
                     grid::GridVoiceEngine& engineToUse, grid::MidiDeviceTarget& midiTargetToUse,
                     GridPanelSettings& panelSettingsToUse, grid::MpeMidiSink& mpeMidiSinkToUse,
                     LiveSetModel& liveSetModelToUse, TouchLiveClient& touchLiveClientToUse,
                     grid::MidiControlInput& midiControlInputToUse)
    : rootState (std::move (rootStateToUse)),
      engine (engineToUse), midiTarget (midiTargetToUse), panelSettings (panelSettingsToUse),
      mpeMidiSink (mpeMidiSinkToUse),
      liveSetModel (liveSetModelToUse), touchLiveClient (touchLiveClientToUse),
      midiControlInput (midiControlInputToUse),
      systemControlRowsAtStartup (panelSettingsToUse.getSystemControlRows()),
      // 8×8-Raster der Grid-Page (padLayoutConfig, User 10.07.2026) — Keyboard
      // und ccLayer teilen sich dieselbe Zellgeometrie.
      keyboard (engineToUse, padLayoutConfig()),
      ccLayer (ccModel, padLayoutConfig().cols, padLayoutConfig().rows),
      // System-Controls des XY+Fader-Modus: eigenes 8×N-Zellraster exakt über
      // den oberen N Pad-Reihen (resized), N = systemControlRowsAtStartup.
      systemLayer (systemCcModel, padLayoutConfig().cols, systemControlRowsAtStartup)
{
    addAndMakeVisible (padsModeTile);
    addAndMakeVisible (xyModeTile);
    addAndMakeVisible (releaseAllButton);
    addAndMakeVisible (octaveUpTile);
    addAndMakeVisible (octaveDownTile);
    addAndMakeVisible (atOffsetRibbon);
    addAndMakeVisible (slideOffsetRibbon);
    addAndMakeVisible (pitchOffsetRibbon);
    addChildComponent (modwheelRibbon);   // sichtbar nur bei aktiviertem Modwheel (Block D1)
    addAndMakeVisible (keyboard);
    addAndMakeVisible (ccLayer);     // NACH keyboard hinzugefügt = liegt darüber
    addChildComponent (systemLayer); // NACH ccLayer = darüber: System-Controls
                                     // gewinnen die Hit-Tests auch im CC-Tab-
                                     // Modus (dort SPIELBAR — TODO(design));
                                     // sichtbar nur im XY+Fader-Modus.
    addAndMakeVisible (chordStrip);  // eigene Spalte NEBEN Keyboard/ccLayer
    addChildComponent (dockPanel);   // sichtbar nur wenn offen (setPanelOpen)

    // Layout-Modus-Kacheln (User 10.07.2026): 64 Pads (Push-Style) vs.
    // XY+Fader über den oberen zwei Pad-Reihen — Tap setzt Persistenz und
    // wendet den Modus an. systemLayer bleibt IMMER im Play-Modus
    // (setCcMode(false) ist der Ctor-Default, es gibt keinen Umschalter).
    padsModeTile.onClick = [this] { setLayoutMode (GridPanelSettings::GridLayoutMode::fullPads); };
    xyModeTile.onClick   = [this] { setLayoutMode (GridPanelSettings::GridLayoutMode::xyFaders); };

    releaseAllButton.onClick = [this]
    {
        engine.allNotesOff();
        keyboard.clearLatched();   // latched Akkord mit beenden (dessen
                                   // noteOffs verpuffen nach allNotesOff)
    };

    // Oktav-Shift (Block D2): über dem Pitch-Ribbon, geklemmt in
    // GridKeyboardComponent (+-kMaxOctaveShift).
    octaveUpTile.onClick   = [this] { keyboard.octaveUp(); };
    octaveDownTile.onClick = [this] { keyboard.octaveDown(); };

    // Akkord-Speicher (Grid-Page v2, Feature 6): Strip ↔ Keyboard/Memory.
    chordStrip.getConstellation = [this] { return keyboard.constellationNormalized(); };
    chordStrip.onRecall = [this] (int slot) { keyboard.latchConstellation (chordMemory.slot (slot)); };
    chordStrip.onMoveBy = [this] (float dx, float dy) { keyboard.moveLatchedBy (dx, dy); };
    chordStrip.isCcMode = [this] { return ccLayer.isCcMode(); };

    // Session-Skala (Schema 6.2): GridPage liest weiterhin für die
    // Keyboard-Einfärbung mit -- die Anzeige-Kacheln selbst leben seit
    // Block D1 im Settings-Tab (GridSettingsView).
    rootState.addListener (this);
    refreshScaleFromState();   // Initialwert beim Konstruieren lesen

    // Bipolar: Mitte (value01 == 0.5) -> Offset 0, oben -> +1, unten -> -1.
    atOffsetRibbon.onValueChanged = [this] (float value) { engine.setPressureOffset ((value - 0.5f) * 2.0f); };
    slideOffsetRibbon.onValueChanged = [this] (float value) { engine.setSlideOffset ((value - 0.5f) * 2.0f); };

    // Bipolar: Mitte -> 0 HT, oben/unten -> ±kPitchBendOffsetSemitones.
    pitchOffsetRibbon.onValueChanged = [this] (float value)
    {
        engine.setPitchBendOffset ((value - 0.5f) * 2.0f * kPitchBendOffsetSemitones);
    };

    // Modwheel (Block D1, unipolar): sendet CC1 auf dem MPE-Master-Kanal
    // direkt über den MidiDeviceTarget -- kein eigener Sink-Pfad, da nur
    // ein globaler Controller-Wert (kein Voice-Bezug) gebraucht wird.
    modwheelRibbon.onValueChanged = [this] (float value01)
    {
        const auto v = juce::jlimit (0, 127, (int) juce::roundToInt (value01 * 127.0f));
        midiTarget.send (juce::MidiMessage::controllerEvent (
            grid::MpeEncoder::Config{}.memberChannelBase - 1, 1, v));
    };
    modwheelRibbon.setVisible (panelSettings.isModwheelEnabled());

    // Achsen-Farben (Grid-Page v2): user-konfigurierbar und persistent in
    // GridPanelSettings — Initialwerte von dort statt hart kodiert.
    using Axis = grid::GridVoiceEngine::Axis;
    pitchOffsetRibbon.setFillColour (panelSettings.getAxisColour (Axis::PitchBend));
    atOffsetRibbon.setFillColour    (panelSettings.getAxisColour (Axis::Pressure));
    slideOffsetRibbon.setFillColour (panelSettings.getAxisColour (Axis::Slide));

    // Editor-Dock-Panel: ein Tab „MPE" mit dem MPE-Shaping-Editor (S2c),
    // Breite/Offen-Zustand aus der Persistenz laden, Live-Resize + Commit
    // verdrahten. Farbwahl in der MpeShapingView (Quick-Swatch/Picker)
    // aktualisiert die Ribbon-Füllfarben live (Persistenz macht die View).
    auto mpeView = std::make_unique<MpeShapingView> (engine, panelSettings);
    mpeView->onAxisColourChanged = [this] (Axis axis, juce::Colour colour)
    {
        switch (axis)
        {
            case Axis::Pressure:  atOffsetRibbon.setFillColour (colour); break;
            case Axis::Slide:     slideOffsetRibbon.setFillColour (colour); break;
            case Axis::PitchBend: pitchOffsetRibbon.setFillColour (colour); break;
        }
    };
    // Sensitivity-/Range-Regler (Block A2/A3): die View meldet nur den Wert,
    // GridPage reicht ihn an das GridKeyboardComponent durch (Laufzeit-only,
    // keine Persistenz -- Block K).
    mpeView->onSensitivityChanged = [this] (Axis axis, double sensitivity)
    {
        if (axis == Axis::Pressure)
            keyboard.setPressureSensitivity (sensitivity);
        else if (axis == Axis::Slide)
            keyboard.setSlideSensitivity (sensitivity);
    };
    mpeView->onPitchBendMultiplierChanged = [this] (float multiplier)
    {
        keyboard.setPitchBendMultiplier (multiplier);
    };
    dockPanel.addTab ("mpe", "MPE", std::move (mpeView));

    // Tab 2 „DIY" (Block F, frueher „CC"): frei baubarer Zusatz-Controller.
    // Die Tab-ID bleibt "cc" (updateCcMode + Persistenz haengen daran),
    // nur der sichtbare Titel heisst DIY. Werkzeugwahl geht ans Overlay;
    // der Edit-Modus gilt bei offenem Panel + aktivem DIY-Tab (updateCcMode).
    auto ccPanel = std::make_unique<CcPanel>();
    ccPanel->onToolChanged = [this] (grid::CcTool tool) { ccLayer.setActiveTool (tool); };
    dockPanel.addTab ("cc", "DIY", std::move (ccPanel));

    // Tab 4 „Macro" (Block E): Ziel-Listen der Controls. Der rohe Zeiger
    // bleibt gueltig, solange das dockPanel (GridPage-Member) lebt.
    auto macroView = std::make_unique<MacroPanel> (macroBindings, midiTarget,
                                                   liveSetModel, touchLiveClient, midiInBindings);
    macroPanel = macroView.get();
    dockPanel.addTab ("macro", "Macro", std::move (macroView));

    // Macro-Wertfluss + Long-Press (Block E), beide Layer: System-Controls
    // des XY+Fader-Modus (layer 0) und DIY-CC-Baukasten (layer 1). Die
    // Layer-Callbacks feuern NUR bei lokalem Touch -- der loest zusaetzlich
    // den Soft-Takeover der MIDI-Eingangs-Bindung (Block G, der externe
    // Fader muss danach neu aufnehmen).
    systemLayer.onControlValueChanged = [this] (const grid::CcControl& control)
    {
        midiInBindings.notifyLocalTouch ({ grid::MacroControlKey::system, control.id, 0 });
        midiInBindings.notifyLocalTouch ({ grid::MacroControlKey::system, control.id, 1 });
        feedMacros (grid::MacroControlKey::system, control);
    };
    ccLayer.onControlValueChanged = [this] (const grid::CcControl& control)
    {
        midiInBindings.notifyLocalTouch ({ grid::MacroControlKey::diy, control.id, 0 });
        midiInBindings.notifyLocalTouch ({ grid::MacroControlKey::diy, control.id, 1 });
        feedMacros (grid::MacroControlKey::diy, control);
    };

    // MIDI-Eingang (Block G): Pumpe fuettert die Bindings, der Tick treibt
    // Glaettung + Soft-Takeover und wendet Werte auf die Controls an.
    midiControlInput.onCcReceived = [this] (int channel, int cc, int value7bit)
    { midiInBindings.handleIncomingCc (channel, cc, value7bit); };
    midiControlInput.onTick = [this]
    {
        midiInBindings.tick ([this] (const grid::MacroControlKey& key) { return controlValueFor (key); },
                             [this] (const grid::MacroControlKey& key, float value01)
                             { applyExternalValue (key, value01); });
    };

    systemLayer.onLongPressControl = [this] (int controlId)
    { openMacroViewFor (grid::MacroControlKey::system, controlId, systemCcModel); };
    ccLayer.onLongPressControl = [this] (int controlId)
    { openMacroViewFor (grid::MacroControlKey::diy, controlId, ccModel); };

    // Tab 3 „Settings" (Block D1): In-Tune Location/Width + Expression Mode
    // (Block B1/B2/B4), Layout-Feinabstimmung (Edit-Grid-Ersatz), Modwheel-
    // Toggle, Performance-Slide-Out (MIDI-Port + Skala, ehemals Top-Row).
    auto settingsView = std::make_unique<GridSettingsView> (
        rootState, midiTarget, midiControlInput, panelSettings,
        keyboard.getInTuneLocation(), padLayoutConfig().inTuneWidthPercent,
        mpeMidiSink.expressionMode());
    settingsView->onInTuneLocationChanged = [this] (grid::InTuneLocation location)
    { keyboard.setInTuneLocation (location); };
    settingsView->onInTuneWidthChanged = [this] (float percent)
    { keyboard.setInTuneWidthPercent (percent); };
    settingsView->onExpressionModeChanged = [this] (grid::ExpressionMode mode)
    { mpeMidiSink.setExpressionMode (mode); };
    settingsView->onLayoutSettingsChanged = [this] { applyRibbonWidth(); };
    settingsView->onModwheelToggled = [this] (bool enabled) { modwheelRibbon.setVisible (enabled); resized(); };
    dockPanel.addTab ("settings", "Settings", std::move (settingsView));

    dockPanel.setPanelWidth (panelSettings.getEditorPanelWidth());
    dockPanel.setPanelOpen (panelSettings.isEditorPanelOpen());

    dockPanel.onWidthChanged   = [this] { resized(); };
    dockPanel.onWidthCommitted = [this] (int width) { panelSettings.setEditorPanelWidth (width); };
    dockPanel.onActiveTabChanged = [this] (const juce::String&) { updateCcMode(); };

    updateCcMode();   // Initialzustand (Panel-Open aus panelSettings, aktiver Tab "mpe")
    applyLayoutMode();   // Initialer Layout-Modus aus der Persistenz
}

GridPage::~GridPage()
{
    rootState.removeListener (this);
}

//==============================================================================
int GridPage::nextScaleRoot (int rootNote) noexcept
{
    return ((rootNote + 1) % 12 + 12) % 12;
}

ScaleType GridPage::nextScaleType (ScaleType type) noexcept
{
    switch (type)
    {
        case ScaleType::chromatic:  return ScaleType::major;
        case ScaleType::major:      return ScaleType::minor;
        case ScaleType::minor:      return ScaleType::pentatonic;
        case ScaleType::pentatonic: return ScaleType::chromatic;
    }

    jassertfalse;
    return ScaleType::chromatic;
}

juce::String GridPage::noteNameFor (int rootNote)
{
    static const char* const noteNames[] = { "C", "C#", "D", "D#", "E", "F",
                                             "F#", "G", "G#", "A", "A#", "B" };
    return noteNames[juce::jlimit (0, 11, rootNote)];
}

juce::String GridPage::scaleDisplayNameFor (ScaleType type)
{
    const auto name = toString (type);
    return name.substring (0, 1).toUpperCase() + name.substring (1);
}

grid::PadGridLayout::Config GridPage::padLayoutConfig() noexcept
{
    // 64 Pads (8×8, Push-Style, User 10.07.2026): die Config-Defaults bleiben
    // 8×4 — nur die Grid-Page setzt rows explizit. lowestNote 48 unverändert,
    // die neuen Reihen wachsen nach OBEN dazu (+5 HT/Reihe).
    grid::PadGridLayout::Config config;
    config.rows = 8;
    return config;
}

void GridPage::refreshScaleFromState()
{
    const auto rootNote = juce::jlimit (0, 11, (int) rootState.getProperty (id::scaleRoot, 0));
    const auto type = scaleTypeFromString (rootState.getProperty (id::scaleType).toString());

    keyboard.setScale (rootNote, type);
}

void GridPage::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree == rootState && (property == id::scaleRoot || property == id::scaleType))
        refreshScaleFromState();
}

//==============================================================================
void GridPage::setDockPanelOpen (bool shouldBeOpen) noexcept
{
    dockPanel.setPanelOpen (shouldBeOpen);
    panelSettings.setEditorPanelOpen (shouldBeOpen);
    updateCcMode();
    resized();
}

void GridPage::updateCcMode()
{
    ccLayer.setCcMode (dockPanel.isPanelOpen() && dockPanel.getActiveTabId() == "cc");
}

void GridPage::setLayoutMode (GridPanelSettings::GridLayoutMode newMode)
{
    panelSettings.setGridLayoutMode (newMode);
    applyLayoutMode();
}

void GridPage::applyLayoutMode()
{
    const auto xyFaders = panelSettings.getGridLayoutMode()
                              == GridPanelSettings::GridLayoutMode::xyFaders;

    padsModeTile.setActive (! xyFaders);
    xyModeTile.setActive (xyFaders);

    // Sauberer Zustand bei jedem Wechsel: Modell leeren, im XY+Fader-Modus
    // frisch bestücken — der Werte-Reset ist akzeptiert (TODO(design):
    // Persistenz der Control-Werte über den Moduswechsel).
    systemCcModel.clear();

    if (xyFaders)
        grid::buildXyFaderLayout (systemCcModel);

    systemLayer.setVisible (xyFaders);
    systemLayer.repaint();
    resized();
}

void GridPage::applyRibbonWidth()
{
    resized();   // liest panelSettings.getRibbonWidthPx() live -- voll dynamisch
}

//==============================================================================
// Macro-System (Block E)

void GridPage::feedMacros (int layer, const grid::CcControl& control)
{
    switch (control.type)
    {
        case grid::CcTool::fader:
            macroBindings.applyValue ({ layer, control.id, 0 }, control.value);
            break;

        case grid::CcTool::push:
        case grid::CcTool::toggle:
            macroBindings.applyValue ({ layer, control.id, 0 }, control.on ? 1.0f : 0.0f);
            break;

        case grid::CcTool::xy:
            // control.y ist 0 = oben -- fuer Macro-Semantik invertieren
            // (oben = 1, wie beim Fader).
            macroBindings.applyValue ({ layer, control.id, 0 }, control.x);
            macroBindings.applyValue ({ layer, control.id, 1 }, 1.0f - control.y);
            break;

        case grid::CcTool::none:
        default:
            break;
    }
}

grid::CcControlModel& GridPage::modelForLayer (int layer) noexcept
{
    return layer == grid::MacroControlKey::system ? systemCcModel : ccModel;
}

float GridPage::controlValueFor (const grid::MacroControlKey& key) noexcept
{
    const auto* control = modelForLayer (key.layer).find (key.controlId);
    if (control == nullptr)
        return 0.0f;

    switch (control->type)
    {
        case grid::CcTool::fader:  return control->value;
        case grid::CcTool::push:
        case grid::CcTool::toggle: return control->on ? 1.0f : 0.0f;
        case grid::CcTool::xy:     return key.axis == 0 ? control->x : 1.0f - control->y;
        case grid::CcTool::none:
        default:                   return 0.0f;
    }
}

void GridPage::applyExternalValue (const grid::MacroControlKey& key, float value01)
{
    auto* control = modelForLayer (key.layer).find (key.controlId);
    if (control == nullptr)
        return;

    switch (control->type)
    {
        case grid::CcTool::fader:
            control->value = juce::jlimit (0.0f, 1.0f, value01);
            break;

        case grid::CcTool::push:
        case grid::CcTool::toggle:
            control->on = value01 >= 0.5f;
            break;

        case grid::CcTool::xy:
            if (key.axis == 0)
                control->x = juce::jlimit (0.0f, 1.0f, value01);
            else
                control->y = juce::jlimit (0.0f, 1.0f, 1.0f - value01);   // y: 0 = oben
            break;

        case grid::CcTool::none:
        default:
            return;
    }

    // Anzeige folgt (Block G) + Macro-Ziele bekommen dieselben weichen Werte.
    (key.layer == grid::MacroControlKey::system ? systemLayer : ccLayer).repaint();
    feedMacros (key.layer, *control);
}

void GridPage::openMacroViewFor (int layer, int controlId, grid::CcControlModel& model)
{
    const auto* control = model.find (controlId);
    if (control == nullptr || macroPanel == nullptr)
        return;

    const auto typeName = [&]() -> juce::String
    {
        switch (control->type)
        {
            case grid::CcTool::fader:  return "Fader";
            case grid::CcTool::push:   return "Push";
            case grid::CcTool::toggle: return "Toggle";
            case grid::CcTool::xy:     return "XY-Pad";
            case grid::CcTool::none:
            default:                   return "Control";
        }
    }();

    macroPanel->showControl (layer, controlId,
                             typeName + " " + juce::String (controlId),
                             control->type == grid::CcTool::xy);

    dockPanel.setPanelOpen (true);
    panelSettings.setEditorPanelOpen (true);
    dockPanel.setActiveTab ("macro");
    updateCcMode();
    resized();
}

void GridPage::resized()
{
    auto bounds = getLocalBounds();

    // Editor-Dock-Panel rechts, VOR dem restlichen Layout reserviert --
    // koexistiert mit dem eine Ebene höher (EngineEditor) angedockten
    // Browser-Panel, da dessen removeFromRight bereits in den an GridPage
    // übergebenen bounds steckt.
    dockPanel.setBounds (bounds.removeFromRight (dockPanel.getPreferredWidth()));

    // Block D2: die frühere MIDI-Port-/Skala-Top-Row entfällt zugunsten des
    // Performance-Slide-Outs (Settings-Tab, GridSettingsView) — nur noch
    // die Layout-Modus-Kacheln oben links, kompakte 28-px-Zeile.
    auto topStrip = bounds.removeFromTop (28);
    constexpr int modeTileSide = 24;
    padsModeTile.setBounds (topStrip.getX() + 4, topStrip.getY() + 2, modeTileSide, modeTileSide);
    xyModeTile.setBounds (padsModeTile.getRight() + 4, topStrip.getY() + 2, modeTileSide, modeTileSide);

    const auto ribbonWidth = panelSettings.getRibbonWidthPx();   // Block D1, live

    // Block D2 (User-Feedback 11.07.): Pitch-Fader-Stapel — Oktav-Buttons
    // darüber, je EIN Pad hoch und volle Ribbon-Breite (statt nebeneinander
    // in einer schmalen Zeile) — Up oben (höherer Pitch = oben am Bildschirm),
    // Down darunter; Release All darunter (ersetzt die frühere Top-Row).
    auto pitchColumn = bounds.removeFromLeft (ribbonWidth);
    const auto padHeight = juce::roundToInt ((float) pitchColumn.getHeight()
                                                 / (float) padLayoutConfig().rows);
    octaveUpTile.setBounds (pitchColumn.removeFromTop (padHeight));
    octaveDownTile.setBounds (pitchColumn.removeFromTop (padHeight));
    auto releaseRow = pitchColumn.removeFromBottom (36);
    releaseAllButton.setBounds (releaseRow.reduced (2));
    pitchOffsetRibbon.setBounds (pitchColumn);

    // Modwheel (Block D1): eigene Spalte direkt neben Pitch, Breite nur
    // reserviert, wenn aktiviert.
    if (modwheelRibbon.isVisible())
        modwheelRibbon.setBounds (bounds.removeFromLeft (ribbonWidth));

    auto rightColumn = bounds.removeFromRight (ribbonWidth);
    atOffsetRibbon.setBounds    (rightColumn.removeFromTop (rightColumn.getHeight() / 2));
    slideOffsetRibbon.setBounds (rightColumn);

    // Akkord-Speicher-Strip (Grid-Page v2, Feature 6) zwischen Pad-Raster
    // und rechter Ribbon-Spalte — Mock-Formel: quadratische Slots aus der
    // verbleibenden Höhe (8 Slots, 2 px Gap, 1 px Padding).
    const auto stripW = juce::jmax (40, juce::roundToInt (((float) bounds.getHeight() - 16.0f) / 8.0f) + 2);
    chordStrip.setBounds (bounds.removeFromRight (stripW));

    keyboard.setBounds (bounds);
    ccLayer.setBounds (bounds);   // Overlay exakt über den Keyboard-Bounds

    // System-Controls (XY+Fader-Modus): exakt die oberen systemControlRowsAtStartup
    // Zellreihen der Keyboard-Fläche — IMMER positioniert, die Sichtbarkeit
    // entscheidet. Zeilenzahl fix seit Konstruktion (CcControlLayer::rows
    // ist const, siehe systemControlRowsAtStartup-Kommentar im Header).
    const auto systemHeight = juce::roundToInt ((float) bounds.getHeight()
                                  * (float) systemControlRowsAtStartup
                                  / (float) padLayoutConfig().rows);
    systemLayer.setBounds (bounds.withHeight (systemHeight));

    if (bounds.getHeight() > 0)
        chordStrip.setSurfaceAspect ((float) bounds.getWidth() / (float) bounds.getHeight());
}

} // namespace conduit
