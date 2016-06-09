CXXFLAGS += -Wall -g
LDFLAGS += -lprotobuf

all: slow_control_pi slow_control_server

protoc_middleman: sc_protobuf.proto
	protoc --cpp_out=. sc_protobuf.proto
	@touch protoc_middleman

slow_control_pi: protoc_middleman slow_control_pi.o sc_network.o
	$(CXX) $(CXXFLAGS) slow_control_pi.o sc_network.o sc_protobuf.pb.cc -o slow_control_pi $(LDFLAGS)

slow_control_server: protoc_middleman slow_control_server.cc sc_network.o 
	$(CXX) $(CXXFLAGS) slow_control_server.cc sc_network.o sc_protobuf.pb.cc -o slow_control_server $(LDFLAGS)

clean:
	rm -f slow_control_pi slow_control_server
	rm -f slow_control_pi.o slow_control_server.o
	rm -f sc_network.o
	rm -f protoc_middleman sc_protobuf.pb.cc sc_protobuf.pb.h
