from hexagon import hexagon_bot
from random import randint
import sys

class random_bot(hexagon_bot):
    def handle_turn_data(self, map):
        
        transfer_from = randint(0,len(map)-1)
        c0 = map[transfer_from][0]

        for cell, neigh in map:
            for n in neigh:
                if n[2] == 0 or (n[2] == 2 and c0[3]/2 > n[3]):
                    self.submit_transaction(round(c0[3]/2), c0[0], c0[1], n[0], n[1])
                    return

        #print("didnt find one :(")
        transfer_from   = randint(0,len(map)-1)
        transfer_to     = randint(0,5)
        transfer_amount = randint(1,map[transfer_from][0][3])
        #print(transfer_from)

        q0 = map[transfer_from][0][0]
        r0 = map[transfer_from][0][1]
        q1 = map[transfer_from][1][transfer_to][0]
        r1 = map[transfer_from][1][transfer_to][1]

        #dlist = []
        #for l in map:
        #    dlist.append(l[0])
        #print("duplicates?: {}", len(map) != len(dlist))

        #print("------")
        #for l in map:
        #    print(l[0])

        #print("id: {}/{}\t{}/{}\t---\t({},{})\t[{}]\t({},{})".format(transfer_from, len(map), transfer_to, 5, q0,r0,transfer_amount,q1,r1))
        #print(map)
        #print(len(map))

        self.submit_transaction(transfer_amount,  q0,r0, q1,r1)

r = random_bot()
r.connect('127.0.0.1', 1111)

s = ' '.join(sys.argv[1:])
r.command(s)

while 1:
    r.receive_data()
