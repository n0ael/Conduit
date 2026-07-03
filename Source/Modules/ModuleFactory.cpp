#include "ModuleFactory.h"

#include "AirwindowsDensityModule.h"
#include "AirwindowsSlewModule.h"
#include "AirwindowsSpiralModule.h"
#include "AirwindowsAir4Module.h"
#include "AirwindowsCansModule.h"
#include "AirwindowsCansAWModule.h"
#include "AirwindowsConsole0BussModule.h"
#include "AirwindowsConsole0ChannelModule.h"
#include "AirwindowsConsoleLABussModule.h"
#include "AirwindowsConsoleMCBussModule.h"
#include "AirwindowsDeBessModule.h"
#include "AirwindowsDeBezModule.h"
#include "AirwindowsDeRez3Module.h"
#include "AirwindowsDigitalBlackModule.h"
#include "AirwindowsDiscontapeityModule.h"
#include "AirwindowsDistance3Module.h"
#include "AirwindowsDubSub2Module.h"
#include "AirwindowsDubly3Module.h"
#include "AirwindowsFatEQModule.h"
#include "AirwindowsFlutter2Module.h"
#include "AirwindowsGatelopeModule.h"
#include "AirwindowsGlitchShifterModule.h"
#include "AirwindowsHypersoftModule.h"
#include "AirwindowsInflamerModule.h"
#include "AirwindowsIsolator3Module.h"
#include "AirwindowsMackityModule.h"
#include "AirwindowsOneCornerClipModule.h"
#include "AirwindowsParametricModule.h"
#include "AirwindowsPearEQModule.h"
#include "AirwindowsPockey2Module.h"
#include "AirwindowsPointyGuitarModule.h"
#include "AirwindowsPop2Module.h"
#include "AirwindowsSilkenModule.h"
#include "AirwindowsSingleEndedTriodeModule.h"
#include "AirwindowsSmoothModule.h"
#include "AirwindowsSmoothEQ3Module.h"
#include "AirwindowsSoftGateModule.h"
#include "AirwindowsStoneFireCompModule.h"
#include "AirwindowsStonefireModule.h"
#include "AirwindowsSweetenModule.h"
#include "AirwindowsTakeCareModule.h"
#include "AirwindowsTapeDelay2Module.h"
#include "AirwindowsTapeDustModule.h"
#include "AirwindowsTapeHack2Module.h"
#include "AirwindowsToneSlantModule.h"
#include "AirwindowsTremoSquareModule.h"
#include "AirwindowsTrianglizerModule.h"
#include "AirwindowsTube2Module.h"
#include "AirwindowsVibratoModule.h"
#include "AirwindowsWeightModule.h"
#include "AirwindowsWiderModule.h"
#include "AirwindowsChamberModule.h"
#include "AirwindowsGalacticModule.h"
#include "AirwindowsVerbTinyModule.h"
#include "AirwindowsKBeyondModule.h"
#include "AirwindowsKCathedral5Module.h"
#include "AirwindowsKWoodRoomModule.h"
#include "AttenuatorModule.h"
#include "CaptureTapModule.h"
#include "LfoModule.h"
#include "LinkAudioSendModule.h"
#include "ScopeModule.h"
#include "StepSequencerModule.h"

namespace conduit
{

void ModuleFactory::registerModule (const juce::String& moduleId, Creator creator)
{
    jassert (creator != nullptr);
    jassert (moduleId.isNotEmpty());

    creators[moduleId] = std::move (creator);
}

bool ModuleFactory::isRegistered (const juce::String& moduleId) const
{
    return creators.contains (moduleId);
}

std::unique_ptr<ConduitModule> ModuleFactory::create (const juce::String& moduleId) const
{
    if (const auto it = creators.find (moduleId); it != creators.end())
        return (it->second)();

    return nullptr;
}

//==============================================================================
void registerDefaultModules (ModuleFactory& factory)
{
    factory.registerModule (AttenuatorModule::staticModuleId,
                            [] { return std::make_unique<AttenuatorModule>(); });
    factory.registerModule (AirwindowsDensityModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsDensityModule>(); });
    factory.registerModule (AirwindowsSlewModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsSlewModule>(); });
    factory.registerModule (AirwindowsSpiralModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsSpiralModule>(); });
    factory.registerModule (AirwindowsAir4Module::staticModuleId,
                            [] { return std::make_unique<AirwindowsAir4Module>(); });
    factory.registerModule (AirwindowsCansModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsCansModule>(); });
    factory.registerModule (AirwindowsCansAWModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsCansAWModule>(); });
    factory.registerModule (AirwindowsConsole0BussModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsConsole0BussModule>(); });
    factory.registerModule (AirwindowsConsole0ChannelModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsConsole0ChannelModule>(); });
    factory.registerModule (AirwindowsConsoleLABussModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsConsoleLABussModule>(); });
    factory.registerModule (AirwindowsConsoleMCBussModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsConsoleMCBussModule>(); });
    factory.registerModule (AirwindowsDeBessModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsDeBessModule>(); });
    factory.registerModule (AirwindowsDeBezModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsDeBezModule>(); });
    factory.registerModule (AirwindowsDeRez3Module::staticModuleId,
                            [] { return std::make_unique<AirwindowsDeRez3Module>(); });
    factory.registerModule (AirwindowsDigitalBlackModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsDigitalBlackModule>(); });
    factory.registerModule (AirwindowsDiscontapeityModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsDiscontapeityModule>(); });
    factory.registerModule (AirwindowsDistance3Module::staticModuleId,
                            [] { return std::make_unique<AirwindowsDistance3Module>(); });
    factory.registerModule (AirwindowsDubSub2Module::staticModuleId,
                            [] { return std::make_unique<AirwindowsDubSub2Module>(); });
    factory.registerModule (AirwindowsDubly3Module::staticModuleId,
                            [] { return std::make_unique<AirwindowsDubly3Module>(); });
    factory.registerModule (AirwindowsFatEQModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsFatEQModule>(); });
    factory.registerModule (AirwindowsFlutter2Module::staticModuleId,
                            [] { return std::make_unique<AirwindowsFlutter2Module>(); });
    factory.registerModule (AirwindowsGatelopeModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsGatelopeModule>(); });
    factory.registerModule (AirwindowsGlitchShifterModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsGlitchShifterModule>(); });
    factory.registerModule (AirwindowsHypersoftModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsHypersoftModule>(); });
    factory.registerModule (AirwindowsInflamerModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsInflamerModule>(); });
    factory.registerModule (AirwindowsIsolator3Module::staticModuleId,
                            [] { return std::make_unique<AirwindowsIsolator3Module>(); });
    factory.registerModule (AirwindowsMackityModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsMackityModule>(); });
    factory.registerModule (AirwindowsOneCornerClipModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsOneCornerClipModule>(); });
    factory.registerModule (AirwindowsParametricModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsParametricModule>(); });
    factory.registerModule (AirwindowsPearEQModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsPearEQModule>(); });
    factory.registerModule (AirwindowsPockey2Module::staticModuleId,
                            [] { return std::make_unique<AirwindowsPockey2Module>(); });
    factory.registerModule (AirwindowsPointyGuitarModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsPointyGuitarModule>(); });
    factory.registerModule (AirwindowsPop2Module::staticModuleId,
                            [] { return std::make_unique<AirwindowsPop2Module>(); });
    factory.registerModule (AirwindowsSilkenModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsSilkenModule>(); });
    factory.registerModule (AirwindowsSingleEndedTriodeModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsSingleEndedTriodeModule>(); });
    factory.registerModule (AirwindowsSmoothModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsSmoothModule>(); });
    factory.registerModule (AirwindowsSmoothEQ3Module::staticModuleId,
                            [] { return std::make_unique<AirwindowsSmoothEQ3Module>(); });
    factory.registerModule (AirwindowsSoftGateModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsSoftGateModule>(); });
    factory.registerModule (AirwindowsStoneFireCompModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsStoneFireCompModule>(); });
    factory.registerModule (AirwindowsStonefireModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsStonefireModule>(); });
    factory.registerModule (AirwindowsSweetenModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsSweetenModule>(); });
    factory.registerModule (AirwindowsTakeCareModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsTakeCareModule>(); });
    factory.registerModule (AirwindowsTapeDelay2Module::staticModuleId,
                            [] { return std::make_unique<AirwindowsTapeDelay2Module>(); });
    factory.registerModule (AirwindowsTapeDustModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsTapeDustModule>(); });
    factory.registerModule (AirwindowsTapeHack2Module::staticModuleId,
                            [] { return std::make_unique<AirwindowsTapeHack2Module>(); });
    factory.registerModule (AirwindowsToneSlantModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsToneSlantModule>(); });
    factory.registerModule (AirwindowsTremoSquareModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsTremoSquareModule>(); });
    factory.registerModule (AirwindowsTrianglizerModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsTrianglizerModule>(); });
    factory.registerModule (AirwindowsTube2Module::staticModuleId,
                            [] { return std::make_unique<AirwindowsTube2Module>(); });
    factory.registerModule (AirwindowsVibratoModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsVibratoModule>(); });
    factory.registerModule (AirwindowsWeightModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsWeightModule>(); });
    factory.registerModule (AirwindowsWiderModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsWiderModule>(); });
    factory.registerModule (AirwindowsChamberModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsChamberModule>(); });
    factory.registerModule (AirwindowsGalacticModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsGalacticModule>(); });
    factory.registerModule (AirwindowsVerbTinyModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsVerbTinyModule>(); });
    factory.registerModule (AirwindowsKBeyondModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsKBeyondModule>(); });
    factory.registerModule (AirwindowsKCathedral5Module::staticModuleId,
                            [] { return std::make_unique<AirwindowsKCathedral5Module>(); });
    factory.registerModule (AirwindowsKWoodRoomModule::staticModuleId,
                            [] { return std::make_unique<AirwindowsKWoodRoomModule>(); });
    factory.registerModule (LfoModule::staticModuleId,
                            [] { return std::make_unique<LfoModule>(); });
    factory.registerModule (ScopeModule::staticModuleId,
                            [] { return std::make_unique<ScopeModule>(); });
    factory.registerModule (StepSequencerModule::staticModuleId,
                            [] { return std::make_unique<StepSequencerModule>(); });
    factory.registerModule (LinkAudioSendModule::staticModuleId,
                            [] { return std::make_unique<LinkAudioSendModule>(); });
    factory.registerModule (CaptureTapModule::staticModuleId,
                            [] { return std::make_unique<CaptureTapModule>(); });
}

} // namespace conduit
