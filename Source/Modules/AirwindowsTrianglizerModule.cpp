#include "AirwindowsTrianglizerModule.h"

namespace conduit
{

AirwindowsTrianglizerModule::AirwindowsTrianglizerModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Trianglizer>(), staticModuleId, "Trianglizer")
{
}

} // namespace conduit
