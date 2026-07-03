#include "AirwindowsOneCornerClipModule.h"

namespace conduit
{

AirwindowsOneCornerClipModule::AirwindowsOneCornerClipModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::OneCornerClip>(), staticModuleId, "OneCornerClip")
{
}

} // namespace conduit
