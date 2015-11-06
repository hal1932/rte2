# encoding: utf-8
import socket
from contextlib import closing

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    with closing(sock):
        print "connect", sock.connect(("127.0.0.1", 0x1234))
        print "send", sock.send(b"1234")
        print "recv", sock.recv(1024)
    print "end"

if __name__ == "__main__":
    main()
