import socket

sock = socket.socket()
sock.connect(('127.0.0.1', 1111))

sock.setblocking

#sock.send(bytes("create_or_join observer hej 4", 'utf8'))

while 1:
    data = input()
    sock.send(bytes(data, 'utf8'))

    #data = sock.recv(1024)
    #print(data)
