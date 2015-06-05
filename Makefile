myrouter: myrouter.o manager.o comm_link.o
	g++ -g comm_link.o manager.o myrouter.o -o myrouter
	
comm_link.o: comm_link.cpp comm_link.h
	g++ -g -c comm_link.cpp  

manager.o: manager.cpp manager.h comm_link.h
	g++ -g -c manager.cpp

myrouter.o: myrouter.cpp comm_link.h manager.h
	g++ -g -c myrouter.cpp

