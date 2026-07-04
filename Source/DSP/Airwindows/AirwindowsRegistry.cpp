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

    // Kategorie-Strings zentral — Tippfehler fielen sonst erst im
    // Registry-Test auf (erlaubte Menge siehe Header-Doku)
    constexpr auto kDynamics   = "Dynamics";
    constexpr auto kFilterEq   = "Filter/EQ";
    constexpr auto kSaturation = "Distortion/Saturation";
    constexpr auto kLoFiTape   = "Lo-Fi/Tape";
    constexpr auto kModulation = "Modulation";
    constexpr auto kConsole    = "Console";
    constexpr auto kReverb     = "Reverb/Delay";
    constexpr auto kUtility    = "Utility";

    constexpr RegistryEntry kRegistry[] = {
        { "density", "Density", kSaturation, "saturation overdrive warmth analog", &make<Density> },
        { "slew",    "Slew",    kFilterEq, "slew limiter treble soften darken", &make<Slew> },
        { "spiral",  "Spiral",  kSaturation, "saturation sine clip smooth", &make<Spiral> },
        { "air4", "Air4", kFilterEq, "air treble brilliance shine high", &make<Air4> },
        { "cans", "Cans", kUtility, "headphone crossfeed monitoring room", &make<Cans> },
        { "cansaw", "CansAW", kUtility, "headphone crossfeed monitoring vintage", &make<CansAW> },
        { "console0buss", "Console0Buss", kConsole, "console summing buss glue analog", &make<Console0Buss> },
        { "console0channel", "Console0Channel", kConsole, "console summing channel glue analog", &make<Console0Channel> },
        { "consolelabuss", "ConsoleLABuss", kConsole, "console summing buss la2a opto", &make<ConsoleLABuss> },
        { "consolemcbuss", "ConsoleMCBuss", kConsole, "console summing buss mackie glue", &make<ConsoleMCBuss> },
        { "debess", "DeBess", kDynamics, "deesser sibilance vocal tame", &make<DeBess> },
        { "debez", "DeBez", kUtility, "declick restoration bezier smooth repair", &make<DeBez> },
        { "derez3", "DeRez3", kLoFiTape, "bitcrusher samplerate lofi digital retro", &make<DeRez3> },
        { "digitalblack", "DigitalBlack", kDynamics, "gate noise silence floor recording", &make<DigitalBlack> },
        { "discontapeity", "Discontapeity", kLoFiTape, "tape flutter artifact wow vintage", &make<Discontapeity> },
        { "distance3", "Distance3", kFilterEq, "distance air absorption depth space", &make<Distance3> },
        { "dubsub2", "DubSub2", kFilterEq, "subbass bass boost octave dub", &make<DubSub2> },
        { "dubly3", "Dubly3", kLoFiTape, "dolby tape noise encode vintage", &make<Dubly3> },
        { "fateq", "FatEQ", kFilterEq, "eq equalizer fat bands tone", &make<FatEQ> },
        { "flutter2", "Flutter2", kLoFiTape, "tape flutter wow pitch vintage", &make<Flutter2> },
        { "gatelope", "Gatelope", kDynamics, "gate envelope filter sidechain rhythmic", &make<Gatelope> },
        { "glitchshifter", "GlitchShifter", kModulation, "pitch shifter glitch granular detune", &make<GlitchShifter> },
        { "hypersoft", "Hypersoft", kSaturation, "clipper soft limit smooth loud", &make<Hypersoft> },
        { "inflamer", "Inflamer", kSaturation, "harmonics drive color loudness", &make<Inflamer> },
        { "isolator3", "Isolator3", kFilterEq, "isolator dj filter crossover kill", &make<Isolator3> },
        { "mackity", "Mackity", kSaturation, "preamp mackie drive input analog", &make<Mackity> },
        { "onecornerclip", "OneCornerClip", kSaturation, "clipper hard corner loud", &make<OneCornerClip> },
        { "parametric", "Parametric", kFilterEq, "eq parametric bands sweep tone", &make<Parametric> },
        { "peareq", "PearEQ", kFilterEq, "eq pear smooth tone gentle", &make<PearEQ> },
        { "pockey2", "Pockey2", kLoFiTape, "lofi hiphop sampler 12bit crush", &make<Pockey2> },
        { "pointyguitar", "PointyGuitar", kSaturation, "guitar amp pointy attack bright", &make<PointyGuitar> },
        { "pop2", "Pop2", kDynamics, "compressor pop punch vocal", &make<Pop2> },
        { "silken", "Silken", kSaturation, "silk sheen highs subtle gloss", &make<Silken> },
        { "singleendedtriode", "SingleEndedTriode", kSaturation, "tube triode class-a warmth harmonics", &make<SingleEndedTriode> },
        { "smooth", "Smooth", kFilterEq, "smooth harshness soften anti-alias", &make<Smooth> },
        { "smootheq3", "SmoothEQ3", kFilterEq, "eq smooth bands gentle tone", &make<SmoothEQ3> },
        { "softgate", "SoftGate", kDynamics, "gate soft hiss noise tail", &make<SoftGate> },
        { "stonefirecomp", "StoneFireComp", kDynamics, "compressor stone fire glue", &make<StoneFireComp> },
        { "stonefire", "Stonefire", kSaturation, "tone fire drive twoband color", &make<Stonefire> },
        { "sweeten", "Sweeten", kSaturation, "sweetener second harmonic subtle polish", &make<Sweeten> },
        { "takecare", "TakeCare", kUtility, "care mastering polish gentle", &make<TakeCare> },
        { "tapedelay2", "TapeDelay2", kReverb, "delay tape echo feedback vintage", &make<TapeDelay2> },
        { "tapedust", "TapeDust", kLoFiTape, "tape dust noise hiss texture", &make<TapeDust> },
        { "tapehack2", "TapeHack2", kLoFiTape, "tape cassette hack lofi character", &make<TapeHack2> },
        { "toneslant", "ToneSlant", kFilterEq, "tilt eq slant tone balance", &make<ToneSlant> },
        { "tremosquare", "TremoSquare", kModulation, "tremolo square chop rhythmic", &make<TremoSquare> },
        { "trianglizer", "Trianglizer", kSaturation, "waveshaper triangle fold harmonics", &make<Trianglizer> },
        { "tube2", "Tube2", kSaturation, "tube valve warmth drive analog", &make<Tube2> },
        { "vibrato", "Vibrato", kModulation, "vibrato pitch chorus fm wobble", &make<Vibrato> },
        { "weight", "Weight", kFilterEq, "bass weight low resonance foundation", &make<Weight> },
        { "wider", "Wider", kUtility, "stereo width image mid side", &make<Wider> },
        { "chamber", "Chamber", kReverb, "reverb chamber space hall", &make<Chamber> },
        { "galactic", "Galactic", kReverb, "reverb galactic huge pad ambient", &make<Galactic> },
        { "verbtiny", "VerbTiny", kReverb, "reverb tiny small room cheap", &make<VerbTiny> },
        { "kbeyond", "kBeyond", kReverb, "reverb beyond deep modulated", &make<KBeyond> },
        { "kcathedral5", "kCathedral5", kReverb, "reverb cathedral church huge", &make<KCathedral5> },
        { "kwoodroom", "kWoodRoom", kReverb, "reverb wood room warm natural", &make<KWoodRoom> },
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
