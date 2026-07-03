#include "AirwindowsTapeHack2Module.h"

namespace conduit
{

AirwindowsTapeHack2Module::AirwindowsTapeHack2Module()
    : AirwindowsProcessorModule (std::make_unique<airwindows::TapeHack2>(), staticModuleId, "TapeHack2")
{
}

} // namespace conduit
