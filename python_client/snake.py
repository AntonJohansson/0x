from hexagon import hexagon_bot
from random import randint
import sys

def hex_max(list):
    largest_hex = list[0]
    for hex in list:
        if hex[0][3] >= largest_hex[0][3]:
            largest_hex = hex
    return largest_hex

def find_in_list(cell, list):
    for hex in list:
        if(cell[0] == hex[0] and cell[1] == hex[1]):
            return True
    return False

class random_bot(hexagon_bot):
    def __init__(self):
        super(random_bot,self).__init__()
        self.done_list = []

    def handle_turn_data(self, map):
        head, head_neighbours = hex_max(map)
        #print("head resources: {}".format(head[3]))
        #print("done_list size: {}".format(len(self.done_list)))

        for cell,neighbours in map:
            if not find_in_list(cell, self.done_list) and (cell != head):
                self.done_list.append(cell)
                self.submit_transaction(cell[3]-1,  cell[0], cell[1], head[0], head[1])
                return;

        self.done_list = []

        enemies = []
        for n in head_neighbours:
            if n[2] == 2:
                enemies.append(n)
        if len(enemies) == 0:
            i = randint(0,5)
            enemies.append(head_neighbours[i])

        target = enemies[0]
        for hex in enemies:
            if hex[3] <= target[3]:
                target = hex

        self.submit_transaction(head[3]-1, head[0], head[1], target[0], target[1])

r = random_bot()
r.connect('127.0.0.1', 1111)

s = ' '.join(sys.argv[1:])
r.command(s)

while 1:
    r.receive_data()
