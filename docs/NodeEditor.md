# Node-Patch-Editor (Multipage-Canvas) — Subsystem-Dossier
> Entstanden aus ADR 008/009 (M0–M4, 18.07.2026). Für Arbeiten an
> diesem Subsystem verbindlich wie die CLAUDE.md selbst (§1.1).
> Pflichtlektüre vor Arbeiten an NodeCanvas, NodeCanvasContent,
> CanvasGestureRecognizer, PageManager, PageOverviewComponent.
> Invarianten: Rule `node-editor`. Entscheidungshistorie: ADR 008/009
> (append-only, inkl. aller Umsetzungsnotizen).

## 1. Architektur (M3a, „Ansatz A")

- `NodeCanvas` (fest, untransformiert): Hintergrund, Drop-Rahmen,
  Overlays (Swipe-Badge, Birdeye-Target, Seiten-Übersicht), Empfänger
  aller Leerraum-Eingaben (Gesten, Wheel, Magnify, Kabel-Klick,
  Doppel-Tap-Attenuator-Provisorium).
- `NodeCanvasContent` (einziges direktes Kind): Transform-Träger
  (AffineTransform = Zoom + Pan), hält ALLE NodeComponents, zeichnet
  die Kabel über `onPaintCables` (Canvas-Logik, Content-Koordinaten
  == Node-Tree-Koordinaten). Bewusst passiv:
  `setInterceptsMouseClicks(false, true)` — Hintergrund-Events fallen
  zum Canvas durch, die Leerraum-Regel entsteht per Konstruktion.
  Feste Riesen-Bounds (200k²; JUCE-Event-Routing braucht Bounds,
  Koordinaten ≥ 0 — Drag klemmt auf ≥ 0).
- Öffentliche Canvas-API nimmt CANVAS-Koordinaten und konvertiert
  intern (`toContentPosition`); Hit-Toleranzen screen-konstant
  (÷ Zoom). Interaktions-Sperre unter `interactionMinZoom`:
  `content.setChildInteraction(false)` + Kabel-Klick/Doppel-Tap-Guard
  — der Patch ist dann NUR Navigation.
- Viewport-Mathe pur in `Core/CanvasViewport.h` (zoomAbout hält den
  Anker fix; Clamps 0.1–2.0); Translation wird beim ANWENDEN auf
  ganze Screen-Pixel gerundet (Anti-Zitter), der ViewState bleibt
  double-genau.

## 2. Gesten-Recognizer (Core/CanvasGestureRecognizer)

Pure Zustandsmaschine (headless getestet), gefüttert NUR mit Touches,
die auf dem Canvas-Hintergrund BEGINNEN (JUCE-Hit-Testing erledigt
die Leerraum-Regel). Fingerzahl = Ebene; jeder Wechsel setzt die
Referenz neu (kein Springen). Verarbeitung pro Move: EMA-Glättung
(Zentroid+Spread, `gestureSmoothing`) → Ebene 2: weiche Zoom-Dead-Zone
auf der AKKUMULIERTEN Spread-Änderung (`pinchDeadZone`, Soft-Bereich
3×) → progressive Antwort (`zoomStrength`·|x|^`zoomCurve`) → Callback;
Ebenen 3–5: `onLevelDrag`-Zentroid-Deltas.

## 3. Eingabe-Tabelle (seitenspezifisch, §10.0 — gilt NUR hier)

| Eingabe | Touch | Trackpad | Maus/Tastatur |
|---|---|---|---|
| Zoom | 2-Finger-Pinch | Magnify / Strg(Cmd)+Scroll | Strg(Cmd)+Rad |
| Pan | 2-Finger-Drag | 2-Finger-Scroll | mittlere Taste ziehen |
| Birdeye (aktive Seite) | 3-Finger-HOLD (transient, Mittel-Target) | — (OS konsumiert) | Strg+Alt+B (Toggle) |
| Seitenwechsel | 4-Finger-Swipe (Peek/Commit 15 %, ins Leere = neue Seite) | Alt+Scroll | Strg+Alt+Pfeile (Grid; Leere = anlegen) |
| Seiten-Übersicht | 5 Finger | — | Strg+Alt+O; Esc/Hintergrund schließt |
| Modul löschen | Doppel-Tap armiert (3 s, roter Rahmen; Output-mit-Kabeln ⚠) + zweiter Doppel-Tap; ×-Button parallel | dito | dito |
| Rename/Farbe | Long-Press Kopfzeile ODER Farbpunkt → NodeAttributePanel | dito | dito |
| Kabel trennen / Modul anlegen | 1-Finger-Tap auf Kabel / Doppel-Tap Leerfläche (Attenuator-Provisorium) | Klick | Klick |

Alle Tuning-Werte im Oberfläche-Tab (Dev-Tuning, nach Erprobung
fixieren): Interakt.-Zoom 50 %, Pinch-Schwelle 6 %, Zoom-Stärke 60 %,
Zoom-Kurve 1.6, Glättung 50 %, Arbeits-Zoom 100 %, Birdeye-Zoom 22 %.

## 4. Seiten (M1/M3b/M4) & I/O (ADR 009)

- Datenmodell: `Pages[]` + `pageUuid`-Node-Property (M1);
  `rootStateVersion` 2 = Pages, 3 = I/O-Wandlung (GETRENNTE Bumps).
  Aktive Seite = Root-Property `activePage` (View-State, kein Undo,
  validiert/repariert); Seitenwechsel läuft über EINEN
  Property-Listener-Pfad (rebuild + Viewport-Restore).
- Canvas filtert auf die aktive Seite (Nodes ohne pageUuid bleiben
  sichtbar — Test-Rigs); Cross-Page-Kabel sind unsichtbar, wirken
  aber im Graph (bis M5/Portal-Badges — zurückgestellt).
- Übersicht: `PageOverviewComponent` — schematische Proxy-Miniaturen
  (Tree-Rechtecke+Linien), juce::Image-Cache, Invalidierung via
  ValueTree-Listener, VBlank max. 1 Miniatur/Frame, paint blittet
  nur. Regel-a-Lösch-UI (× auf leeren, nicht-aktiven Kacheln).
- I/O: `AudioEndpointModule` (Pass-Through, factoryIds unverändert),
  implizite Anker-Kabel zum AudioGraphIOProcessor (kein
  Patch-Zustand); kein Auto-Repair ab V3 — Patch ohne Output =
  bewusste Stille; Mehrfach-Outs summieren nativ.

## 5. Lektionen (18.07.2026)

- Sub-Pixel-Transform-Offsets lassen Kacheln beim Pannen zittern →
  Translation gerundet anwenden, State double halten.
- Touch-Sensor-Rauschen wackelt den Zentroid → EMA-Glättung im
  Recognizer, einstellbar (geräteabhängig).
- Inkrementelle Pinch-Dämpfung ist falsch (dauerhaft träge) — Kurven
  IMMER auf die akkumulierte Gesten-Auslenkung rechnen.
- JUCE-Event-Routing prüft Bounds VOR hitTest → „unendlicher" Canvas
  braucht große feste Bounds, nicht hitTest-Overrides.
- `juce::Timer::callAfterDelay` + SafePointer für Zusatz-Timeouts,
  wenn der Member-Timer schon vergeben ist (Delete-Armierung vs.
  Long-Press: mouseUp stoppt den Member-Timer).
- Birdeye braucht keine Miniaturen (aktive Seite rendert live);
  Miniaturen nur für fremde Seiten (keine Components) — schematisch
  aus dem Tree.
