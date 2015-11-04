# encoding: utf-8
import socket
import time
from contextlib import closing

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    with closing(sock):
        sock.bind(("127.0.0.1", 0x1234))
        sock.listen(1)
        while True:
            conn, addr = sock.accept()
            with closing(conn):
                data = conn.recv(1024)
                print data
                conn.send(data)

if __name__ == "__main__":
    main()
