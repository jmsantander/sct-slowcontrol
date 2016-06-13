// slow_control_server.cc
// Receive data from the Raspberry Pi and transmit to the GUI

#include <iostream>
#include <string>

#include "sc_network.h"
#include "sc_protobuf.pb.h"

int main(int argc, char *argv[])
{
    // Verify that the version of the library we linked against is compatible
    // with the version of the headers we compiled against
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // Parse command line arguments
    if (argc != 2) {
        fprintf(stderr, "usage: slow_control_server hostname\n");
        return 1;
    }
    char *hostname = argv[1];

    // Set up protocol buffer
    slow_control::Backplane backplane;
    
    // Connect to the Pi
    std::cout << "connecting to the pi...\n";
    int pi = -1; 
    if (!open_connection(pi, hostname))
        return 1;

    // Communicate with the Pi: on each loop receive updated data and 
    // send updated settings
    std::cout << "communicating with the Pi...\n";
    std::string message;
    srand(12345); // Initialize random seed
    for (int i = 0; i < 10; i++) {
        std::cout << "Iteration " << i << std::endl;
        // Set data to those sent by Pi
        if (!receive_message(pi, message)) {
            // End communication to the Pi when no more messages
            std::cout << "breaking the connection.\n";
            close_connection(pi);
            return 1;
        }
        if (!backplane.ParseFromString(message))
            return 1;
        // Set settings to random numbers
        backplane.set_desired_voltage(rand() % 10);
        backplane.set_desired_current(rand() % 10);
        if (!backplane.SerializeToString(&message))
            return 1;
        if (!send_message(pi, message))
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
    
    // End communication to the Pi
    std::cout << "breaking the connection.\n";
    close_connection(pi);
    
    return 0;
}
