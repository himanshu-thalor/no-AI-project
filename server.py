import os
import socket
import struct
import sys
import time


def read(path):
    m = [list(map(int, l.split())) for l in open(path) if l.strip()[:1] not in ("", "#")]
    n = len(m)
    if all(len(r) == n for r in m) and all(
        m[i][j] == m[j][i] and (i == j or m[i][j] > 0 or m[i][j] == -1) for i in range(n) for j in range(n)
    ):
        return m


def send(i):
    nb = [(j, c) for j, c in enumerate(m[i]) if j != i and c != -1]
    msg = struct.pack("!BBB", 2, 65 + i, len(nb))
    for j, c in nb:
        msg += struct.pack("!BI4sH", 65 + j, c, vns[j][1], vns[j][2])
    vns[i][0].sendall(msg)
    print(",".join([f"{chr(65 + i)}=0"] + [f"{chr(65 + j)}={c}" for j, c in nb]))


path, ip = sys.argv[1], sys.argv[2]
m = read(path)
if m is None:
    sys.exit("Invalid adjacency matrix")

srv = socket.create_server((ip, 5000))
print("ON listening on", ip, "port 5000, waiting for", len(m), "VNs")

vns = []
while len(vns) < len(m):
    conn, _ = srv.accept()
    _, vn_ip, port = struct.unpack("!B4sH", conn.recv(7, socket.MSG_WAITALL))
    vns.append((conn, vn_ip, port))
    print(chr(64 + len(vns)), "connected:", socket.inet_ntoa(vn_ip), port)

for i in range(len(m)):
    send(i)

last = os.stat(path).st_mtime
while True:
    time.sleep(1)
    if os.stat(path).st_mtime == last:
        continue
    last = os.stat(path).st_mtime
    new = read(path)
    if new is None or len(new) != len(m):
        print("Invalid adjacency matrix")
        continue
    old, m = m, new
    for i in range(len(m)):
        if m[i] != old[i]:
            send(i)
