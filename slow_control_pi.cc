// slow_control_pi.cc
/* Listen for and execute remote user commands for the backplane. Continually
 * send out a status report back to the user for display by the GUI. */

#include <iostream>
#include <string>
#include <unistd.h>
#include <cstdlib>

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

    // Communicate with the server: on each loop send updated data and 
    // recieve updated settings
    std::cout << "communicating with the server...\n";
    std::string message;
    srand(54321); // Initialize random seed
    for (int i = 0; i < 10; i++) {
        std::cout << "Iteration " << i << std::endl;
        // Set data to random numbers
        backplane.set_voltage(rand() % 10);
        backplane.set_current(rand() % 10);
        if (!backplane.SerializeToString(&message))
            return 1;
        if (!send_message(server, message))
            return 1;
        sleep(1);
        // Set settings to those sent by server
        if (!receive_message(server, message)) {
            // End communication to the server when no more messages
            std::cout << "breaking the connection.\n";
            close_connection(server);
            return 1;
        }
        if (!backplane.ParseFromString(message))
            return 1;
        // Display updated values
        std::cout << "updated voltage is: " << backplane.voltage() 
            << std::endl;
        std::cout << "updated current is: " << backplane.current() 
            << std::endl;
        std::cout << "updated desired voltage is: "
            << backplane.desired_voltage() << std::endl;
        std::cout << "updated desired current is: "
            << backplane.desired_current() << std::endl;
    }


    // End communication to the server
    std::cout << "breaking the connection.\n";
    close_connection(server);

    return 0;
}
