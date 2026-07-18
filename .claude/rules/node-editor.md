---
paths:
  - "Source/UI/NodeCanvas*"
  - "Source/UI/NodeComponent*"
  - "Source/UI/PageOverviewComponent*"
  - "Source/Core/PageManager*"
  - "Source/Core/CanvasGestureRecognizer*"
  - "Source/Core/CanvasViewport*"
  - "Source/Modules/AudioEndpointModule*"
---

# Rule: node-editor — Multipage-Canvas (ADR 008/009)

**Pflichtlektüre vor jeder Arbeit am Node-Patch-Editor:
docs/NodeEditor.md** (Architektur, Eingabe-Tabelle, Lektionen).
Entscheidungshistorie: docs/adr/008 + 009 (append-only).

- Seiten sind eine reine VIEW-Schicht über EINEM AudioProcessorGraph —
  der Audio-Thread kennt keine Seiten; PageManager ist
  Message-Thread-only und arbeitet NUR auf dem ValueTree.
- Seitenwechsel eines Nodes ist ein `setProperty` (pageUuid), NIE
  removeChild/addChild — sonst feuern Delete-Pfad und
  OSC-Deregistrierung fälschlich.
- Leerraum-Regel: Canvas-Gesten zählen NUR Touches, die auf dem
  Canvas-Hintergrund BEGINNEN — NodeCanvasContent bleibt passiv
  (`setInterceptsMouseClicks(false, true)`); Modul-Touches gehören dem
  Modul. Der CanvasGestureRecognizer bleibt eine pure, headless
  testbare Zustandsmaschine (kein Component-Zugriff).
- Öffentliche NodeCanvas-API nimmt CANVAS-Koordinaten; intern
  Content-Koordinaten (== Tree-x/y, ≥ 0); Hit-Toleranzen
  screen-konstant (÷ Zoom); Transform-Translation gerundet ANWENDEN,
  ViewState double-genau halten (Anti-Zitter).
- View-State (viewOffset/viewZoom pro Seite, activePage am Root) wird
  OHNE UndoManager geschrieben; Seitenwechsel laufen über den
  activePage-Property-Listener (EIN Pfad für Swipe/Tastatur/Übersicht).
- Übersichts-Miniaturen: juce::Image-Cache, Invalidierung via
  ValueTree-Listener, Neuaufbau VBlank-gesteuert max. EINE pro Frame,
  NIE in paint() (paint blittet nur).
- Interaktions-Sperre (Zoom < `UiSettings::interactionMinZoom`): der
  Patch ist NUR Navigation — auch Kabel-Trennen und Canvas-Doppel-Tap
  bleiben gesperrt.
- I/O-Endpunkte sind reguläre Module (ADR 009): kein Auto-Repair beim
  Laden (Patch ohne Output = bewusste Stille); die impliziten
  Anker-Kabel zum AudioGraphIOProcessor sind KEIN Patch-Zustand und
  stehen nie im Tree.
- Gesten-/Zoom-Tuning ausschließlich über die UiSettings-Werte
  (Oberfläche-Tab) — keine neuen Magic Numbers in den Gesten-Pfaden.
