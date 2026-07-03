#include "AirwindowsDigitalBlackModule.h"

namespace conduit
{

AirwindowsDigitalBlackModule::AirwindowsDigitalBlackModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::DigitalBlack>(), staticModuleId, "DigitalBlack")
{
}

} // namespace conduit
