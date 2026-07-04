#include <catch2/catch_test_macros.hpp>

#include "Core/Browser/BrowserContextProvider.h"

namespace
{
using Provider = conduit::BrowserContextProvider;
using Section  = Provider::Section;

constexpr int pageGrid   = 0;
constexpr int pageMixer  = 1;
constexpr int pageClip   = 2;
constexpr int pageDevice = 3;
constexpr int pageLooper = 4;
} // namespace

TEST_CASE ("Browser-Kontext: MODULE nur auf der Device-Page sichtbar", "[browser]")
{
    Provider provider;

    // Default ist die Device-Page
    REQUIRE (provider.getActivePage() == pageDevice);
    REQUIRE (provider.isSectionVisible (Section::modules));

    for (const auto page : { pageGrid, pageMixer, pageClip, pageLooper })
    {
        provider.setActivePage (page);
        INFO ("Page " << page);
        REQUIRE_FALSE (provider.isSectionVisible (Section::modules));

        // PROJEKTE und AUDIO sind global sichtbar
        REQUIRE (provider.isSectionVisible (Section::projects));
        REQUIRE (provider.isSectionVisible (Section::audio));
    }

    provider.setActivePage (pageDevice);
    REQUIRE (provider.isSectionVisible (Section::modules));
}

TEST_CASE ("Browser-Kontext: Startbereich pro Page", "[browser]")
{
    Provider provider;

    provider.setActivePage (pageDevice);
    REQUIRE (provider.startSection() == Section::modules);

    for (const auto page : { pageGrid, pageMixer, pageClip, pageLooper })
    {
        provider.setActivePage (page);
        REQUIRE (provider.startSection() == Section::projects);
    }
}

TEST_CASE ("Browser-Kontext: onContextChanged feuert nur bei echtem Wechsel", "[browser]")
{
    Provider provider;

    int fired = 0;
    provider.onContextChanged = [&fired] { ++fired; };

    provider.setActivePage (pageDevice);   // schon aktiv — kein Broadcast
    REQUIRE (fired == 0);

    provider.setActivePage (pageMixer);
    REQUIRE (fired == 1);

    provider.setActivePage (pageMixer);    // unverändert
    REQUIRE (fired == 1);

    provider.setActivePage (pageDevice);
    REQUIRE (fired == 2);
}
