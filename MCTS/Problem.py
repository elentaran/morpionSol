class AllRight:

    maxProf = 100

    def __init__(self,situation=None):
        if situation is None:
            self.situation = []
        else:
            self.situation = situation

    def __str__(self):
        string = ""
        for move in self.situation:
            string += move + " "
        return string

    def getMoves(self):
        if len(self.situation) >= self.maxProf:
            return [] 
        else:
            return ["left","right"]

    def getValue(self, incr=1):
        val = 0
        inc = 1
        for move in self.situation:
            if move == "right":
                val+=inc
            inc*=incr

        return val/self.maxProf

    def playMove(self,move):
        self.situation.append(move)

