// runcontrol.cc
// Implementation of class for high-level run control
// Lower level programs inherit from this, adding their specific functionality

#include "runcontrol.h"
#include "sc_protobuf.pb.h"

RunControl::RunControl()
{
    // Verify that the version of the Protocol Buffer library we linked against
    // is compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    
    run_settings.set_highlevel_command("");
    run_settings.set_highlevel_parameter("");
}
