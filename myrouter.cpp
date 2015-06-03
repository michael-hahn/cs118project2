#include <iostream>
#include "comm_link.h"
#include "manager.h"
#include <fstream>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <cstring>


using namespace std;

int find_port_number (char * port_name);
int print_data_info_before_sending (string msg, int dest_port_number, char* dest_node_name);
//for now: the user provides the "name of the router" {A,B,C,D,E,F}, we gotta figure out officially, how a router is supposed to know who s/he is
int main(int argc, char**argv)
{
	if(argc < 2)
	{
		cout << "Missing router name!" << endl;
		return 1;
	}

	if(strcmp(argv[1],"data") == 0)
	{
		if(argc < 4)
		{
			cout << "Missing router names!" << endl;
			return 1;
		}
		else
		{
			int my_socketfd, port_numbr;
			port_numbr = find_port_number(argv[2]);
		    my_socketfd=socket(AF_INET, SOCK_DGRAM,0);
		    if(my_socketfd < 0)
		    {
		        perror("socket() failed: ");
		        return -1;
		    }
			struct sockaddr_in my_side;
			bzero(&my_side, sizeof(my_side));
		    my_side.sin_family = AF_INET;
		    my_side.sin_addr.s_addr = inet_addr("127.0.0.1"); //since we will be talking on the local host
		    my_side.sin_port = 0;//htons(0);
		    if(bind(my_socketfd,(struct sockaddr *)&(my_side),sizeof(my_side)) < 0)
		    {
		        perror("bind() failed: ");
		        return -1;
		    }
			comm_link cl = comm_link(argv[2], 8008, port_numbr);
			vector<string> no_use;
			string the_message;
			the_message += '$';
			int length_of_argument = strlen(argv[3]);
			for(int j = 0; j < length_of_argument; ++j)
			{
				the_message += argv[3][j];
			}
			the_message += " Here is the payload! Taa-daa!";
			int rv = print_data_info_before_sending(the_message, port_numbr, argv[3]);
			if (rv < 0)
			{
				cerr << "Printing data info before sending is not working!" << endl;
			}
			cl.send_distance_vector(the_message.c_str(), my_socketfd, no_use);
		}
	}
	else
	{
	    manager m = manager("./initialize.txt", argv[1]);
	    m.print_out();
	    cout << "The manager will now try to make the routers communicate..." << endl << endl;
	    m.communicate();
	}
    return 0;
}

int find_port_number (char* port_name)
{
	ifstream inputfile;
    char c;
    string line;
    inputfile.open("./initialize.txt"); //open the file + error checking
    if(!inputfile)
    {
        cout << "An error occurered while attempting to open the file\n";
        exit(1);
    }

    while(inputfile.get(c)) //grab a character from the file
    {
        //parse here
        while(true)
        {
            line = line + c;
            if(!inputfile.get(c))   //if the end of the file was reached
            {
                break;  //this signifies the end of the line
            }
            if(c == '\n')   //if a new line was reached
            {
                line = line + c;    //this signifies the end of the line
                break;  //enough collecting characters
            }
        }

        //parse the string
        int length_of_line = (int)line.length();
        char *input = new char[length_of_line+1];
        strcpy(input,line.c_str());
        input[length_of_line+1] = '\0';
        char source_name[16];
        char destination_name[16];
        char s_source_port[16];
        char s_cost[16];
        int source_port;
        // cout << "here -> " << input ;
        sscanf (input, "%8[^,],%8[^,],%8[^,],%8[^,]", source_name, destination_name, s_source_port, s_cost);
        source_port = atoi(s_source_port);
        line = "";
        if (strcmp(destination_name, port_name) == 0)
        {
        	return source_port;
        }
    }
}

int print_data_info_before_sending (string msg, int dest_port_number, char* dest_node_name)
{
    ofstream file_to_print ("router-outputH.txt", ios::out | ios::app);
    string s_dest_node_name;
    int dnn_size = strlen(dest_node_name);
    for (int j = 0; j < dnn_size; ++j)
    {
    	s_dest_node_name += dest_node_name[j];
    }
    if(file_to_print.is_open())
    {
    	file_to_print << "Destination port number: " << dest_port_number << endl;
    	file_to_print << "Destination node name: " << s_dest_node_name << endl;
    	file_to_print << "Data payload: " << endl;
    	file_to_print << msg << endl;
    }
    else 
    {
    	return -1;
    }
    return 0;
}
