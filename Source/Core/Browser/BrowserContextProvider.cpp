#include "BrowserContextProvider.h"

namespace conduit
{

namespace
{
    constexpr int pageDevice = 3;  // == TransportBar::pageDevice
} // namespace

void BrowserContextProvider::setActivePage (int pageIndex)
{
    if (activePage == pageIndex)
        return;

    activePage = pageIndex;

    if (onContextChanged != nullptr)
        onContextChanged();
}

bool BrowserContextProvider::isSectionVisible (Section section) const
{
    switch (section)
    {
        case Section::projects:
        case Section::audio:
            return true;

        case Section::modules:
            return activePage == pageDevice;
    }

    jassertfalse;
    return false;
}

BrowserContextProvider::Section BrowserContextProvider::startSection() const
{
    return activePage == pageDevice ? Section::modules : Section::projects;
}

} // namespace conduit
