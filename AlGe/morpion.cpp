#include "morpion.h"

// algorithm variables and default values
extern int level;
extern int nbTimes;
extern int nbSearches;


extern int bestscore;
extern int nbPlayouts;
extern int nbEval;

Morp Morp::copy() {
    Morp newM;
    newM.hash = hash;
    newM.sigma = sigma;
    newM.score = score;
    newM.lengthVariation =  lengthVariation;
    newM.scoreBestRollout = scoreBestRollout;
    newM.lengthBestRollout = lengthBestRollout;

    for (list<Move>::iterator it = moves.begin (); it != moves.end (); ++it) {
        newM.moves.push_back(*it);
    }

    for (int i=0; i<Size; i++) {
        for (int j=0; j<Size; j++) {
            newM.damier[i][j] = damier[i][j];
            newM.hashDamier[i][j] = hashDamier[i][j];
            for (int k=0;k<8;k++) {
                newM.edge[i][j][k] = edge[i][j][k];
                newM.hashEdge[i][j][k] = hashEdge[i][j][k];
            }
        }
    }
        
    for (int i=0; i<MaxCode; i++) {
        newM.policy[i] = policy[i];
        newM.totalScoreMove[i] = totalScoreMove[i];
        newM.nbPlayMove[i] = nbPlayMove[i];
    }

    for (int i=0; i<MaxLengthPlayout; i++) {
        newM.variation[i] = variation[i];
        newM.bestRollout[i] = bestRollout[i];
    }

    return newM;

}

void Morp::initRand(double sigma) {
    init();
    for (int i=0; i<MaxCode; i++) 
        policy[i]=generateGauss();

}

void Morp::initFast() {
    init();
    for (int i=0; i<MaxCode; i++) 
        policy[i]=0;

}

void Morp::mutateSA() {

    //sigma +=  sigma*generateGauss();
    sigma +=  generateGauss();

    // mutate each policy value
    for (int i=0; i<MaxCode; i++) {
        policy[i] = policy[i] + sigma * generateGauss();
    }
    eval();
}

void Morp::mutate() {

    // mutate each policy value
    for (int i=0; i<MaxCode; i++) {
        policy[i] = policy[i] + sigma * generateGauss();
    }
    eval();
}

void Morp::eval() {
    double tempScore = 0;
    double bestScore = 0;
    Morp p;
    double sum=0.0;
    int nbTry=100;
    vector<double> res;
    for (int i=0;i<nbTry;i++) {
        p = *this;
        tempScore = p.playoutNRPA();
        sum+=tempScore;
        if (tempScore > bestScore)
            bestScore = tempScore;
        res.push_back(tempScore);
    }
    double sumE=0;
    for (int i=0;i<res.size();i++) {
        sumE+=(res[i]-sum/nbTry) * (res[i]-sum/nbTry);
    }

    //score = bestScore;
    sd=sqrt(sumE/nbTry);
    score=sum/nbTry;
}

double Morp::compVal() {
    return score + 0.5*sd;
    //return score;
}

// generate a random number based on a normal distribution
// using Box Muller transform
double Morp::generateGauss() {
    double num1 = (double) rand() / (RAND_MAX);
    double num2 = (double) rand() / (RAND_MAX);
    return sqrt(-2*log(num1))*cos(2*M_PI*num2);

}


void Morp::computeHash () {
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

void Morp::init () {
    //initHash();

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

    sigma=1;

}

void Morp::initHash () {
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


void Morp::initPolicy () {
    for (int i = 0; i < MaxCode; i++)
        policy [i] = 0.0;
}

void Morp::printMap (FILE *fp) {
    for (int i = 0 ; i < Size; i++) {
        for (int j = 0; j < Size; j++)
            fprintf (fp, "%c", damier [j] [i]);
        fprintf (fp, "\n");
    }
}


bool Morp::alignement (int x, int y, int k) {
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

void Morp::findMovesAround (int i, int j, list<int> & mvs) {
    for (int k = 0; k < 4; k++)
        for (int m = -4; m <= 0; m++) {
            int x = i + m * dxdir [k], y = j + m * dydir [k];
            if (alignement (x, y, k)) {
                int move = i | (j << 7) | (x << 14) | (y << 21) | (k << 28);
                mvs.push_back (move);
            }
        }
}

void Morp::findMoves () {
    moves.clear ();
    for (int i = 0 ; i < Size; i++)
        for (int j = 0; j < Size; j++)
            if (damier [i] [j] == '.') {
                damier [i] [j] = '+';
                findMovesAround (i, j, moves);
                damier [i] [j] = '.';
            }
}

void Morp::printMovePS (FILE *fp, int num, int move) {
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

void Morp::printMovesPS (FILE *fp) {
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

void Morp::printMovesPS (char *name) {
    FILE * fp = fopen (name,"w");
    if (fp != NULL) {
        printMovesPS (fp);
        fprintf (fp, "\n");
        fclose (fp);
    }
}

int Morp::playMove (int move) {
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

int Morp::code (int move) {
    int i = move & 127, j = (move >> 7) & 127, x = (move >> 14) & 127, y = (move >> 21) & 127, k = (move >> 28) & 7, oppositek;
    if (k >= 4)
        k = k - 4;
    return x + Size * y + k * Size * Size;
}

bool Morp::operator == (const Morp & p) {
    if (scoreBestRollout != p.scoreBestRollout)
        return false;
    for (int i = 0; i < scoreBestRollout; i++) 
        if (bestRollout [i] != p.bestRollout [i])
            return false;
    return true;
}

int Morp::undoMove (int move) {
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

bool Morp::legalMove (int move) {
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


bool Morp::alignementIncluding (int x, int y, int k, int i1, int j1) {
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

void Morp::findMovesAroundIncluding (int i, int j, int i1, int j1, int k, list<int> & mvs) {
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

void Morp::findMovesIncluding (int i1, int j1, list<int> & mvs) {
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

void Morp::updatePossibleMoves (int move) {
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

void Morp::writeBest(string fileName) {
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


void Morp::updateBest() {

#ifdef SAVEBEST
    if (score > bestVal) {
        cerr << "new reccord!!: " << score << endl;
#ifdef SENDMAIL
        stringstream message;
        message << "./sendMail.rb ";
        message << "'new value: "<< score << "\n";
        message << "touching: " << touching << "\n";
        message << "'";
        message << " 'new highscore'";
        if (system(message.str().c_str()) != 0) {
            cerr << "error sending mail" << endl;
            exit(0);
        }
        cerr << "mail sent" << endl;
#endif
        writeBest();
        bestVal = score;
    }
#endif
}


double Morp::getValMove(Move move) {
    return exp (policy [code (move)]);
}

int Morp::chooseRandomMoveNRPA (list<int> & moves) {
    double totalSum = 0.0;
    for (list<Move>::iterator it = moves.begin (); it != moves.end (); ++it) {
        totalSum += getValMove(*it);
    }
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

int Morp::playoutNRPA () {
    nbPlayouts++;
    while (true) {
        if (moves.size () == 0) {
            score = lengthVariation;
            //  int countDiff=0;
            //  for (int i=0; i<lengthVariation; i++) {
            //      if (variation[i] != bestRollout[i])
            //          countDiff++;
            //  }
            //  cerr << "diff: " << countDiff << "/" << lengthVariation << endl;
            if (score>bestscore) {
                bestscore=score;
#ifdef SAVEBEST
                updateBest();
#endif
            }
            return score;
        }
        int move;
        move = chooseRandomMoveNRPA (moves);
        playMove (move);
    }
}


/*
void Morp::adapt () {
    Morp p;
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
*/
/*
int Morp::NRPA (int n) {
    if (n == 0) 
        return playoutNRPA ();
    else {
        lengthBestRollout = 0;
        scoreBestRollout = 0;
        for (int i = 0; i < nbSearches; i++) {
            Morp p = *this;
            int scoreRollout = p.NRPA (n - 1);
            if (scoreRollout >= scoreBestRollout) {
                p.updateBest();
#ifdef VERBOSE
                if (((level - n) < 2) && (scoreRollout > scoreBestRollout)) {
                //if (( n > 2) && (scoreRollout > scoreBestRollout)) {
                    for (int t = 0; t < n - 1; t++)
                        fprintf (stderr, "\t");
                    fprintf (stderr, "n = %d, progress = %d, score = %d\n", n, i, scoreRollout);
                }
#endif
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
            if (nbPlayouts % (int)(pow(nbSearches,level)/nbPoints) == 0 && n == 1) {
                cout << "score: " << bestscore << endl;
                cout << "eval: " << nbPlayouts << endl;
            }
            
        }
    }
    return scoreBestRollout;
}
*/




