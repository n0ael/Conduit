#include <catch2/catch_test_macros.hpp>

#include "Core/MpeMidiSink.h"
#include "FakeMidiTarget.h"

namespace grid = conduit::grid;

//==============================================================================
TEST_CASE ("MpeMidiSink: voiceStart sendet NoteOn", "[grid]")
{
    grid::FakeMidiTarget fake;
    grid::MpeMidiSink sink (fake);

    sink.voiceStart (0, 60, 100);

    REQUIRE (fake.messages.size() == 1);
    REQUIRE (fake.messages[0].isNoteOn());
    REQUIRE (fake.messages[0].getChannel() == 2);
    REQUIRE (fake.messages[0].getNoteNumber() == 60);
    REQUIRE (fake.messages[0].getVelocity() == 100);
}

TEST_CASE ("MpeMidiSink: voicePitchBend sendet PitchWheel", "[grid]")
{
    grid::FakeMidiTarget fake;
    grid::MpeMidiSink sink (fake);

    sink.voicePitchBend (0, 0.0f);

    REQUIRE (fake.messages.size() == 1);
    REQUIRE (fake.messages[0].isPitchWheel());
    REQUIRE (fake.messages[0].getChannel() == 2);
    REQUIRE (fake.messages[0].getPitchWheelValue() == 8192);
}

TEST_CASE ("MpeMidiSink: voiceStop sendet NoteOff mit gemerkter Note", "[grid]")
{
    grid::FakeMidiTarget fake;
    grid::MpeMidiSink sink (fake);

    sink.voiceStart (0, 60, 100);
    sink.voiceStop (0, 0);

    REQUIRE (fake.messages.size() == 2);
    REQUIRE (fake.messages[1].isNoteOff());
    REQUIRE (fake.messages[1].getChannel() == 2);
    REQUIRE (fake.messages[1].getNoteNumber() == 60);
}

TEST_CASE ("MpeMidiSink: voiceStop ohne aktive Note sendet nichts", "[grid]")
{
    grid::FakeMidiTarget fake;
    grid::MpeMidiSink sink (fake);

    sink.voiceStop (0, 0);

    REQUIRE (fake.messages.empty());
}

TEST_CASE ("MpeMidiSink: allNotesOff beendet alle aktiven Stimmen", "[grid]")
{
    grid::FakeMidiTarget fake;
    grid::MpeMidiSink sink (fake);

    sink.voiceStart (0, 60, 100);
    sink.voiceStart (1, 64, 90);
    fake.messages.clear();

    sink.allNotesOff();

    REQUIRE (fake.messages.size() == 2);
    REQUIRE (fake.messages[0].isNoteOff());
    REQUIRE (fake.messages[0].getChannel() == 2);
    REQUIRE (fake.messages[0].getNoteNumber() == 60);
    REQUIRE (fake.messages[1].isNoteOff());
    REQUIRE (fake.messages[1].getChannel() == 3);
    REQUIRE (fake.messages[1].getNoteNumber() == 64);
}

TEST_CASE ("MpeMidiSink: voicePressure adressiert die aktive Note (Poly-AT, Block B4)", "[grid]")
{
    grid::FakeMidiTarget fake;
    grid::MpeMidiSink sink (fake);
    sink.setExpressionMode (grid::ExpressionMode::polyAftertouch);

    sink.voiceStart (0, 60, 100);
    fake.messages.clear();

    sink.voicePressure (0, 1.0f);

    REQUIRE (fake.messages.size() == 1);
    REQUIRE (fake.messages[0].isAftertouch());
    REQUIRE (fake.messages[0].getChannel() == 1);
    REQUIRE (fake.messages[0].getNoteNumber() == 60);
    REQUIRE (fake.messages[0].getAfterTouchValue() == 127);
}

TEST_CASE ("MpeMidiSink: voicePressure ohne aktive Note sendet nichts (Block B4)", "[grid]")
{
    grid::FakeMidiTarget fake;
    grid::MpeMidiSink sink (fake);

    sink.voicePressure (0, 0.5f);

    REQUIRE (fake.messages.empty());
}

TEST_CASE ("MpeMidiSink: monoAftertouch buendelt alles auf Kanal 1 (Block B4)", "[grid]")
{
    grid::FakeMidiTarget fake;
    grid::MpeMidiSink sink (fake);
    sink.setExpressionMode (grid::ExpressionMode::monoAftertouch);

    sink.voiceStart (0, 60, 100);
    sink.voiceStart (1, 64, 90);
    sink.voicePressure (1, 0.5f);
    sink.voicePitchBend (0, 0.0f);

    REQUIRE (fake.messages.size() == 4);
    for (const auto& message : fake.messages)
        REQUIRE (message.getChannel() == 1);

    REQUIRE (fake.messages[2].isChannelPressure());
    REQUIRE (fake.messages[3].isPitchWheel());
}

TEST_CASE ("MpeMidiSink: setExpressionMode beendet haengende Noten auf den alten Kanaelen (Block B4)", "[grid]")
{
    grid::FakeMidiTarget fake;
    grid::MpeMidiSink sink (fake);

    sink.voiceStart (0, 60, 100);   // NoteOn Kanal 2 (mpe)
    fake.messages.clear();

    sink.setExpressionMode (grid::ExpressionMode::monoAftertouch);

    // NoteOff muss noch mit der ALTEN Kanalzuteilung (Kanal 2) rausgehen
    REQUIRE (fake.messages.size() == 1);
    REQUIRE (fake.messages[0].isNoteOff());
    REQUIRE (fake.messages[0].getChannel() == 2);
    REQUIRE (fake.messages[0].getNoteNumber() == 60);

    // Kein-Wechsel ist ein No-Op (kein doppeltes allNotesOff)
    fake.messages.clear();
    sink.setExpressionMode (grid::ExpressionMode::monoAftertouch);
    REQUIRE (fake.messages.empty());
}

TEST_CASE ("MpeMidiSink: setGlobalVolume sendet CC7 auf Kanal 1", "[grid]")
{
    grid::FakeMidiTarget fake;
    grid::MpeMidiSink sink (fake);

    sink.setGlobalVolume (1.0f);

    REQUIRE (fake.messages.size() == 1);
    REQUIRE (fake.messages[0].isController());
    REQUIRE (fake.messages[0].getChannel() == 1);
    REQUIRE (fake.messages[0].getControllerNumber() == 7);
    REQUIRE (fake.messages[0].getControllerValue() == 127);
}
