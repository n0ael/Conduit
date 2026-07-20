# ---------------------------------------------------------------------------
# ConduitAirwindows — portierte Airwindows-DSP (Chris Johnson / Airwindows,
# MIT) als eigenständige Static Library.
#
# Einbindung ins Root-Projekt erfolgt manuell via
#     add_subdirectory(Source/DSP/Airwindows)
# NACH FetchContent(JUCE) — dieses Target erwartet ein bereits konfiguriertes
# juce::juce_audio_basics (ScopedNoDenormals, Leak-Detektor) im umgebenden
# Projekt. Die Root-CMakeLists bleibt unangetastet (Session-Scope).
#
# Das Test-Target ConduitAirwindowsTests entsteht nur, wenn das umgebende
# Projekt Catch2 bereitstellt (Muster: CONDUIT_BUILD_TESTS im Root).
# ---------------------------------------------------------------------------

set(CONDUIT_AIRWINDOWS_CPP_SOURCES
    AirwindowsPlugin.cpp
    AirwindowsRegistry.cpp
    Plugins/Density.cpp
    Plugins/Slew.cpp
    Plugins/Spiral.cpp
    Plugins/Air4.cpp
    Plugins/Cans.cpp
    Plugins/CansAW.cpp
    Plugins/Console0Buss.cpp
    Plugins/Console0Channel.cpp
    Plugins/ConsoleLABuss.cpp
    Plugins/ConsoleMCBuss.cpp
    Plugins/DeBess.cpp
    Plugins/DeBez.cpp
    Plugins/DeRez3.cpp
    Plugins/DigitalBlack.cpp
    Plugins/Discontapeity.cpp
    Plugins/Distance3.cpp
    Plugins/DubSub2.cpp
    Plugins/Dubly3.cpp
    Plugins/FatEQ.cpp
    Plugins/Flutter2.cpp
    Plugins/Gatelope.cpp
    Plugins/GlitchShifter.cpp
    Plugins/Hypersoft.cpp
    Plugins/Inflamer.cpp
    Plugins/Isolator3.cpp
    Plugins/Mackity.cpp
    Plugins/OneCornerClip.cpp
    Plugins/Parametric.cpp
    Plugins/PearEQ.cpp
    Plugins/Pockey2.cpp
    Plugins/PointyGuitar.cpp
    Plugins/Pop2.cpp
    Plugins/Silken.cpp
    Plugins/SingleEndedTriode.cpp
    Plugins/Smooth.cpp
    Plugins/SmoothEQ3.cpp
    Plugins/SoftGate.cpp
    Plugins/StoneFireComp.cpp
    Plugins/Stonefire.cpp
    Plugins/Sweeten.cpp
    Plugins/TakeCare.cpp
    Plugins/TapeDelay2.cpp
    Plugins/TapeDust.cpp
    Plugins/TapeHack2.cpp
    Plugins/ToneSlant.cpp
    Plugins/TremoSquare.cpp
    Plugins/Trianglizer.cpp
    Plugins/Tube2.cpp
    Plugins/Vibrato.cpp
    Plugins/Weight.cpp
    Plugins/Wider.cpp
    Plugins/Chamber.cpp
    Plugins/Galactic.cpp
    Plugins/VerbTiny.cpp
    Plugins/KBeyond.cpp
    Plugins/KCathedral5.cpp
    Plugins/KWoodRoom.cpp
)

add_library(ConduitAirwindows STATIC
    ${CONDUIT_AIRWINDOWS_CPP_SOURCES}
)

# Include-Root ist Source/ — Konsumenten und die Lib selbst inkludieren im
# Stil des Hauptprojekts: "DSP/Airwindows/AirwindowsPlugin.h"
target_include_directories(ConduitAirwindows PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

target_compile_features(ConduitAirwindows PUBLIC cxx_std_20)

# PUBLIC, damit die System-Abhängigkeiten der JUCE-Module (Linux: pthread/dl,
# Windows: via #pragma comment) zu den Konsumenten propagieren.
target_link_libraries(ConduitAirwindows
    PUBLIC
        juce::juce_audio_basics
        juce::juce_recommended_config_flags
)

# Dieses Target kompiliert seine eigene Kopie von juce_core.cpp (JUCE-Module
# werden pro Target übersetzt) — ohne diese Defines sucht Linux/Clang unter
# Third-Party-curl.h (auf dem CI-Runner nicht installiert), wo MSVC/Windows
# den Pfad gar nicht erst betritt (CI-Fund: lokal grün, Linux-CI rot).
# Muss dieselben Defines tragen wie Conduit/ConduitTests in der Root-CMakeLists.
target_compile_definitions(ConduitAirwindows PUBLIC
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

# Warnungen als Fehler NUR für eigene Sources (Muster Root-CMakeLists) —
# die JUCE-Modul-Sources werden im selben Target mitkompiliert und dürfen
# den Build nicht brechen.
if(MSVC)
    set_source_files_properties(${CONDUIT_AIRWINDOWS_CPP_SOURCES} PROPERTIES COMPILE_OPTIONS "/W4;/WX")
else()
    set_source_files_properties(${CONDUIT_AIRWINDOWS_CPP_SOURCES} PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Werror")
endif()

# ---------------------------------------------------------------------------
# Tests — Quellen liegen in Tests/DSP/Airwindows/ (Spiegelstruktur 13.5);
# das Target wird hier definiert, damit Tests/ keine eigene CMake-Änderung
# braucht.
# ---------------------------------------------------------------------------
if(TARGET Catch2::Catch2WithMain)
    set(CONDUIT_AIRWINDOWS_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../Tests/DSP/Airwindows)

    set(CONDUIT_AIRWINDOWS_TEST_SOURCES
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/AirwindowsRegistryTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/DensityTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/SlewTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/SpiralTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/Air4Tests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/CansTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/CansAWTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/Console0BussTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/Console0ChannelTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/ConsoleLABussTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/ConsoleMCBussTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/DeBessTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/DeBezTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/DeRez3Tests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/DigitalBlackTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/DiscontapeityTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/Distance3Tests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/DubSub2Tests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/Dubly3Tests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/FatEQTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/Flutter2Tests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/GatelopeTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/GlitchShifterTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/HypersoftTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/InflamerTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/Isolator3Tests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/MackityTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/OneCornerClipTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/ParametricTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/PearEQTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/Pockey2Tests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/PointyGuitarTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/Pop2Tests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/SilkenTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/SingleEndedTriodeTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/SmoothTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/SmoothEQ3Tests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/SoftGateTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/StoneFireCompTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/StonefireTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/SweetenTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/TakeCareTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/TapeDelay2Tests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/TapeDustTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/TapeHack2Tests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/ToneSlantTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/TremoSquareTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/TrianglizerTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/Tube2Tests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/VibratoTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/WeightTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/WiderTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/ChamberTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/GalacticTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/VerbTinyTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/KBeyondTests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/KCathedral5Tests.cpp
        ${CONDUIT_AIRWINDOWS_TESTS_DIR}/KWoodRoomTests.cpp
    )

    add_executable(ConduitAirwindowsTests
        ${CONDUIT_AIRWINDOWS_TEST_SOURCES}
    )

    target_link_libraries(ConduitAirwindowsTests
        PRIVATE
            ConduitAirwindows
            Catch2::Catch2WithMain
    )

    if(MSVC)
        set_source_files_properties(${CONDUIT_AIRWINDOWS_TEST_SOURCES} PROPERTIES COMPILE_OPTIONS "/W4;/WX")
    else()
        set_source_files_properties(${CONDUIT_AIRWINDOWS_TEST_SOURCES} PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Werror")
    endif()

    add_test(NAME ConduitAirwindowsTests COMMAND ConduitAirwindowsTests)
endif()
