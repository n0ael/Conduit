# Build, Defines & Test-Workflow — Dossier
> Ausgelagert aus CLAUDE.md v4.7 §13.2/§13.3, Juli 2026. Für
> Build-/CI-Arbeiten verbindlich wie die CLAUDE.md selbst (§1.1).
> Compiler-/Dependency-Regeln (C++20, FetchContent, Werror) und die
> Test-Pflichten stehen weiterhin in CLAUDE.md 13.1/13.4.

## 13.2 Preprocessor Defines (RT Safety Guardrails)

```cmake
target_compile_definitions(${PROJECT_NAME} PUBLIC
    JUCE_MODAL_LOOPS_PERMITTED=0    # verhindert blockierende Modal-Loops im Message Thread
    JUCE_WEB_BROWSER=0              # keine unnötigen Abhängigkeiten
    JUCE_USE_CURL=0
)

# Plattform-conditional — NICHT global setzen:
if(APPLE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC JUCE_USE_CORE_AUDIO=1)
elseif(WIN32)
    # ASIO erfordert Steinberg ASIO SDK (separater Download + Lizenz, nicht in JUCE!)
    # SDK-Pfad via JUCE_ASIO_SDK_PATH, erst dann:
    target_compile_definitions(${PROJECT_NAME} PUBLIC JUCE_ASIO=1)
endif()
```

## 13.3 Quick Start & Build-Workflow

```bash
# Configure (Windows — auf diesem System ist VS 2026 installiert, kein VS 2022)
cmake -B build -G "Visual Studio 18 2026" -A x64
# Configure (Ninja, alle Plattformen)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build --config Debug

# Test: Standalone-Target aus build/ ausführen

# Sanitizer-Presets (CLAUDE.md 13.4):
cmake --preset asan && cmake --build --preset asan   # ASan (MSVC) — läuft lokal unter Windows
cmake --preset tsan && cmake --build --preset tsan   # TSan (Clang) — NUR Linux/macOS/WSL,
                                                     # unter Windows nicht verfügbar
# TSan + ASan laufen außerdem automatisch in GitHub Actions (Ubuntu) bei jedem
# Push auf master — .github/workflows/ci.yml ('tsan' + 'asan-linux' Presets)
```

- TSan/ASan-Builds laufen mit Dummy-Audio-Device — kein ASIO nötig.

## 9.1 macOS CoreAudio (ausgelagert aus CLAUDE.md v4.8 §9.1)

- `juce_add_gui_app` mit `BUNDLE_ID` und `JUCE_USE_CORE_AUDIO=1`
- `AudioDeviceManager.setAudioDeviceSetup()` — sampleRate 48000, bufferSize 128
- Tatsächliche Buffer-Size nach Setup abfragen — Hardware kann Minimum erzwingen
- `initAudio()` reagiert defensiv auf abweichende Werte, kein Crash,
  Abweichung in ValueTree-Property `audioSetupWarning` speichern

## 9.2 Linux Kiosk-Mode / LinkBox (ausgelagert aus CLAUDE.md v4.8 §9.2)

- App startet fullscreen/borderless, kein Window Manager nötig
- Cursor ausblenden wenn Touch aktiv
- PREEMPT_RT: keine RT-inkompatiblen Kernel-Calls im Audio Thread
- Touchscreen-Kalibrierung beim Start prüfen (`xinput set-prop`)
