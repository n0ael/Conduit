#include "AirwindowsKWoodRoomModule.h"

namespace conduit
{

AirwindowsKWoodRoomModule::AirwindowsKWoodRoomModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::KWoodRoom>(), staticModuleId, "kWoodRoom")
{
}

} // namespace conduit
