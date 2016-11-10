pong: pong.o ../../libcube.a
	cd ../..; make
	g++ -o $@ $< -L../.. -lcube

%.o: %.cpp
	g++ -o $@ -c $< -I. -I../.. -std=c++0x

clean:
	@rm -f *.o ping pong
