#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
using namespace std ;
int main (int argc  , char * argv[] ){
   if (argc != 5){
        cout << "Usage: ./node <oracle-ip> <own-ip> <udp-port> <oracle-port>\n";
        return 1;
    }
   string oracle_ip = argv[1];
    string own_ip = argv[2];
    int udp_port = atoi(argv[3]);
    int oracle_port = atoi(argv[4]);


    //creating udp port 
    int udp_socket = socket(AF_INET,SOCK_DGRAM,0);
    if (udp_socket < 0) {
    perror("UDP socket creation failed");
    return 1;
   }

    sockaddr_in own_address{};
    own_address.sin_family = AF_INET;
    own_address.sin_port = htons(udp_port);
    inet_pton(AF_INET , own_ip.c_str() , &own_address.sin_addr);
    bind(udp_socket , (sockaddr*)&own_address , sizeof(own_address));

    // tcp now 
    int tcp_socket  = socket(AF_INET , SOCK_STREAM , 0);
    sockaddr_in orcale_address{};
    oracle_address.sin_family = AF_INET;
    orcale_address.sin_port  = htons(5000);

    inet_pton(AF_INET  orcale_ip.c_str(), &orcale_address.sin_addr);
    connect(tcp_socket , (sockaddr*)&orcale_address , sizeof(orcale_address));





}