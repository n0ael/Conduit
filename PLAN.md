# PLAN — Grid-Page v2 (MPE-Achsen, CC-Baukasten, Skala-Anzeige, Akkord-Speicher)

> Branch: `feature/grid-page-v2` · Referenz: `design_handoff_grid_page/Grid Page.dc.html`
> Auftrag: PROMPT.md (Design-Handoff, 10.07.2026). Ein Feature = ein Commit.

## Phase 0 — Baseline
- [x] Branch `feature/grid-page-v2` angelegt
- [x] Pflichtlektüre: docs/Grid.md, Rules `grid` + `ui-design`, Design-Mock
- [ ] Baseline: Build grün, Tests grün

## Phase 1 — Recherche (read-only, parallel)
- [ ] R1: GridPage/GridKeyboardComponent/ExpressionRibbon/PadGridLayout/RingTouchModel/GridVoiceEngine
- [ ] R2: EditorDockPanel/MpeShapingView/LockToggle/GridPanelSettings/PushLookAndFeel/Header-Skala

## Phase 2 — Implementierung (sequenziell, je Feature Build + Tests + Commit)
- [ ] **Feature 3** — Skala-Anzeige: Pads nach Session-Skala einfärben (Root `#383838`,
      Skalenton `#262626`, fremd `#1a1a1a`), Root-(44×24)- + Skala-(104×24)-Kachel
      in der Top-Row, Pad-Glow über alle Sonnen → Ziel `#454545`
- [ ] **Feature 1** — Ribbons: VOL raus; links Pitch (voll, bipolar), rechts
      Pressure über Slide (je halbe Höhe, bipolar); Labels „Pitch/Pressure/Slide"
      (Jost 12, textDim); Füllung in Achsenfarbe
- [ ] **Feature 4** — Sonne (80 px, Blur ~2.5 px) / Mond (48 px, Blur ~2 px)
      weiche Kante (radialer Gradient/vorgerendertes Image), Orbit (1.5 px) scharf
- [ ] **Feature 2** — Achsen-Farben (Persistenz via GridPanelSettings) +
      „Color"-Sektion (5 Swatches, Tap=wählen, Halten ~450 ms=Picker) +
      NEU `ConduitColorPicker` (SV-Fläche 180 px, Hue-Slider, 8×5 Presets,
      live) + Unit-Test HSV↔Hex
- [ ] **Feature 5** — CC-Tab im EditorDockPanel (MPE/CC-Tabs), 2×2 Werkzeuge
      (Fader/Push/Toggle/XY-Pad), CcControlLayer: Aufziehen (gestrichelte
      Vorschau, grid-snapped), Verschieben, ✕-Löschen; Spielen im MPE-Tab;
      Pads unter Controls stumm; CC-Zuweisung später (`TODO(design)`)
- [ ] **Feature 6** — ChordMemoryStrip: 8 quadratische LCD-Screens (#0a0a0a,
      Kontur #3a3a3a, Nr. unten rechts) zwischen Raster und Pressure/Slide;
      Speichern (normalisiert, Mond-Offset über Breite), Mini-Ansicht (Sonne
      6 px, Mond 4 px, Orbit-Ellipse ×Aspekt), Abrufen + starres Verschieben
      (kein Per-Noten-Clamping), Löschen bei CC-Tab; Core-Logik UI-frei + Tests

## Phase 3 — Abschluss
- [ ] Gesamtbuild + alle Tests
- [ ] HANDOFF_RESULT.md (Zusammenfassung + manueller Smoke-Plan)
- [ ] STATUS.md-Eintrag
- [ ] Finaler Commit

## Design-Konstanten (aus Mock)
- Achsen-Defaults: Pressure `#ffa726` (ledOrange), Slide `#00bfd8` (ledCyan), Pitch `#3ddc84` (ledGreen)
- Swatch-Palette: `#ffa726 #00bfd8 #3ddc84 #ff453a #f0f0f0`
- Picker-Presets (8×5): siehe PRESETS-Array im Mock (Zeile 266 ff.)
- Neue Farbwerte für colours-Namespace: `#383838 #1a1a1a #454545 #0a0a0a #1e1e1e #4a4a4a`
- CC-Control-Optik: Fläche `#1e1e1e`, Kontur 1.5 px `#4a4a4a`, Füllung ledWhite, Label 10 px unten links
- Slot-Spalte: quadratisch, Kantenlänge = (Spaltenhöhe − Gaps)/8, Nr. 9 px `#4a4a4a`
