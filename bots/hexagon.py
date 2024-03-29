from crc32 import buffer_crc32
import socket
import struct

# struct.unpack.. format
#   x 	pad byte 	    no value 	  	 
#   c 	char 	            string of length 1 	 
#   b 	signed char 	    integer 	        1 	
#   B 	unsigned char 	    integer 	        1 	
#   ? 	_Bool 	            bool 	        1 	
#   h 	short 	            integer 	        2 	
#   H 	unsigned            short integer 	2 	
#   i 	int 	            integer 	        4 	
#   I 	unsigned int 	    integer 	        4 	
#   l 	long 	            integer 	        4 	
#   L 	unsigned long 	    integer 	        4 	
#   q 	long long 	    integer 	        8 	
#   Q 	unsigned long long  integer 	        8 	
#   f 	float 	            float 	        4 	
#   d 	double 	            float 	        8 	
#   s 	char[] 	            string 	  	 
#   p 	char[] 	            string 	  	 
#   P 	void * 	            integer 	  	        

# Packet ids
INVALID             = 0
SESSION_LIST        = 1
OBSERVER_MAP        = 2
PLAYER_MAP          = 3
COMMAND             = 4
TURN_TRANSACTION    = 5
ERROR_MESSAGE       = 6
CONNECTED_TO_LOBBY  = 7

class hexagon_bot:
    def __init__(self):
        self.server = socket.socket()
        self.lobby_id = -1
        self.turn_stack = ""

    def unpack_hex(self, data, index):
        q, r, player, res = struct.unpack_from('>iiiI', data, index)
        return q,r,player,res
    
    def interpret_turn_data(self, data):
        result = []
    
        ## 4 ints per hex -> 16 bytes per hex
        ## 7 hexes per set
    
        hex_sets = int(len(data)/(4*4*7))
        for i in range(hex_sets):
            h = self.unpack_hex(data, i*4*4*7)
            neighbour_list = []
            for j in range(6):
                n = self.unpack_hex(data, i*4*4*7 + (j+1)*4*4)
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
        header = struct.pack('>IB', len(message), COMMAND)
        self.server.send(header + bytes(message,'utf8'))

    def submit_transaction(self, amount, q0,r0, q1,r1):
        #print("sending transaction ({},{}) -{}-> {},{}".format(q0,r0,amount,q1,r1))
        self.turn_stack += "\tsubmitting transaction {}: {},{} - {}-> {},{}\n".format(self.lobby_id, q0,r0,amount,q1,r1)
        payload = struct.pack('>QIiiii', self.lobby_id, amount, q0,r0, q1,r1)
        header = struct.pack('>IB', len(payload), TURN_TRANSACTION);
        self.server.send(header + payload)

    def handle_turn_data(self, map):
        print('You forgot to override handle_turn_data(...)')

    def receive_data(self):
        print("received data")
        #print("begin");
        # packet_size is 4 bytes
        # packet_size only concerns actual payload and not 
        # the packet id. This might be bad practice or something idunno
        self.turn_stack += "\treceiving packet size\n"
        packet_size_data = self.server.recv(4)
        packet_size = struct.unpack('>I', packet_size_data)[0]

        # get the packet id which is an unsigned char
        self.turn_stack += "\treceiving packet id\n"
        packet_id_data = self.server.recv(1)
        packet_id = struct.unpack('>B', packet_id_data)[0]

        # crc
        self.turn_stack += "\treceiving payload crc\n"
        crc_data = self.server.recv(4)
        crc = struct.unpack('>I', crc_data)[0]

        self.turn_stack += "\treceiving payload\n"
        data = self.server.recv(packet_size)

        payload_crc = buffer_crc32(data, packet_size)
        if crc != payload_crc:
            print("CRC mismatch ({:04x} != {:04x})!".format(crc,payload_crc % 2**32))

        if packet_id == INVALID:
            print('Invalid packet, server (or network) error')
        elif packet_id == PLAYER_MAP:
            self.turn_stack += "\thandling player map\n"
            #print("receiveed palyer map")
            map = self.interpret_turn_data(data)
            self.handle_turn_data(map)
        elif packet_id == ERROR_MESSAGE:
            self.turn_stack += "\thandling error message\n"
            length = struct.unpack_from('>I', data)[0]
            string = struct.unpack_from('>'+str(length)+'s', data, 4)[0]
            print("Error: " + string.decode('utf8'))
            #print("callstack-ish:\n" + self.turn_stack)
            self.turn_stack = ""
            #exit()
        elif packet_id == CONNECTED_TO_LOBBY:
            self.lobby_id = struct.unpack_from('>Q', data)[0]
            print("connected to lobby: {}".format(self.lobby_id))
        else:
            print('Unknown packet id {}'.format(packet_id))
