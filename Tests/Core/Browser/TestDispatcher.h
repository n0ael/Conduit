#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "Core/Browser/BrowserSearchIndex.h"

namespace conduit::test
{

//==============================================================================
/**
    Test-Dispatcher für BrowserSearchIndex/-FileScanner: sammelt die vom
    Pool-Thread dispatchten Ergebnis-Lambdas in einer Queue, die der Test
    auf dem (Test-)Message-Thread selbst pumpt — runDispatchLoopUntil
    existiert mit JUCE_MODAL_LOOPS_PERMITTED=0 nicht.

    Lebensdauer (TSan-Fund CI 04.07.2026): das fn()-Lambda hält den
    Queue-Zustand als shared_ptr — ein Pool-Job, der NACH der Zerstörung
    des Rig-Dispatchers (aber vor dem ThreadPool-Join) noch dispatcht,
    schreibt dann in einen überlebenden State statt in freigegebenen
    Speicher. Unabhängig von der Deklarationsreihenfolge im Rig sicher.
*/
struct QueueDispatcher
{
    [[nodiscard]] BrowserSearchIndex::Dispatcher fn()
    {
        return [sharedState = state] (std::function<void()> task)
        {
            const std::lock_guard<std::mutex> lock (sharedState->mutex);
            sharedState->queue.push_back (std::move (task));
        };
    }

    /** Pumpt die Queue, bis die Bedingung erfüllt ist (max. timeoutMs). */
    bool pumpUntil (const std::function<bool()>& done, int timeoutMs = 2000)
    {
        const auto deadline = juce::Time::getMillisecondCounterHiRes() + timeoutMs;

        while (juce::Time::getMillisecondCounterHiRes() < deadline)
        {
            std::vector<std::function<void()>> pending;
            {
                const std::lock_guard<std::mutex> lock (state->mutex);
                pending.swap (state->queue);
            }

            for (auto& task : pending)
                task();

            if (done())
                return true;

            juce::Thread::sleep (5);
        }

        return done();
    }

private:
    struct State
    {
        std::mutex mutex;
        std::vector<std::function<void()>> queue;
    };

    std::shared_ptr<State> state = std::make_shared<State>();
};

} // namespace conduit::test
