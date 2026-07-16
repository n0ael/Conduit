#include <catch2/catch_test_macros.hpp>

#include <map>
#include <vector>

#include "Core/PositionFeedbackRouter.h"

using namespace conduit::midirig;
namespace grid = conduit::grid;

namespace
{
    /** Mini-AlphaTrack: Motorfader (Pitch Bend ch1, position-Feedback,
        Touch-Note 104), Ribbon (Pitch Bend ch10, scrub, Touch-Note 116 +
        reines type=touch-Control fuer die 2-Finger-Note 123) und ein
        CC-Motorfader (generisches Geraet) -- ueber den echten CSV-Parser
        (Roundtrip der M8-Schema-Erweiterung inklusive). */
    ControllerProfile testProfile()
    {
        const auto csv = juce::String (
            "device,id,type,mode,steps,touch_number,send_kind,send_channel,send_number,"
            "feedback1_kind,feedback1_channel,feedback1_number,feedback1_meaning\n"
            "TestTrack,fader,fader,,,104,pitchbend,1,,pitchbend,1,,position\n"
            "TestTrack,strip,ribbon,scrub,,116,pitchbend,10,,,,,\n"
            "TestTrack,strip_touch2,touch,,,,note,1,123,,,,\n"
            "TestTrack,ccfader,fader,,,105,cc,1,20,cc,1,20,position\n"
            "TestTrack,enc_l,encoder,relative,72,120,cc,1,16,,,,\n");

        ControllerParseReport report;
        const auto profile = parseControllerProfileCsv (csv, &report);
        REQUIRE (report.accepted == 5);
        return profile;
    }

    struct RouterRig
    {
        PositionFeedbackRouter router;
        std::vector<std::pair<FeedbackAddress, int>> sent;
        std::map<std::pair<int, bool>, float> values;   // Bindungs-Nummer -> Wert

        RouterRig()
        {
            router.send = [this] (const FeedbackAddress& address, int value14)
            { sent.emplace_back (address, value14); };

            router.currentBoundValueFor = [this] (int number, bool isNote)
            {
                const auto it = values.find ({ number, isNote });
                return it != values.end() ? it->second : -1.0f;
            };

            router.setProfile (testProfile());
        }

        void ticks (int n) { for (int i = 0; i < n; ++i) router.tick(); }
    };

    constexpr int kFaderNumber = grid::kPitchBendBindingBase + 1;   // PB ch1
}

TEST_CASE ("PositionFeedbackRouter: Profil-Parse liefert position-Controls und Touch-Noten", "[positionrouter]")
{
    const auto profile = testProfile();

    // Pitch-Bend-Konvention: findBySendAddress matcht ueber den KANAL.
    const auto* fader = profile.findBySendAddress (AddressKind::pitchBend, 1);
    REQUIRE (fader != nullptr);
    CHECK (fader->id == "fader");
    CHECK (fader->touchNumber == 104);
    REQUIRE (fader->feedback.size() == 1);
    CHECK (fader->feedback[0].kind == AddressKind::pitchBend);
    CHECK (fader->feedback[0].meaning == "position");

    const auto* strip = profile.findBySendAddress (AddressKind::pitchBend, 10);
    REQUIRE (strip != nullptr);
    CHECK (strip->mode == kModeScrub);

    const auto* encoder = profile.findBySendAddress (AddressKind::cc, 16);
    REQUIRE (encoder != nullptr);
    CHECK (encoder->mode == kModeRelative);
    CHECK (encoder->steps == 72);
}

TEST_CASE ("PositionFeedbackRouter: sendet gebundenen Wert einmal und difft danach", "[positionrouter]")
{
    RouterRig rig;
    rig.values[{ kFaderNumber, false }] = 0.5f;

    rig.ticks (3);

    // Nur EIN Send trotz drei Ticks (Dedupe auf dem 14-bit-Wert).
    REQUIRE (rig.sent.size() == 1);
    CHECK (rig.sent[0].first.kind == AddressKind::pitchBend);
    CHECK (rig.sent[0].first.channel == 1);
    CHECK (rig.sent[0].second == 8192);

    // Wert-Aenderung (z. B. Ebenen-Wechsel/UI-Drag) -> genau ein weiterer Send.
    rig.values[{ kFaderNumber, false }] = 1.0f;
    rig.ticks (2);
    REQUIRE (rig.sent.size() == 2);
    CHECK (rig.sent[1].second == 16383);
}

TEST_CASE ("PositionFeedbackRouter: ungebunden = stumm", "[positionrouter]")
{
    RouterRig rig;
    rig.ticks (5);
    CHECK (rig.sent.empty());
}

TEST_CASE ("PositionFeedbackRouter: Touch-Gate unterdrueckt Sends, Loslassen faehrt nach", "[positionrouter]")
{
    RouterRig rig;
    rig.values[{ kFaderNumber, false }] = 0.25f;
    rig.ticks (1);
    REQUIRE (rig.sent.size() == 1);

    // Finger auf dem Fader: Wert-Aenderungen werden NICHT gesendet.
    rig.router.handleControllerNote (104, true);
    rig.values[{ kFaderNumber, false }] = 0.75f;
    rig.ticks (3);
    CHECK (rig.sent.size() == 1);

    // Loslassen: der naechste Tick faehrt den aktuellen Wert an.
    rig.router.handleControllerNote (104, false);
    rig.ticks (1);
    REQUIRE (rig.sent.size() == 2);
    CHECK (rig.sent[1].second == juce::roundToInt (0.75f * 16383.0f));
}

TEST_CASE ("PositionFeedbackRouter: fremde Noten gaten nicht", "[positionrouter]")
{
    RouterRig rig;
    rig.router.handleControllerNote (60, true);   // keine Touch-Note
    rig.values[{ kFaderNumber, false }] = 0.5f;
    rig.ticks (1);
    CHECK (rig.sent.size() == 1);
}

TEST_CASE ("PositionFeedbackRouter: isTouchNote deckt touch_number UND type=touch ab", "[positionrouter]")
{
    RouterRig rig;
    CHECK (rig.router.isTouchNote (104));   // Fader-Touch
    CHECK (rig.router.isTouchNote (116));   // Strip-Touch (touch_number)
    CHECK (rig.router.isTouchNote (123));   // Strip 2-Finger (type=touch-Zeile)
    CHECK (rig.router.isTouchNote (120));   // Encoder-Touch
    CHECK_FALSE (rig.router.isTouchNote (60));
}

TEST_CASE ("PositionFeedbackRouter: CC-position-Feedback wird bedient", "[positionrouter]")
{
    RouterRig rig;
    rig.values[{ 20, false }] = 1.0f;
    rig.ticks (1);
    REQUIRE (rig.sent.size() == 1);
    CHECK (rig.sent[0].first.kind == AddressKind::cc);
    CHECK (rig.sent[0].first.number == 20);
    CHECK (rig.sent[0].second == 16383);
}

TEST_CASE ("PositionFeedbackRouter: reset verwirft Dedupe-Stand (Geraete-Wechsel)", "[positionrouter]")
{
    RouterRig rig;
    rig.values[{ kFaderNumber, false }] = 0.5f;
    rig.ticks (1);
    REQUIRE (rig.sent.size() == 1);

    rig.router.reset();
    rig.ticks (1);
    REQUIRE (rig.sent.size() == 2);   // gleicher Wert, aber neuer Stand
}
