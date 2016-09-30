import sys
import time
from random import random,choice
from copy import deepcopy
from Node import Node
from Problem import AllRight

prob = AllRight()
root = Node(problem=prob)

# parametre
incr = 1
cUCT = 1.0

# save the best solution during the run
bestVal = 0
bestSol = None

def mcts(nbEval=10):
    for i in range(nbEval):
        frontierNode = descent(root)
        finalVal = monteCarlo(frontierNode)
        frontierNode.add()
        frontierNode.update(finalVal)
        if (root.final):
            break
    #return root.getVal()
    return bestVal

def descent(node):
    curNode = node
    while curNode.confirm and not(curNode.final) :
        curNode = curNode.getBestChildUct(cUCT)
    return curNode

def choseMC(moves):
    return choice(moves)

def monteCarlo(node):
    newProb = deepcopy(node.problem)
    moves = newProb.getMoves()
    while (len(moves) > 0):
        selectedMove = choseMC(moves)
        newProb.playMove(selectedMove)
        moves = newProb.getMoves()
    #print( newProb)
    #print newProb.getValue()
    val = newProb.getValue(incr)
    global bestVal
    global bestSol
    if val > bestVal:
        bestVal = val
        bestSol = newProb
    return val

def randAlg(nbEval=10):
    resVal = 0
    for i in range(nbEval):
        res = monteCarlo(root)
        if res > resVal:
            resVal = res
    return resVal






print( "coucou")

if (len(sys.argv)>3):
    cUCT = float(sys.argv[3])

if (len(sys.argv)>2):
    incr = float(sys.argv[2])


if (len(sys.argv)>1):
    print( "debut mcts")
    startT = time.clock()
    print("Score " + str(mcts(int(sys.argv[1]))))
    print("Time " + str( time.clock() - startT))
    print( "debut rand")
    startT = time.clock()
    print("Score " + str( randAlg(int(sys.argv[1]))))
    print("Time " + str( time.clock() - startT))
else:
    print( mcts(100))
    print( randAlg(100))

"""
print( root.nbValues)
print( root.children[0].strVal())
print( root.children[1].strVal())
print( bestVal)
print( bestSol)
"""
