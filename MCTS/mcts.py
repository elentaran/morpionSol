from Node import Node
from Problem import AllRight

prob = AllRight()

def mcts(nbEvl=1000):
    for i in range(nbEval):
        frontierNode = descent()
        finalVal = monteCarlo(frontierNode)
        frontierNode.add()
        frontierNode.update(finalVal)
    return res

def descent():
    return nextNode

print "coucou"
a = Node()


print mcts()


