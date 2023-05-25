# 046267 Computer Architecture - Winter 20/21 - HW #2
CXX = g++
CC = gcc
CFLAGS = -Wall -c -g -fpic 
CXXFLAGS = -Wall -g -fpic
OBJS = cache.o
EXEC = cacheSim.exe

 
cacheSim.o: $(OBJS) 
	$(CXX) $(CXXFLAGS) cacheSim.cpp -o cacheSim

cache.o: cache.c
	$(CC) $(CFLAGS) -c cache.c



.PHONY: clean
clean:
	rm -f *.o
	rm -f cacheSim