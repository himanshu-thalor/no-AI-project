import socket, sys

server_ip = sys.argv[1]

c = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
c.connect((server_ip, 5000))
c.sendall(b"hello")
print("Got:", c.recv(1024))
c.close()