#include "AirwindowsTremoSquareModule.h"

namespace conduit
{

AirwindowsTremoSquareModule::AirwindowsTremoSquareModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::TremoSquare>(), staticModuleId, "TremoSquare")
{
}

} // namespace conduit
