target=libcuberedis.a
sources=$(wildcard *.cpp)
#sources=$(shell find . -name *.cpp)
objects=$(patsubst %.cpp, %.o, $(sources))
deps=$(patsubst %.cpp, %.cpp.d, $(sources))
CXX=g++
include=-I. -I../.. -I$(HIREDIS_ROOT)
library=
CXXFLAGS=$(include) -g -std=c++0x -Wall 

all: $(target)

$(target): $(objects)
	ar -cr $(target) $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $< 

%.cpp.d: %.cpp
	@set -e; rm -f $@; \
	$(CXX) -MM $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

debug:
	@echo $(sources)
	@echo $(deps)
	@echo $(objects)

clean:
	@rm -fr $(objects) $(deps) $(target) $(shell find ./ -name *.cpp.d.*)

-include $(deps)
