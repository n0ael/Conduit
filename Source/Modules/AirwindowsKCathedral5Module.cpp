#include "AirwindowsKCathedral5Module.h"

namespace conduit
{

AirwindowsKCathedral5Module::AirwindowsKCathedral5Module()
    : AirwindowsProcessorModule (std::make_unique<airwindows::KCathedral5>(), staticModuleId, "kCathedral5")
{
}

} // namespace conduit
