#include "AirwindowsTapeDustModule.h"

namespace conduit
{

AirwindowsTapeDustModule::AirwindowsTapeDustModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::TapeDust>(), staticModuleId, "TapeDust")
{
}

} // namespace conduit
