///* Sample UDP client */
//
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <iostream>
#include <cstdio>
#include <fstream>
#include <cstring>
#include <vector>
#include <stdlib.h>
#include <algorithm>
#include <string>
#include <sstream>
using namespace std;

struct neighboure
{
    string name;
    int cost;
    int port_from_me_to_neighboure;
    int port_from_neighboure_to_me;
};



//for now: the user provides the "name of the router" {A,B,C,D,E,F}, we gotta figure out officially, how a router is supposed to know who s/he is
int main(int argc, char**argv)
{
    int sockfd,n;
    struct sockaddr_in servaddr,cliaddr;
    char sendline[1000];
    char recvline[1000];
    
    if (argc != 2)
    {
        cout << "You forgot to name the router" << endl;
        exit(1);
    }
    
    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    // servaddr.sin_port=htons(32000);
    
    //read through the file to inialize the node
    ifstream inputfile;
    char c;
    string line;
    inputfile.open("/Users/Michael/Desktop/cs118/initialize.txt");
    if(!inputfile)
    {
        cout << "An error occurered while attem[ting to open the file\n";
        exit(1);
    }
    
    int myport;
    vector<neighboure> neighborhood;
    
    while(inputfile.get(c))
    {
        //parse here
        while(true)
        {
            line = line + c;
            if(!inputfile.get(c))
            {
                break;
            }
            if(c == '\n')
            {
                line = line + c;
                break;
            }
        }
        int length_of_line = line.length();
        string source_name, destination_name, s_source_port, s_cost;
        int source_port, cost, field;
        field = 0;
        for(int i = 0; i < length_of_line; i++)
        {
            if(line[i] == ',')
            {
                field++;
                i++;
            }
            switch(field)
            {
                case 0:
                {
                    source_name += line[i];
                    break;
                }
                case 1:
                {
                    destination_name += line[i];
                    break;
                }
                case 2:
                {
                    s_source_port += line[i];
                    char * copy = new char [s_source_port.length()];
                    for(int i = 0 ; i < s_source_port.length(); i++)
                    {
                        copy[i] = s_source_port[i];
                    }
                    source_port = atoi(copy);
                    delete [] copy;
                    break;
                }
                case 3:
                {
                    s_cost += line[i];
                    char * copy = new char [s_cost.length()];
                    for(int i = 0 ; i < s_cost.length(); i++)
                    {
                        copy[i] = s_cost[i];
                    }
                    cost = atoi(copy);
                    delete [] copy;
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
        
        if(source_name == argv[1])
        {
            bool b = false;
            for (vector<neighboure>::iterator it = neighborhood.begin(); it != neighborhood.end(); it++) {
                if (it -> name == destination_name) {
                    it->port_from_me_to_neighboure = source_port;
                    b = true;
                }
            }
            if (!b) {
                neighboure n;
                n.name = destination_name;
                n.cost = cost;
                n.port_from_me_to_neighboure = source_port;
                n.port_from_neighboure_to_me = -1;
                neighborhood.push_back(n);
            }
        }
        if(destination_name == argv[1])
        {
            if (neighborhood.empty()) {
                neighboure n;
                n.name = source_name;
                n.cost = cost;
                n.port_from_me_to_neighboure = -1;
                n.port_from_neighboure_to_me = source_port;
                neighborhood.push_back(n);
            }
            bool inTheNeighbor = false;
            for (vector<neighboure>::iterator it = neighborhood.begin(); it != neighborhood.end(); it++) {
                if (it -> name == source_name) {
                    inTheNeighbor = true;
                }
            }
            if (inTheNeighbor == false) {
                neighboure n;
                n.name = source_name;
                n.cost = cost;
                n.port_from_me_to_neighboure = -1;
                n.port_from_neighboure_to_me = source_port;
                neighborhood.push_back(n);
            }
            for(vector<neighboure>::iterator it = neighborhood.begin();
                it != neighborhood.end(); it++)
            {
                if(it->name == source_name)
                {
                    it->port_from_neighboure_to_me = source_port;
                }
            }
        }
        line = "";
    }
    cout << "Current node: " << argv[1] << endl << "Neighboures: " << endl << endl;
    for (vector<neighboure>::iterator it = neighborhood.begin(); it != neighborhood.end(); it++) {
        cout << "Neighboure's name: " << it->name << endl << "Cost:" << it->cost << endl;
        cout << "Neighboure's port number: " <<it->port_from_neighboure_to_me << endl;
        cout << "Current Node's port number: " <<it -> port_from_me_to_neighboure << endl << endl;
    }
    
    
    // while (fgets(sendline, 10000,stdin) != NULL)
    // {
    //    sendto(sockfd,sendline,strlen(sendline),0,
    //           (struct sockaddr *)&servaddr,sizeof(servaddr));
    //    n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
    //    recvline[n]=0;
    //    fputs(recvline,stdout);
    // }
    
    return 0;
}

