# AUDIT-08 — Build-/CI-Audit (CLAUDE.md §13.1–§13.4)

> Read-only-Audit von CMake-Struktur, Compiler-Flags, Sanitizer-Presets
> und CI · Referenz CLAUDE.md v5.6 (Version-Check bestanden) +
> docs/Build.md · Durchgeführt: 20.07.2026 · Branch: master (b1f998c)
>
> Pflichtlektüre: CLAUDE.md §13, docs/Build.md,
> .github/workflows/ci.yml, CMakeLists.txt (Root),
> Source/DSP/Airwindows/CMakeLists.txt, CMakePresets.json —
> vollständig gelesen. Kein Build ausgeführt (kein Prüfpunkt
> erforderte es; alle Punkte statisch verifizierbar).

---

## 1. Zusammenfassung

Die Build-Infrastruktur entspricht in allen Kernpunkten der CLAUDE.md:
C++20 ist strikt erzwungen (inkl. Reparatur nach dem Link-Config-Reset
auf 17), alle drei Abhängigkeiten (JUCE 8.0.4, Ableton Link 4.0,
Catch2 v3.7.1) kommen via FetchContent und sind auf Release-Tags
gepinnt, die §13.2-Defines sind vollständig und die Plattform-Defines
korrekt konditional (CoreAudio nur APPLE, ASIO nur WIN32 + SDK-Pfad-
Prüfung). Die Sanitizer-Presets decken §13.4 ab: `asan` (MSVC, lokal
Windows), `tsan` + `asan-linux` (Clang/Ninja) laufen in GitHub Actions
bei jedem Push auf master — inklusive ConduitTests und
ConduitAirwindowsTests via ctest. **0 Verstöße** gegen explizite
MD-Forderungen.

Bemerkenswerte Abweichung mit dokumentierter Begründung: Werror gilt
nicht targetweit, sondern per `set_source_files_properties` nur auf
den Conduit-eigenen Quelldateien — die im selben Target mitkompilierten
JUCE-Modul-Sources dürfen den Build nicht brechen. Das erfüllt den
Geist von §13.1 (eigener Code ist Werror-sauber) und ist an allen vier
C++-Targets konsistent umgesetzt.

Lücken (Befunde ohne MD-Verstoß, Details §5): kein Windows-/MSVC-Job
in der CI (der /W4-/WX-Pfad wird nur lokal verifiziert), das GUI-App-
Target `Conduit` wird in der CI nicht gebaut (Compile-Lücke faktisch
nur `Main.cpp` + App-Link), Tags statt Commit-Hashes beim Pinning,
und `ConduitAirwindows` trägt entgegen seinem eigenen Parität-Kommentar
kein `JUCE_MODAL_LOOPS_PERMITTED=0`.

---

## 2. Target-/Flag-Matrix

Vier C++-Targets + ein Asset-Target. `CMAKE_CXX_STANDARD 20` +
`REQUIRED ON` + `EXTENSIONS OFF` gelten global (Root, Zeilen 9–11);
Links Config-Datei setzt den Standard auf 17 zurück und wird in
Zeile 72 wieder auf 20 korrigiert. ConduitAirwindows erzwingt
zusätzlich `cxx_std_20` via `target_compile_features` (PUBLIC).

| Target | Art | Standard | Warnflags | Defines |
|---|---|---|---|---|
| `Conduit` | `juce_add_gui_app` | C++20 (global) | /W4 /WX bzw. -Wall -Wextra -Werror — **nur auf `CONDUIT_CPP_SOURCES`** (per-Source, Zeilen 563–567); zusätzlich `juce_recommended_warning_flags` (targetweit, ohne Werror) | `JUCE_MODAL_LOOPS_PERMITTED=0`, `JUCE_WEB_BROWSER=0`, `JUCE_USE_CURL=0` (PUBLIC); `CONDUIT_RT_ALLOCATION_CHECKS=1` (nur Debug); APPLE: `JUCE_USE_CORE_AUDIO=1`; WIN32: `JUCE_ASIO=1` nur bei gesetztem + existierendem `JUCE_ASIO_SDK_PATH`, sonst Status-Meldung „ohne ASIO" |
| `ConduitTests` | `juce_add_console_app` | C++20 (global) | dito — auf `CONDUIT_TEST_CPP_SOURCES` (Zeilen 998–1002); die mitkompilierten `Source/`-Dateien tragen die Properties aus der `CONDUIT_CPP_SOURCES`-Liste (gleiche Directory-Scope); `juce_recommended_warning_flags` | `JUCE_MODAL_LOOPS_PERMITTED=0`, `JUCE_WEB_BROWSER=0`, `JUCE_USE_CURL=0` (PRIVATE); `CONDUIT_RT_ALLOCATION_CHECKS=1` (Debug); **keine** Plattform-Audio-Defines (konsistent: Tests öffnen kein Device) |
| `ConduitAirwindows` | `add_library STATIC` | C++20 (`cxx_std_20` PUBLIC) | dito — auf `CONDUIT_AIRWINDOWS_CPP_SOURCES` (Zeilen 167–171); **kein** `juce_recommended_warning_flags` | `JUCE_USE_CURL=0`, `JUCE_WEB_BROWSER=0` (PUBLIC); **fehlt: `JUCE_MODAL_LOOPS_PERMITTED=0`** (siehe §5.4) |
| `ConduitAirwindowsTests` | `add_executable` (nur wenn Catch2-Target existiert) | C++20 (transitiv via ConduitAirwindows PUBLIC) | dito — auf `CONDUIT_AIRWINDOWS_TEST_SOURCES` (Zeilen 253–257) | erbt die PUBLIC-Defines von ConduitAirwindows |
| `ConduitAssets` | `juce_add_binary_data` | — (generierter Code) | keine Werror-Properties (generiert, unkritisch) | — |

Sanitizer-Optionen (Root, Zeilen 18–44): `CONDUIT_ENABLE_TSAN` /
`CONDUIT_ENABLE_ASAN`, gegenseitiger Ausschluss via `FATAL_ERROR`,
TSan verweigert Nicht-Clang; MSVC-ASan entfernt `/RTC1` (inkompatibel)
und linkt `/INCREMENTAL:NO`. Flags gelten global (`add_compile_options`)
— erfassen also auch JUCE-Module, wie §13.4 es für TSan/ASan braucht.

Werror-Konstruktion: §13.1 fordert „Warnungen als Fehler" ohne
Scope-Angabe. Die per-Source-Lösung ist in allen drei CMakeLists
gleichlautend kommentiert begründet (JUCE-Sources im selben Target)
und deckt 100 % des eigenen Codes; kein Verstoß, bewusste Bauform.

## 3. Preset-Matrix + CI-Abdeckung

### 3.1 CMakePresets.json (Version 6)

| Preset | Generator/Compiler | Plattform-Condition | binaryDir | Cache-Variablen | Build-/Test-Preset |
|---|---|---|---|---|---|
| `windows-vs` | Visual Studio 18 2026, x64 | nur Windows | `build/` | — | Build: Debug; **kein Test-Preset** |
| `asan` | erbt windows-vs (MSVC) | nur Windows | `build-asan/` | `CONDUIT_ENABLE_ASAN=ON` | Build: Debug; Test: Debug, outputOnFailure |
| `tsan` | Ninja, clang/clang++ | NICHT Windows | `build-tsan/` | `CMAKE_BUILD_TYPE=Debug`, `CONDUIT_ENABLE_TSAN=ON` | Build + Test (single-config) |
| `asan-linux` | Ninja, clang/clang++ | NICHT Windows | `build-asan-linux/` | `CMAKE_BUILD_TYPE=Debug`, `CONDUIT_ENABLE_ASAN=ON` | Build + Test (single-config) |

Entspricht §13.3: `asan` = MSVC lokal unter Windows, `tsan` = Clang nur
Linux/macOS/WSL (Windows-Condition verhindert Fehlbedienung mechanisch).
`asan-linux` ist das dokumentierte CI-Pendant (docs/Build.md 13.3).

### 3.2 ci.yml

| Job | Trigger | Runner | Presets/Targets | Optionen |
|---|---|---|---|---|
| `sanitizers` (Matrix, fail-fast off, 40 min Timeout) | Push auf `master` + `claude/**`; PR gegen `master` | ubuntu-latest | Configure + Build `--target ConduitTests --target ConduitAirwindowsTests`, dann `ctest --preset` — Matrix: `tsan`, `asan-linux` | `TSAN_OPTIONS=halt_on_error=1` (jedes Race bricht rot ab); `ASAN_OPTIONS=detect_leaks=0` (LSan-Ausschluss begründet: JUCE-Singletons, Leak-Erkennung via JUCE-LeakDetector) |
| `remote-script` | dito | ubuntu-latest | pytest `Tools/Live/ConduitRemote/tests` (Python 3.12) | 10 min Timeout |

- §13.3-Forderung „TSan + ASan bei jedem Push auf master": **erfüllt**
  (zusätzlich `claude/**` und PRs — überobligatorisch).
- ConduitTests läuft in CI: **ja**, via `ctest --preset` (beide
  registrierten Tests: `ConduitTests`, `ConduitAirwindowsTests`).
- FetchContent-Sources werden gecacht (`build-*/_deps/*-src`, Key =
  Hash der Root-CMakeLists — Tag-Wechsel invalidiert den Cache korrekt).
- Windows-Build in CI: **nicht vorhanden** (nur Ubuntu; Befund §5.1).
- GUI-App-Target `Conduit` wird in CI **nicht gebaut** (Befund §5.2).

## 4. Pinning-Befunde

| Abhängigkeit | Quelle | Ref | Bewertung |
|---|---|---|---|
| JUCE | github.com/juce-framework/JUCE | `GIT_TAG 8.0.4`, `GIT_SHALLOW` | Release-Tag, erfüllt §13.1 (min. 8.0.0, FetchContent, kein Submodule/System-Install) |
| Ableton Link | github.com/Ableton/link | `GIT_TAG Link-4.0`, `GIT_SHALLOW`, `SOURCE_SUBDIR cmake_include` (bewusst: keine Link-Beispiel-Targets) | Release-Tag, header-only via Config-Datei — §13.1-konform |
| Catch2 | github.com/catchorg/Catch2 | `GIT_TAG v3.7.1`, `GIT_SHALLOW` | Release-Tag, v3 wie §13.4 gefordert; nur bei `CONDUIT_BUILD_TESTS=ON` (Default ON) |

**Kein Branch-Pinning** — kein Reproduzierbarkeits-Verstoß. Restrisiko
(Befund, keine MD-Forderung): Tags sind bewegliche Refs; wer maximale
Reproduzierbarkeit will, pinnt Commit-Hashes (`GIT_SHALLOW` müsste dann
weichen). GitHub-Actions sind auf Major-Versionen gepinnt
(`checkout@v6`, `cache@v5`, `setup-python@v6`) — üblich, nicht
SHA-gepinnt.

**Dummy-Audio-Device-Regel (§13.4) — verifiziert:** Kein einziger
Treffer für `AudioDeviceManager`/Device-Initialisierung im gesamten
`Tests/`-Baum — die Testläufe öffnen konstruktionsbedingt gar kein
Audio-Device (headless; ci.yml Kopfkommentar „kein ASIO/JACK-Setup
nötig", Root-CMakeLists Zeilen 15–16 und 617 dokumentieren dasselbe).
Die apt-Pakete (libasound2, libjack) sind reine Compile-Abhängigkeiten
der JUCE-Module, kein Laufzeit-Audio. Regel erfüllt.

**build-Verzeichnis-Hygiene — sauber:** `.gitignore` deckt `build/`
und `build-*/` — damit alle vier Preset-binaryDirs (`build`,
`build-asan`, `build-tsan`, `build-asan-linux`) plus zukünftige
`build-*`-Varianten; dazu `out/`, `cmake-build-*/`, `CMakeCache.txt`,
`CMakeFiles/`, `CMakeUserPresets.json`. Keine Build-Artefakte im
git-status.

## 5. Lücken & Risiken

Alle Punkte sind Befunde ohne expliziten MD-Verstoß (die CLAUDE.md
fordert an den betreffenden Stellen nichts anderes):

1. **Kein Windows-/MSVC-Job in der CI.** Der `/W4 /WX`-Pfad und der
   MSVC-ASan-Zweig (`/fsanitize=address`, /RTC1-Entfernung) werden
   ausschließlich lokal verifiziert. Ein MSVC-only-Warning (häufig:
   C4244/C4267-Konvertierungen) kann lokal grün gehen, wenn nur unter
   Clang gebaut wurde — und umgekehrt bricht master erst beim nächsten
   lokalen Windows-Build. Für Remote-Sessions (`claude/**`-Trigger
   existiert genau dafür) gibt es keine Windows-Verifikation.
2. **GUI-App-Target `Conduit` wird in CI nicht gebaut** (bewusst:
   „die GUI-App braucht die CI nicht"). Faktische Compile-Lücke ist
   klein — ConduitTests kompiliert alle `Source/`-Dateien außer
   `Source/Core/Main.cpp` mit — aber App-Link, `juce_add_gui_app`-Glue
   und `ConduitAssets`-Einbindung der App bleiben unverifiziert.
3. **Kein Release-/Optimized-Build in CI.** Beide Sanitizer-Läufe sind
   Debug. Optimizer-abhängige Warnungen (z. B. `-Wmaybe-uninitialized`-
   Klassen) und Release-only-Codepfade bleiben ungeprüft; das
   Debug-only-Define `CONDUIT_RT_ALLOCATION_CHECKS` ist im Release nie
   kompiliert worden, bis jemand lokal Release baut.
4. **`ConduitAirwindows`: Define-Parität unvollständig.** Der
   Kommentar (Zeile 158) verlangt „dieselben Defines wie
   Conduit/ConduitTests", gesetzt sind aber nur `JUCE_USE_CURL=0` +
   `JUCE_WEB_BROWSER=0` — `JUCE_MODAL_LOOPS_PERMITTED=0` fehlt.
   Funktional aktuell folgenlos (Target linkt nur `juce_audio_basics`,
   keine GUI-Module), aber der Kommentar verspricht mehr als der Code
   hält; bei späterem Modul-Zuwachs (juce_events/juce_gui_basics) wäre
   der Guardrail stillschweigend offen. Ebenso fehlt — anders als bei
   Conduit/ConduitTests — `juce_recommended_warning_flags`.
5. **Tag- statt Hash-Pinning** (§4): tolerierbar, aber ein upstream
   verschobener Tag würde durch den CI-Cache (Key hasht nur die
   CMakeLists) sogar zeitweise maskiert.
6. **Kein Test-Preset für `windows-vs`:** lokal läuft ctest via Preset
   nur über `asan`; der normale Windows-Debug-Build hat kein
   `testPresets`-Pendant (Tests dort nur manuell/`ctest -C Debug`).
7. **Doppelkompilierung des Quellbaums:** ConduitTests kompiliert
   ~alle Conduit-Sources erneut statt eine gemeinsame Static-Lib zu
   nutzen. Kein Korrektheitsproblem, aber ~2× Compile-Zeit pro
   CI-Lauf und lokalem Vollbuild — reine Effizienz-Beobachtung.

## 6. Empfohlene Folgeaufträge

1. **CI-Windows-Job ergänzen** (Priorität hoch): `windows-latest`,
   Preset `windows-vs`, Build `Conduit` + `ConduitTests` + ctest —
   schließt Lücken 1, 2 und 6 in einem Schritt und macht die
   `claude/**`-Remote-Verifikation plattformvollständig. (VS-Version
   auf dem Runner prüfen; lokal ist VS 2026 gesetzt.)
2. **ConduitAirwindows-CMakeLists angleichen** (klein, read-only-safe
   als eigener Mini-Auftrag): `JUCE_MODAL_LOOPS_PERMITTED=0` ergänzen
   oder den Parität-Kommentar präzisieren; `juce_recommended_warning_flags`
   nachziehen.
3. **Release-Smoke in CI erwägen**: ein einzelner
   `CMAKE_BUILD_TYPE=Release`-Ninja-Build (ohne Sanitizer, nur
   ConduitTests) fängt Optimizer-Warnklassen und Release-only-Brüche.
4. **Pinning-Härtung erwägen** (niedrig): Tags → Commit-Hashes für
   JUCE/Link/Catch2, falls Reproduzierbarkeit über Tag-Vertrauen
   gestellt werden soll; Actions optional SHA-pinnen.
5. **Doppelkompilierung** (niedrig, nur bei Leidensdruck):
   gemeinsame `ConduitCore`-Static-Lib für App + Tests — vorher
   prüfen, ob JUCE-Modul-Kompilierung pro Target dem entgegensteht.
