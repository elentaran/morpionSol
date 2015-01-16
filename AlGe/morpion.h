#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <list>
#include <algorithm>
#include <vector>

using namespace std;

//#define SENDMAIL
#define VERBOSE
//#define SAVEBEST

#ifdef SAVEBEST
extern int bestVal;
#endif


const string highScoreLocation = "/home/rimmel_arp/Research/morpionSol/AlGe/allTimeBest";
const string highScoreLocationNoTouching = "/home/rimmel_arp/Research/morpionSol/AlGe/allTimeBestNoTouching";

const int MaxLengthPlayout = 250;
const int MaxMoveToUpdate = 1000;
const int Size = 50;
const int MaxCode = 4 * Size * Size;
const int MaxLevel = 5;
const int MaxBeam = 100;

const int nbPoints = 10000;

const bool touching = true;
//const bool touching = false;


const int dxdir [8] = {-1, -1, 0, 1, 1, 1, 0, -1};
const int dydir [8] = { 0,  1, 1, 1, 0,-1,-1, -1};

typedef int Move;



class Morp{

    
    public:
        char damier [Size] [Size];
        char edge [Size] [Size] [8];
        unsigned long long hash;
        unsigned long long hashDamier [Size] [Size];
        unsigned long long hashEdge [Size] [Size] [8];

        // the value of the individual
        double policy [MaxCode];
        double sigma;
        int totalScoreMove [MaxCode];
        int nbPlayMove [MaxCode];


        double score;
        double sd;

        list<int> moves;

        int lengthVariation;
        Move variation [MaxLengthPlayout];

        int scoreBestRollout, lengthBestRollout;
        Move bestRollout [MaxLengthPlayout];

        void initRand(double sigma=1);
        void initFast();
        void mutate();
        void mutateSA();
        void eval();
        Morp copy();
        double compVal();

        double generateGauss();
        void computeHash (); 
        void init () ;
        void initHash () ;
        void initPolicy () ;
        void printMap (FILE *fp) ;
        bool alignement (int x, int y, int k) ;
        void findMovesAround (int i, int j, list<int> & mvs) ;
        void findMoves () ;
        void printMovePS (FILE *fp, int num, int move) ;
        void printMovesPS (FILE *fp) ;
        void printMovesPS (char *name) ;
        int playMove (int move) ;
        int code (int move) ;
        bool operator == (const Morp & p) ;
        int undoMove (int move) ;
        bool legalMove (int move) ;
        bool alignementIncluding (int x, int y, int k, int i1, int j1) ;
        void findMovesAroundIncluding (int i, int j, int i1, int j1, int k, list<int> & mvs) ;
        void findMovesIncluding (int i1, int j1, list<int> & mvs) ;
        void updatePossibleMoves (int move) ;
        void writeBest(string fileName="") ;
        void updateBest() ;
        double getValMove(Move move) ;
        int chooseRandomMoveNRPA (list<int> & moves) ;
        int playoutNRPA () ;
        //void adapt () ;
        //int NRPA (int n) ;
};
