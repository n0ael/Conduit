#include <catch2/catch_test_macros.hpp>

#include "Core/NrpnAssembler.h"

using conduit::midi::ControllerEvent;
using conduit::midirig::NrpnAssembler;
using Result = conduit::midirig::NrpnAssembler::Result;

//==============================================================================
TEST_CASE ("NrpnAssembler: volle Sequenz 99/98/6/38 liefert MSB- und 14-bit-Event", "[midirig]")
{
    NrpnAssembler assembler;
    ControllerEvent event;

    REQUIRE (assembler.feed (1, 99, 0, event) == Result::consumed);    // Adresse MSB
    REQUIRE (assembler.feed (1, 98, 70, event) == Result::consumed);   // Adresse LSB

    REQUIRE (assembler.feed (1, 6, 100, event) == Result::event);      // Daten MSB
    REQUIRE (event.kind == ControllerEvent::Kind::nrpn);
    REQUIRE (event.channel == 1);
    REQUIRE (event.number == 70);
    REQUIRE (event.value == 100);
    REQUIRE_FALSE (event.is14Bit);

    REQUIRE (assembler.feed (1, 38, 5, event) == Result::event);       // Daten LSB
    REQUIRE (event.value == (100 << 7) + 5);
    REQUIRE (event.is14Bit);
}

TEST_CASE ("NrpnAssembler: CC6/38 ohne aktive Adresse = passthrough", "[midirig]")
{
    NrpnAssembler assembler;
    ControllerEvent event;

    REQUIRE (assembler.feed (1, 6, 42, event) == Result::passthrough);
    REQUIRE (assembler.feed (1, 38, 42, event) == Result::passthrough);
    REQUIRE (assembler.feed (1, 74, 42, event) == Result::passthrough);   // normale CC
}

TEST_CASE ("NrpnAssembler: RPN-Adresse deaktiviert NRPN (auch Null-Adresse 127/127)", "[midirig]")
{
    NrpnAssembler assembler;
    ControllerEvent event;

    assembler.feed (1, 99, 0, event);
    assembler.feed (1, 98, 70, event);
    REQUIRE (assembler.feed (1, 6, 10, event) == Result::event);

    REQUIRE (assembler.feed (1, 101, 127, event) == Result::consumed);   // RPN/Null
    REQUIRE (assembler.feed (1, 100, 127, event) == Result::consumed);
    REQUIRE (assembler.feed (1, 6, 11, event) == Result::passthrough);   // NRPN inaktiv
}

TEST_CASE ("NrpnAssembler: Kanaele sind isoliert", "[midirig]")
{
    NrpnAssembler assembler;
    ControllerEvent event;

    assembler.feed (1, 99, 1, event);   // Kanal 1: Adresse 1*128+2
    assembler.feed (1, 98, 2, event);
    assembler.feed (5, 99, 3, event);   // Kanal 5: Adresse 3*128+4
    assembler.feed (5, 98, 4, event);

    REQUIRE (assembler.feed (1, 6, 10, event) == Result::event);
    REQUIRE (event.number == 1 * 128 + 2);
    REQUIRE (event.channel == 1);

    REQUIRE (assembler.feed (5, 6, 20, event) == Result::event);
    REQUIRE (event.number == 3 * 128 + 4);
    REQUIRE (event.channel == 5);

    // Kanal 2 hatte nie eine Adresse.
    REQUIRE (assembler.feed (2, 6, 30, event) == Result::passthrough);
}

TEST_CASE ("NrpnAssembler: Adress-Wechsel mitten in der Fahrt", "[midirig]")
{
    NrpnAssembler assembler;
    ControllerEvent event;

    assembler.feed (1, 99, 0, event);
    assembler.feed (1, 98, 12, event);
    REQUIRE (assembler.feed (1, 6, 50, event) == Result::event);
    REQUIRE (event.number == 12);

    assembler.feed (1, 98, 70, event);   // nur LSB wechselt
    REQUIRE (assembler.feed (1, 6, 60, event) == Result::event);
    REQUIRE (event.number == 70);
    REQUIRE (event.value == 60);
}
