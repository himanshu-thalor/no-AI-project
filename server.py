import socket

srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("", 5000))
srv.listen()

conn, addr = srv.accept()
print("Connected by", addr)

data = conn.recv(1024)
print("Got:", data)
conn.sendall(b"hello back")

conn.close()
srv.close()