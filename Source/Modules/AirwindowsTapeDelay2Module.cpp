#include "AirwindowsTapeDelay2Module.h"

namespace conduit
{

AirwindowsTapeDelay2Module::AirwindowsTapeDelay2Module()
    : AirwindowsProcessorModule (std::make_unique<airwindows::TapeDelay2>(), staticModuleId, "TapeDelay2")
{
}

} // namespace conduit
