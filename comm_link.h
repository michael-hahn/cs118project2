#ifndef COMM_LINK_H
#define COMM_LINK_H

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>

/*Each instance of this class bundles all of the information &
 functionality needed for communication to occur between the
 main router pertaining to the process and all of its neighbor
 routers*/

class comm_link
{
public:
    //constructor
    comm_link(char * n, int c, int neighbors_prt);
    //setters 
    void set_neighbors_port(int port);
    //getters
    std::string get_name();
    int get_cost();
    int get_port();
    //udp comms
    int send_distance_vector(const char * msg, int socketfd, std::vector<std::string> table_sent);
    //random helper functions
    void register_(std::vector<std::string> &list);
    //debug
    void print_out();
    void rescind_(std::vector<std::string> &list);
private:
    std::string name;
    int cost;
    int neighbors_port; //A,B,10001,3
    struct sockaddr_in neighbors_side;
};


#endif
