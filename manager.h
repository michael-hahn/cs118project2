#ifndef MANAGER_H
#define MANAGER_H

#include <vector>
#include <map>
#include <utility>
#include <string>
#include "comm_link.h"
//for polling
#include <sys/poll.h>

/*Manages communication between the main router and the neighbors*/

class manager
{
    public:
    	//constructor
        manager(char const * file_name, char * main_router);
        //UDP comm
        int bind_();
        int procure_socket();
        //communication between routers
        int communicate();
        std::string collect_();
        //small helper functions
        comm_link * identify_commlink_using_socketfd(int fd);
        //calculation function
        int process_distance_vector(std::string data);
        //debugging
        void print_out();
        int print_dv_table();
        //helper function: returns a comm_link object pointer that given the name
        comm_link* get_comm_link(std::string name);
        char* convert_string_to_char(std::string data);

        //Make message
        std::string create_message(std::map<std::string,std::pair<std::string,int> > dv_table); 
    private:
        char * main_router;
        std::string main_router_output_file_name;
        struct sockaddr_in my_side;
        int my_port;
        int my_socketfd;
        bool has_been_bound;
        bool has_socket;
        std::vector<comm_link> comm_links;
        std::map<std::string,std::pair<std::string,int> > dv_table;
        std::vector<pollfd> sockets_to_poll;
        bool table_has_changed;
        int recommunicate;
};


#endif
