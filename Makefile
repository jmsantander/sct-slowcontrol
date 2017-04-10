CXXFLAGS += -std=c++11 -Wall -g
LDFLAGS += -lprotobuf

all: slow_control_pi slow_control_server slow_control_interface slow_control_tm

protoc_middleman: sc_protobuf.proto
	protoc --cpp_out=. sc_protobuf.proto
	@touch protoc_middleman

slow_control_pi: protoc_middleman slow_control_pi.o sc_network.o picontrol.o backplane_lowlevel.o
	$(CXX) $(CXXFLAGS) slow_control_pi.o sc_network.o picontrol.o backplane_lowlevel.o sc_protobuf.pb.cc -o slow_control_pi $(LDFLAGS) -lbcm2835

slow_control_tm: protoc_middleman slow_control_tm.o sc_network.o tmcontrol.o runcontrol.o
	$(CXX) $(CXXFLAGS) slow_control_tm.o sc_network.o tmcontrol.o runcontrol.o sc_protobuf.pb.cc -o slow_control_tm $(LDFLAGS)

slow_control_server: protoc_middleman slow_control_server.cc sc_network.o servercontrol.o
	$(CXX) $(CXXFLAGS) slow_control_server.cc sc_network.o servercontrol.o sc_protobuf.pb.cc -o slow_control_server $(LDFLAGS) -lmysqlcppconn

slow_control_interface: protoc_middleman slow_control_interface.o sc_network.o interfacecontrol.o runcontrol.o
	$(CXX) $(CXXFLAGS) slow_control_interface.o sc_network.o interfacecontrol.o runcontrol.o sc_protobuf.pb.cc -o slow_control_interface $(LDFLAGS)

clean:
	rm -f slow_control_pi slow_control_server slow_control_interface slow_control_tm
	rm -f slow_control_pi.o slow_control_server.o slow_control_interface.o slow_control_tm.o
	rm -f sc_network.o backplane_lowlevel.o
	rm -f interfacecontrol.o servercontrol.o picontrol.o tmcontrol.o
	rm -f protoc_middleman sc_protobuf.pb.cc sc_protobuf.pb.h
