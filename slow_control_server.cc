// slow_control_server.cc
// Receive data from the Raspberry Pi and transmit to the GUI

#include <iostream>
#include <string>

#include "sc_network.h"
#include "sc_protobuf.pb.h"

int main(void)
{
    // Verify that the version of the library we linked against is compatible
    // with the version of the headers we compiled against
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // Set up protocol buffer
    slow_control::Backplane backplane;
    
    // Set up networking info
    Network_info netinfo;
    
    // Connect to the Pi and GUI
    if (!(setup_network(netinfo, SERVER)))
        return 1;
    
    // Shut down network
    if (!shutdown_network(netinfo))
        return 1;

//    // Communicate with the Pi: on each loop receive updated data and 
//    // send updated settings
//    std::cout << "communicating with the Pi...\n";
//    std::string message;
//    srand(12345); // Initialize random seed
//    for (int i = 0; i < 10; i++) {
//        std::cout << "Iteration " << i << std::endl;
//        // Set data to those sent by Pi
//        if (!receive_message(pi, message)) {
//            // End communication to the Pi when no more messages
//            std::cout << "breaking the connection.\n";
//            close_connection(pi);
//            return 1;
//        }
//        if (!backplane.ParseFromString(message))
//            return 1;
//        // Set settings to random numbers
//        backplane.set_desired_voltage(rand() % 10);
//        backplane.set_desired_current(rand() % 10);
//        if (!backplane.SerializeToString(&message))
//            return 1;
//        if (!send_message(pi, message))
//            return 1;
//        // Display updated values
//        std::cout << "updated voltage is: " << backplane.voltage() 
//            << std::endl;
//        std::cout << "updated current is: " << backplane.current() 
//            << std::endl;
//        std::cout << "updated desired voltage is: "
//            << backplane.desired_voltage() << std::endl;
//        std::cout << "updated desired current is: "
//            << backplane.desired_current() << std::endl;
//    }
    
    return 0;
}
