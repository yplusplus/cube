target=main
cppflags=-I. -I../../ -I$(HIREDIS_ROOT) -g

$(target): main.o ../../net/redis/libcuberedis.a ../../libcube.a $(HIREDIS_ROOT)/libhiredis.a
	cd ../..; make
	cd ../../net/redis; make
	g++ -o $@ $^

%.o: %.cpp
	g++ -o $@ -c $< $(cppflags) -std=c++0x

clean:
	@rm -f *.o $(target)
