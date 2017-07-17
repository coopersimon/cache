CXX=clang++

CXXFLAGS= -std=c++11 -Wall

all : cache

cache : mem_sim_cache.cpp mem_sim.cpp cache.hpp
	$(CXX) $(CXXFLAGS) mem_sim.cpp mem_sim_cache.cpp -o cache

debug : mem_sim_cache.cpp mem_sim.cpp cache.hpp
	$(CXX) $(CXXFLAGS) mem_sim.cpp mem_sim_cache.cpp -DDEBUG -o cache_debug
