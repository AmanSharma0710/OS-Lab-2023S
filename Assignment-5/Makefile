main: main.cpp cleaning.o guest.o header.h
	g++ -o main main.cpp guest.o cleaning.o -lpthread -lrt

guest.o: guest.cpp header.h
	g++ -o guest.o -c guest.cpp -lpthread -lrt

cleaning.o: cleaning.cpp header.h
	g++ -o cleaning.o -c cleaning.cpp -lpthread -lrt

clean:
	rm -f *.o main
	