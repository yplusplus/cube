target=libcube.a
sources=$(wildcard ./base/*.cpp ./net/*.cpp ./net/http/*.cpp)
objects=$(patsubst %.cpp, %.o, $(sources))
deps=$(patsubst %.cpp, %.cpp.d, $(sources))
CXX=g++
include=-I. 
library=
CXXFLAGS=$(include) -g -std=c++0x -Wall -O2 #-DNDEBUG

all: $(target)

$(target): $(objects)
	ar -cr $(target) $?

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $< 

%.cpp.d: %.cpp
	@set -e; rm -f $@; \
	$(CXX) -MM $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

redis:
	cd net/redis; make

debug:
	@echo $(sources)
	@echo $(deps)
	@echo $(objects)

clean:
	@rm -fr $(objects) $(deps) $(target) $(shell find . -name *.cpp.d.*)

-include $(deps)
