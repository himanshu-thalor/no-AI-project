#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
using namespace std;

bool receive_exact(int fd, void *buffer, size_t length)
{
    char *bytes = static_cast<char *>(buffer);
    size_t received = 0;
    while (received < length)
    {
        ssize_t n = recv(fd, bytes + received, length - received, 0);
        if (n <= 0) return false;
        received += static_cast<size_t>(n);
    }
    return true;
}

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
    if (bind(udp_socket, (sockaddr *)&own_address, sizeof(own_address)) < 0)
    {
        perror("UDP bind failed");
        close(udp_socket);
        return 1;
    }

    // tcp now
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in oracle_address{};
    oracle_address.sin_family = AF_INET;
    oracle_address.sin_port = htons(oracle_port);

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

    // Receive LINK-STATE messages from the Oracle over TCP
    while (true)
    {
        unsigned char header[3];
        if (!receive_exact(tcp_socket, header, sizeof(header)))
            break;

        size_t count = header[2];
        vector<unsigned char> entries(count * 11);
        if (!receive_exact(tcp_socket, entries.data(), entries.size()))
            break;

        cout << static_cast<char>(header[1]) << "=0";
        for (size_t i = 0; i < count; ++i)
        {
            uint32_t cost;
            memcpy(&cost, entries.data() + i * 11 + 1, sizeof(cost));
            cout << "," << static_cast<char>(entries[i * 11])
                 << "=" << ntohl(cost);
        }
        cout << endl;
    }

    close(tcp_socket);
    close(udp_socket);
    return 0;
}