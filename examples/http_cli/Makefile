target=main

$(target): main.o ../../libcube.a
	cd ../..; make
	g++ -o $@ main.o -L../.. -lcube

main.o: main.cpp
	g++ -o $@ -c $< -I. -I../.. -std=c++0x

clean:
	@rm -f *.o $(target)
