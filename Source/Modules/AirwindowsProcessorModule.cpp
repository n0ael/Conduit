#include "AirwindowsProcessorModule.h"

namespace conduit
{

AirwindowsProcessorModule::AirwindowsProcessorModule (std::unique_ptr<airwindows::AirwindowsPlugin> pluginToUse,
                                                      juce::String moduleIdToUse,
                                                      juce::String displayNameToUse)
    : ProcessorModule (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      plugin (std::move (pluginToUse)),
      moduleIdString (std::move (moduleIdToUse)),
      displayNameString (std::move (displayNameToUse))
{
    jassert (plugin != nullptr);

    for (int i = 0; i < plugin->getNumParameters(); ++i)
        targets[(size_t) i].store (plugin->getParameterInfo (i).defaultValue, std::memory_order_relaxed);
}

//==============================================================================
juce::String AirwindowsProcessorModule::getModuleId() const          { return moduleIdString; }
juce::String AirwindowsProcessorModule::getModuleDisplayName() const { return displayNameString; }
int AirwindowsProcessorModule::getStateVersion() const                { return 1; }

void AirwindowsProcessorModule::appendParametersTo (juce::ValueTree& parameters)
{
    for (int i = 0; i < plugin->getNumParameters(); ++i)
    {
        const auto& info = plugin->getParameterInfo (i);
        parameters.appendChild (makeParameter (info.id, info.defaultValue, 0.0, 1.0, info.defaultValue), nullptr);
    }
}

std::atomic<float>* AirwindowsProcessorModule::getParameterTarget (const juce::String& parameterId) noexcept
{
    for (int i = 0; i < plugin->getNumParameters(); ++i)
        if (parameterId == plugin->getParameterInfo (i).id)
            return &targets[(size_t) i];

    return nullptr;
}

//==============================================================================
void AirwindowsProcessorModule::prepareToPlay (double sampleRate, int)
{
    plugin->prepare (sampleRate);
}

void AirwindowsProcessorModule::releaseResources()
{
}

void AirwindowsProcessorModule::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int i = 0; i < plugin->getNumParameters(); ++i)
        plugin->setParameter (i, targets[(size_t) i].load (std::memory_order_relaxed));

    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);

    plugin->process (left, right, left, right, buffer.getNumSamples());
}

} // namespace conduit
