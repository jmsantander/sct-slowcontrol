CXXFLAGS += -Wall -g
LDFLAGS += -lprotobuf -lbcm2835

all: slow_control_pi slow_control_server slow_control_interface

protoc_middleman: sc_protobuf.proto
	protoc --cpp_out=. sc_protobuf.proto
	@touch protoc_middleman

slow_control_pi: protoc_middleman slow_control_pi.o sc_network.o sc_backplane.o sc_lowlevel.o sc_logistics.o
	$(CXX) $(CXXFLAGS) slow_control_pi.o sc_network.o sc_backplane.o sc_lowlevel.o sc_logistics.o sc_protobuf.pb.cc -o slow_control_pi $(LDFLAGS)

slow_control_server: protoc_middleman slow_control_server.cc sc_network.o sc_backplane.o sc_lowlevel.o sc_logistics.o
	$(CXX) $(CXXFLAGS) slow_control_server.cc sc_network.o sc_backplane.o sc_lowlevel.o sc_logistics.o sc_protobuf.pb.cc -o slow_control_server $(LDFLAGS) -lmysqlcppconn

slow_control_interface: protoc_middleman slow_control_interface.o sc_network.o sc_backplane.o sc_lowlevel.o sc_logistics.o
	$(CXX) $(CXXFLAGS) slow_control_interface.o sc_network.o sc_backplane.o sc_lowlevel.o sc_logistics.o sc_protobuf.pb.cc -o slow_control_interface $(LDFLAGS)

clean:
	rm -f slow_control_pi slow_control_server slow_control_interface
	rm -f slow_control_pi.o slow_control_server.o slow_control_interface.o
	rm -f sc_network.o sc_backplane.o sc_logistics.o sc_lowlevel.o
	rm -f protoc_middleman sc_protobuf.pb.cc sc_protobuf.pb.h
