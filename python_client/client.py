import socket
import struct
import sys
from random import randint

INVALID             = 0
SESSION_LIST        = 1
OBSERVER_MAP        = 2
PLAYER_MAP          = 3
COMMAND             = 4
TURN_TRANSACTION    = 5
ERROR_MESSAGE       = 6

SIZE_PER_HEX = 4 # bytes

sock = socket.socket()
sock.connect(('127.0.0.1', 1111))

sock.setblocking(1)

s = ' '.join(sys.argv[1:])
sock.send(bytes(s, 'utf8'))

def create_transfer_packet(res,q0,r0,q1,r1):
    return struct.pack('>Biiiii', 3, res,q0,r0,q1,r1)

def hex(data, i):
    return data[i*4:(i+1)*4]

#def hex_neighbours(data, i):
#    return data[i*28 + 4:i*28 + 7*4], 6

def unpack_hex(data, index):
    q, r, player, res = struct.unpack_from('>4i', data, index)
    return q,r,player,res

def interpret_turn_data(data):
    result = []

    ## 4 ints per hex -> 16 bytes per hex
    ## 7 hexes per set

    hex_sets = int(len(data)/(16*7))
    for i in range(hex_sets):
        h = unpack_hex(data, i*28*4)
        neighbour_list = []
        for j in range(6):
            n = unpack_hex(data, (j+1)*4*4)
            neighbour_list.append(n)
        result.append([h, neighbour_list])
        #struct.unpack_from('>' + str(16*7) + 'i', data)
    #print(len(data)/4)
    #unpacked_data = struct.unpack('>' + str(int(len(data)/4)) + 'i', data)
    #print(unpacked_data)
    #print(len(unpacked_data)/(28))
    return result
    #return unpacked_data, len(unpacked_data)/(SIZE_PER_HEX*7)

while 1:
    #try:
    #print("- waiting on packet size")
    packet_size_data = sock.recv(4)
    packet_size = struct.unpack('>I', packet_size_data)[0]

    # packet_size only concerns actual payload and not 
    # the packet id. This might be bad practice or something idunno
    packet_id_data = sock.recv(1)
    packet_id = struct.unpack('>B', packet_id_data)[0]
    #packet_size -= 1;

    print("data size: {}".format(packet_size))
    data = sock.recv(packet_size)

    #packet_id_data = sock.recv(1);
    #packet_id = data[0]
    #print("packet id: {}".format(packet_id))

    #packet_size_data -= 1;
    

    if packet_id == ERROR_MESSAGE:
        length = struct.unpack_from('>I', data)[0]
        string = struct.unpack_from('>'+str(length)+'s', data, 4)[0]
        print("Error: " + str(string))
    elif packet_id == PLAYER_MAP:
        #print("-- received packet of size {}!".format(packet_size))
        #print("- waiting for data")
        #print("-- received data of size {}".format(len(data)))
        #except:
        #    continue

        #print("{} bytes received".format(len(data)))
        map = interpret_turn_data(data)

        transfer_from   = randint(0,len(map)-1)
        transfer_to     = randint(0,5)
        transfer_amount = randint(1,map[transfer_from][0][3])
        print(transfer_from)

        q0 = map[transfer_from][0][0]
        r0 = map[transfer_from][0][1]
        q1 = map[transfer_from][1][transfer_to][0]
        r1 = map[transfer_from][1][transfer_to][1]

        #print(" - Transferring {} from ({},{}) to ({},{})".format(transfer_amount, q0,r0,q1,r1))
        b = create_transfer_packet(transfer_amount, q0,r0,q1,r1)
        sock.send(b);
        #transfer_amount = 

        #random_hex_id = randint(0, i-1)
        #first_hex = hex(U, random_hex_id*7)

        #n,l = hex_neighbours(U, random_hex_id)
        #random_new_hex_id = randint(0,l-1)
        #new_hex = hex(n, random_new_hex_id)

        ##print(random_hex_id)
        ##print(first_hex)
        #transfer_amount = randint(1,first_hex[3])

