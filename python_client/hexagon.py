from crc32 import buffer_crc32
import socket
import struct

# Packet ids
INVALID             = 0
SESSION_LIST        = 1
OBSERVER_MAP        = 2
PLAYER_MAP          = 3
COMMAND             = 4
TURN_TRANSACTION    = 5
ERROR_MESSAGE       = 6

class hexagon_bot:
    def __init__(self):
        self.server = socket.socket()
        self.last_packet_info = ""

    def unpack_hex(self, data, index):
        q, r, player, res = struct.unpack_from('>4i', data, index)
        return q,r,player,res
    
    def interpret_turn_data(self, data):
        result = []
    
        ## 4 ints per hex -> 16 bytes per hex
        ## 7 hexes per set
    
        hex_sets = int(len(data)/(16*7))
        for i in range(hex_sets):
            h = self.unpack_hex(data, i*28*4)
            neighbour_list = []
            for j in range(6):
                n = self.unpack_hex(data, (j+1)*4*4)
                neighbour_list.append(n)
            result.append([h, neighbour_list])
            #struct.unpack_from('>' + str(16*7) + 'i', data)
        #print(len(data)/4)
        #unpacked_data = struct.unpack('>' + str(int(len(data)/4)) + 'i', data)
        #print(unpacked_data)
        #print(len(unpacked_data)/(28))
        return result
        #return unpacked_data, len(unpacked_data)/(SIZE_PER_HEX*7)

    def connect(self, ip, port):
        self.server.connect((ip, port))
        self.server.setblocking(1)

    def command(self, message):
        self.server.send(bytes(message, 'utf8'))

    def submit_transaction(self, amount, q0,r0, q1,r1):
        self.server.send(struct.pack('>Biiiii', 3, amount, q0,r0, q1,r1))

    def handle_turn_data(self, map):
        print('You forgot to override handle_turn_data(...)')

    def receive_data(self):
        # packet_size is 4 bytes
        # packet_size only concerns actual payload and not 
        # the packet id. This might be bad practice or something idunno
        packet_size_data = self.server.recv(4)
        packet_size = struct.unpack('>I', packet_size_data)[0]

        # get the packet id which is an unsigned char
        packet_id_data = self.server.recv(1)
        packet_id = struct.unpack('>B', packet_id_data)[0]

        # crc
        crc_data = self.server.recv(4)
        crc = struct.unpack('>I', crc_data)[0]

        data = self.server.recv(packet_size)

        payload_crc = buffer_crc32(data, packet_size)
        if crc != payload_crc:
            print("CRC mismatch ({:04x} != {:04x})!".format(crc,payload_crc % 2**32))

        if packet_id == INVALID:
            print('Invalid packet, server (or network) error')
        elif packet_id == PLAYER_MAP:
            map = self.interpret_turn_data(data)
            self.handle_turn_data(map)
        elif packet_id == ERROR_MESSAGE:
            length = struct.unpack_from('>I', data)[0]
            string = struct.unpack_from('>'+str(length)+'s', data, 4)[0]
            print("Error: " + string.decode('utf8'))
        else:
            print('Unknown packet id {}'.format(packet_id))
