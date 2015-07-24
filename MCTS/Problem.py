class AllRight:

    maxProf = 10

    def __init__(self):
        self.seqMove=[]

    def __str__(self):
        string = ""
        for move in self.seqMove:
            string += move + " "
        return string

    def getMoves(self):
        if len(self.seqMove) >= self.maxProf:
            return None 
        else:
            return ["left","right"]

    def getValue(self):
        val = 0
        for move in self.seqMove:
            if move == "right":
                val+=1

        return val

    def playMove(self,move):
        self.seqMove.append(move)

