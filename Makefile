CXXFLAGS += -std=c++11 -Wall -g
LDFLAGS += -lprotobuf

all: library server pi

protoc_middleman: slow_control.proto
	protoc --cpp_out=. slow_control.proto
	@touch protoc_middleman

#target_modules: network.o tm_control.o
#	$(CXX) $(CXXFLAGS) tm_control.o network.o slow_control.pb.cc -o tm_control $(LDFLAGS)

#interface: network.o interface_control.o
#	$(CXX) $(CXXFLAGS) network.o interface_control.o slow_control.pb.cc -o interface_control $(LDFLAGS)

#library: protoc_middleman target_modules interface

server: protoc_middleman server.o network.o run_control.o
	$(CXX) $(CXXFLAGS) server.o network.o run_control.o slow_control.pb.cc -o server $(LDFLAGS) -lmysqlcppconn

pi: protoc_middleman pi.o network.o pi_control.o backplane_spi.o
	$(CXX) $(CXXFLAGS) pi.o network.o pi_control.o backplane_spi.o slow_control.pb.cc -o pi $(LDFLAGS) -lbcm2835

clean:
	rm -f server pi
	rm -f server.o pi.o
	rm -f network.o backplane_spi.o
	rm -f interface_control.o run_control.o pi_control.o tm_control.o
	rm -f protoc_middleman slow_control.pb.cc slow_control.pb.h
