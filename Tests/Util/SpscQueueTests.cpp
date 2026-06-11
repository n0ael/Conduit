#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "Util/SpscQueue.h"

// Pflicht-Unit-Tests vor Integration (CLAUDE.md 13.4). Der Threading-Test
// ist primär für die Sanitizer-Presets (TSan: cmake --preset tsan) gedacht,
// läuft aber auch im normalen Build als Smoke-Test.

//==============================================================================
TEST_CASE ("SpscQueue: FIFO-Semantik und Voll-/Leer-Verhalten", "[spsc]")
{
    conduit::SpscQueue<int> queue (4);

    SECTION ("pop auf leerer Queue schlägt fehl und lässt result unverändert")
    {
        int result = 42;
        REQUIRE_FALSE (queue.pop (result));
        REQUIRE (result == 42);
        REQUIRE (queue.getNumReady() == 0);
    }

    SECTION ("Elemente kommen in Push-Reihenfolge heraus")
    {
        for (int i = 0; i < 4; ++i)
            REQUIRE (queue.push (i));

        for (int expected = 0; expected < 4; ++expected)
        {
            int result = -1;
            REQUIRE (queue.pop (result));
            REQUIRE (result == expected);
        }
    }

    SECTION ("push auf voller Queue schlägt fehl und überschreibt nichts")
    {
        for (int i = 0; i < 4; ++i)
            REQUIRE (queue.push (i));

        REQUIRE (queue.getNumFree() == 0);
        REQUIRE_FALSE (queue.push (99));

        int result = -1;
        REQUIRE (queue.pop (result));
        REQUIRE (result == 0);          // 99 hat Element 0 nicht verdrängt
        REQUIRE (queue.getNumFree() == 1);  // genau ein Slot wieder frei
        REQUIRE (queue.push (4));
        REQUIRE (queue.getNumReady() == 4);
    }
}

//==============================================================================
TEST_CASE ("SpscQueue: Wrap-Around über die Index-Maske", "[spsc]")
{
    conduit::SpscQueue<int> queue (4);

    // Mehrere hundert Umläufe — die monotonen Indizes laufen weit über die
    // Kapazität hinaus, die Maske muss immer den richtigen Slot treffen.
    for (int i = 0; i < 1000; ++i)
    {
        REQUIRE (queue.push (i));
        REQUIRE (queue.push (i + 1'000'000));

        int first = -1, second = -1;
        REQUIRE (queue.pop (first));
        REQUIRE (queue.pop (second));
        REQUIRE (first == i);
        REQUIRE (second == i + 1'000'000);
    }

    REQUIRE (queue.getNumReady() == 0);
}

//==============================================================================
TEST_CASE ("SpscQueue: Kapazität wird auf Zweierpotenz aufgerundet", "[spsc]")
{
    REQUIRE (conduit::SpscQueue<int> (5).getCapacity() == 8);
    REQUIRE (conduit::SpscQueue<int> (8).getCapacity() == 8);
    REQUIRE (conduit::SpscQueue<int> (1).getCapacity() == 1);
    REQUIRE (conduit::SpscQueue<int> (0).getCapacity() == 1);   // defensiv geclamped
    REQUIRE (conduit::SpscQueue<int> (100).getCapacity() == 128);
}

//==============================================================================
TEST_CASE ("SpscQueue: Producer und Consumer auf getrennten Threads", "[spsc][threading]")
{
    // Payload größer als ein Register, damit ein Race im Slot-Zugriff als
    // zerrissene (inkonsistente) Message sichtbar würde
    struct Message
    {
        int sequence;
        float value;
    };

    constexpr int messageCount = 100'000;
    constexpr int maxRetries   = 10'000'000;  // begrenztes Warten — hängt nie

    conduit::SpscQueue<Message> queue (64);
    std::atomic<bool> producerGaveUp { false };

    // Producer (simulierter Netzwerk-Thread, OSC-Takt 6.1)
    std::thread producer ([&queue, &producerGaveUp]
    {
        for (int i = 0; i < messageCount; ++i)
        {
            const Message message { i, static_cast<float> (i) * 0.25f };

            int retries = 0;
            while (! queue.push (message))
            {
                if (++retries > maxRetries)
                {
                    producerGaveUp.store (true, std::memory_order_release);
                    return;
                }

                std::this_thread::yield();
            }
        }
    });

    // Consumer (simulierter Audio Thread): strikte Reihenfolge, kein Verlust,
    // kein Duplikat, konsistenter Payload
    int  expectedSequence = 0;
    bool payloadIntact    = true;
    int  retries          = 0;

    while (expectedSequence < messageCount && retries <= maxRetries)
    {
        Message received {};

        if (! queue.pop (received))
        {
            ++retries;
            std::this_thread::yield();
            continue;
        }

        if (received.sequence != expectedSequence
            || received.value != static_cast<float> (received.sequence) * 0.25f)
        {
            payloadIntact = false;
            break;
        }

        ++expectedSequence;
    }

    producer.join();

    REQUIRE_FALSE (producerGaveUp.load());
    REQUIRE (payloadIntact);
    REQUIRE (expectedSequence == messageCount);  // alle Messages, exakt einmal
    REQUIRE (queue.getNumReady() == 0);
}
