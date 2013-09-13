CFLAGS = -g -O0 -std=c++11
CPPFLAGS = -Ithird_party/boost -Ithird_party/boost/libs/system/src
LDFLAGS = 

all: test_client

boost_system_dist: error_code.o

error_code.o: third_party/boost/libs/system/src/error_code.cpp
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c third_party/boost/libs/system/src/error_code.cpp

test_client: test_client.o boost_system_dist
	$(CXX) $(CFLAGS) $(LDFLAGS) -o test_client test_client.o error_code.o -lpthread

test_client.o: test_client.cc
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c test_client.cc

clean:
	rm -rf *.o test_client

