import socket

sock = socket.socket()
sock.connect(('127.0.0.1', 1111))

sock.setblocking

while 1:
    data = input()
    sock.send(bytes(data, 'utf8'))

    data = sock.recv(1024)
    print(data)
