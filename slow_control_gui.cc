// slow_control_gui.cc
// Receive data from and transmit settings to the server

#include <iostream>
#include <string>
#include <unistd.h>
#include <cstdlib>

#include "sc_network.h"
#include "sc_protobuf.pb.h"

int main(int argc, char *argv[])
{
    // Verify that the version of the Protocol Buffer library we linked against
    // is compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // Parse command line arguments
    if (argc != 2) {
        fprintf(stderr, "usage: slow_control_gui hostname\n");
        return 1;
    }
    std::string hostname = argv[1];
    
    // Set up protocol buffer
    slow_control::Backplane backplane;

    // Set up networking info
    Network_info netinfo(hostname);

    // Connect to the server
    if (!setup_network(netinfo, GUI))
        return 1;

    // Communicate with the server: on each loop receive updated data and 
    // send updated settings
    std::cout << "communicating with the server...\n";
    std::string message;
    float i = 0.0;
    while (true) {
        // Set settings to some numbers
        backplane.set_desired_voltage(i);
        backplane.set_desired_current(i*-1);
        i = i + 0.01;
        if (!backplane.SerializeToString(&message))
            return 1;
        // Send and receive messages
        if (update_network(netinfo, message)) {
            if (!backplane.ParseFromString(netinfo.connections[0].message))
                return 1;
        } else if (!backplane.ParseFromString(message)) {
            return 1;
        }
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

    // Shut down network
    if (!shutdown_network(netinfo))
        return 1;

    return 0;
}
