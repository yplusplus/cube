target=main

$(target): main.o ../../libcube.a
	cd ../..; make
	g++ -o $@ main.o -L../.. -lcube -O2

main.o: main.cpp
	g++ -o $@ -c $< -I. -I../.. -std=c++0x -O2

clean:
	@rm -f *.o $(target)
