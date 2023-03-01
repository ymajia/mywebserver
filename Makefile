CXX ?= g++

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2

endif

server: main.cc  ./server/*.cc ./http/*.cc ./threadpool/*.cc ./timer/*.cc
	$(CXX) -o server_test  $^ $(CXXFLAGS) -lpthread -lmysqlclient

debug: main.cc  ./server/*.cc ./http/*.cc ./threadpool/*.cc ./timer/*.cc
	$(CXX) -g -o test.out  $^ $(CXXFLAGS) -lpthread -lmysqlclient


clean:
	rm  -r server
