# ADR 014 — Soll-Layering & Kompositionsschicht

Datum: 20.07.2026 · Status: beschlossen (User-Entscheidung im
Rahmen von AUDIT-05)

Kontext: §13.5 nannte Verzeichnisse ohne Include-Richtungen;
AUDIT-05 erhob 1241 interne Kanten, zyklenfrei, mit Core als
faktischer App-Kompositionsschicht.

Entscheidung: Das Layering aus CLAUDE.md §13.5 (v5.7) ist
normativ. Core ist Kompositionsschicht; die Whitelist
(EngineProcessor, EngineEditor, GraphManager, Main) ist
abschließend. DSP bleibt ohne projektinterne Includes
(Bibliotheks-Isolation). UI→Modules ist verboten (§5); Bestand
(30 Kanten + transitive GraphManager.h-Kette, AUDIT-05 §4) ist
Altlast mit Abbaupfad (Forward-Decl GraphManager.h,
ChassisSchema→Core, Tap-Muster-ADR vor Mixer-Page).

Konsequenz: Künftige Audits werten Kanten gegen dieses Soll;
neue Verzeichnisse erfordern einen ADR-Nachtrag.
