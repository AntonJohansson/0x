import socket
import struct
from random import randint

SIZE_PER_HEX = 4 # bytes

sock = socket.socket()
sock.connect(('127.0.0.1', 1111))

sock.setblocking(0)

#data = sock.recv(1024);
#print(struct.unpack('>Bi', data))

#sock.send(bytes("create_or_join observer hej 4", 'utf8'))

def create_transfer_packet(res,q0,r0,q1,r1):
    return struct.pack('>Biiiii', 3, res,q0,r0,q1,r1)

def hex(data, i):
    return data[i*4:(i+1)*4]

def hex_neighbours(data, i):
    return data[i*28 + 4:i*28 + 7*4], 6


def interpret_turn_data(data):
    print(len(data)/4)
    unpacked_data = struct.unpack('>' + str(int(len(data)/4)) + 'i', data)
    return unpacked_data, len(unpacked_data)/(SIZE_PER_HEX*7)

while 1:
    data = input()
    sock.send(bytes(data, 'utf8'))

    try:
        data = sock.recv(1024)
    except:
        continue

    print("{} bytes received".format(len(data)))
    U, i = interpret_turn_data(data)
    random_hex_id = randint(0, i-1)
    first_hex = hex(U, random_hex_id)

    n,l = hex_neighbours(U, random_hex_id)
    random_new_hex_id = randint(0,l-1)
    new_hex = hex(n, random_new_hex_id)

    print(random_hex_id)
    print(first_hex)
    transfer_amount = randint(0,first_hex[3])

    print("Transferring {} from ({},{}) to ({},{})".format(transfer_amount, first_hex[0], first_hex[1], new_hex[0], new_hex[1]))
    b = create_transfer_packet(transfer_amount, first_hex[0],first_hex[1],new_hex[0],new_hex[1])
    sock.send(b);
