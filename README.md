# Slow control software repository for the SCT prototype

## Description

This software implements slow control for the CTA pSCT camera. Three programs run simultaneously. One runs on the Raspberry Pi, connecting to the backplane, sending SPI commands, and receiving SPI data. A user interface program runs on any remote machine (e.g. the server computer). A server program runs on the server computer, handles networking between the previous programs, and logs slow control data to a database.

This setup permits continuous slow control monitoring (in principle), allows for multiple users to connect simultaneously, and simplifies maintenance by centralizing logging.

Primary author: Ari Brill, aryeh.brill@columbia.edu

Contributing authors: Marcos Santander, Ori Weiner

This project makes use of code by Phil Moore, Richard Bose, and Jim Buckley.

## Dependencies

- [Protocol Buffers](https://github.com/google/protobuf)
- [C library for Broadcom BCM 2835](http://www.airspayce.com/mikem/bcm2835/)
- [MySQL Connector/C++](https://dev.mysql.com/doc/connector-cpp/en/)

To install the Protocol Buffers and BCM 2835 libraries, follow the instructions at the linked pages. Both are required on all computers running any of the slow control programs.

MySQL Connector is only required on the computer running the server program. It has several dependencies:

- MySQL client library
- CMake 2.6.2
- Boost 1.56.0

To install them on Ubuntu, run:

```
sudo apt-get install libmysqlclient-dev cmake libboost-all-dev
```

To install the MySQL client library on Red Hat, run:

```  
sudo yum install mysql-devel
```

## Compilation

In the directory containing the slow control code on the server computer, run `make slow_control_server`.

In the directory containing the slow control code on the Raspberry Pi, run `make slow_control_pi`.

In the directory containing the slow control code where the user interface will be used, run `make slow_control_interface`.

To compile all code, run `make`, to recompile run `make all`, and to remove all compiled code, run `make clean`.

## Use

First, on the server computer, run `./slow_control_server [db_host] [db_username] [db_password]`. The database must have been set up previously.

Next, on the Pi, run `./slow_control_pi [hostname]` where [hostname] is the host address where the server program is running.

To open and use the user interface, run `./slow_control_interface [hostname]` where [hostname] is again the host address for the server program, and enter commands on the command line.

## Available Commands

- c: Monitor trigger rate 
- d: Set trigger at time
- g: Enable or disable trigger/TACK
- i: Read FEE currents
- j: Set trigger mask
- l: Reset trigger counter and timer
- m: Display menu
- n: Power on/off modules
- o: Set holdoff time
- p: Read FEEs present
- s: Send sync command
- v: Read FEE voltages
- x: Exit user interface
- z: Set TACK type and mode
