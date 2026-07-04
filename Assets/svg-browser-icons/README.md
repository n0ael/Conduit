# Browser-Icon-Referenzen

Design-Referenz-SVGs fuer die Browser-Panel-Icons — dieselbe Rolle wie
`Assets/Icons/` (Round-Trip-Format): die Geometrie wird von Hand als
`juce::Path` nach `Source/UI/PushIcons.cpp` portiert, es gibt KEIN
SVG-Loading zur Laufzeit (CLAUDE.md 10, User-Entscheidung 04.07.2026).

| Datei | push::Icon | Verwendung |
|---|---|---|
| projects.svg | browserProjects | Bereich PROJEKTE |
| audio.svg | browserAudio | Bereich AUDIO |
| modules-cv-control.svg | browserCvControl | MODULE-Ast CV/Control |
| modules-audiofx.svg | browserAudioFx | MODULE-Ast AudioFX |
| search.svg | search | Suchfeld unten im Panel |

Konvention wie Assets/Icons: viewBox 0 0 100 100, Gruppe "stroke" =
Mittellinien (Strichstaerke setzt der Code), Gruppe "fill" = exakte
Flaechen. Austausch erfolgt spaeter designseitig.
