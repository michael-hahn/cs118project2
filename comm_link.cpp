#include "comm_link.h"
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <iostream>

using namespace std;


//constructor
comm_link::comm_link(char * n, int c, int neighbors_prt)
{
    //save the name
    for(int i = 0; i < strlen(n); i++)
    {
        this->name += n[i];
    }
    
    //save the cost of the link
    this->cost = c;
    
    //store the port numbers assigned to each participant in the communication link
    this->neighbors_port = neighbors_prt;

    //next the neighbor
    bzero(&this->neighbors_side, sizeof(this->neighbors_side));
    this->neighbors_side.sin_family = AF_INET;
    this->neighbors_side.sin_addr.s_addr = inet_addr("127.0.0.1"); //since we will be talking on the local host
                                                                    //, just hard code it?
    this->neighbors_side.sin_port = htons(this->neighbors_port);
}

//setters
void comm_link::set_neighbors_port(int port)
{
    this->neighbors_port = port;
    this->neighbors_side.sin_port = htons(port);
}

//getters
string comm_link::get_name() {
    return this->name;
}

int comm_link::get_cost() {
    return this->cost;
}

//udp comms
int comm_link::send_distance_vector(const char * msg, int socketfd, vector<string> table_sent) 
{
    cout << this->name << " wants to send a vector...\n";
    int ts_length = table_sent.size();
    for (int i = 0; i < ts_length; ++i)
    {
        if(table_sent[i] == this->name)
        {
            cout << this->name << " match found...\n";
            return 0;
        }
    }

    if(sendto(socketfd,msg,strlen(msg),0, (struct sockaddr *)&(this->neighbors_side),sizeof(this->neighbors_side)) < 0)
    {
        // printf("Error sending datagram: ", strerror(errno));
        perror("sendto failed: ");
        return -1;
    }
    cout << this->name << " sent a vector!" << endl;
    return 0;
}

//random helper functions
void comm_link::register_(std::vector<std::string> &list)
{
    cerr << this-> name << " is trying to register..." << endl;
    bool add = true;
    for (vector<string>::iterator it = list.begin(); it != list.end(); ++it)
    {
        if(*it == this->name)
        {
            cerr << this->name << " is registered already..." << endl;
            add = false;
            break;
        }
    }

    if(add)
    {
        cerr << "Now registered " << this->name << endl;
        list.push_back(this->name);
    }
    return;
}


void comm_link::rescind_(vector<string> &list)
{
    int list_length = list.size();
    for (int i = 0; i < list_length; ++i)
    {
        if(list[i] == this->name)
        {
            list.erase(list.begin() + i);
            return;
        }
    }
    return;
}

//debug
void comm_link::print_out() {
    cout << "Destination node name: " << this->name << endl;
    cout << "Cost: " << this->cost << endl;
    cout << "Neighbors port on this link: " << this->neighbors_port << endl;
    return;
}
