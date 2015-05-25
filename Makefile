myRouter: myrouter.o
	g++ -o myrouter myrouter.o

myRouter.o: myrouter.cpp
	g++ -c -o myrouter.o myrouter.cpp
