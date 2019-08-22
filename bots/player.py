from hexagon import hexagon_bot
from random import randint
import sys

class random_bot(hexagon_bot):
    def handle_turn_data(self, map):
        amount, q0,r0,q1,r1 = eval(input("res, q0,r0, q1,r1: "))
        
        self.submit_transaction(amount,  q0,r0, q1,r1)

r = random_bot()
r.connect('127.0.0.1', 1111)

s = ' '.join(sys.argv[1:])
r.command(s)

while 1:
    r.receive_data()
