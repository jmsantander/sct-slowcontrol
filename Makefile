CXX = g++
CXXFLAGS += -std=c++11 -Wall -g -fPIC
SWIG = swig
SWIGFLAGS = -c++ -python
PYTHONFLAGS = -I/usr/include/python2.7
LDFLAGS += -lprotobuf

all: library server pi

protoc_middleman: slow_control.proto
	protoc --cpp_out=. slow_control.proto
	@touch protoc_middleman

swig: slow_control.i
	$(SWIG) $(SWIGFLAGS) slow_control.i

library: protoc_middleman swig interface_control.o tm_control.o network.o
	$(CXX) $(CXXFLAGS) -shared slow_control_wrap.cxx interface_control.o tm_control.o network.o slow_control.pb.cc -o _slow_control.so $(PYTHONFLAGS) $(LDFLAGS)

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
	rm -f slow_control.py slow_control.pyc
	rm -f slow_control_wrap.cxx _slow_control.so
