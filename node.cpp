#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
using namespace std;
int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        cout << "Usage: ./node <oracle-ip> <own-ip> <udp-port> <oracle-port>\n";
        return 1;
    }
    string oracle_ip = argv[1];
    string own_ip = argv[2];
    int udp_port = atoi(argv[3]);
    int oracle_port = atoi(argv[4]);

    // creating udp port
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0)
    {
        perror("UDP socket creation failed");
        return 1;
    }

    sockaddr_in own_address{};
    own_address.sin_family = AF_INET;
    own_address.sin_port = htons(udp_port);
    inet_pton(AF_INET, own_ip.c_str(), &own_address.sin_addr);
    bind(udp_socket, (sockaddr *)&own_address, sizeof(own_address));

    // tcp now
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in oracle_address{};
    oracle_address.sin_family = AF_INET;
    oracle_address.sin_port = htons(5000);

    inet_pton(AF_INET, oracle_ip.c_str(), &oracle_address.sin_addr);
    connect(tcp_socket, (sockaddr *)&oracle_address, sizeof(oracle_address));

    unsigned char reg[7];
    reg[0] = 1;
    memcpy(reg + 1, &own_address.sin_addr, 4);
    uint16_t p = htons(udp_port);
    memcpy(reg + 5, &p, 2);
    send(tcp_socket, reg, 7, 0);

    cout << "Registration sent. Node is listening on UDP port "
         << udp_port << endl;

    // Keep the node alive and receive UDP messages
    char buffer[2048];

    while (true)
    {
        sockaddr_in sender{};
        socklen_t sender_len = sizeof(sender);

        ssize_t n = recvfrom(
            udp_socket,
            buffer,
            sizeof(buffer),
            0,
            (sockaddr *)&sender,
            &sender_len);

        cout << "Received " << n << " bytes from "
             << inet_ntoa(sender.sin_addr) << ":"
             << ntohs(sender.sin_port) << endl;

        cout << "Message: ";
        cout.write(buffer, n);
        cout << endl;
    }

    close(tcp_socket);
    close(udp_socket);
    return 0;
}