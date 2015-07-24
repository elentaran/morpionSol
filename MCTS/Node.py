class Node:

    def __init__(self,father=None):
        self.father = father
        self.children = []
        self.sumValues = 0
        self.nbValues = 0

    def __str__(self):
        if (self.nbValues != 0):
            return str(self.sumValues/self.nbValues)
        else:
            return "0"

    def add(self):

    def update(self, val):
        self.


print "Node loaded"

