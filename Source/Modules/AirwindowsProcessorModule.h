#pragma once

#include <array>
#include <atomic>
#include <memory>

#include "DSP/Airwindows/AirwindowsPlugin.h"
#include "ProcessorModule.h"

namespace conduit
{

//==============================================================================
/**
    Generischer Wrapper für portierte Airwindows-Effekte (Source/DSP/Airwindows).

    Kein Template — die konkrete DSP-Instanz ist bereits polymorph
    (airwindows::AirwindowsPlugin), der Wrapper arbeitet rein über
    getNumParameters()/getParameterInfo() generisch. Konkrete Subklassen
    (AirwindowsDensityModule etc.) reichen nur eine fertige Plugin-Instanz
    plus moduleId/Displayname durch — sonst identisch.

    Bewusst KEIN SmoothedValue: AirwindowsPlugin::process() snapshottet
    Parameter bereits selbst einmal pro Block (block-konstant, exakt wie
    beim VST-Original — siehe AirwindowsPlugin.h). Zusätzliches
    Sample-Ramping in dieser Schicht würde dieses dokumentierte, gegen die
    DoD-Tests verifizierte Originalverhalten verändern.

    Bus: fest stereo (2 in / 2 out) — passt zu "stereo, 2 in / 2 out" aus
    AirwindowsPlugin.h; kein Mono-Fallback nötig.

    Thread-Ownership:
      - getParameterTarget()-Rückgabe wird von Fremd-Threads beschrieben
        (Dual-State 6.1)
      - prepareToPlay() → Message Thread (ruft plugin->prepare())
      - processBlock() → Audio Thread, lock-free, allocation-free
*/
class AirwindowsProcessorModule : public ProcessorModule
{
public:
    AirwindowsProcessorModule (std::unique_ptr<airwindows::AirwindowsPlugin> pluginToUse,
                              juce::String moduleIdToUse,
                              juce::String displayNameToUse);

    //==========================================================================
    // Pflicht-Methoden (CLAUDE.md 4.4) — für alle Subklassen fertig implementiert
    [[nodiscard]] juce::String getModuleId() const override;
    [[nodiscard]] juce::String getModuleDisplayName() const override;
    [[nodiscard]] int getStateVersion() const override;

    //==========================================================================
    [[nodiscard]] std::atomic<float>* getParameterTarget (const juce::String& parameterId) noexcept override;

    //==========================================================================
    // AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

protected:
    void appendParametersTo (juce::ValueTree& parameters) override;

private:
    std::unique_ptr<airwindows::AirwindowsPlugin> plugin;
    juce::String moduleIdString;
    juce::String displayNameString;

    // Ein Slot pro Airwindows-Parameter (0 bei Spiral) — Adresse stabil über
    // die Modul-Lebensdauer (ConduitModule/AudioProcessor nicht kopierbar).
    std::array<std::atomic<float>, airwindows::AirwindowsPlugin::maxParameters> targets {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsProcessorModule)
};

} // namespace conduit
