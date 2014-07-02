#include <iostream>
#include <cstdlib>

#include "morpion.h"

using namespace std;

#ifndef MU
#define MU 10
#endif

#ifndef LAMBDA
#define LAMBDA 10
#endif

#ifndef GEN
#define GEN 10000
#endif


#define POINTS_PER_CURVE 100

float percentBest = 1.0;
float percentGood = 0;

#ifdef POINTS_PER_CURVE
float limPPC=0;
float incPPC=(GEN * LAMBDA)/POINTS_PER_CURVE;
#endif


//#define RESTART 5
//#define SAVEBEST

int level = 3;
int nbTimes = 1;
int nbSearches = 100;


int bestscore = 0;
int nbPlayouts = 0;
int nbEval=0;

class Pop{

    public:
        Morp population[LAMBDA*2];
        double bestScore;


        Pop() {
            init();
        }

        void init()
        {
            for (short p=0; p<LAMBDA; p++)
                population[p].initRand();
            for (short p=LAMBDA; p<LAMBDA*2; p++)
                population[p].initFast();
        }

        void print()
        {
            //cout<<"Score: " << population[0].score<<endl;
            cout<<"best Score: " << bestScore<<endl;
        }


        void evolve()
        {
#ifdef RESTART
            for (short restart=0; restart<RESTART; restart++)
            {
                //cerr<<"Restart"<<endl;
                //perturb();
                //double bestVal = population[0].readBest();
                for (short l=0; l<LAMBDA; l++)
                    population[l].reinit();
                init();
#endif
                int generation=0;

                    while (generation<GEN)
                    {
                        generate();
                        select();
#ifdef SAVEBEST
                        population[0].updateBest();
#endif
                        if (population[0].score > bestScore)
                            bestScore = population[0].score;
                        generation++;
                        if (generation%int(GEN/100)==0)
                        {
                            //cout<<"Score: "<<generation<< endl;
                            //cout<<"Score: "<<population[0].score<<endl;

                        }

#ifdef POINTS_PER_CURVE
                        if (nbEval > limPPC) {
                            cout << "Score: " << bestScore << endl;
                            cout << "eval: " << nbEval << endl;
                            limPPC += incPPC;
                        }
#endif

                    }
#ifdef RESTART
            }
#endif
        }


        void generate()
        {
            srand(time(NULL));
            short nbGenFromBest = percentBest * LAMBDA;
            short nbGenFromGood = percentGood * LAMBDA;

            for (short i=0; i<LAMBDA; i++)
            {
                if (i<nbGenFromGood)
                {
                    population[i+LAMBDA]=population[i];
                    population[i+LAMBDA].mutate();
                }
                else if (i<nbGenFromGood + nbGenFromBest)
                {
                    population[i+LAMBDA]=population[0];
                    population[i+LAMBDA].mutate();
                } 
                else 
                {
                    population[i+LAMBDA].initRand();
                }
            }
        }


        int part(int p, int r)
        {
            short compt=p;
            short pivot=population[p].score;
            int i;

            for (i=p+1; i<=r; i++)
            {
                if ( population[i].score > pivot )
                        {
                            compt++;
                            Morp temp;
                            temp = population[i];
                            population[i] = population[compt];
                            population[compt] = temp;
                        }
            }
            Morp temp;
            temp = population[p];
            population[p] = population[compt];
            population[compt] = temp;
            return compt;
        }


        void sortVector(int p, int r)
        {
            int q;
            if (p<r)
            {
                q = part(p, r);
                sortVector(p, q-1);
                sortVector(q+1, r);
            }
        }

        void select()
        {
            //#define COMMA
#ifdef COMMA
            sortVector(LAMBDA, LAMBDA*2-1);
            for (short i=0; i<LAMBDA; i++)
                population[i]=population[i+LAMBDA];
#else
            sortVector(0, LAMBDA*2-1);
#endif
        }



};




void usage () {
    fprintf (stderr, "morpionBeam -level 2 -nbTimes 17 -nbSearches 20\n");
}

double SA(int nbGen) {
    Morp pop[MU+LAMBDA] ;
    for (int i=0; i<MU; i++)
        pop[i].initRand();
    for (int i=0; i<=nbGen; i++) {
        for (int j=0; j<MU; j++)
            pop[j].eval();
        for (int j=MU; j<LAMBDA+MU; j++) {
            pop[j] = pop[j%MU].copy();
            pop[j].mutate();
        }

        // order pop




        if (i%(nbGen/10) == 0) {
            cout << "eval: " << i << endl;
            cout << "gen score: " << pop[0].score << endl;
            cout << "best score: " << bestscore << endl;
            cout << "sigma: " << pop[0].sigma << endl;
            cout << "nbPlayout: " << nbPlayouts << endl;
        }
    }
    return pop[0].score;
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
            cout << "eval: " << i << endl;
            cout << "gen score: " << parent.score << endl;
            cout << "best score: " << bestscore << endl;
            cout << "sigma: " << parent.sigma << endl;
            cout << "nbPlayout: " << nbPlayouts << endl;
        }
    }
    return parent.score;
}


double onePlusOne(int nbGen) {
    Morp parent,child;
    parent.initRand();
    for (int i=0; i<=nbGen; i++) {
        parent.eval();
        child = parent.copy();
        child.mutate();
        //cout << "test2 score: " << test2.score << "/" << test.score << endl;
        if (child.score >= parent.score) {
            parent = child;
            parent.sigma = 2.0 * parent.sigma;
        } else {
            parent.sigma = pow(2.0,-1.0/4)*parent.sigma;
        }
        if (i%(nbGen/nbPoints) == 0) {
            cout << "eval: " << i << endl;
            cout << "gen score: " << parent.score << endl;
            cout << "best score: " << bestscore << endl;
            cout << "sigma: " << parent.sigma << endl;
            cout << "nbPlayout: " << nbPlayouts << endl;
        }
    }
    return parent.score;
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
     //cout << onePlusOne(100000) << endl;
     //exit(0);


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
        score = p.NRPA(level);
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


