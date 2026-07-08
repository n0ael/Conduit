# ADR 005 — Subsystem-Invarianten als path-scoped Rules (.claude/rules/)

Status: akzeptiert (08.07.2026) · Ergänzt ADR 004 (Subsystem-Dossiers)

## Kontext

Die CLAUDE.md wuchs mit jedem Feature-Meilenstein (Juli 2026: 714
Zeilen / 33k Zeichen — nahe der 40k-Zeichen-Warnschwelle von Claude
Code; Anthropic-Zielwert < 200 Zeilen pro Datei, weil lange, nicht
universell relevante Instruktionen die Befolgung messbar verschlechtern).
Stufe 2 der Diät (v4.8) lagerte Detail-Spezifikationen in Dossiers
unter `docs/` aus — deren „Pflichtlektüre"-Bindung ist aber reine
Prompt-Disziplin.

Claude Code bietet mit `.claude/rules/*.md` einen mechanischen
Ersatz: Rules mit `paths:`-Frontmatter laden automatisch, sobald
Dateien gelesen werden, die auf die Globs passen. Vor dem Umbau wurde
das Verhalten auf diesem System per Testrules verifiziert (headless
`claude -p`-Läufe, 08.07.2026):

- `paths:` funktioniert wie dokumentiert (lädt lazy beim Read).
- `globs:` ist KEIN Scoping — solche Rules laden unconditional
  (unbekanntes Frontmatter). Nie verwenden.
- Rules triggern bei READS, nicht beim Anlegen neuer Dateien.
- Mid-session angelegte Rules laden erst ab der nächsten Session.

## Entscheidung

1. Die Subsystem-INVARIANTEN wandern aus der CLAUDE.md in path-scoped
   Rules (`fx-chassis`, `linkaudio`, `osc-remote`, `looper`, `grid`,
   `transport`, `ui-design`). Jede Rule endet inhaltlich an der
   Pflichtlektüre-Weisung auf ihr Dossier.
2. Die Dossiers unter `docs/` bleiben die Spezifikation (Menschen- und
   Web-lesbar, historisch verlinkt) — sie werden NICHT in die Rules
   verschoben. Die Rule ist der mechanische Wächter, das Dossier die
   Quelle. `docs/adr/` bleibt Archiv.
3. Die CLAUDE.md behält den Querschnitts-Kern: §1–3 (Rolle, Stack,
   Audio-Thread-Non-Negotiables), §4-Kern, §5 Patch-Engine-Kern,
   §6 Datenmodell-Kern, §8.0 Spannungs-Konvention, §11–13. §3 bleibt
   IMMER unconditional (Rules decken neue Dateien nicht ab; stille
   RT-Verstöße sind zu teuer).
4. Abschnittsnummern und Überschriften der CLAUDE.md sind API
   (> 100 Code-Kommentare referenzieren sie) — sie bleiben bei jeder
   Kürzung erhalten; gekürzte Abschnitte verweisen auf ihre Rule.

## Konsequenzen

- Session-Startkontext sinkt (Kern statt Vollausbau); Subsystem-Wissen
  lädt genau dann, wenn passende Dateien gelesen werden — die etablierte
  Phase-1-Inventur (Bestandsdateien lesen, Dossier lesen) garantiert
  das Laden vor der ersten Änderung.
- Beim Anlegen NEUER Subsystem-Dateien ohne vorherigen Read greift die
  Rule nicht — Restrisiko bewusst akzeptiert und durch Phase-1-Disziplin
  + unconditional §3 gedeckt.
- Rules und Dossiers müssen bei Subsystem-Änderungen gemeinsam gepflegt
  werden (Rule = Invarianten-Delta, Dossier = Spec) — gleiche Regel wie
  bisher „CLAUDE.md-Block + Dossier".
- DoD jeder Rules-Änderung: headless Marker-Nachweis
  (`claude -p` mit Read einer Subsystem-Datei + Abfrage der geladenen
  Rule-Titel), nicht nur grep.
