# encoding: utf-8
import socket
from contextlib import closing

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    with closing(sock):
        sock.connect(("127.0.0.1", 0x1234))
        sock.send(b"1234")
        print sock.recv(1024)

if __name__ == "__main__":
    main()
