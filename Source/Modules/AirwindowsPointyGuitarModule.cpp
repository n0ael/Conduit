#include "AirwindowsPointyGuitarModule.h"

namespace conduit
{

AirwindowsPointyGuitarModule::AirwindowsPointyGuitarModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::PointyGuitar>(), staticModuleId, "PointyGuitar")
{
}

} // namespace conduit
