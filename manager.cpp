#include "manager.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstring>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctime>

using namespace std;

#define BIG_NUMBER 1000000

//Terminology note: Main router will refer to the router that the running process is maintaining, and all other routers
//are referred to as neighbors

//Constructor
manager::manager(char const * file_name, char * main_router)
{
    this->recommunicate = 0;
    this->table_has_changed = false;
    this->main_router = main_router;
    int name_length = strlen(this->main_router);
    this->main_router_output_file_name += "router-output";
    for(int i = 0; i < name_length; i++)
    {
        this->main_router_output_file_name += this->main_router[i];
    }
    this->main_router_output_file_name += ".txt";
    this->has_been_bound = false;
    this->has_socket = false;
    //read through the file to inialize the node
    ifstream inputfile;
    char c;
    string line;
    inputfile.open(file_name); //open the file + error checking
    if(!inputfile)
    {
        cerr << "An error occurered while attempting to open the file\n";
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
        int source_port, cost;
        // cout << "here -> " << input ;
        sscanf (input, "%8[^,],%8[^,],%8[^,],%8[^,]", source_name, destination_name, s_source_port, s_cost);
        source_port = atoi(s_source_port);
        cost = atoi(s_cost);
        // cout << "source name - " << source_name << endl;
        // cout << "destination name - " << destination_name << endl;
        // cout << "source port - " << source_port << endl;
        // cout << "source cost - " << cost<< endl;
        // cout << endl;
        line = ""; //reset the line string to be empty
        //if the first name on the line matches the name of the router represented by the running process
        if(strcmp(source_name, this->main_router) == 0)
        {
            bool b = false;
            for (vector<comm_link>::iterator it = this->comm_links.begin(); it != this->comm_links.end(); it++)
            {
                string x = it->get_name();  //if (main_router, destination router) combination is aready represented in
                if (strcmp(x.c_str(), destination_name) == 0)   //the vector
                {
                    it->set_neighbors_port(source_port); //attempt to fix the port issue 
                    // it->set_my_port(source_port);   //set the port number for the main router on this communication link
                    b = true;   //let it be known that it is true that this (main router, neighbor) pair is already
                    //represented within the vector data structure
                }
            }
            //if the (main router, neighbor) pair is not represented in the vector
            if (!b) {
                //create a new commincation link class instance, giving it the proper information
                // comm_link n = comm_link(destination_name, cost, source_port, -1);
                comm_link n = comm_link(destination_name, cost, source_port);   //attempt to fix the port issue
                //push it into the vector
                this->comm_links.push_back(n);
            }
        }
        //if the line provides information for data coming from a neighbor into the main router, i.e. the main
        //router is the destination
        if(strcmp(destination_name, this->main_router) == 0)
        {
            //if the communication links vector is empty
            if (comm_links.empty()) 
            {
                //just add it into the vector, providing the appropriate info
                comm_link n = comm_link(source_name, cost, -1);    //attempt to fix the port issue
                this->my_port = source_port;
                this->comm_links.push_back(n);
            }
            else
            {
                //else check if the neighbor is already in the communication links vector
                bool inTheNeighbor = false;
                for (vector<comm_link>::iterator it = this->comm_links.begin(); it != this->comm_links.end(); it++) {
                    string x = it->get_name();
                    if (strcmp(x.c_str(),source_name)==0) {
                        inTheNeighbor = true;
                    }
                }
                //if the neighbor is not already in the communication links vector, just add it
                if (inTheNeighbor == false) {
                    // comm_link n = comm_link(source_name, cost, -1, source_port);
                    comm_link n = comm_link(source_name, cost, -1);    //attempt to fix the port issue
                    this->comm_links.push_back(n);
                }
                //else the neighbor is already in the communication links vector
                this->my_port = source_port;
            }
        }
        line = ""; //reset the line string CHECK: again???
    }

    //next me
    bzero(&this->my_side, sizeof(this->my_side));
    this->my_side.sin_family = AF_INET;
    this->my_side.sin_addr.s_addr = inet_addr("127.0.0.1"); //since we will be talking on the local host
                                                                    //, just hard code it?
    this->my_side.sin_port = htons(this->my_port);

    //insert itself in the table with value 0 as the cost and next hop is itself
    int size_of_name = strlen(this->main_router);
    string string_version_of_name;
    for (int i = 0; i < size_of_name; i++)
    {
        string_version_of_name += this->main_router[i];
    }
    pair<string, int> self_ (string_version_of_name, 0);
    this->dv_table.insert(pair<string, pair<string, int> > (string_version_of_name, self_));

    //insert from the initializer.txt file
    int cl_size = this->comm_links.size();
    for (int j = 0; j < cl_size; j++) 
    {
        pair<string, int> pear (comm_links[j].get_name(), comm_links[j].get_cost());
        this->dv_table.insert(pair<string, pair<string, int> > (comm_links[j].get_name(), pear));
    }
    this->table_has_changed = true;
    //make socket
    if(this->procure_socket() != 0)
    {
        cerr << "Something went wrong procuring the socket...\n";
    }
    //bind
    if(this->bind_() != 0)
    {
        cerr << "Something went wrong with binding the main router.\n";
    }
    this->print_dv_table();
}

string manager::create_message(map<string,pair<string,int> > dv_table) 
{
    string msg = "";
    int str_length = strlen(this->main_router);
    for (int i = 0; i < str_length; i++)
    {
        msg += this->main_router[i];
    }
    msg += " ";
    for (map<string, pair<string, int> >::iterator it = dv_table.begin(); it != dv_table.end(); it++)
    {
        string dest = it->first;
        int cost = it->second.second;
        char s_cost[1000];
        sprintf(s_cost, "%d", cost);
        msg += dest;
        msg += " ";
        msg += s_cost;
        msg += " ";
    }
    return msg;
}

//Debugging
void manager::print_out() 
{
    cout << this->main_router << " has the port: " << this->my_port << endl << endl;
    for (vector<comm_link>::iterator it = this->comm_links.begin(); it != this->comm_links.end(); it++) {
        it->print_out();
        cout << endl << endl;
    }
}

comm_link* manager::get_comm_link(string name)
{
    int cl_length = this->comm_links.size();
    for (int i = 0; i < cl_length; ++i) 
    {
        if (this->comm_links[i].get_name() == name)
        {
            return &comm_links[i];
        }
    }
    return NULL;
}

//UDP comms
int manager::bind_()
{
    if(bind(this->my_socketfd,(struct sockaddr *)&(this->my_side),sizeof(this->my_side)) < 0)
    {
        perror("bind() failed: ");
        return -1;
    }
    this->has_been_bound = true;
    return 0;
}

int manager::procure_socket()
{
    //try to get a socket, store its file descriptor
    this->my_socketfd=socket(AF_INET, SOCK_DGRAM,0);
    if(this->my_socketfd < 0)
    {
        perror("socket() failed: ");
        return -1;
    }
    this->has_socket = true;
    //1.Set up array of file descriptors
    pollfd poll_member;
    poll_member.fd = this->my_socketfd;
    poll_member.events = POLLIN;
    this->sockets_to_poll.push_back(poll_member);
    return 0;
}

string manager::collect_()
{
    int n;
    char recvline[10000];
    socklen_t len = sizeof(this->got_from);
    n=recvfrom(this->my_socketfd,recvline,10000,0,(struct sockaddr *)&(this->got_from), &len);
    if(n < 0)
    {
        cerr << "Error receiving datagram " << endl;
        perror("");
        return "";
    }
    recvline[n]=0;
    for (vector<comm_link>::iterator it = this->comm_links.begin(); it != this->comm_links.end(); it++)
    {
        if (htons(it->get_port()) == this->got_from.sin_port)
        {
            if(it->record_time() < 0)
            {
                cerr << endl << "=:=Problem recoding the time for "<< it->get_name() << " =:=" << endl;
            }
        }
    }
    printf("\n\n%s has received: ",this->main_router);
    string s;
    for (int i = 0; i < n; i++) 
    {
        s += recvline[i];
    }
    cout << s << endl <<endl;
    return s;
}

int manager::communicate() 
{
    if((!this->has_been_bound)||(!this->has_socket))
    {
        cerr << "You can not communicate with your neighbors if you have not gotten a socket and bound it" << endl;
        return -1;
    }

    bool first_time_record_time;

    // vector<comm_link> bad_links;
    vector<string> table_sent;
    //while there are still communication links that need to be used for introductions
    while(true)
    {
        if (this->recommunicate == 5) 
        {
            this->table_has_changed = true;
            table_sent.clear();
            this->recommunicate = 0;
            cout << "\n\nrecommunicate reset...\n\n";
        }
        string the_msg = create_message(this->dv_table);
        //send messages from the main router to the neighbors = for each neighbor send a message
        if (table_has_changed)
        {
            for (vector<comm_link>::iterator it = this->comm_links.begin(); it != this->comm_links.end(); it++) 
            {
                int rv = it->send_distance_vector(the_msg.c_str(), this->my_socketfd, table_sent);
                //register as acquainted, meaning that there is no need, for now to send a message in the near future
                if (rv == 0)
                {
                    it->register_(table_sent);
                }
            }
        }
        //receive messages
        if(this->sockets_to_poll.size() != 1)
        {
            cerr << "For some reason the vector with sockets to poll is not size 1..." << endl;
            return -1;
        }
        //2.Poll
        for(int j = 0; j < 10; ++j)
        {
            pollfd * sockets_to_poll_pointer = &sockets_to_poll[0];
            if(sockets_to_poll_pointer == NULL) //error checking
            {
                cerr << "Something went wrong trying to point to the array of sockets that need to be polled." << endl
                << "The program wil now return from the function manager::communicate with a negative value to indicate an error." 
                << endl;
                return -1;
            }

            int poll_return_value = 0;
            int time_in_seconds = 3; //Integers only, no 3.5 seconds or anything like that 
            poll_return_value = poll(sockets_to_poll_pointer, sockets_to_poll.size(), time_in_seconds * 1000);
            
            if(first_time_record_time)
            {
                for(vector<comm_link>::iterator it = this->comm_links.begin(); it != this->comm_links.end(); ++it)
                {
                    if(it->record_time() < 0)
                    {
                        cerr << "Trouble recording the time for " << it->get_name() << endl << endl; 
                    }
                }
                first_time_record_time = false;
            }

            if(poll_return_value < 0) //error checking
            {
                cerr << "Poll returned a negative value indicating the following error: " << endl;
                perror("");
                cerr<< "Will now continue the manager::communicate function." << endl;
                for(vector<comm_link>::iterator it = this->comm_links.begin(); it != this->comm_links.end(); ++it)
                {
                    if(it->is_dead()) //checking the pulse and seeing that someone is not dead
                            //resets the how_many_times_dead counter in the comm_link class instance
                    {
                        cerr << "\n*****************************\n" << it->get_name() 
                        << " has died\n**************************\n" << endl << endl;
                        //let everyone know
                        if(this->get_how_many_times_dead(it->get_name()) < 0)
                        {
                            this->add_to_how_many_times_told_dead_on(it->get_name());
                            cout << "\n\n())))))))))))))))))\n\n" << "Just added " << it->get_name() << " to the map with value " <<
                            this->get_how_many_times_dead(it->get_name()) << "\n)((((((((((((((((((((\n\n";

                        }

                        cout << endl << endl << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl; 
                        printf("%s", this->main_router);
                        cout << " has told on " << it->get_name() << " " <<this->get_how_many_times_dead(it->get_name()) << " times\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" << endl;
                        if(this->get_how_many_times_dead(it->get_name()) < 4)
                        {
                            string obituary = "^";
                            obituary += it->get_name();
                            for(vector<comm_link>::iterator itt = this->comm_links.begin(); itt != this->comm_links.end(); ++itt)
                            {
                                if(it->get_name() != itt->get_name())
                                {
                                    vector<string> dummy;
                                    cout << endl << endl << "*****************************" <<endl;
                                    printf("%s", this->main_router);
                                    cout << " wants to tell " << itt->get_name() << " that " << it->get_name() << " died " << "\n********************" << endl << endl;
                                    int rv = itt->send_distance_vector(obituary.c_str(), this->my_socketfd, dummy);
                                    //register as acquainted, meaning that there is no need, for now to send a message in the near future
                                    if (rv != 0)
                                    {
                                        cerr << "Couldn't send obituary :(\n\n";
                                    }
                                }
                            }
                            this->increase_how_many_times_told_dead_on(it->get_name());
                            cout << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\nNow, " << it->get_name() << " has the value " 
                            << this->get_how_many_times_dead(it->get_name()) 
                            << " for the amount of times  it has gotton told on\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
                        }
                    }
                    else
                    {
                        this->reset_how_many_times_told_dead_on(it->get_name());
                    }
                }
                break;
            }
            else if(poll_return_value == 0)
            {
                cout << "Time out occured after " << time_in_seconds <<" seconds, going to try sending messages to neighbors again...\n";
                for(vector<comm_link>::iterator it = this->comm_links.begin(); it != this->comm_links.end(); ++it)
                {
                    if(it->is_dead()) //checking the pulse and seeing that someone is not dead
                            //resets the how_many_times_dead counter in the comm_link class instance
                    {
                        cerr << "\n*****************************\n" << it->get_name() 
                        << " has died\n**************************\n" << endl << endl;
                        //let everyone know
                        if(this->get_how_many_times_dead(it->get_name()) < 0)
                        {
                            this->add_to_how_many_times_told_dead_on(it->get_name());
                            cout << "\n\n())))))))))))))))))\n\n" << "Just added " << it->get_name() << " to the map with value " <<
                            this->get_how_many_times_dead(it->get_name()) << "\n)((((((((((((((((((((\n\n";

                        }
                        cout << endl << endl << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl; 
                        printf("%s", this->main_router);
                        cout << " has told on " << it->get_name() << this->get_how_many_times_dead(it->get_name()) << " times\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" << endl;
                        
                        if(this->get_how_many_times_dead(it->get_name()) < 4)
                        {
                            string obituary = "^";
                            obituary += it->get_name();
                            for(vector<comm_link>::iterator itt = this->comm_links.begin(); itt != this->comm_links.end(); ++itt)
                            {
                                if(it->get_name() != itt->get_name())
                                {
                                    vector<string> dummy;
                                    cout << endl << endl << "*****************************" <<endl;
                                    printf("%s\n", this->main_router);
                                    cout << " wants to tell " << itt->get_name() << " that " << it->get_name() << " died " << "\n********************" << endl << endl;
                                    int rv = itt->send_distance_vector(obituary.c_str(), this->my_socketfd, dummy);
                                    //register as acquainted, meaning that there is no need, for now to send a message in the near future
                                    if (rv != 0)
                                    {
                                        cerr << "Couldn't send obituary :(\n\n";
                                    }
                                }
                            }
                            this->increase_how_many_times_told_dead_on(it->get_name());
                            cout << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\nNow, " << it->get_name() << " has the value " 
                            << this->get_how_many_times_dead(it->get_name()) 
                            << " for the amount of times  it has gotton told on\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
                        }
                    }
                    else
                    {
                        this->reset_how_many_times_told_dead_on(it->get_name());
                    }
                }
                break;
            }
            else
            {
                //now collect the data waiting at the socket
                //check if POLLIN
                if(this->sockets_to_poll[0].revents & POLLIN)
                {
                    //next, let that communication link collect whatever is waiting on its socket
                    string data = this->collect_();
                    for(vector<comm_link>::iterator it = this->comm_links.begin(); it != this->comm_links.end(); ++it)
                    {
                        if(it->is_dead()) //checking the pulse and seeing that someone is not dead
                                //resets the how_many_times_dead counter in the comm_link class instance
                        {
                            cerr << "\n*****************************\n" << it->get_name() 
                            << " has died\n**************************\n" << endl << endl;
                            //let everyone know
                            if(this->get_how_many_times_dead(it->get_name()) < 0)
                            {
                                this->add_to_how_many_times_told_dead_on(it->get_name());
                                cout << "\n\n())))))))))))))))))\n\n" << "Just added " << it->get_name() << " to the map with value " <<
                                this->get_how_many_times_dead(it->get_name()) << "\n)((((((((((((((((((((\n\n";
                            }
                            cout << endl << endl << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl; 
                            printf("%s", this->main_router);
                            cout << " has told on " << it->get_name() << this->get_how_many_times_dead(it->get_name()) << " times\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" << endl;
                            
                            if(this->get_how_many_times_dead(it->get_name()) < 4)
                            {
                                string obituary = "^";
                                obituary += it->get_name();
                                for(vector<comm_link>::iterator itt = this->comm_links.begin(); itt != this->comm_links.end(); ++itt)
                                {
                                    if(it->get_name() != itt->get_name())
                                    {
                                        vector<string> dummy;
                                        cout << endl << endl << "*****************************" <<endl;
                                        printf("%s\n", this->main_router);
                                        cout << " wants to tell " << itt->get_name() << " that " << it->get_name() << " died " << "********************" << endl << endl;
                                        int rv = itt->send_distance_vector(obituary.c_str(), this->my_socketfd, dummy);
                                        //register as acquainted, meaning that there is no need, for now to send a message in the near future
                                        if (rv != 0)
                                        {
                                            cerr << "Couldn't send obituary :(\n\n";
                                        }
                                    }
                                }
                                this->increase_how_many_times_told_dead_on(it->get_name());
                                cout << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\nNow, " << it->get_name() << " has the value " 
                                << this->get_how_many_times_dead(it->get_name()) 
                                << " for the amount of times  it has gotton told on\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
                            }
                        }
                        else
                        {
                            this->reset_how_many_times_told_dead_on(it->get_name());
                        }
                    }
                    if(data == "") //error checking
                    {
                        cerr << "Something went wrong trying to collect the vector from neighbors..." << endl
                        << "Moving on to a new neighbor" << endl;
                        break;
                    }
                    if (data[0] == '$')
                    {
                        string data_dest_node;
                        if (data[1] == ' ')
                        {
                            cerr << "Wrong format!" << endl;
                        }
                        else
                        {
                            int pos = 1;
                            while(data[pos] != ' ')
                            {
                                data_dest_node += data[pos];
                                pos++;
                            }
                            comm_link* cl;
                            vector<string> no_use;
                            cl = next_hop(data_dest_node);
                            pos++;
                            string payload;
                            int data_size = data.length();
                            for (int i = pos; i < data_size; i++)
                            {
                                payload += data[i];
                            }
                            int rv = this->data_path_info(cl, payload);
                            if (rv < 0)
                            {
                                cerr << "Printing data failed!" << endl;
                            }
                            int size_of_name = strlen(this->main_router);
                            string string_version_of_name;
                            for (int i = 0; i < size_of_name; i++)
                            {
                                string_version_of_name += this->main_router[i];
                            }
                            if (string_version_of_name != data_dest_node)
                            {
                                cl->send_distance_vector(data.c_str(), this->my_socketfd, no_use);
                            }
                        }
                    }
                    else if(data[0] == '^')
                    {
                        bool prit = false;
                        string dead_node;
                        if (data[1] == ' ')
                        {
                            cerr << endl << endl << "Wrong format!: " << data << endl << endl;
                        }
                        else
                        {
                            int pos = 1;
                            int data_size = data.length();
                            for (int j = pos; j < data_size; ++j)
                            {
                                dead_node += data[pos];
                                pos++;
                            }
                            for (map<string, pair<string, int> >::iterator itt = this->dv_table.begin(); itt != this->dv_table.end(); ++itt)
                            {
                                if ((itt->first == dead_node || itt->second.first == dead_node) && itt->second.second < BIG_NUMBER)
                                {
                                    prit = true;
                                    itt->second.second = BIG_NUMBER;
                                }
                            }

                            for(vector<comm_link>::iterator it = this->comm_links.begin(); it != this->comm_links.end(); ++it)
                            {
                                cerr << "\n*****************************\n" << dead_node 
                                << " has died\n**************************\n" << endl << endl;
                                //let everyone know
                                if(this->get_how_many_times_dead(dead_node) < 0)
                                {
                                    this->add_to_how_many_times_told_dead_on(dead_node);

                                }

                                if(this->get_how_many_times_dead(dead_node) < 4)
                                {
                                    string obituary = "^";
                                    obituary += dead_node;
                                    for(vector<comm_link>::iterator itt = this->comm_links.begin(); itt != this->comm_links.end(); ++itt)
                                    {
                                        if(dead_node != itt->get_name())
                                        {
                                            vector<string> dummy;
                                            int rv = itt->send_distance_vector(obituary.c_str(), this->my_socketfd, dummy);
                                            //register as acquainted, meaning that there is no need, for now to send a message in the near future
                                            if (rv == 0)
                                            {
                                                cerr << "Couldn't send obituary :(\n\n";
                                            }
                                        }
                                    }
                                    this->increase_how_many_times_told_dead_on(dead_node);
                                }
                            }
                            
                            ofstream file_to_print (this->main_router_output_file_name.c_str(), ios::out | ios::app);
                            if(file_to_print.is_open())
                            {
                                string get_info_from;
                                for (vector<comm_link>::iterator wit = this->comm_links.begin(); wit != this->comm_links.end(); ++wit)
                                {
                                    if (this->got_from.sin_port == htons(wit->get_port()))
                                    {
                                        get_info_from = wit->get_name();
                                        break;
                                    }
                                }
                                // file_to_print << "Router " << this->main_router << " knows that " << dead_node << " is dead!" 
                                // << "from the message sent from Router " << get_info_from << endl;
                                // file_to_print << "So the entries that contain " << dead_node << " are updated as:" << endl;
                                // for (map<string, pair<string, int> >::iterator itt = this->dv_table.begin(); itt != this->dv_table.end(); ++itt)
                                // {
                                //     if (itt->first == dead_node || itt->second.first == dead_node)
                                //     {
                                //         file_to_print << endl << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
                                //         file_to_print << itt->first << "      " << itt->second.first << "         " << itt->second.second << endl;
                                //     }
                                // }
                                // file_to_print << "Here is the dv_table after the death is known..." << endl;
                                // file_to_print << endl << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;

                            }
                            if (prit)
                            {
                                print_dv_table();
                            }
                        }
                    }
                    else 
                    {
                        //next, let that link instance process the data that just came in
                        int rv = this->process_distance_vector(data);
                        if(rv < 0)
                        {
                            cerr << "Something went wrong processing the distance vector...\nMoving on to a new neighbor..." << endl;
                            break;
                        }
                        else if(rv > 0)
                        {
                            for(vector<comm_link>::iterator it = this->comm_links.begin(); it !=  this->comm_links.end(); ++it)
                            {

                                cerr << endl << "**************************************************************" << endl; 
                                cerr << "Table is updated, so we need to rescind all the members..." << endl << "Rescind starts..." << endl;
                                it->rescind_(table_sent);
                                cerr << "Table_sent now has the member: " << endl;
                                for (int i = 0; i < table_sent.size(); ++i) {
                                    cerr << table_sent[i] << " ";
                                }
                                cerr << endl << "**************************************************************" << endl; 
                            }
                            cerr << "Rescind finished...The table sent should be empty..." << endl;
                        }
                    }
                }
            }
        }
        recommunicate++;
    }
    return 0;
}

//functions for calculations
int manager::process_distance_vector(string data)
{
    //first parse the data message
    //int length = (int)data.length();
    vector<string> dest_v;
    vector<int> cost_v;
    this->table_has_changed = false;
    string src;
    string destination;
    string s_cost;
    int cost;
    string input;
    int j = 0;
    int count = 0;
    int pear = 0;
    while (j < (data.length()-1) && data.at(j))
    {
        while(j < (data.length() - 1))
        {
            if (data.at(j) == ' ') 
            {
                j++;
                break;
            }
            else 
            {
                input = input + data.at(j);
                j++;
            }
        }
        //cerr << "The input is: " << input << endl;
        // cerr << "Count is: " << count << endl;
        //first = source
        //odd number afterwards = destination
        //even number afterwards = cost
        if (count == 0) {   //first = source
            src = input;
            // cerr << "The src is: " << src << endl;
        }
        else if (count % 2 == 1) {  //odd number afterwards = destination
            destination = input;
            dest_v.push_back(destination);
            pear ++;
            // cerr << "The destination is: " << destination << endl;
            // cerr << "Pair number is: " << pear << endl; 
        }
        else {  //count % 2 == 0 -> even number afterwards = cost
            s_cost = input;
            cost = atoi(s_cost.c_str());
            cost_v.push_back(cost);
            pear ++;
            // cerr << "The cost is: " << cost << endl;
            // cerr << "Pair number is now: " << pear << endl;
        }
        if (pear == 2) {    //we have a new pair of destination and cost
            //see if the map already contains such path
            std::map<std::string,std::pair<std::string,int> >::iterator it;
            // cerr << "Now discovering the destination node " << destination << " in the table" << endl;
            it = dv_table.find(destination);
            
            //none discovered, if so add new entry to the table
            //this is based on the fact that we will not be adding new nodes, if this is not the case, we could add new destination
            if (it == dv_table.end())
            {  
                // cerr << "No destination node found in the current table" << endl;
                comm_link* link = this->get_comm_link(src);
                if (link == NULL)
                {
                    cerr << "Get the comm_link failed" << endl;
                    return -1;
                }
                int new_link_cost = link->get_cost();
                new_link_cost += cost;
                
                pair<string,int> new_pear;
                new_pear = make_pair(src, new_link_cost); 
                dv_table.insert(pair<string, pair<string,int> >(destination, new_pear));
                this->table_has_changed = true;
                // cerr << "Succeed in inserting a new destination node" << endl;
                //print_dv_table();
            }    
            else 
            {
                // cerr << "Found the entry, start to compare the values" << endl;
                comm_link* link = this->get_comm_link(src);
                if (link == NULL)
                {
                    cerr << "Get the comm_link failed" << endl;
                    return -1;
                }
                int new_link_cost = link->get_cost();
                new_link_cost += cost;

                int curr_link_cost = it->second.second;
                
                if (new_link_cost < curr_link_cost) 
                {
                    // cerr << "Updating the cost" << endl;
                    it->second.first = src;
                    it->second.second = new_link_cost;
                    table_has_changed = true;
                    //print_dv_table();
                }
                else if (new_link_cost == curr_link_cost)
                {
                    if (src < it->second.first)
                    {
                        it->second.first = src;
                        it->second.second = new_link_cost;
                        table_has_changed = true;
                        //print_dv_table();
                    }
                }
                if (cost >= BIG_NUMBER && curr_link_cost < BIG_NUMBER)
                {
                    print_dv_table();
                }
            }
            pear = 0;
            destination = "";
            s_cost = "";
            cerr << "ready for the next pair" << endl << endl;
        }
        count ++;
        input = "";
    } 
    if (table_has_changed)
    {
        ofstream file_to_print (this->main_router_output_file_name.c_str(), ios::out | ios::app);
        if(file_to_print.is_open())
        {
            file_to_print << "The DV table we received from " << src << endl; 
            file_to_print << "******************************" << endl;
            file_to_print << "Destination        cost       " << endl;
            int vec_size = dest_v.size();
            for (int i = 0; i < vec_size; ++i)
            {
                if (cost_v[i] < BIG_NUMBER)
                {
                    file_to_print << dest_v[i] << "                  " << cost_v[i] << endl;
                }
                else 
                {
                    file_to_print << dest_v[i] << "                  " << "INF" << endl;
                }
            }
        }
        file_to_print << "******************************" << endl << endl;
        file_to_print << "The following is the resulting table after update:" << endl;
        print_dv_table();
    } 
    return 0; 
}


//functions for accounting who we have notified their death on behalf of
int manager::increase_how_many_times_told_dead_on(string comm_link_name)
{
    for(map<string,int>::iterator iter = this->how_many_times_told_dead_on.begin(); iter != this->how_many_times_told_dead_on.end(); ++iter)
    {
        if(iter->first == comm_link_name)
        {
            iter->second++;
            return 0;
        }
    }
    return -1;
}

int manager::get_how_many_times_dead(string comm_link_name)
{
    for(map<string,int>::iterator iter = this->how_many_times_told_dead_on.begin(); iter != this->how_many_times_told_dead_on.end(); ++iter)
    {
        if(iter->first == comm_link_name)
        {
            return iter->second;
        }
    }
    return -1;
}

int manager::add_to_how_many_times_told_dead_on(string comm_link_name)
{
    if(this->get_how_many_times_dead(comm_link_name) == -1)
    {
        this->how_many_times_told_dead_on.insert(pair<string,int>(comm_link_name, 0));
        return 0;
    }
    return -1;
}

int manager::reset_how_many_times_told_dead_on(string comm_link_name)
{
    for(map<string,int>::iterator iter = this->how_many_times_told_dead_on.begin(); iter != this->how_many_times_told_dead_on.end(); ++iter)
    {
        if(iter->first == comm_link_name)
        {
            iter->second = 0;
            return 0;
        }
    }
    return -1;
}



char* manager::convert_string_to_char(string data) 
{
    int length_of_line = (int)data.length();
    char *char_array = new char[length_of_line+1];
    strcpy(char_array,data.c_str());
    char_array[length_of_line+1] = '\0';
    
    return char_array;
}

int manager::print_dv_table()
{
    //printing out the DV table to the proprietary file belonging to this router

    // string my_name = "routing-output" + this->main_router_output_file_name + ".txt";
    ofstream file_to_print (this->main_router_output_file_name.c_str(), ios::out | ios::app);
    if(file_to_print.is_open())
    {
        time_t current_time = time(NULL);
        file_to_print << asctime(localtime(&current_time)) << endl;
        file_to_print << "====================================" << endl;
        file_to_print << "Destination     First Hop     Cost" << endl;
        for ( map<string,pair<string,int> >::iterator it = dv_table.begin(); it != dv_table.end(); it++) {
            if (it->second.second < BIG_NUMBER)
            {
                file_to_print << it->first << "              " << 
                it->second.first << "              " << it->second.second << endl;
            }
            else 
            {
                file_to_print << it->first << "              " <<
                it->second.first << "              " << "INF" << endl;
            }

        }
        
        file_to_print << "====================================" << endl << endl;
        file_to_print.close();
    }
    else
    {
        cerr << "File " << this->main_router_output_file_name << " could not be opened or created.\n";
        return -1;
    }
    return 0;
}

comm_link* manager::next_hop(string dest_node)
{
    int size_of_name = strlen(this->main_router);
    string string_version_of_name;
    for (int i = 0; i < size_of_name; i++)
    {
        string_version_of_name += this->main_router[i];
    }
    string next_hop_node;
    for (map<string, pair<string, int> >::iterator it = this->dv_table.begin(); it != dv_table.end(); ++it)
    {
        if (it->first == dest_node)
        {
            next_hop_node = it->second.first;
            break;
        }
    }
    int size_of_vector = this->comm_links.size();
    for (int i = 0; i < size_of_vector; ++i)
    {
        if (comm_links[i].get_name() == next_hop_node)
        {
            return &comm_links[i];
        }
    }
    return NULL;
}

int manager::data_path_info(comm_link* next_hop, string data) 
{
    int size_of_name = strlen(this->main_router);
    string string_version_of_name;
    for (int i = 0; i < size_of_name; i++)
    {
        string_version_of_name += this->main_router[i];
    }
    ofstream file_to_print (this->main_router_output_file_name.c_str(), ios::out | ios::app);
    if(file_to_print.is_open())
    {
        string source_node_name;
        int source_port_number;
        for (vector<comm_link>::iterator it = comm_links.begin(); it != comm_links.end(); it++)
        {
            if (htons(it->get_port()) == this->got_from.sin_port)
            {
                source_node_name = it->get_name();
                source_port_number = it->get_port();
            }
        }
        if (source_node_name == "")
        {
            cerr << "Cannot find source node name!" << endl;
            return -1;
        }
        time_t current_time = time(NULL);
        file_to_print << endl << asctime(localtime(&current_time)) << endl;
        file_to_print << "Source node is: " << source_node_name << endl;
        if(next_hop != NULL)
        {
          file_to_print << "Destination node is " << next_hop->get_name() << endl;    
        }
        else
        {
            file_to_print << "Destination node is " <<  string_version_of_name << endl;   
        }
        file_to_print << "UDP port in which the packet arrived: " << this->my_port << endl;
        file_to_print << "UDP source port along which the packet was forwarded: " << source_port_number << endl << endl;
        if (next_hop == NULL)
        {
            file_to_print << data << endl << endl;
        }
        return 0;
    }
    else return -1;
}
