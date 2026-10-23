#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
using namespace std;

struct Neighbor {
    char id;
    sockaddr_in addr;
    int cost;
};

// Ensure entire message is received in the buffer 
bool receive_exact(int fd, char *buffer, int length)
{
    int received = 0;
    while (received < length)
    {
        int n = recv(fd, buffer + received, length - received, 0);
        if (n <= 0) return false;
        received += n;
    }
    return true;
}

void broadcast_lsp(int udp_socket, char my_id, int &my_seq, int highest_seq[], vector<Neighbor> &neighbors)
{
    if (my_id == 0 || neighbors.empty())
        return;

    my_seq++;
    highest_seq[my_id - 'A'] = my_seq;

    //storing info in binary 
    char pkt[256];
    pkt[0] = my_id;
    int seq_net = htonl(my_seq);
    memcpy(pkt + 1, &seq_net, sizeof(int));
    pkt[5] = neighbors.size();

    int offset = 6;
    for (int i = 0; i < neighbors.size(); ++i)
    {
        pkt[offset] = neighbors[i].id;
        int cost_net = htonl(neighbors[i].cost);
        memcpy(pkt + offset + 1, &cost_net, sizeof(int));
        offset += 5;
    }

    //broadcasting Neighbor info 
    for (int i = 0; i < neighbors.size(); ++i)
    {
        sendto(udp_socket, pkt, offset, 0, (sockaddr *)&neighbors[i].addr, sizeof(neighbors[i].addr));
    }
}

int main(int argc, char *argv[])
{
    //getting ip and port info 
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


    // tcp setup
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in oracle_address{};
    oracle_address.sin_family = AF_INET;
    oracle_address.sin_port = htons(oracle_port);

    inet_pton(AF_INET, oracle_ip.c_str(), &oracle_address.sin_addr);
    connect(tcp_socket, (sockaddr *)&oracle_address, sizeof(oracle_address));

    char reg[7];
    reg[0] = 1;
    memcpy(reg + 1, &own_address.sin_addr, 4);
    uint16_t p = htons(udp_port);
    memcpy(reg + 5, &p, 2);
    send(tcp_socket, reg, 7, 0);
    cout << "Registration sent. Node is listening on UDP port "
         << udp_port << endl;

    //intialising variables for lsp 
    char my_id = 0;
    int num_nodes = 0;
    time_t last_periodic = time(nullptr);
    vector<Neighbor> neighbors;
    int graph[26][26];
    for (int i = 0; i < 26; ++i)
    {
        for (int j = 0; j < 26; ++j)
        {
            if (i == j)
                graph[i][j] = 0;
            else
                graph[i][j] = -1;
        }
    }
    int my_seq = 0;
    int highest_seq[26] = {0};

    // Event loop multiplexing TCP (Oracle) and UDP (Neighbor nodes)
    while (true)
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(tcp_socket, &read_fds);
        FD_SET(udp_socket, &read_fds);
        timeval tv{1, 0}; // 1-second timeout for periodic tasks
        int activity = select(max(tcp_socket, udp_socket) + 1, &read_fds, nullptr, nullptr, &tv);
        if (activity < 0)
        {
            if (errno == EINTR) continue;
            break;
        }

        // Oracle Node update (TCP)
        if (FD_ISSET(tcp_socket, &read_fds))
        {
            char header[3];
            if (!receive_exact(tcp_socket, header, sizeof(header)))
                break;

            my_id = header[1];
            int u = my_id - 'A';
            if (u + 1 > num_nodes)
                num_nodes = u + 1;

            int count = (unsigned char)header[2];
            vector<char> entries(count * 11);
            if (!receive_exact(tcp_socket, entries.data(), entries.size()))
                break;

            neighbors.clear();
            if (u >= 0 && u < 26)
            {
                for (int j = 0; j < 26; ++j)
                {
                    if (j != u) graph[u][j] = -1;
                }
            }

            cout << my_id << "=0";
            for (int i = 0; i < count; ++i)
            {
                char *p = entries.data() + i * 11;
                char n_id = p[0];

                int cost_net;
                memcpy(&cost_net, p + 1, sizeof(cost_net));
                int cost = ntohl(cost_net);

                sockaddr_in n_addr{};
                n_addr.sin_family = AF_INET;
                memcpy(&n_addr.sin_addr, p + 5, 4);
                memcpy(&n_addr.sin_port, p + 9, 2);

                neighbors.push_back({n_id, n_addr, cost});

                int v = n_id - 'A';
                if (u >= 0 && u < 26 && v >= 0 && v < 26)
                {
                    if (v + 1 > num_nodes)
                        num_nodes = v + 1;
                    graph[u][v] = cost;
                    graph[v][u] = cost;
                }

                cout << "," << n_id << "=" << cost;
            }
            cout << endl;

            broadcast_lsp(udp_socket, my_id, my_seq, highest_seq, neighbors);
        }

        // Neighbor LSPs (UDP)
        if (FD_ISSET(udp_socket, &read_fds))
        {
            char buf[1024];
            sockaddr_in src_addr{};
            socklen_t addr_len = sizeof(src_addr);
            int bytes = recvfrom(udp_socket, buf, sizeof(buf), 0, (sockaddr *)&src_addr, &addr_len);
            if (bytes >= 6)
            {
                char origin = buf[0];
                int u = origin - 'A';
                if (u >= 0 && u < 26 && origin != my_id)
                {
                    int seq_net;
                    memcpy(&seq_net, buf + 1, sizeof(int));
                    int seq = ntohl(seq_net);

                    if (seq > highest_seq[u])
                    {
                        highest_seq[u] = seq;
                        if (u + 1 > num_nodes)
                            num_nodes = u + 1;

                        int count = (unsigned char)buf[5];
                        if (bytes >= 6 + count * 5)
                        {
                            for (int j = 0; j < 26; ++j)
                            {
                                if (j != u)
                                    graph[u][j] = -1;
                            }

                            for (int i = 0; i < count; ++i)
                            {
                                char n_id = buf[6 + i * 5];
                                int cost_net;
                                memcpy(&cost_net, buf + 6 + i * 5 + 1, sizeof(int));
                                int cost = ntohl(cost_net);

                                int v = n_id - 'A';
                                if (v >= 0 && v < 26)
                                {
                                    if (v + 1 > num_nodes)
                                        num_nodes = v + 1;
                                    graph[u][v] = cost;
                                    graph[v][u] = cost;
                                }
                            }

                            // Forward to other neighbors
                            for (int i = 0; i < neighbors.size(); ++i)
                            {
                                if (neighbors[i].addr.sin_addr.s_addr == src_addr.sin_addr.s_addr &&
                                    neighbors[i].addr.sin_port == src_addr.sin_port)
                                {
                                    continue;
                                }
                                sendto(udp_socket, buf, bytes, 0, (sockaddr *)&neighbors[i].addr, sizeof(neighbors[i].addr));
                            }
                        }
                    }
                }
            }
        }

        // Periodic tasks (periodic LSP broadcast & periodic graph printing)
        time_t now = time(nullptr);
        if (my_id != 0 && now - last_periodic >= 10)
        {
            last_periodic = now;
            broadcast_lsp(udp_socket, my_id, my_seq, highest_seq, neighbors);

            for (int i = 0; i < num_nodes; ++i)
            {
                for (int j = 0; j < num_nodes; ++j)
                {
                    if (j > 0)
                        cout << "\t";
                    cout << graph[i][j];
                }
                cout << endl;
            }
        }
    }

    close(tcp_socket);
    close(udp_socket);
    return 0;
}