# HANDOFF_RESULT — Grid-Page v2 (Design-Handoff 10.07.2026)

> Branch: `feature/grid-page-v2` (lokal, NICHT gepusht) · Basis: `c76c7aa` (master)
> Auftrag: `design_handoff_grid_page/PROMPT.md` + Mock `Grid Page.dc.html`
> Endstand: **Build grün (Conduit + ConduitTests, /W4 /WX) · 631 Testfälle / 27 566 Assertions grün**
> (Baseline vor Start: 610 / 27 400 — 21 neue Testfälle)

## Commits (ein Feature = ein Commit)

| Commit | Feature |
|---|---|
| `2a56a10` | PLAN.md (Checkliste) |
| `bd99a74` | **F3** Skala-Anzeige (Push-3) + Pad-Glow über alle Sonnen |
| `c4d0ddc` | **F1+F4** Ribbons Pitch/Pressure/Slide + weiche Sonnen-Kanten |
| `5540865` | **F2** Achsen-Farben + ConduitColorPicker (HSV, Presets, live) |
| `7cab81d` | **F5** CC-Baukasten (Fader/Push/Toggle/XY auf dem Pad-Raster) |
| `2d52d8e` | **F6** Akkord-Speicher (8 LCD-Slots, Abruf + starres Verschieben) |
| final | STATUS.md + PLAN.md + dieses Dokument |

## Was ist neu (Kurzfassung)

- **F3 Skala:** Pads färben nach Session-Skala (Grundton `padRoot #383838`, Skalenton
  `tile`, fremd `padUnlit #1a1a1a`); Root- (44 px) und Skala-Kachel (104 px) in der
  Top-Row zykeln per Tap und binden an `id::scaleRoot`/`id::scaleType` (ValueTree-
  Listener — Änderungen aus der TransportBar kommen an). Pad-Glow hellt jetzt JEDES
  Pad nach Distanz zur nächsten Sonne auf (Ziel `padGlow #454545`).
- **F1 Ribbons:** VOL entfernt. Links: Pitch (volle Höhe, bipolar). Rechts: Pressure
  über Slide (je halbe Höhe, bipolar). Klartext-Labels (Jost 12), Füllung in
  Achsenfarbe (`ExpressionRibbon::setFillColour`).
- **F4 Weichzeichnung:** Sonne (80 px, ~2.5 px) und Mond (48 px, ~2 px) mit weicher
  Kante via radialem Alpha-Gradienten (kein Live-Blur); Orbit bleibt scharf (1.5 px).
- **F2 Farben:** Achsenfarben (Pressure/Slide/Pitch) konfigurierbar + persistent
  (`GridPanelSettings`, Hex-Keys). „Color"-Sektion unterster Punkt der Detailspalte
  (ab Panel-Breite ≥ 340): 5 Quick-Swatches, Tap = wählen, Halten ~450 ms = Picker.
  **NEU `ConduitColorPicker`** (app-weit wiederverwendbar, CallOutBox): SV-Fläche
  180 px + Hue-Slider + 8×5-Presets, live; HSV↔RGB-Statics 8-bit-exakt getestet.
  Farbe wirkt auf Kurve/Label/Noten-Punkte/Offset-Schloss/Ribbon-Füllung.
- **F5 CC-Baukasten:** Zweiter Dock-Tab **CC** (Hinweistext + 2×2 Werkzeuge:
  Fader/Push/Toggle/XY-Pad). `CcControlLayer` über dem Raster: bei aktivem CC-Tab
  Controls aufziehen (gestrichelte, grid-snapped Vorschau), verschieben (geklemmt),
  ×-Löschen; ohne Werkzeug schluckt der CC-Modus Noten-Eingaben (wie Mock). Im
  MPE-Tab spielbar (multi-touch, Fader-Drag / Push-Momentan / Toggle-Rast / XY);
  Pads unter Controls spielen keine Noten (hitTest). UI-freies `CcControlModel`
  (Catch2-getestet). CC-Nummern-Zuweisung + MIDI-Versand: `TODO(design)`-Andockstelle
  (`onControlValueChanged`-Stub).
- **F6 Akkord-Speicher:** `ChordMemoryStrip` — 8 quadratische, rein schwarze
  LCD-Screens (`lcdScreen #0a0a0a`, Kontur `outline`, Nr. unten rechts) zwischen
  Raster und Pressure/Slide-Spalte (Kantenlänge = (Höhe−16)/8 + 2, fluchtet bei
  8 Reihen). Tap auf leeren Slot speichert die aktuelle Konstellation normalisiert
  (Mond-Offset über Flächen-BREITE → Kreis bleibt beim Rescale rund); Mini-Ansicht
  Sonne 6 px / Mond 4 px / Orbit-Ellipse (y-Radius ×Flächen-Aspekt). Tap auf
  belegten Slot latcht die Konstellation (noteOn für alle, synthetische fingerIds
  ab `0x10000`); ohne Loslassen ziehen verschiebt ALLE Noten starr (X=PitchBend,
  Y=Pressure via `PadGridLayout`-Mathematik, KEIN Per-Noten-Clamping, nur visuelles
  Clipping); Pad-Glow folgt. Löschen: Tap bei aktivem CC-Tab. Release All löst auch
  die gelatchte Konstellation. UI-freie `ChordMemory` (Catch2-getestet).

## Neue Dateien

- `Source/UI/ConduitColorPicker.h/.cpp` · `Source/UI/CcPanel.h/.cpp` ·
  `Source/UI/CcControlLayer.h/.cpp` · `Source/UI/ChordMemoryStrip.h/.cpp`
- `Source/Core/CcControlModel.h/.cpp` · `Source/Core/ChordMemory.h/.cpp`
- Tests: `Tests/UI/ConduitColorPickerTests.cpp`, `Tests/Core/CcControlModelTests.cpp`,
  `Tests/Core/ChordMemoryTests.cpp` (+ Erweiterungen GridKeyboard-/EditorDockPanel-Tests)
- Neue Farb-Tokens (`push::colours`): `padRoot #383838`, `padUnlit #1a1a1a`,
  `padGlow #454545`, `controlSurface #1e1e1e`, `controlOutline #4a4a4a`,
  `lcdScreen #0a0a0a`

## Bewusste Abweichungen / offene Punkte (`TODO(design)`)

1. **✕ → ×** (U+00D7) bei der Control-Löschzone — Jost-Glyph-sicher.
2. **CallOutBox ungestylt** — kein PushLookAndFeel-Override für den Callout-Rahmen
   des ColorPickers (Inhalt selbst ist voll gestylt); bei Bedarf später LnF-Override.
3. **Speichern auf leeren Slot geht auch im CC-Modus** (wörtliche Spez-Auslegung;
   nur Löschen ist CC-exklusiv).
4. **TransportBar-Combos lauschen nicht** auf den ValueTree — Kachel-Taps auf der
   Grid-Page aktualisieren die Header-Combos nicht (Bestandsverhalten, umgekehrt
   funktioniert es).
5. `TODO(design)`: CC-Nummern-Zuweisung pro Control, MIDI-CC-Versand,
   Akkord-Slot-Persistenz (Preset/Settings), CcPanel-Seitenpadding.
6. Orbit-Ellipsen-Mapping der Mini-Ansicht wörtlich wie Mock übernommen (inkl. der
   leichten Mock-Inkonsistenz Mond-y vs. Ellipsen-y — bei quadratischen Slots
   visuell identisch).

## Manueller Smoke-Plan (Was klicke ich wo?)

Start: `build/Conduit_artefacts/Debug/Conduit Alpha v3.exe` → Grid-Page (Ω).

1. **Skala (F3):** Top-Row neben dem MIDI-Dropdown: Root-Kachel mehrfach tippen
   (C→C#→…), Skala-Kachel tippen (Chromatic→Major→Minor→Pentatonic). Pads färben
   um: Grundtöne am hellsten, Skalentöne mittel, Rest dunkel. Gegenprobe: Skala im
   Header (TransportBar) ändern → Kacheln + Pads ziehen mit.
2. **Ribbons (F1):** Links ein einziger voller Streifen „Pitch" (grün gefüllt ab
   Mitte), rechts „Pressure" (orange) über „Slide" (cyan). Ziehen → Füllung folgt,
   Mittellinie sichtbar; Noten halten und Pitch-Ribbon ziehen → Tonhöhe aller
   Stimmen verschiebt sich.
3. **Sonne/Mond (F4):** Finger aufs Raster → Sonne mit weichem Rand + Glow auf
   umliegenden Pads; zweiten Finger nahe der Sonne aufsetzen und kreisen → Mond
   weich, Orbit-Linie scharf.
4. **Farben (F2):** Editor-Panel öffnen (Header-Button wie bisher), Splitter auf
   ≥ 340 px ziehen → Detailspalte. Unterster Punkt „Color": Swatch antippen →
   Kurve/Label/Punkte/Schloss UND Ribbon-Füllung wechseln sofort. Swatch ~½ s
   halten → ColorPicker: in der SV-Fläche und am Hue-Slider ziehen (live), Preset
   antippen. App neu starten → Farben bleiben.
5. **CC-Baukasten (F5):** Dock-Tab „CC": Werkzeug „Fader" antippen (helle Kontur),
   auf dem Raster 1×3 Zellen diagonal aufziehen (gestrichelte Vorschau) →
   loslassen erzeugt den Fader. Control ziehen → snappt auf Zellen; × oben rechts
   → weg. Push/Toggle/XY genauso platzieren. Tab „MPE": Fader ziehen (Balken),
   Push halten (leuchtet nur solange), Toggle tippen (rastet), XY-Handle ziehen;
   Pads unter den Controls bleiben stumm, daneben normal spielbar.
6. **Akkord-Speicher (F6, Touch nötig):** 2–3 Finger als Akkord halten (ggf. Mond
   dazu), mit einem weiteren Finger einen leeren Slot in der Spalte rechts neben
   dem Raster antippen → Mini-Konstellation erscheint im Slot. Alle Finger heben,
   belegten Slot antippen → Akkord erklingt, Sonnen liegen auf dem Grid; Finger
   auf dem Slot LASSEN und ziehen → alle Sonnen wandern starr mit (Pitch auf X,
   Ausdruck auf Y), auch über den Rand hinaus. Loslassen → Akkord bleibt liegen;
   „Release All" → alles aus. Slot löschen: CC-Tab aktivieren, belegten Slot tippen.
   ⚠️ Mit Maus allein lässt sich kein Akkord SPEICHERN (Maus-Up beendet die Note,
   bevor der Slot klickbar ist) — Speichern braucht Multi-Touch; Abruf/Verschieben/
   Löschen gehen auch per Maus.

## Nicht gemacht (bewusst)

- Branch nicht gepusht, kein Merge nach master — Review durch dich zuerst.
- Kein ASan-Lauf (reine UI-/Message-Thread-Arbeit ohne Graph-/Delete-Pfade);
  bei Bedarf: `cmake --preset asan`.
- Kein automatisierter Screenshot-Smoke (Auftrag verlangte den manuellen Plan).
