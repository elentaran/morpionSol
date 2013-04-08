#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#include <iostream>
#include <list>
#include <algorithm>

using namespace std;

#define TYPE_PLAYOUT 0
#define TYPE_NESTED  1
#define TYPE_NRPA    2
#define TYPE_NEWSEARCH    3

string tabSearch[4] = {"PLAYOUT","NESTED","NRPA", "NEW SEARCH"};

#define SENDMAIL
//#define VERBOSE



// algorithm variables and default values
int level = 2;
int nbTimes = 1;
int nbSearches = 100;
int searchType = TYPE_NEWSEARCH; 


const string highScoreLocation = "/home/rimmel_arp/Research/morpionSol/allTimeBest";
const string highScoreLocationNoTouching = "/home/rimmel_arp/Research/morpionSol/allTimeBestNoTouching";

const int MaxLengthPlayout = 250;
const int MaxMoveToUpdate = 1000;
const int Size = 50;
const int MaxCode = 4 * Size * Size;
const int MaxLevel = 5;
const int MaxBeam = 100;

int nbPlayouts = 0;


bool touching = true;
//bool touching = false;
int nbRollouts = 10;

int bestOverall = 70;


int dxdir [8] = {-1, -1, 0, 1, 1, 1, 0, -1};
int dydir [8] = { 0,  1, 1, 1, 0,-1,-1, -1};

typedef int Move;

unsigned long long hashDamier [Size] [Size];
unsigned long long hashEdge [Size] [Size] [8];

void initHash () {
    for (int i = 0; i < Size; i++)
        for (int j = 0; j < Size; j++) {
            hashDamier [i] [j] = 0;
            for (int k = 0; k < 64; k++)
                if ((rand () / (RAND_MAX + 1.0)) > 0.5)
                    hashDamier [i] [j] |= (1ULL << k);
            for (int e = 0; e < 8; e++) {
                hashEdge [i] [j] [e] = 0;
                for (int k = 0; k < 64; k++)
                    if ((rand () / (RAND_MAX + 1.0)) > 0.5)
                        hashEdge [i] [j] [e] |= (1ULL << k);
            }
        }
}


class Problem {
    public:
        char damier [Size] [Size];
        char edge [Size] [Size] [8];
        unsigned long long hash;

        double policy [MaxCode];
        int totalScoreMove [MaxCode];
        int nbPlayMove [MaxCode];

        int score;

        list<int> moves;

        int lengthVariation;
        Move variation [MaxLengthPlayout];

        int scoreBestRollout, lengthBestRollout;
        Move bestRollout [MaxLengthPlayout];


        void computeHash () {
            hash = 0;
            for (int i = 0; i < Size; i++)
                for (int j = 0; j < Size; j++)
                    for (int e = 0; e < 8; e++) {
                        if (edge [i] [j] [e] == '+')
                            hash ^= hashEdge [i] [j] [e];
                    }
        }

        /* un coup est codé sur un entier
         * 7 dernier bit = i
         * 7 bits precedents = j
         * 7 bits precedents = x
         * 7 bits precedents = y
         * 3 bits precedents = dir
         */

        void init () {
            int mid = Size / 2;
            for (int i = 0 ; i < Size; i++)
                for (int j = 0; j < Size; j++) {
                    damier [i] [j] = '.';
                    for (int k = 0; k < 8; k++)
                        edge [i] [j] [k] = '.';
                }

            for (int i = mid - 5 ; i < mid - 1; i++) {
                damier [i] [mid - 2] = '+';
                damier [i] [mid + 1] = '+';
                damier [mid - 2] [i] = '+';
                damier [mid + 1] [i] = '+';
            }
            damier [mid - 5] [mid - 1] = '+';
            damier [mid - 5] [mid] = '+';
            damier [mid - 1] [mid - 5]  = '+';
            damier [mid] [mid - 5]  = '+';
            for (int i = mid + 1 ; i < mid + 5; i++) {
                damier [i] [mid - 2] = '+';
                damier [i] [mid + 1] = '+';
                damier [mid - 2] [i] = '+';
                damier [mid + 1] [i] = '+';
            }
            damier [mid + 4] [mid - 1] = '+';
            damier [mid + 4] [mid] = '+';
            damier [mid - 1] [mid + 4] = '+';
            damier [mid] [mid + 4] = '+';


            hash = 0;
            lengthVariation = 0;
            findMoves ();
        }

        void initPolicy () {
            for (int i = 0; i < MaxCode; i++)
                policy [i] = 0.0;
        }

        void initScoreMove () {
            for (int i = 0; i < MaxCode; i++){
                totalScoreMove [i] = 0;
                nbPlayMove [i] = 0;
            }
        }

        void printMap (FILE *fp) {
            for (int i = 0 ; i < Size; i++) {
                for (int j = 0; j < Size; j++)
                    fprintf (fp, "%c", damier [j] [i]);
                fprintf (fp, "\n");
            }
        }


        bool alignement (int x, int y, int k) {
            int dx = 0, dy = 0;

            // bug test x et y dans la map

            if (damier [x] [y] != '+')
                return false;
            for (int m = 0; m < 4; m++) {
                if (edge [x + dx] [y + dy] [k] == '+')
                    return false;
                dx += dxdir [k];
                dy += dydir [k];

                // bug x+dx peut etre negatif à l'initialisation

                if (damier [x + dx] [y + dy] != '+')
                    return false;
            }
            if (!touching) {
                if (edge [x - dxdir [k]] [y - dydir [k]] [k] == '+')
                    return false;
                if (edge [x + 4 * dxdir [k]] [y + 4 * dydir [k]] [k] == '+')
                    return false;
            }
            return true;
        }

        void findMovesAround (int i, int j, list<int> & mvs) {
            for (int k = 0; k < 4; k++)
                for (int m = -4; m <= 0; m++) {
                    int x = i + m * dxdir [k], y = j + m * dydir [k];
                    if (alignement (x, y, k)) {
                        int move = i | (j << 7) | (x << 14) | (y << 21) | (k << 28);
                        mvs.push_back (move);
                    }
                }
        }

        void findMoves () {
            moves.clear ();
            for (int i = 0 ; i < Size; i++)
                for (int j = 0; j < Size; j++)
                    if (damier [i] [j] == '.') {
                        damier [i] [j] = '+';
                        findMovesAround (i, j, moves);
                        damier [i] [j] = '.';
                    }
        }

        void printMovePS (FILE *fp, int num, int move) {
            int mid = Size / 2;
            int i = 1 + (move&127) - mid;
            int j = 1 + ((move>>7)&127) - mid;
            int x = 1 + ((move>>14)&127) - mid;
            int y = 1 + ((move>>21)&127) - mid;
            int dir = (move>>28)&7;
            int finx = x + 4 * dxdir [dir];
            int finy = y + 4 * dydir [dir];
            fprintf (fp, "%d %d %d %d %d %d %d\n",num, i, j, x, y, finx, finy);
        }

        void printMovesPS (FILE *fp) {
            fprintf (fp, "%%!PS-Adobe-2.0 EPSF-2.0\n%%%%BoundingBox: 0 0 420 630\n%%%%Pages: %d\n",lengthBestRollout + 2);
            fprintf (fp, "/n0 36 def\n\n/board0\n5  0   5  1   5  2   4  2   3  2   2  2   2  3   2  4   2  5\n1  5   0  5  -1  5  -1  4  -1  3  -1  2  -2  2  -3  2  -4  2\n-4 1  -4  0  -4 -1  -3 -1  -2 -1  -1 -1  -1 -2  -1 -3  -1 -4\n0 -4   1 -4   2 -4   2 -3   2 -2   2 -1   3 -1   4 -1   5 -1\nn0 2 mul array astore def\n\n/step 30 def\n/stone_radius 10 def\n/X {step mul} def\n/Y {step mul neg} def\n\n/Times-Bold findfont stone_radius scalefont setfont\n\n/circle {\n    1 setlinewidth\n    newpath exch X exch Y stone_radius 0 360 arc stroke\n} def\n\n/fillcircle {\n    1 setlinewidth\n    newpath exch X exch Y stone_radius 0 360 arc fill stroke\n} def\n\n/number {\n    exch X exch Y moveto\n    4 string cvs show\n} def\n\n/line {\n    0.5 setlinewidth\n    newpath\n    exch X exch Y moveto\n    exch X exch Y lineto\n    stroke\n} def\n\n/thickline {\n    2 setlinewidth\n    newpath\n    exch X exch Y moveto\n    exch X exch Y lineto\n    stroke\n} def\n\n/start_game {\n    0 1 n0 1 sub {\n	2 mul /i exch def\n	board0 i get board0 i 1 add get circle\n    } for\n} def\n\n/up_to_numbered {\n    /n exch def\n    n 7 mul copy\n    start_game\n    n { line number } repeat\n} def\n\n/up_to {\n    /n exch def\n    n 7 mul copy\n    start_game\n    n 0 ne {\n	n 1 sub { line circle pop } repeat\n	thickline fillcircle pop\n    } if\n} def\n");

            for (int i = lengthBestRollout; i > 0; i--) {
                printMovePS (fp, i, bestRollout [i - 1]);
            }
            fprintf (fp, "\n");

            fprintf (fp, "%%%%Page: -1 -1\n180 210 translate\n%d up_to_numbered\nshowpage\n", lengthBestRollout);

            for (int i = 0; i <= lengthBestRollout; i++) {
                fprintf (fp, "%%%%Page: %d %d\n180 210 translate\n%d up_to\nshowpage\n", i, i, i);
            }
            fprintf (fp, "\n");
        }

        void printMovesPS (char *name) {
            FILE * fp = fopen (name,"w");
            if (fp != NULL) {
                printMovesPS (fp);
                fprintf (fp, "\n");
                fclose (fp);
            }
        }

        int playMove (int move) {
            int i = move & 127, j = (move >> 7) & 127, x = (move >> 14) & 127, y = (move >> 21) & 127, k = (move >> 28) & 7, oppositek;
            damier [i] [j] = '+';
            int dx = 0, dy = 0;
            if (k < 4) oppositek = k + 4;
            else oppositek = k - 4;
            for (int m = 0; m < 4; m++) {
                edge [x + dx] [y + dy] [k] = '+';
                hash ^= hashEdge [x + dx] [y + dy] [k];
                dx += dxdir [k];
                dy += dydir [k];
                edge [x + dx] [y + dy] [oppositek] = '+';
                hash ^= hashEdge [x + dx] [y + dy] [oppositek];
            }
            variation [lengthVariation] = move;
            lengthVariation++;
            score = lengthVariation;
            updatePossibleMoves (move);
        }

        int code (int move) {
            int i = move & 127, j = (move >> 7) & 127, x = (move >> 14) & 127, y = (move >> 21) & 127, k = (move >> 28) & 7, oppositek;
            if (k >= 4)
                k = k - 4;
            return x + Size * y + k * Size * Size;
        }

        bool operator == (const Problem & p) {
            if (scoreBestRollout != p.scoreBestRollout)
                return false;
            for (int i = 0; i < scoreBestRollout; i++) 
                if (bestRollout [i] != p.bestRollout [i])
                    return false;
            return true;
        }

        int undoMove (int move) {
            int i = move & 127, j = (move >> 7) & 127, x = (move >> 14) & 127, y = (move >> 21) & 127, k = (move >> 28) & 7, oppositek;
            damier [i] [j] = '.';
            int dx = 0, dy = 0;
            if (k < 4) oppositek = k + 4;
            else oppositek = k - 4;
            for (int m = 0; m < 4; m++) {
                edge [x + dx] [y + dy] [k] = '.';
                hash ^= hashEdge [x + dx] [y + dy] [k];
                dx += dxdir [k];
                dy += dydir [k];
                edge [x + dx] [y + dy] [oppositek] = '.';
                hash ^= hashEdge [x + dx] [y + dy] [oppositek];
            }
        }

        bool legalMove (int move) {
            int i = move & 127, j = (move >> 7) & 127;
            int x = (move >> 14) & 127, y = (move >> 21) & 127;
            int k = (move >> 28) & 7, oppositek;

            if (damier [i] [j] == '+')
                return false;
            int dx = 0, dy = 0;
            if (k < 4) oppositek = k + 4;
            else oppositek = k - 4;
            for (int m = 0; m < 4; m++) {
                if (edge [x + dx] [y + dy] [k] == '+')
                    return false;
                dx += dxdir [k];
                dy += dydir [k];
                if (edge [x + dx] [y + dy] [oppositek] == '+')
                    return false;
            }
            dx = 0, dy = 0;
            for (int m = 0; m < 5; m++) {
                if ((x + dx != i) || (y +  dy != j))
                    if (damier [x + dx] [y + dy] != '+')
                        return false;
                dx += dxdir [k];
                dy += dydir [k];
            }
            if (!touching) {
                if (edge [x - dxdir [k]] [y - dydir [k]] [k] == '+')
                    return false;
                if (edge [x + 4 * dxdir [k]] [y + 4 * dydir [k]] [k] == '+')
                    return false;
            }
            return true;
        }


        bool alignementIncluding (int x, int y, int k, int i1, int j1) {
            int dx = 0, dy = 0;
            bool included = false;
            if (damier [x] [y] != '+')
                return false;
            if ((x == i1) && (y == j1))
                included = true;
            for (int m = 0; m < 4; m++) {
                if (edge [x + dx] [y + dy] [k] == '+')
                    return false;
                dx += dxdir [k];
                dy += dydir [k];
                if (damier [x + dx] [y + dy] != '+')
                    return false;
                if ((x + dx == i1) && (y + dy == j1))
                    included = true;
            }
            if (!touching) {
                if (edge [x - dxdir [k]] [y - dydir [k]] [k] == '+')
                    return false;
                if (edge [x + 4 * dxdir [k]] [y + 4 * dydir [k]] [k] == '+')
                    return false;
            }
            return included;
        }

        void findMovesAroundIncluding (int i, int j, int i1, int j1, int k, list<int> & mvs) {
            /*   for (int k = 0; k < 4; k++) */
            if (k > 4) k -= 4;
            for (int m = -4; m <= 0; m++) {
                int x = i + m * dxdir [k], y = j + m * dydir [k];
                if (alignementIncluding (x, y, k, i1, j1)) {
                    int move = i | (j << 7) | (x << 14) | (y << 21) | (k << 28);
                    mvs.push_back (move);
                }
            }
        }

        void findMovesIncluding (int i1, int j1, list<int> & mvs) {
            for (int k = 0; k < 8; k++)
                for (int m = -4; m <= 0; m++) {
                    int i = i1 + m * dxdir [k], j = j1 + m * dydir [k];
                    if (damier [i] [j] == '.') {
                        damier [i] [j] = '+';
                        findMovesAroundIncluding (i, j, i1, j1, k, mvs);
                        damier [i] [j] = '.';
                    }
                }
        }

        void updatePossibleMoves (int move) {
            /* remove illegal moves */
            list<int>::iterator it, it1;
            list<int> copyMoves = moves;

            for (it = copyMoves.begin (); it != copyMoves.end (); ++it) {
                if (!legalMove (*it)) {
                    moves.remove (*it);
                }
            }
            int i = move & 127, j = (move >> 7) & 127;
            findMovesIncluding (i,j, moves);
        }

        void writeBest(string fileName="") {
            if (fileName == "") {
                if (touching)
                    fileName = highScoreLocation;
                else
                    fileName = highScoreLocationNoTouching;
            }
            char s [1000];
            sprintf (s, "%s.ps", fileName.c_str());
            printMovesPS(s);

            ofstream myfile;
            myfile.open(fileName.c_str());
            if (myfile.is_open())
            {
                myfile << score << endl;
                myfile.close();
            }
            else
            {
                cerr << "could not write the file: "<<fileName.c_str() << endl;
            }
        }

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


        void updateBest() {

            int bestVal = readBest();
            if (score > bestVal) {
                cerr << "new reccord!!: " << score << endl;
#ifdef SENDMAIL
                stringstream message;
                message << "./sendMail.rb ";
                message << "'new value: "<< score << "\n";
                message << "touching: " << touching << "\n";
                message << "search type: " << tabSearch[searchType] << "\n";
                message << "level: " << level << "\n";
                message << "nbSearches: " << nbSearches << "\n";
                message << "nbTimes: " << nbTimes << "'";
                message << " 'new highscore'";
                system(message.str().c_str());
                cerr << "mail sent" << endl;
#endif
                writeBest();
            }
        }


        double getValMove(Move move) {
            return exp (policy [code (move)]);
        }

        int chooseRandomMoveNRPA (list<int> & moves) {
            double totalSum = 0.0;
            for (list<Move>::iterator it = moves.begin (); it != moves.end (); ++it) 
                totalSum += getValMove(*it);
            double index = totalSum * (rand() / (RAND_MAX + 1.0));
            int current = 0;
            double sum = 0.0;
            list<int>::iterator i = moves.begin ();
            sum += getValMove(*i);
            while (sum < index) {
                i++;
                sum += getValMove(*i);
            }
            return *i;
        }

        int playoutNRPA () {
            nbPlayouts++;
            while (true) {
                if (moves.size () == 0) {
                    score = lengthVariation;
                    return score;
                }
                int move;
                move = chooseRandomMoveNRPA (moves);
                playMove (move);
            }
        }

        double getScoreNew(int indice) {
            double res;
            if (nbPlayMove [indice] == 0)
                res = scoreBestRollout;
            else
                res = (double) totalScoreMove [indice] / nbPlayMove [indice];
            cerr << exp(res) << endl;
            return exp(res);
        }

        int chooseRandomMoveNew (list<int> & moves) {
            double totalSum = 0.0;
            for (list<Move>::iterator it = moves.begin (); it != moves.end (); ++it) 
                totalSum += getScoreNew(code(*it));
            //exit(0);
            double index = totalSum * (rand() / (RAND_MAX + 1.0));
            int current = 0;
            double sum = 0.0;
            list<int>::iterator i = moves.begin ();
            sum += getScoreNew(code(*i));
            while (sum < index) {
                i++;
                sum += getScoreNew(code(*i));
            }
            return *i;
        }

        int playoutNew () {
            nbPlayouts++;
            while (true) {
                if (moves.size () == 0) {
                    score = lengthVariation;
                    return score;
                }
                int move;
                move = chooseRandomMoveNew (moves);
                playMove (move);
            }
        }


        void updateScoreMove(Problem p) {
            for (int i=0; i<p.lengthVariation; i++) {
                nbPlayMove[code(p.variation[i])]++;
                totalScoreMove[code(p.variation[i])]+=p.lengthVariation;
            }
        }

        void adapt () {
            Problem p;
            p.init ();
            for (int i = 0; i < MaxCode; i++)
                p.policy [i] = policy [i];
            for (int j = 0; j < lengthBestRollout; j++) {
                p.policy [code (bestRollout [j])] += 1.0;
                double totalSum = 0.0;
                for (list<Move>::iterator it = p.moves.begin (); it != p.moves.end (); ++it) 
                    totalSum += exp (policy [code (*it)]);
                for (list<Move>::iterator it = p.moves.begin (); it != p.moves.end (); ++it) 
                    p.policy [code (*it)] -= exp (policy [code (*it)]) / totalSum;
                p.playMove (bestRollout [j]);
            }
            for (int i = 0; i < MaxCode; i++)
                policy [i] = p.policy [i];
        }

        void adaptNew () {
            Problem p;
            p.init ();
            for (int i = 0; i < MaxCode; i++)
                p.policy [i] = policy [i];
            for (int j = 0; j < lengthBestRollout; j++) {
                p.policy [code (bestRollout [j])] += 1.;
                double totalSum = 0.0;
                for (list<Move>::iterator it = p.moves.begin (); it != p.moves.end (); ++it) 
                    totalSum += exp (policy [code (*it)]);
                for (list<Move>::iterator it = p.moves.begin (); it != p.moves.end (); ++it) 
                    p.policy [code (*it)] -= exp (policy [code (*it)]) / totalSum;
                p.playMove (bestRollout [j]);
            }
            for (int i = 0; i < MaxCode; i++)
                policy [i] = p.policy [i];
        }

        int chooseRandomMove (list<int> & moves) {
            int index = (int) (moves.size () * (rand() / (RAND_MAX + 1.0)));
            return getMove(moves,index);
        }

        int chooseRandomIndex (list<int> & moves) {
            int index = (int) (moves.size () * (rand() / (RAND_MAX + 1.0)));
            return index;
        }


        int chooseUCBIndex(list<int> &moves, int *sumScore, int *nbTry, int nbTryFather) {
            for (int i=0; i<moves.size(); i++) {
                if(sumScore[i]==0)
                    return i;
            }
            float bestScore=0;
            int indexBest=0;
            for (int i=0; i<moves.size(); i++) {
                float coef=2.;
                float currentScore= (float) sumScore[i]/nbTry[i] + sqrt(coef*log(nbTryFather)/nbTry[i]); //UCB
                //TODO ucb tuned?
                if (currentScore>bestScore) {
                    bestScore = currentScore;
                    indexBest=i;
                }
            }
            return indexBest;
        }

        int getMove(list<int> & moves, int index) {
            int current=0;
            list<int>::iterator i = moves.begin ();
            while (current < index) {
                current++;
                i++;
            }
            return *i;
        }

        int playout () {
            nbPlayouts++;
            while (true) {
                if (moves.size () == 0) {
                    score = lengthVariation;
                    return score;
                }
                int move;
                move = chooseRandomMove (moves);
                playMove (move);
            }
        }



        int nestedRollout (int n) {
            //findMoves ();
            lengthBestRollout = 0;
            scoreBestRollout = 0;
            while (true) {
                if (moves.size () == 0)
                    break;
                Move bestMove = bestRollout [lengthVariation];

                for (int i=0; i<nbSearches*moves.size(); i++) {
                    int currentIndex=i%moves.size();
                    Move currentMove= getMove(moves,currentIndex);
                    Problem p = *this;
                    p.playMove (currentMove);
                    int scoreRollout;
                    if (n == 1) {
                        p.playout ();
                        scoreRollout = p.score;
                    }
                    else {
                        scoreRollout = p.nestedRollout (n - 1);
                    }
                    if (scoreRollout > scoreBestRollout) {
                        p.updateBest();
                        scoreBestRollout = scoreRollout;
                        bestMove = currentMove;
                        lengthBestRollout = p.lengthVariation;
                        for (int j = 0; j < p.lengthVariation; j++)
                            bestRollout [j] = p.variation [j];
                        if (n > 0) {
#ifdef VERBOSE
                            for (int t = 0; t < n - 1; t++)
                                fprintf (stderr, "\t");
                            fprintf (stderr, "n = %d, progress = %d, length = %d, score = %d, nbMoves = %d\n", n, lengthVariation, lengthBestRollout, scoreBestRollout, (int)moves.size ());
#endif
                        }
                    }
                }
                playMove (bestMove);
                //findMoves ();
            }
            score = lengthVariation;
            return score;
        }



        int NRPA (int n) {
            if (n == 0) 
                return playoutNRPA ();
            else {
                lengthBestRollout = 0;
                scoreBestRollout = 0;
                for (int i = 0; i < nbSearches; i++) {
                    Problem p = *this;
                    int scoreRollout = p.NRPA (n - 1);
                    if (scoreRollout >= scoreBestRollout) {
                        p.updateBest();
                        if ((n > 1) && (scoreRollout > scoreBestRollout)) {
#ifdef VERBOSE
                            for (int t = 0; t < n - 1; t++)
                                fprintf (stderr, "\t");
                            fprintf (stderr, "n = %d, progress = %d, score = %d\n", n, i, scoreRollout);
#endif
                        }
                        scoreBestRollout = scoreRollout;
                        if (n == 1) {
                            lengthBestRollout = p.lengthVariation;
                            for (int j = 0; j < lengthBestRollout; j++)
                                bestRollout [j] = p.variation [j];
                        }
                        else {
                            lengthBestRollout = p.lengthBestRollout;
                            for (int j = 0; j < lengthBestRollout; j++)
                                bestRollout [j] = p.bestRollout [j];
                        }
                    }

                    adapt ();
                }
            }
            return scoreBestRollout;
        }

        int newSearch (int n) {
            lengthBestRollout = 0;
            scoreBestRollout = 0;
            int nbStagnate = 0;
            while (true) {
                if (moves.size () == 0)
                    break;

            //    if (nbStagnate >= 2) {
            //        //cerr << "stagnate!!!" << endl;
            //        for (int i=lengthVariation; i<lengthBestRollout; i++) {
            //            playMove(bestRollout[i]);
            //        }
            //        break;
            //    }

                int nbStagnateSearches = 0;
                for (int i=0; i<nbSearches; i++) {
                //for (int i=0; i<10*moves.size(); i++) {
                    //int currentIndex=i%moves.size();
                    //Move currentMove= getMove(moves,currentIndex);
                    if (nbStagnateSearches >= 3) {
                        if (n>0)
                        cerr << "blak " << scoreBestRollout << endl;
                        break;
                    }
                    Problem p = *this;
                    //p.playMove (currentMove);
                    int scoreRollout;
                    if (n == 1) {
                        p.playoutNRPA ();
                        scoreRollout = p.score;
                    }
                    else {
                        scoreRollout = p.newSearch (n - 1);
                    }

                    if (scoreRollout == scoreBestRollout) {
                        nbStagnateSearches++;
                        nbStagnate++;
                    } else  {
                        nbStagnate=0;
                        nbStagnateSearches=0;
                    }
#ifdef VERBOSE
                    if (scoreRollout > scoreBestRollout)
                        if (n > 0) {
                            for (int t = 0; t < n - 1; t++)
                                fprintf (stderr, "\t");
                            fprintf (stderr, "n = %d, progress = %d, score = %d, nbMoves = %d\n", n, lengthVariation, scoreRollout, (int)moves.size ());
                        }
                    if (n>2) {
                            for (int t = 0; t < n - 1; t++)
                                fprintf (stderr, "\t");
                            fprintf (stderr, "n = %d, progress = %d, score = %d / %d, stagnate= %d, nbMoves = %d\n", n, lengthVariation, scoreRollout, scoreBestRollout, nbStagnateSearches, (int)moves.size ());
                    }

#endif

                    if (scoreRollout >= scoreBestRollout) {
                        p.updateBest();
                        scoreBestRollout = scoreRollout;
                        lengthBestRollout = p.lengthVariation;
                       // cout << "new best score: " << scoreBestRollout << endl;
                        for (int j = 0; j < p.lengthVariation; j++) {
                            bestRollout [j] = p.variation [j];
                        //    cout << bestRollout[j] << " ";
                        }
                       // cout << endl;
                    }
                    adapt();
                }
                Move bestMove = bestRollout [lengthVariation];
                //cout << bestMove << endl;
                if (bestMove == 0) {
                    cerr << " error bestmove " << endl;
                    exit(0);
                }
                playMove (bestMove);
            }
            score = lengthVariation;
            return score;
        }

        int newSearchTemp (int n) {
            if (n == 0) 
                return playoutNRPA ();
            else {
                lengthBestRollout = 0;
                scoreBestRollout = 0;
                int nbStagnate = 0;
                //for (int i = 0; i < nbSearches; i++) {
                while(true) {
                    if (nbStagnate >= 2) {
                        break;
                    }
                    Problem p = *this;
                    int scoreRollout = p.newSearchTemp (n - 1);
                    if (scoreRollout == scoreBestRollout) {
                        nbStagnate++;
                    } else {
                        nbStagnate = 0;
                    }
                    if (scoreRollout >= scoreBestRollout) {
                        p.updateBest();
                        if ((n > 1) && (scoreRollout > scoreBestRollout)) {
#ifdef VERBOSE
                            for (int t = 0; t < n - 1; t++)
                                fprintf (stderr, "\t");
                            fprintf (stderr, "n = %d, score = %d, stagnate = %d\n", n, scoreRollout, nbStagnate);
#endif
                        }
                        scoreBestRollout = scoreRollout;
                        if (n == 1) {
                            lengthBestRollout = p.lengthVariation;
                            for (int j = 0; j < lengthBestRollout; j++)
                                bestRollout [j] = p.variation [j];
                        }
                        else {
                            lengthBestRollout = p.lengthBestRollout;
                            for (int j = 0; j < lengthBestRollout; j++)
                                bestRollout [j] = p.bestRollout [j];
                        }
                    }

                    adapt ();
                }
            }
            return scoreBestRollout;
        }

        int distSeq (int* seq1, int seq1size, int* seq2, int seq2size) {
            if (seq1size != seq2size) {
                return 1000;
            } else {
                int nbDiff = 0;
                for (int i=0; i<seq1size; i++) {
                    if (seq1[i] != seq2[i])
                        nbDiff++;
                }
                return nbDiff;
            }
        }

        int newSearch2 (int n) {
            if (n == 0) 
                return playoutNRPA ();
                //return playoutNew ();
            else {
                lengthBestRollout = 0;
                scoreBestRollout = 0;
                int nbStagnate = 0;
                while(true) {
                    if (nbStagnate >= 1) {
                        break;
                    }
                    Problem p = *this;
                    int scoreRollout = p.newSearch2 (n - 1);
                    //cout << distSeq(bestRollout,lengthBestRollout,p.variation, p.lengthVariation) << endl;
                    //if (scoreRollout == scoreBestRollout) {
                    if (distSeq(bestRollout,lengthBestRollout,p.variation, p.lengthVariation) <= (int) (0.25 * lengthBestRollout)) {
                        nbStagnate++;
                    } 
                    if (scoreRollout >= scoreBestRollout) {
                        p.updateBest();
                        if ((n > 1) && (scoreRollout > scoreBestRollout)) {
#ifdef VERBOSE
                            for (int t = 0; t < n - 1; t++)
                                fprintf (stderr, "\t");
                            fprintf (stderr, "n = %d, score = %d, stagnate = %d\n", n, scoreRollout, nbStagnate);
#endif
                        }
                        scoreBestRollout = scoreRollout;
                        
                        lengthBestRollout = p.lengthVariation;
                        for (int j = 0; j < lengthBestRollout; j++)
                            bestRollout [j] = p.variation [j];
                    }
                    //updateScoreMove(p);
                    //adaptNew ();
                    adapt ();
                }
            }
            lengthVariation = lengthBestRollout;
            for (int i=0; i< lengthBestRollout; i++)
                variation[i] = bestRollout[i];
         //   if (n>1) {
         //       for (int i=0; i< lengthVariation; i++) {
         //           int indice = code(variation[i]);
         //           cerr << indice << " " << policy[indice] << " " << nbPlayMove[indice] << " " << totalScoreMove[indice] << " " << (double) totalScoreMove[indice] / nbPlayMove[indice]<< endl;
         //       }
         //   }
            return scoreBestRollout;
        }


};



void usage () {
    fprintf (stderr, "morpionBeam -level 2 -nbTimes 17 -nbSearches 20\n");
}





int main (int argc, char ** argv) {
    initHash (); 

    struct timeval time;
    gettimeofday(&time,NULL);
    srand((time.tv_sec * 1000) + (time.tv_usec / 1000));
    //srand (time(NULL));

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
        else if (!strcmp (argv [i], "-searchType")) {
            if (sscanf (argv [++i], "%d", & searchType) != 1)
                usage();
        }
        else if (!strcmp (argv [i], "-touching")) {
            touching = true;
        }
        else {
            cerr << "argument non reconnu: " << argv[i] << endl;
            exit(0);
        }
    }

    Problem p,bestP;
    int bestscore = 0;
    for (int i=0; i<nbTimes; i++) {
        p.init ();
        int score;
        switch(searchType) {
            case TYPE_PLAYOUT:
                score= p.playout();
                break;
            case TYPE_NESTED:
                score = p.nestedRollout (level);
                break;
            case TYPE_NRPA:
                score = p.NRPA(level);
                break;
            case TYPE_NEWSEARCH:
                score = p.newSearch2(level);
                break;
            default:
                cerr << "wrong search type" << endl;
                exit(0);
        }
        if (score>bestscore) {
            bestscore=score;
            bestP = p;
        }
    }


    //char s [1000];
    //sprintf (s, "highScore/nested.%d.ps", bestscore);
    //p.printMovesPS(s);
    cout << "Score: " << bestscore << endl;
    cout << "nbPlayouts: " << nbPlayouts << endl;



    exit (0);


}

