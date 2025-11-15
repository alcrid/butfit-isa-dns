CXX = g++
CXXFLAGS = -g -pedantic -Wall -Wextra -O2
TARGET = dns

.PHONY: all run clean test

all: dns

dns: dns.o dns_filter.o dns_message.o blocklist.o logger.o
	$(CXX) $(CXXFLAGS) -o dns dns.o dns_filter.o dns_message.o blocklist.o logger.o -pthread

dns.o: dns.cpp dns_filter.hpp blocklist.hpp logger.hpp
	$(CXX) $(CXXFLAGS) -c dns.cpp

dns_filter.o: dns_filter.cpp dns_filter.hpp dns_message.hpp blocklist.hpp logger.hpp
	$(CXX) $(CXXFLAGS) -c dns_filter.cpp

dns_message.o: dns_message.cpp dns_message.hpp
	$(CXX) $(CXXFLAGS) -c dns_message.cpp

blocklist.o: blocklist.cpp blocklist.hpp logger.hpp
	$(CXX) $(CXXFLAGS) -c blocklist.cpp

logger.o: logger.cpp logger.hpp
	$(CXX) $(CXXFLAGS) -c logger.cpp

test: dns test-args test-filter

test-args: dns
	@./tests/argument_tests.sh

test-filter: dns
	@./tests/filter_tests.sh

clean:
	rm -f *.o $(TARGET)

run: dns
	./dns -s 8.8.8.8 -p 4400 -f blocked_domains
