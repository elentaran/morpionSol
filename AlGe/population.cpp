#include <iostream>
#include <cstdlib>

#include "morpion.h"

using namespace std;



float percentBest = 1.0;
float percentGood = 0;



//#define RESTART 5
//#define SAVEBEST

int level = 3;
int nbTimes = 1;
int nbSearches = 100;


int bestscore = 0;
int nbPlayouts = 0;
int nbEval=0;

int allTimeBest=0;


void usage () {
    fprintf (stderr, "morpionBeam -level 2 -nbTimes 17 -nbSearches 20\n");
}

double simulatedAnnealing(int nbGen) {
    double proba = 0.01;
    Morp parent,child;
    parent.initRand();
    for (int i=0; i<=nbGen; i++) {
        parent.eval();
        child = parent.copy();
        child.mutate();
        //cout << "test2 score: " << test2.score << "/" << test.score << endl;
        if (child.score >= parent.score) {
            parent = child;
        //    parent.sigma = 2.0 * parent.sigma;
        } else {
            double randVal = (double) rand() / (RAND_MAX);
            if (randVal < proba)
                parent = child;

         //   parent.sigma = pow(2.0,-1.0/4)*parent.sigma;
        }
        if (i%(nbGen/10000) == 0) {
            //cout << "eval: " << i << endl;
            cout << "gen score: " << parent.score << endl;
            cout << "best score: " << bestscore << endl;
            cout << "sigma: " << parent.sigma << endl;
            cout << "nbPlayout: " << nbPlayouts << endl;
        }
    }
    return parent.score;
}


Morp onePlusOne(int nbGen) {
    Morp parent,child;
    parent.initRand();
    for (int i=0; i<=nbGen; i++) {
        parent.eval();
        child = parent.copy();
        //child.mutate();
        child.mutateSA();
        //cout << "parent: " << parent.score  << " child: " << child.score<< endl;
        //cout << "sigma: " << parent.sigma << endl;
        //cout << "test2 score: " << test2.score << "/" << test.score << endl;
        //if (child.score >= parent.score) {
        if (child.compVal() >= parent.compVal()) {
            parent = child;
           // parent.sigma = 2.0 * parent.sigma;
        } else {
           // parent.sigma = pow(2.0,-1.0/4)*parent.sigma;
        }
        if ( i%(nbGen/nbPoints) == 1) {
            cout << "eval: " << i << endl;
            cout << "gen mean: " << parent.score << " sd: " << parent.sd << " score: " << parent.compVal() << endl;
            cout << "best score: " << bestscore << endl;
            cout << "sigma: " << parent.sigma << endl;
           // cout << "nbPlayout: " << nbPlayouts << endl;
        }
    }
    return parent;
}

#ifdef SAVEBEST
int bestVal;
#endif

int readBest(string fileName="") {
    if (fileName == "") {
        if (touching)
            fileName = highScoreLocation;
        else
            fileName = highScoreLocationNoTouching;
    }

    int res=0;
    ifstream myfile;
    myfile.open(fileName.c_str());
    if (myfile.is_open())
    {
        myfile >> res;
        myfile.close();
    } else {
        cerr << "could not read the file" << endl;
    }

    return res;
}


int main (int argc, char ** argv) {

#ifdef SAVEBEST
    bestVal=readBest() ;
#endif
    struct timeval time;
    gettimeofday(&time,NULL);
    srand((time.tv_sec * 1000) + (time.tv_usec / 1000));
    
    
    //double init = atoi(argv[1]);
    //cout << init << endl;
    //srand(init);

    //srand (time(NULL));

    //Morp randM;
    //int bestS=0;
    //for (int i=0; i<1001; i++) {
    //    randM.initRand();
    //    randM.eval();
    //    if (randM.score > bestS)
    //        bestS = randM.score;
    //    if (i%100 == 0) {
    //        cout << "eval: " << i << endl;
    //        cout << "randM score: " << bestS << endl;
    //        cout << "best score: " << bestscore << endl;
    //    }
    //}
    //exit(0);

     //cout << simulatedAnnealing(100000) << endl;

     //Morp randM;
     //randM.initRand();
     //double val=0;
     //int nbTry=100;
     //for (int i=0; i<nbTry; i++) {
     //    randM.eval();
     //    val+= randM.score;
     //}
     //cout << "rand: " << val/nbTry << endl;
     
     Morp oPo = onePlusOne(1000000);

     //val=0;
     //for (int i=0; i<nbTry; i++) {
     //    oPo.eval();
     //    val+= oPo.score;
     //}
     //cout << "opo: " << val/nbTry << endl;
     
     cout << oPo.score << endl;
     exit(0);


    //Morp test,test2;
    //test.initRand();
    //test.eval();
    //for (int i=0; i<1001; i++) {
    //    test2 = test;
    //    test2.mutate();
    //    //cout << "test2 score: " << test2.score << "/" << test.score << endl;
    //    if (test2.score > test.score)
    //        test = test2;
    //    if (i%100 == 0) {
    //        cout << "eval: " << i << endl;
    //        cout << "gen score: " << test.score << endl;
    //    }
    //}
    ////cout << "eval: " << test.score << endl;
    //exit(0);

    for (int i = 1; i < argc; i++) {
        if (!strcmp (argv [i], "-level")) {
            if (sscanf (argv [++i], "%d", & level) != 1)
                usage();
        } 
        else if (!strcmp (argv [i], "-nbTimes")) {
            if (sscanf (argv [++i], "%d", & nbTimes) != 1)
                usage();
        }
        else if (!strcmp (argv [i], "-nbSearches")) {
            if (sscanf (argv [++i], "%d", & nbSearches) != 1)
                usage();
        }
        else {
            cerr << "argument non reconnu: " << argv[i] << endl;
            exit(0);
        }
    }


    Morp p,bestP;
    for (int i=0; i<nbTimes; i++) {
        p.init ();
        int score;
        //score = p.NRPA(level);
        if (score>bestscore) {
            bestscore=score;
            bestP = p;
        }
    }


    //char s [1000];
    //sprintf (s, "highScore/nested.%d.ps", bestscore);
    //p.printMovesPS(s);
    cout << "Score: " << bestscore << endl;
    cout << "Playout: " << nbPlayouts << endl;



    exit (0);


}


