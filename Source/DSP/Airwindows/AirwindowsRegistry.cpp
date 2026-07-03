/* ========================================
 *  AirwindowsRegistry.cpp — Katalog der portierten Airwindows-Effekte
 *  DSP-Originale: Chris Johnson / Airwindows (MIT-Lizenz)
 * ======================================== */

#include "DSP/Airwindows/AirwindowsRegistry.h"

#include "DSP/Airwindows/Plugins/Density.h"
#include "DSP/Airwindows/Plugins/Slew.h"
#include "DSP/Airwindows/Plugins/Spiral.h"
#include "DSP/Airwindows/Plugins/Air4.h"
#include "DSP/Airwindows/Plugins/Cans.h"
#include "DSP/Airwindows/Plugins/CansAW.h"
#include "DSP/Airwindows/Plugins/Console0Buss.h"
#include "DSP/Airwindows/Plugins/Console0Channel.h"
#include "DSP/Airwindows/Plugins/ConsoleLABuss.h"
#include "DSP/Airwindows/Plugins/ConsoleMCBuss.h"
#include "DSP/Airwindows/Plugins/DeBess.h"
#include "DSP/Airwindows/Plugins/DeBez.h"
#include "DSP/Airwindows/Plugins/DeRez3.h"
#include "DSP/Airwindows/Plugins/DigitalBlack.h"
#include "DSP/Airwindows/Plugins/Discontapeity.h"
#include "DSP/Airwindows/Plugins/Distance3.h"
#include "DSP/Airwindows/Plugins/DubSub2.h"
#include "DSP/Airwindows/Plugins/Dubly3.h"
#include "DSP/Airwindows/Plugins/FatEQ.h"
#include "DSP/Airwindows/Plugins/Flutter2.h"
#include "DSP/Airwindows/Plugins/Gatelope.h"
#include "DSP/Airwindows/Plugins/GlitchShifter.h"
#include "DSP/Airwindows/Plugins/Hypersoft.h"
#include "DSP/Airwindows/Plugins/Inflamer.h"
#include "DSP/Airwindows/Plugins/Isolator3.h"
#include "DSP/Airwindows/Plugins/Mackity.h"
#include "DSP/Airwindows/Plugins/OneCornerClip.h"
#include "DSP/Airwindows/Plugins/Parametric.h"
#include "DSP/Airwindows/Plugins/PearEQ.h"
#include "DSP/Airwindows/Plugins/Pockey2.h"
#include "DSP/Airwindows/Plugins/PointyGuitar.h"
#include "DSP/Airwindows/Plugins/Pop2.h"
#include "DSP/Airwindows/Plugins/Silken.h"
#include "DSP/Airwindows/Plugins/SingleEndedTriode.h"
#include "DSP/Airwindows/Plugins/Smooth.h"
#include "DSP/Airwindows/Plugins/SmoothEQ3.h"
#include "DSP/Airwindows/Plugins/SoftGate.h"
#include "DSP/Airwindows/Plugins/StoneFireComp.h"
#include "DSP/Airwindows/Plugins/Stonefire.h"
#include "DSP/Airwindows/Plugins/Sweeten.h"
#include "DSP/Airwindows/Plugins/TakeCare.h"
#include "DSP/Airwindows/Plugins/TapeDelay2.h"
#include "DSP/Airwindows/Plugins/TapeDust.h"
#include "DSP/Airwindows/Plugins/TapeHack2.h"
#include "DSP/Airwindows/Plugins/ToneSlant.h"
#include "DSP/Airwindows/Plugins/TremoSquare.h"
#include "DSP/Airwindows/Plugins/Trianglizer.h"
#include "DSP/Airwindows/Plugins/Tube2.h"
#include "DSP/Airwindows/Plugins/Vibrato.h"
#include "DSP/Airwindows/Plugins/Weight.h"
#include "DSP/Airwindows/Plugins/Wider.h"
#include "DSP/Airwindows/Plugins/Chamber.h"
#include "DSP/Airwindows/Plugins/Galactic.h"
#include "DSP/Airwindows/Plugins/VerbTiny.h"
#include "DSP/Airwindows/Plugins/KBeyond.h"
#include "DSP/Airwindows/Plugins/KCathedral5.h"
#include "DSP/Airwindows/Plugins/KWoodRoom.h"

namespace conduit::airwindows
{

namespace
{
    template <typename PluginType>
    std::unique_ptr<AirwindowsPlugin> make()
    {
        return std::make_unique<PluginType>();
    }

    constexpr RegistryEntry kRegistry[] = {
        { "density", "Density", &make<Density> },
        { "slew",    "Slew",    &make<Slew> },
        { "spiral",  "Spiral",  &make<Spiral> },
        { "air4", "Air4", &make<Air4> },
        { "cans", "Cans", &make<Cans> },
        { "cansaw", "CansAW", &make<CansAW> },
        { "console0buss", "Console0Buss", &make<Console0Buss> },
        { "console0channel", "Console0Channel", &make<Console0Channel> },
        { "consolelabuss", "ConsoleLABuss", &make<ConsoleLABuss> },
        { "consolemcbuss", "ConsoleMCBuss", &make<ConsoleMCBuss> },
        { "debess", "DeBess", &make<DeBess> },
        { "debez", "DeBez", &make<DeBez> },
        { "derez3", "DeRez3", &make<DeRez3> },
        { "digitalblack", "DigitalBlack", &make<DigitalBlack> },
        { "discontapeity", "Discontapeity", &make<Discontapeity> },
        { "distance3", "Distance3", &make<Distance3> },
        { "dubsub2", "DubSub2", &make<DubSub2> },
        { "dubly3", "Dubly3", &make<Dubly3> },
        { "fateq", "FatEQ", &make<FatEQ> },
        { "flutter2", "Flutter2", &make<Flutter2> },
        { "gatelope", "Gatelope", &make<Gatelope> },
        { "glitchshifter", "GlitchShifter", &make<GlitchShifter> },
        { "hypersoft", "Hypersoft", &make<Hypersoft> },
        { "inflamer", "Inflamer", &make<Inflamer> },
        { "isolator3", "Isolator3", &make<Isolator3> },
        { "mackity", "Mackity", &make<Mackity> },
        { "onecornerclip", "OneCornerClip", &make<OneCornerClip> },
        { "parametric", "Parametric", &make<Parametric> },
        { "peareq", "PearEQ", &make<PearEQ> },
        { "pockey2", "Pockey2", &make<Pockey2> },
        { "pointyguitar", "PointyGuitar", &make<PointyGuitar> },
        { "pop2", "Pop2", &make<Pop2> },
        { "silken", "Silken", &make<Silken> },
        { "singleendedtriode", "SingleEndedTriode", &make<SingleEndedTriode> },
        { "smooth", "Smooth", &make<Smooth> },
        { "smootheq3", "SmoothEQ3", &make<SmoothEQ3> },
        { "softgate", "SoftGate", &make<SoftGate> },
        { "stonefirecomp", "StoneFireComp", &make<StoneFireComp> },
        { "stonefire", "Stonefire", &make<Stonefire> },
        { "sweeten", "Sweeten", &make<Sweeten> },
        { "takecare", "TakeCare", &make<TakeCare> },
        { "tapedelay2", "TapeDelay2", &make<TapeDelay2> },
        { "tapedust", "TapeDust", &make<TapeDust> },
        { "tapehack2", "TapeHack2", &make<TapeHack2> },
        { "toneslant", "ToneSlant", &make<ToneSlant> },
        { "tremosquare", "TremoSquare", &make<TremoSquare> },
        { "trianglizer", "Trianglizer", &make<Trianglizer> },
        { "tube2", "Tube2", &make<Tube2> },
        { "vibrato", "Vibrato", &make<Vibrato> },
        { "weight", "Weight", &make<Weight> },
        { "wider", "Wider", &make<Wider> },
        { "chamber", "Chamber", &make<Chamber> },
        { "galactic", "Galactic", &make<Galactic> },
        { "verbtiny", "VerbTiny", &make<VerbTiny> },
        { "kbeyond", "kBeyond", &make<KBeyond> },
        { "kcathedral5", "kCathedral5", &make<KCathedral5> },
        { "kwoodroom", "kWoodRoom", &make<KWoodRoom> },
    };
}

std::span<const RegistryEntry> getRegisteredPlugins() noexcept
{
    return kRegistry;
}

std::unique_ptr<AirwindowsPlugin> createPlugin (std::string_view id)
{
    for (const auto& entry : kRegistry)
        if (id == entry.id)
            return entry.create();

    return nullptr;
}

} // namespace conduit::airwindows
