# Link State Emulator

## Compile
make

## 1. Start Oracle Node (Terminal 1)
python3 server.py topology.txt 127.0.0.1

## 2. Start Virtual Nodes (Run each in a separate terminal)
# Usage: ./node <oracle-ip> <own-ip> <oracle-port> <udp-port>
./node 127.0.0.1 127.0.0.1 5000 6001
./node 127.0.0.1 127.0.0.1 5000 6002
./node 127.0.0.1 127.0.0.1 5000 6003
./node 127.0.0.1 127.0.0.1 5000 6004
./node 127.0.0.1 127.0.0.1 5000 6005
