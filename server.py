import os
import socket
import struct
import sys


def read(path):
    rows = [list(map(int, l.split())) for l in open(path) if l.strip()[:1] not in ("", "#")]
    n = len(rows) + 1
    m = [[0] * n for _ in range(n)]
    for i, r in enumerate(rows):
        if len(r) != n - 1 - i or any(c == 0 or c < -1 for c in r):
            return None
        for k, c in enumerate(r):
            m[i][i + 1 + k] = m[i + 1 + k][i] = c
    return m


def send(i):
    # extra VNs beyond the config file are islands: only their own tuple
    nb = [(i, 0)] + ([(j, c) for j, c in enumerate(m[i]) if j != i and c != -1] if i < len(m) else [])
    msg = struct.pack("!BB", 2, len(nb))
    for j, c in nb:
        msg += struct.pack("!BI4sH", 65 + j, c, vns[j][1], vns[j][2])
    vns[i][0].sendall(msg)
    print(",".join(f"{chr(65 + j)}={c}" for j, c in nb))


def accept():
    conn, _ = srv.accept()
    _, vn_ip, port = struct.unpack("!B4sH", conn.recv(7, socket.MSG_WAITALL))
    vns.append((conn, vn_ip, port))
    print(chr(64 + len(vns)), "connected:", socket.inet_ntoa(vn_ip), port)


path, ip = sys.argv[1], sys.argv[2]
m = read(path)
if m is None:
    sys.exit("Invalid adjacency matrix")

srv = socket.create_server((ip, 5000))
print("ON listening on", ip, "port 5000, waiting for", len(m), "VNs")

vns = []
while len(vns) < len(m):
    accept()

for i in range(len(m)):
    send(i)

last = os.stat(path).st_mtime
srv.settimeout(1)
while True:
    try:
        accept()
        send(len(vns) - 1)
    except socket.timeout:
        pass
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
