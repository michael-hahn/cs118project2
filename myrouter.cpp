#include <iostream>
#include "comm_link.h"
#include "manager.h"


using namespace std;

//for now: the user provides the "name of the router" {A,B,C,D,E,F}, we gotta figure out officially, how a router is supposed to know who s/he is
int main(int argc, char**argv)
{
	if(argc < 2)
	{
		cout << "Missing router name!" << endl;
		return 1;
	}
    manager m = manager("./initialize.txt", argv[1]);
    m.print_out();
    cout << "The manager will now try to make the routers communicate..." << endl << endl;
    m.communicate();
    return 0;
}
