CC = g++
CFLAGS = -g -pedantic -Wall -Wextra -O2 

.PHONY: all run clean

dns: dns.cpp dns.h
	$(CC) $(CFLAGS) -o dns dns.cpp