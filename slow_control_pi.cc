// slow_control_pi.cc
/* Listen for and execute remote user commands for the backplane. Continually
 * send out a status report back to the user for display by the GUI. */

#include <iostream>
#include <string>
#include <unistd.h>

#include "sc_network.h"
#include "sc_protobuf.pb.h"

int main(void)
{
    // Verify that the version of the Protocol Buffer library we linked against
    // is compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // Set up protocol buffer
    slow_control::Backplane backplane;

    // Connect to the server
    std::cout << "connecting to the server...\n";
    int server = -1;
    if (!open_connection(server))
        return 1;

    // Send some messages to the server
    std::cout << "sending messages to the server...\n";
    std::string message;
    for (int i = 0; i < 10; i++) {
        backplane.set_voltage(i);
        backplane.set_current(i*2);
        if (!backplane.SerializeToString(&message))
            return 1;
        if (!send_message(server, message))
            return 1;
        sleep(1);
    }

    // End communication to the server
    std::cout << "breaking the connection.\n";
    close_connection(server);

    return 0;
}
