from hexagon import hexagon_bot
from random import randint
import sys

class random_bot(hexagon_bot):
    def handle_turn_data(self, map):
        transfer_from   = randint(0,len(map)-1)
        transfer_to     = randint(0,5)
        transfer_amount = randint(1,map[transfer_from][0][3])
        #print(transfer_from)

        q0 = map[transfer_from][0][0]
        r0 = map[transfer_from][0][1]
        q1 = map[transfer_from][1][transfer_to][0]
        r1 = map[transfer_from][1][transfer_to][1]

        self.submit_transaction(transfer_amount,  q0,r0, q1,r1)

r = random_bot()
r.connect('127.0.0.1', 1111)

s = ' '.join(sys.argv[1:])
r.command(s)

while 1:
    r.receive_data()
