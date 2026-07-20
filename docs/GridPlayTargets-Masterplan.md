# Grid-PlayTargets — Masterplan (Multi-Ziel-MPE)
> Planungsstand: 20.07.2026 · Quelle: Planungs-Chat (claude.ai) +
> Fortsetzung in Claude Code · Ergänzt durch die M0-Inventur
> ([GridPlayTargets-M0.md](GridPlayTargets-M0.md)).
>
> Dieses Dokument hält den GENEHMIGTEN Plan fest (Entscheidungen E1–E7
> sind final, nicht neu diskutieren). Umgesetzte Meilensteine wandern
> nach Abschluss in docs/Grid.md bzw. das M0-Dossier; hier bleibt der
> Fahrplan.

---

## 1. Ziel & Kern-Einsicht

Die Grid soll MEHRERE MIDI-Ziele spielen können (Hardware-Synths direkt,
Ableton-Tracks über benannte Loopback-Ports) statt nur eines
Grid-Output-Ports. Drei Bedienmodi:

| Modus | Zielwahl |
|---|---|
| Mode 1 | Ziel wechselt per Tap |
| Mode 2 | Ziel wechselt per Scroll (zentrierter Track im Strip) |
| Mode 3 | Ziel pro Note durch gleichzeitige Strip-Berührung; Fallback = zentriertes Ziel |

**Kern-Einsicht:** Die drei Modi unterscheiden sich fast nur in der UI.
Der Engine-Unterbau ist identisch, sobald eine Invariante gilt:

> **Das Routing-Ziel wird bei `noteOn` gebunden und bleibt für die
> Lebensdauer der Note unveränderlich.** Alle Expression-Daten
> (PitchBend, Pressure, Slide) folgen dem bei noteOn gebundenen Ziel —
> auch nach Latch, Scroll oder Zielwechsel.

Damit sind Hold/Latch über Zielwechsel hinweg trivial korrekt: alte
Noten senden weiter an ihr altes Ziel, neue Noten an das neue.

## 2. Architektur-Bausteine

1. **PlayTarget** = benannter MIDI-Sink mit eigenem MPE-Channel-Allocator.
   Zwei Typen: **Direkt-HW** (existierender MidiRig-Port) und
   **Loopback → Ableton-Track** (virtueller Port `C-<Name>`, der Track
   hört per MIDI-From darauf).
   *Technisches Argument für Loopback-pro-Gerät:* MPE braucht pro Ziel
   eine eigene Zone (Ch. 2–16) — Kanäle allein können Ziele nicht
   unterscheiden, jedes Ziel braucht seinen eigenen Port.
2. **Kein Ableton-MIDI-Routing mehr:** Conduit wird die zentrale
   Patchbay, Ableton konsumiert nur noch benannte Ports. Für HW-Geräte
   ist kein Loopback nötig (MidiRig sendet direkt).
3. **Track↔Port-Mapping** über das REALE Ableton-Routing (E2), nicht
   über Namenskonventionen.
4. **Threading:** Der gesamte Pfad ist Message Thread (M0-Befund) — der
   ursprünglich geplante „lock-freie Ziel-Lookup" entfällt, ein
   einfacher Zielindex pro Voice-Slot genügt.

## 3. Entscheidungen E1–E7 (FINAL)

| # | Thema | Entscheidung |
|---|---|---|
| **E1** | Route-Binding | **noteOn-immutabel** (siehe Invariante §1) |
| **E2** | Mapping | Ableton-Routing **auslesen** (`track.input_routing_type`) statt Namenskonvention; Trackname/-farbe sind reine Anzeige-Daten (Trackfarben fallen gratis ab). Umbenennen wäre invasiv → **lesen statt schreiben**. Mapping-Tabelle bleibt Fallback für uneindeutiges Routing. |
| **E3** | Auto-Mode pro Ziel | Feld `preferredGridMode` im PlayTarget-Profil — Zielwechsel stellt den Grid-Modus automatisch um (einmal konfigurieren, nie wieder Settings) |
| **E4** | Mode-3-Fallback | Note ohne Strip-Touch → **zentriertes Ziel** (nicht stumm) |
| **E5** | Latch-Semantik | Latched Notes bleiben beim alten Ziel; Zielwechsel betrifft nur neue Noten (Folge aus E1) |
| **E6** | Virtuelle Ports | **Dual:** Windows MIDI Services als Default (Conduit erzeugt `C-<Track>`-Ports automatisch), **loopMIDI als Legacy-Fallback** per Einstellung. Der WMS-Teil braucht VOR dem Auftrag einen Recherche-Spike (JUCE-8-Support für app-erzeugte virtuelle Ports, Mindest-Windows-Build, API-Reifegrad). Der loopMIDI-Pfad ist risikofrei → **kein Meilenstein blockiert auf E6**. |
| **E7** | MIDI im Node-Graph | **Option B — MIDI wird dritter Kabeltyp**, aber **GATED**: erst nach der Mono/Stereo-Kabel-Entscheidung (ADR 011). Bis dahin B-ready bauen (§4). MIDI-Kabel werden **RECHTWINKLIG** gerendert (Manhattan-Routing, Patchbay-/Schaltplan-Optik — hebt sich klar von Audio/CV-Kurven ab). |

**Vorbehalt zu E2:** Die Routing-Properties müssen in der eingesetzten
AbletonOSC-Version vorhanden sein → AbletonOSC-Gap-Check vor der
Fork-Frage. Der M0-Befund verschärft das: der bestehende OSC-Code ist
**Command-only**, es gibt bis heute keinen einzigen Lesepfad.

## 4. B-Readiness-Prinzip (bis das α/β-Gate fällt)

Jede Zeile aus M1–M6 wird gegen die Frage geprüft: *„Funktioniert das
unverändert weiter, wenn MIDI ein Kabeltyp wird?"* Vier Invarianten:

1. **Transport-Naht (die wichtigste):** Zwischen Notenquelle und
   Port-Ausgabe liegt eine Interface-Grenze. Die Grid darf NIEMALS
   direkt gegen den `MidiPortHub` programmieren.
   → **M0-Befund: diese Naht existiert bereits** als `IVoiceSink` +
   `IMidiOutputTarget` + Rollen-Fassaden (ADR 003). In Phase B
   implementiert ein Graph-Node dasselbe Interface; die Quellen merken
   nichts. Das ist die eine Stelle, an der B billig oder teuer wird.
2. **Registry graphneutral:** Geräteprofil = reine Daten (Port-Wahl
   Direct/Loopback, `preferredGridMode`, Name, Farbe, Voice-Zahl,
   CSV-Profil-Referenz). Kein UI-Zustand, keine Annahme darüber, wer das
   Ziel anspricht. Ein Gerätemodul in Phase B ist dann ein dünner Node,
   der ein Registry-Profil referenziert.
3. **Event-Format graphtauglich:** intern sample-positionierte Events
   (natives `MidiBuffer`-Modell), nicht „sofort raus auf den Port". Der
   spätere Umzug in `processBlock` ist dann eine Verschiebung, keine
   Umkodierung.
4. **Kein MIDI-Zustand in der UI:** Modus-Umschaltung, Zielauswahl,
   Mapping — alles ValueTree (CLAUDE.md §6), damit Phase B dieselben
   Subtrees aus Nodes heraus liest.

**Vorbereitung fürs Orthogonal-Routing:** Der Kabel-Renderer bekommt
beim ohnehin anstehenden α/β-Umbau die Trennung
**Topologie → Pfad-Strategie → Zeichnen**. Bezier und Orthogonal sind
dann zwei Strategien; der MIDI-Kabeltyp ist später nur „neue Strategie +
neue Farbe/Port-Typ". Diese Anforderung gehört in den Stereo-Kabel-
Auftrag (kostet dort fast nichts) — Orthogonal-Routing ist reine
Renderer-Sache und von der Stereo-Frage unabhängig.

## 5. Sequenzierung

```
Grid-Erweiterung (M0 … M6)          ← B-ready geschnitten
        │
Stereo-Kabel α/β  ──────────────────← GATE; Auftrag bekommt die
  (inkl. Pfad-Strategie-Trennung)      B-Anforderungen mitgegeben
        │
ADR: MIDI als dritter Kabeltyp      ← erst jetzt entscheidbar
        │
Phase B: Geräte-Nodes + MIDI-Kabel  ← die Sink-Naht wandert in den Graph
```

## 6. Meilensteine

| MS | Inhalt | B-Vorbereitung | Status |
|---|---|---|---|
| **M0** | Read-only-Inventur: Grid-Note-Pipeline, Touch↔Note-Rebinding, MidiRig-Sendepfad | prüft, wo die Sink-Naht sitzt | **erledigt** 20.07.2026 → [GridPlayTargets-M0.md](GridPlayTargets-M0.md) |
| **M0-Fix** | Slide-Reacquisition-Bug + Regressionstests | — | **erledigt** 20.07.2026 (Block M2, master `346e716`) |
| **M1** | PlayTarget + per-Target-MPE-Allocator; Solo-Modus funktional unverändert | Allocator sitzt hinter der Sink-Naht, nicht in der Grid | offen |
| **M2** | Target-Registry: HW-Ports, OSC-Routing-Lookup, Mapping-Fallback, `preferredGridMode`, Settings-UI | Profil graphneutral, Events sample-positioniert | offen |
| **M3** | Mode 2: Strip → Scrollbalken, Latch über Zielwechsel, Trackfarben in der Notendarstellung | — | offen |
| **M4** | Mode 3: Touch-Route-Arbitrierung (Strip-Touch zählt nur solange gehalten), Fallback zentriertes Ziel | — | offen |
| **M5** | Loopback-Setup-Doku + loopMIDI-Legacy-Pfad | — | offen |
| **M6** | Modul-Browser-Kategorie **MIDI**: Gerätemodule als Registry-Views, Umschalter Direct / via Ableton | bewusst noch KEIN Node — Platzhalter, der in Phase B zum Node wird | offen |
| **Gate** | Stereo-Kabel α/β + Pfad-Strategie-Trennung im Renderer | — | offen (User) |
| **B-ADR** | MIDI-Kabeltyp: Semantik, Orthogonal-Routing, Port-Farben | — | offen |
| **B1+** | Geräte-Nodes; die Sink-Naht wird Graph-Node | — | offen |
| **Spike** | WMS-Recherche (E6), läuft parallel, blockiert nichts | — | offen |

## 7. Endzustand (User-Vision, 20.07.2026)

Conduit als **MIDI-Patchbay**: Gerätemodule im Node-Graph, bei denen die
Position des Klangerzeugers (in Ableton gehostet vs. direkt an der
Hardware) ein reines Port-Detail ist — ein Modul „Mopho" mit Umschalter
*Direct / via Ableton* wählt nur, worauf das PlayTarget zeigt. Das öffnet
mehr als die Grid: LFOs, generative Module und künftige Sequencer können
MIDI an Gerätemodule patchen. Das ist die Idee „MIDI-Loops statt
Ableton-Routing" zu Ende gedacht.

## 8. Randnotiz zur Risikoabsicherung

Der Umbau geht mitten durch die Touch→MIDI-Pipeline. Die Audit-Serie
AUDIT-01 (RT-Safety), AUDIT-02 (Threading) und AUDIT-04 (Lifetime) wurde
am 20.07.2026 auf `b1f998c` durchgeführt — 0 harte Verstöße
(docs/audit/). Die Befunde decken genau die Dateien ab, die M1–M6
anfassen.
