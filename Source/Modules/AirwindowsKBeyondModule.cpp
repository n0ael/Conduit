#include "AirwindowsKBeyondModule.h"

namespace conduit
{

AirwindowsKBeyondModule::AirwindowsKBeyondModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::KBeyond>(), staticModuleId, "kBeyond")
{
}

} // namespace conduit
