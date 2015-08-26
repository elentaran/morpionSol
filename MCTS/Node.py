from math import *
from random import random,choice
from copy import deepcopy

class Node:

    def __init__(self, problem = None, father=None, move=None):
        self.father = father
        self.move = move
        self.problem = problem
        self.children = []
        self.sumValues = 0
        self.nbValues = 0
        self.confirm = False
        self.final = False

    def __str__(self):
        res = str(self.problem)
        if (self.nbValues != 0):
            res += str(self.sumValues/self.nbValues)
        else:
            res += "0"
        return res

    def strVal(self):
        return str(self.move) + " " + str(self.getVal()) + " " + str(self.nbValues)

    def add(self):
        moveChildren = self.problem.getMoves()
        for move in moveChildren:
            child = Node.createChild(self,self.problem,move)
            self.children.append(child)
        self.confirm=True
        self.updateFinal()

    def updateFinal(self):
        isFinal = True
        for child in self.children:
            if not(child.final):
                isFinal = False
        self.final = isFinal
        if isFinal and not(self.father is None):
            self.father.updateFinal()

    def update(self, val):
        self.sumValues += val
        self.nbValues += 1
        if (not(self.father is None)):
            self.father.update(val)

    def createChild(father, prob, move):
        newProb = deepcopy(prob)
        newProb.playMove(move)
        child = Node(newProb,father,move)
        return child

    def getBestChildUct(self,c):
        bestChild = None
        bestVal = 0
        for child in self.children:
            if not(child.final):
                if not(child.confirm):
                    bestChild = child
                    bestVal = 1000000
                else:
                    val = self.getValUCT(child,c)
                    if val>bestVal:
                        bestChild = child
                        bestVal = val
        return bestChild

    def getVal(self):
        return self.sumValues/self.nbValues

    def getValUCT(self,child, c=1.0):
        #return child.getVal() + random()
        #val = child.getVal() + c * sqrt(log(self.nbValues)/child.nbValues)
        #print(str(child.getVal()) + " " + str(val))
        return child.getVal() + c * sqrt(log(self.nbValues)/child.nbValues)



