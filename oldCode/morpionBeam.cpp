#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#include <iostream>
#include <list>
#include <algorithm>

#ifdef USEMPI
// mpic++ morpionBeam.c -o morpionBeamMPI -O2 -DUSEMPI
#include <mpi.h>
#endif

const int MaxSize = 15;
const int MaxProblem = 20;
const int MaxLevel = 5;
const int MaxBeam = 100;
const int MaxColor = 20;
const int MaxLengthPlayout = 200;
const int Size = 50;
const int MaxCode = 4 * Size * Size;

int nbPlayouts = 0;

double nextTimeBeam;
int indexTimeBeam;
float sumValueAfterTime [100];
int nbSearchTime [100];
static struct timeval timeStart, timeEnd;
int bestBeam = 0;
bool plot = false;

bool touching = true;
bool stopFirstMove = false;
int nbRollouts = 10;
bool RoundRobin = false;
bool ScheduleAvecClientLibre = true;
bool useBestSequence = true;
bool useForcedMoves = false;

int bestOverall = 70;

using namespace std;

int dxdir [8] = {-1, -1, 0, 1, 1, 1, 0, -1};
int dydir [8] = { 0,  1, 1, 1, 0,-1,-1, -1};

typedef int Move;

bool quiet = false;

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

struct Message {
  char damier [Size] [Size];
  char edge [Size] [Size] [8];
  
  int score;
   
  int lengthVariation;
  int currentLengthVariation;
  Move variation [MaxLengthPlayout];

  int scoreBestRollout, lengthBestRollout;
  Move bestRollout [MaxLengthPlayout];
};

class Problem {
 public:
  char damier [Size] [Size];
  char edge [Size] [Size] [8];
  unsigned long long hash;
  
  int score;

  list<int> moves;
   
  int lengthVariation;
  Move variation [MaxLengthPlayout];
  
  int scoreBestRollout, lengthBestRollout;
  Move bestRollout [MaxLengthPlayout];

  double policy [MaxCode];

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

  void printMap (FILE *fp) {
    for (int i = 0 ; i < Size; i++) {
      for (int j = 0; j < Size; j++)
	fprintf (fp, "%c", damier [j] [i]);
      fprintf (fp, "\n");
    }
  }
  
  void set (Message & m) {
    for (int i = 0 ; i < Size; i++)
      for (int j = 0; j < Size; j++) {
	damier [i] [j] = m.damier [i] [j];
	for (int k = 0; k < 8; k++)
	  edge [i] [j] [k] = m.edge [i] [j] [k];
      }
    score = m.score;
    
    lengthVariation= m.lengthVariation;
    for (int i = 0; i < MaxLengthPlayout; i++)
      variation [i] = m.variation [i];
    
    scoreBestRollout = m.scoreBestRollout;
    lengthBestRollout = m.lengthBestRollout;
    for (int i = 0; i < MaxLengthPlayout; i++)
      bestRollout [i] = m.bestRollout [i];

    findMoves ();
  }
    
  void get (Message & m) {
    for (int i = 0 ; i < Size; i++)
      for (int j = 0; j < Size; j++) {
	m.damier [i] [j] = damier [i] [j];
	for (int k = 0; k < 8; k++)
	  m.edge [i] [j] [k] = edge [i] [j] [k];
      }
    m.score = score;
    
    m.lengthVariation= lengthVariation;
    for (int i = 0; i < MaxLengthPlayout; i++)
      m.variation [i] = variation [i];
    
    m.scoreBestRollout = scoreBestRollout;
    m.lengthBestRollout = lengthBestRollout;
    for (int i = 0; i < MaxLengthPlayout; i++)
      m.bestRollout [i] = bestRollout [i];
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
    //if (!incrementalMoves) {
    //findPossibleMoves (moves);
      /*    moves.sort (); */
      //return;
    //}
    /* remove illegal moves */
    list<int>::iterator it, it1;
    list<int> copyMoves = moves;

    /*   list<int> possibleMoves; */
    /*   findPossibleMoves (possibleMoves); */

    /*   printMove (stderr, move); */
    /*   printPossibleMoves (stderr); */
    for (it = copyMoves.begin (); it != copyMoves.end (); ++it) {
      /*     printMove (stderr, *it); */
      if (!legalMove (*it)) {
	moves.remove (*it);
	/*       bool element = false; */
	/*       for (it1 = possibleMoves.begin (); it1 != possibleMoves.end (); ++it1) */
	/* 	if (*it == *it1) */
	/* 	  element = true; */
	/*       if (element) { */
	/* 	printMap (stderr); */
	/* 	fprintf (stderr, "pb move retire present :"); */
	/* 	printMove (stderr, *it); */
	/* 	fprintf (stderr, "\n"); */
	/* 	getchar (); */
	/*       } */
      }
      /*     getchar (); */
    }
    /* add the new moves */
    /*   if (move == 48712601) */
    /*     fprintf (stderr, "debug\n"); */
    int i = move & 127, j = (move >> 7) & 127;
    findMovesIncluding (i,j, moves);
    /*   moves.sort (); */

    /*   for (it = possibleMoves.begin (); it != possibleMoves.end (); ++it) { */
    /*     bool element = false; */
    /*     for (it1 = moves.begin (); it1 != moves.end (); ++it1) */
    /*       if (*it == *it1) */
    /* 	element = true; */
    /*     if (!element) { */
    /*       printMap (stderr); */
    /*       fprintf (stderr, "pb move absent (%d) :", *it); */
    /*       printMove (stderr, *it); */
    /*       fprintf (stderr, "\n"); */
    /*       fprintf (stderr,"move effectue (%d) :", move); */
    /*       printMove (stderr, move); */
    /*       fprintf (stderr, "\n"); */
    /*       getchar (); */
    /*     } */
    /*   } */
  }

  int chooseRandomMove (list<int> & moves) {
    int current = 0, index = (int) (moves.size () * (rand() / (RAND_MAX + 1.0)));
    list<int>::iterator i = moves.begin ();
    while (current < index) {
      current++;
      i++;
    }
    return *i;
  }

  int playout () {
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

      for (list<Move>::iterator it = moves.begin (); it != moves.end (); ++it) {
	if (n == 1) {
	  Problem p = *this;
	  p.playMove (*it);
	  p.playout ();
	  int scoreRollout = p.score;
	  if (scoreRollout > scoreBestRollout) {
	    scoreBestRollout = scoreRollout;
	    bestMove = *it;
	    lengthBestRollout = p.lengthVariation;
	    for (int j = 0; j < p.lengthVariation; j++)
	      bestRollout [j] = p.variation [j];
	  }
	}
	else {
	  Problem p = *this;
	  p.playMove (*it);
	  int scoreRollout = p.nestedRollout (n - 1);
	  if (scoreRollout > scoreBestRollout) {
	    scoreBestRollout = scoreRollout;
	    bestMove = *it;
	    lengthBestRollout = p.lengthVariation;
	    for (int j = 0; j < p.lengthVariation; j++)
	      bestRollout [j] = p.variation [j];
	    if (n > 0) {
	  //    for (int t = 0; t < n - 1; t++)
	  //  fprintf (stderr, "\t");
	  //    fprintf (stderr, "n = %d, progress = %d, length = %d, score = %d, nbMoves = %d\n", n, lengthVariation, lengthBestRollout, scoreBestRollout, (int)moves.size ());
	    }
	  }
	}
      }
      playMove (bestMove);
      //findMoves ();
    }
    score = lengthVariation;
    return score;
  }

  int chooseRandomMoveNRPA (list<int> & moves) {
    double totalSum = 0.0;
    for (list<Move>::iterator it = moves.begin (); it != moves.end (); ++it) 
      totalSum += exp (policy [code (*it)]);
    double index = totalSum * (rand() / (RAND_MAX + 1.0));
    int current = 0;
    double sum = 0.0;
    list<int>::iterator i = moves.begin ();
    sum += exp (policy [code (*i)]);
    while (sum < index) {
      i++;
      sum += exp (policy [code (*i)]);
    }
    return *i;
  }

  int playoutNRPA () {
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
  
  int NRPA (int n) {
    if (n == 0) 
      return playoutNRPA ();
    else {
      lengthBestRollout = 0;
      scoreBestRollout = 0;
      for (int i = 0; i < 100; i++) {
	Problem p = *this;
	int scoreRollout = p.NRPA (n - 1);
	if (scoreRollout >= scoreBestRollout) {
	  if ((n > 1) && (scoreRollout > scoreBestRollout)) {
	 //   for (int t = 0; t < n - 1; t++)
	 //     fprintf (stderr, "\t");
	 //   fprintf (stderr, "n = %d, progress = %d, score = %d\n", n, i, scoreRollout);
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
	  if (scoreBestRollout > bestOverall) {
	    bestOverall = scoreBestRollout;
	    char s [1000];
	    sprintf (s, "nrpa.%d.ps", bestOverall);
	  //  fprintf (stderr, "j'ecris %s\n", s);
	  //  printMovesPS (s);
	  }
	}
	adapt ();
      }
    }
    return scoreBestRollout;
  }

  int nestedBeamRollout (int n, int sizeBeam [MaxLevel]);
  int beamNRPA (int n, int sizeBeam [MaxLevel]);

#ifdef USEMPI
  int nestedBeamRolloutMPI (int n, int sizeBeam [MaxLevel]);
#endif
};

Problem * stackProblem [MaxLevel * MaxBeam];
int sizeStack = MaxLevel * MaxBeam;

void initStackProblem () {
  for (int i = 0; i < MaxLevel * MaxBeam; i++)
    stackProblem [i] = new Problem;
}

Problem * getProblem () {
  sizeStack--;
  if (sizeStack < 0) {
    fprintf (stderr, "sizeStack < 0\n");
    exit (0);
  }
  return stackProblem [sizeStack];
}

void putProblem (Problem  * p) {
  stackProblem [sizeStack] = p;
  sizeStack++;
}

Problem * beam [MaxLevel] [MaxBeam], * nextBeam [MaxLevel] [MaxBeam];
int nbBeamLevel [MaxLevel];
int sizeBeam [MaxLevel] = {1, 8, 8, 8, 1};
int level = 2;
bool loop = true, average = false;

int Problem::nestedBeamRollout (int n, int sizeBeam [MaxLevel]) {
  int nbBeam = 1;
 
  for (int b = 0; b < nbBeam; b++) {
    beam [n] [b] = getProblem ();
    *beam [n] [b] = *this;
    beam [n] [b]->lengthBestRollout = 0;
    beam [n] [b]->scoreBestRollout = 0;
  }

  int best = 0;
  while (true) {
    int nbNextBeam = 0;
    bool oneNewPosition = false;
      
    for (int b = 0; b < nbBeam; b++) {
      // insert the best problem in the next beam
      bool inserted = false;
      for (int i = 0; i < nbNextBeam; i++)
	if (nextBeam [n] [i]->scoreBestRollout < beam [n] [b]->scoreBestRollout) {
	  inserted = true;
	  if (nbNextBeam < sizeBeam [n])
	    nbNextBeam = nbNextBeam + 1;
	  else
	    putProblem (nextBeam [n] [nbNextBeam - 1]);
	  for (int j = nbNextBeam - 1; j > i; j--)
	    nextBeam [n] [j] = nextBeam [n] [j-1];
	  nextBeam [n] [i] = getProblem ();
	  *nextBeam [n] [i] = *beam [n] [b];
	  if (beam [n] [b]->scoreBestRollout > 0)
	    if (beam [n] [b]->lengthVariation < beam [n] [b]->lengthBestRollout)
	      nextBeam [n] [i]->playMove (beam [n] [b]->bestRollout [beam [n] [b]->lengthVariation]);
	  break;
	}
      if (!inserted && (nbNextBeam < sizeBeam [n])) {
	nextBeam [n] [nbNextBeam] = getProblem ();
	*nextBeam [n] [nbNextBeam] = *beam [n] [b];
	if (beam [n] [b]->scoreBestRollout > 0)
	  if (beam [n] [b]->lengthVariation < beam [n] [b]->lengthBestRollout)
	    nextBeam [n] [nbNextBeam]->playMove (beam [n] [b]->bestRollout [beam [n] [b]->lengthVariation]);
 	nbNextBeam = nbNextBeam + 1;
      }
     
      //beam [b].findMoves ();
      if (beam [n] [b]->moves.size () != 0)
	oneNewPosition = true;
      for (list<Move>::iterator it = beam [n] [b]->moves.begin (); it != beam [n] [b]->moves.end (); ++it)
	if (true) {
	  /* 	  if ((beam [b].bestRollout [beam [b].lengthVariation] != beam [b].moves [m]) || */
	  /* 	      (beam [b].scoreBestRollout == 0) */
	  /* 	      ) { */
	  Problem p = *beam [n] [b];
	  p.playMove (*it);
	  if (n == 1) {
	    p.playout ();
	  }
	  else {
	    /* 	      if ((n == 2) && endgame && (beam [b].nbMoves < 15)) */
	    /* 		p.nestedBeamRollout (2, sizeBeam, false); */
	    /* 	      else */
	    p.nestedBeamRollout (n - 1, sizeBeam);
	  }
	  // insert the  problem in the next beam
	  bool inserted = false;
	  for (int i = 0; i < nbNextBeam; i++)
	    if (nextBeam [n] [i]->scoreBestRollout < p.score) {
	      inserted = true;
	      if (nbNextBeam < sizeBeam [n])
		nbNextBeam = nbNextBeam + 1;
	      else
		putProblem (nextBeam [n] [nbNextBeam - 1]);
	      for (int j = nbNextBeam - 1; j > i; j--)
		nextBeam [n] [j] = nextBeam [n] [j-1];
	      nextBeam [n] [i] = getProblem ();
	      *nextBeam [n] [i] = *beam [n] [b];
	      nextBeam [n] [i]->playMove (*it);
	      nextBeam [n] [i]->scoreBestRollout = p.score;
	      nextBeam [n] [i]->lengthBestRollout = p.lengthVariation;
	      for (int j = 0; j < p.lengthVariation; j++)
		nextBeam [n] [i]->bestRollout [j] = p.variation [j];
	      break;
	    }
	  if (!inserted && (nbNextBeam < sizeBeam [n])) {
	    nextBeam [n] [nbNextBeam] = getProblem ();
	    *nextBeam [n] [nbNextBeam] = *beam [n] [b];
	    nextBeam [n] [nbNextBeam]->playMove (*it);
	    nextBeam [n] [nbNextBeam]->scoreBestRollout = p.score;
	    nextBeam [n] [nbNextBeam]->lengthBestRollout = p.lengthVariation;
	    for (int j = 0; j < p.lengthVariation; j++)
	      nextBeam [n] [nbNextBeam]->bestRollout [j] = p.variation [j];
	    nbNextBeam = nbNextBeam + 1;
	  }
	}
    }
    if (!oneNewPosition) {
      for (int b = 0; b < nbNextBeam; b++)
	putProblem (nextBeam [n] [b]);
      break;
    }
    for (int b = 0; b < nbBeam; b++)
      putProblem (beam [n] [b]);
    for (int b = 0; b < nbNextBeam; b++)
      beam [n] [b] = nextBeam [n] [b];
    nbBeam = nbNextBeam ;
    if (beam [n] [0]->scoreBestRollout > best) {
      best = beam [n] [0]->scoreBestRollout;
      if ((n >= 2)  || (n == level)) {
	for (int t = 0; t < n - 1; t++)
	  fprintf (stderr, "\t");
	fprintf (stderr, "n = %d, beam = %d, progress = %d, length = %d, score = %d\n", n, sizeBeam [n], nextBeam [n] [0]->lengthVariation, nextBeam [n] [0]->lengthBestRollout, nextBeam [n] [0]->scoreBestRollout);
	if (nextBeam [n] [0]->lengthBestRollout >= 80)
	  if (n == level) {
	    char s [1000];
	    sprintf (s, "ms.Beam%d%d%d%d%d.Progress%d.Length.%d.ps", sizeBeam [0], sizeBeam [1], sizeBeam [2], sizeBeam [3], sizeBeam [4], nextBeam [n] [0]->lengthVariation, nextBeam [n] [0]->lengthBestRollout);
	    nextBeam [n] [0]->printMovesPS (s);
	  }
      }
    }
  }
  score = beam [n] [0]->score;
  if (plot) {
    if (score > bestBeam)
      bestBeam = score;
    gettimeofday (&timeEnd, NULL);
    double time = ((float)(timeEnd.tv_sec - timeStart.tv_sec)) + 
      ((float)(timeEnd.tv_usec - timeStart.tv_usec))/1000000.0;
    if (time > nextTimeBeam) {
      while (time > 2 * nextTimeBeam) {
	indexTimeBeam++;
	nextTimeBeam *= 2;
      }
      sumValueAfterTime [indexTimeBeam] += bestBeam;
      nbSearchTime [indexTimeBeam]++;
      indexTimeBeam++;
      nextTimeBeam *= 2;
    }
  }
  lengthVariation = beam [n] [0]->lengthVariation;
  for (int j = 0; j < beam [n] [0]->lengthVariation; j++)
    variation [j] = beam [n] [0]->variation [j];
  for (int b = 0; b < nbBeam; b++)
    putProblem (beam [n] [b]);
  return score;
}

class BeamProblem {
 public:
  int nbProblems;
  int sizeBeam;
  Problem pb [MaxBeam];

  BeamProblem (int s = 1) {
    sizeBeam = s;
    nbProblems = 0;
  }

  void init (int s) {
    sizeBeam = s;
    nbProblems = 0;
  }

  void insert (Problem & p) {
    bool present = false;
    for (int i = 0; i < nbProblems; i++)
      if (p == pb [i])
	present = true;
    if (!present) {
      int i = nbProblems - 1;
      while ((i >= 0) && (pb [i].scoreBestRollout <= p.scoreBestRollout)) {
	if (i < sizeBeam - 1)
	  pb [i + 1] = pb [i];
	i--;
      }
      if (i < sizeBeam - 1)
	pb [i + 1] = p;
      if (nbProblems < sizeBeam)
	nbProblems++;
    }
  }

};

int N = 100;
BeamProblem beamProblem [5], nextBeamProblem [5];

int Problem::beamNRPA (int n, int sizeBeam [MaxLevel]) {
    if (n == 0) 
      return playoutNRPA ();
    else {
      beamProblem [n].init (sizeBeam [n]);
      lengthBestRollout = 0;
      scoreBestRollout = 0;
      Problem p = *this;
      beamProblem [n].insert (p);
      for (int i = 0; i < N; i++) {
	nextBeamProblem [n].init (sizeBeam [n]);
	for (int b = 0; b < beamProblem [n].nbProblems; b++) {
	  Problem p = beamProblem [n].pb [b];
	  nextBeamProblem [n].insert (p);
	  p.beamNRPA (n - 1, sizeBeam);
	  if (n == 1) {
	    p.lengthBestRollout = p.lengthVariation;
	    p.scoreBestRollout = p.lengthVariation;
	    for (int j = 0; j < p.lengthBestRollout; j++)
	      p.bestRollout [j] = p.variation [j];
	    p.init ();
	    nextBeamProblem [n].insert (p);
	  }
	  else {
	    for (int b1 = 0; b1 < beamProblem [n - 1].nbProblems; b1++) {
	      p = beamProblem [n - 1].pb [b1];
	      for (int j = 0; j < MaxCode; j++)
		p.policy [j] = beamProblem [n].pb [b].policy [j];
	      nextBeamProblem [n].insert (p);
	    }
	  }
	}
	beamProblem [n].init (sizeBeam [n]);
	for (int b = 0; b < nextBeamProblem [n].nbProblems; b++)
	  beamProblem [n].insert (nextBeamProblem [n].pb [b]);
	for (int b = 0; b < beamProblem [n].nbProblems; b++) 
	  beamProblem [n].pb [b].adapt ();
	if (beamProblem [n].pb [0].scoreBestRollout > scoreBestRollout) {
	  scoreBestRollout = beamProblem [n].pb [0].scoreBestRollout;
	  if (n > 1) {
	   // for (int t = 0; t < n - 1; t++)
	   //   fprintf (stderr, "\t");
	   // fprintf (stderr, "n = %d, progress = %d, score = %d\n", n, i, scoreBestRollout);
	  }
	  if (beamProblem [n].pb [0].scoreBestRollout > bestOverall) {
	    bestOverall = beamProblem [n].pb [0].scoreBestRollout;
	    char s [1000];
	    sprintf (s, "nrpa.beam[%d,%d,%d,%d,%d].%d.ps", sizeBeam [0], sizeBeam [1], sizeBeam [2], sizeBeam [3], sizeBeam [4], bestOverall);
	  //  fprintf (stderr, "j'ecris %s\n", s);
	  //  beamProblem [n].pb [0].printMovesPS (s);
	  }
	}
/* 	  if (n == 1) { */
/* 	    lengthBestRollout = p.lengthVariation; */
/* 	    for (int j = 0; j < lengthBestRollout; j++) */
/* 	      bestRollout [j] = p.variation [j]; */
/* 	  } */
/* 	  else { */
/* 	    lengthBestRollout = p.lengthBestRollout; */
/* 	    for (int j = 0; j < lengthBestRollout; j++) */
/* 	      bestRollout [j] = p.bestRollout [j]; */
/* 	  } */
/* 	  if (scoreBestRollout > bestOverall) { */
/* 	    bestOverall = scoreBestRollout; */
/* 	    char s [1000]; */
/* 	    sprintf (s, "nrpa.%d.ps", bestOverall); */
/* 	    fprintf (stderr, "j'ecris %s\n", s); */
/* 	    printMovesPS (s); */
/* 	  } */
/* 	} */
/* 	  adapt (); */
      }
    }
    return scoreBestRollout;
}

/* int Problem::beamNRPA (int n, int sizeBeam [MaxLevel]) { */
/*   if (n == 0) { */
/*     beam [n] [0]->init (); */
/*     beam [n] [0]->playoutNRPA (); */
/*     nbBeamLevel [n] = 1; */
/*     beam [n] [0]->lengthBestRollout = lengthVariation; */
/*     beam [n] [0]->scoreBestRollout = lengthVariation; */
/*     for (int j = 0; j < lengthVariation; j++) */
/*       beam [n] [0]->bestRollout [j] = variation [j]; */
/*     return lengthVariation; */
/*   } */
/*   else { */
/*     int best = 0; */
/*     nbBeamLevel [n] = 1; */
/*     beam [n] [0]->scoreBestRollout = 0; */
/*     for (int i = 0; i < 100; i++) { */
/*       int nbNextBeam = 0; */
      
/*       for (int b = 0; b < nbBeamLevel [n]; b++) { */
/* 	// insert the best problem in the next beam */
/* 	bool inserted = false; */
/* 	for (int i = 0; i < nbNextBeam; i++) */
/* 	  if (nextBeam [n] [i]->scoreBestRollout < beam [n] [b]->scoreBestRollout) { */
/* 	    inserted = true; */
/* 	    if (nbNextBeam < sizeBeam [n]) */
/* 	      nbNextBeam = nbNextBeam + 1; */
/* 	    else */
/* 	      putProblem (nextBeam [n] [nbNextBeam - 1]); */
/* 	    for (int j = nbNextBeam - 1; j > i; j--) */
/* 	      nextBeam [n] [j] = nextBeam [n] [j - 1]; */
/* 	    nextBeam [n] [i] = getProblem (); */
/* 	    *nextBeam [n] [i] = *beam [n] [b]; */
/* 	    break; */
/* 	  } */
/* 	if (!inserted && (nbNextBeam < sizeBeam [n])) { */
/* 	  nextBeam [n] [nbNextBeam] = getProblem (); */
/* 	  *nextBeam [n] [nbNextBeam] = *beam [n] [b]; */
/* 	  nbNextBeam = nbNextBeam + 1; */
/* 	} */
/*       } */
	
/*       for (int b = 0; b < nbBeamLevel [n]; b++) { */
/* 	nbBeamLevel [n - 1] = 1;   */
/* 	beam [n - 1] [0] = getProblem (); */
/* 	*beam [n - 1] [0] = *beam [n] [b]; */
/* 	beam [n - 1] [0]->beamNRPA (n - 1, sizeBeam); */

/* 	for (int k = 0; k < nbBeamLevel [n - 1]; k++) { */
/* /\* 	  beam [n - 1] [k]->adapt (); *\/ */
/* 	  // insert the best problem in the next beam */
/* 	  bool inserted = false; */
/* 	  for (int i = 0; i < nbNextBeam; i++) */
/* 	    if (nextBeam [n] [i]->scoreBestRollout <= beam [n - 1] [k]->scoreBestRollout) { */
/* 	      inserted = true; */
/* 	      if (nbNextBeam < sizeBeam [n]) */
/* 		nbNextBeam = nbNextBeam + 1; */
/* 	      else */
/* 		putProblem (nextBeam [n] [nbNextBeam - 1]); */
/* 	      for (int j = nbNextBeam - 1; j > i; j--) */
/* 		nextBeam [n] [j] = nextBeam [n] [j - 1]; */
/* 	      nextBeam [n] [i] = getProblem (); */
/* 	      *nextBeam [n] [i] = *beam [n - 1] [k]; */
/* 	      break; */
/* 	    } */
/* 	  if (!inserted && (nbNextBeam < sizeBeam [n])) { */
/* 	    nextBeam [n] [nbNextBeam] = getProblem (); */
/* 	    *nextBeam [n] [nbNextBeam] = *beam [n - 1] [k]; */
/* 	    nbNextBeam = nbNextBeam + 1; */
/* 	  } */
/* 	} */
/* 	for (int k = 0; k < nbBeamLevel [n - 1]; k++) */
/* 	  putProblem (beam [n - 1] [k]); */
/*       } */
/*       for (int b = 0; b < nbBeamLevel [n]; b++) */
/* 	putProblem (beam [n] [b]); */
/*       for (int b = 0; b < nbNextBeam; b++) { */
/* 	beam [n] [b] = nextBeam [n] [b]; */
/* 	beam [n] [b]->adapt ();  */
/*       } */
/*       nbBeamLevel [n] = nbNextBeam ; */
/*       if (beam [n] [0]->scoreBestRollout > best) { */
/* 	best = beam [n] [0]->scoreBestRollout; */
/* 	if ((n >= 2)  || (n == level)) { */
/* 	  for (int t = 0; t < n - 1; t++) */
/* 	    fprintf (stderr, "\t"); */
/* 	  fprintf (stderr, "n = %d, progress = %d, score = %d\n", n, i, best); */
/* 	  if (scoreBestRollout > bestOverall) { */
/* 	    bestOverall = scoreBestRollout; */
/* 	    char s [1000]; */
/* 	    sprintf (s, "nrpa.%d.ps", bestOverall); */
/* 	    beam [n] [0]->printMovesPS (s); */
/* 	  } */
/* 	} */
/*       } */
/*     } */
/*   } */
/*   return beam [n] [0]->scoreBestRollout; */
/* } */

int nbTimes = 14;
float firstTime = 0.01;
int nbSearches = 20;

void plotTime (int level, int beam [5]) {
  char s [1000];
  plot = true;
  for (int i = 0; i < 100; i++) {
    sumValueAfterTime [i] = 0.0;
    nbSearchTime [i] = 0;
  }
  for (int i = 0; i < nbSearches; i++) {
    bestBeam = 0;
    nextTimeBeam = firstTime;
    indexTimeBeam = 0;
    gettimeofday (&timeStart, NULL);
    while (true) {
      Problem p;
      p.init ();
      int score = p.nestedBeamRollout (level, beam);
      if (indexTimeBeam > nbTimes)
	break;
    }
    double t = firstTime;
    fprintf (stderr, "level %d, beam %d, iteration %d\n", level, beam [level], i + 1);
    for (int j = 0; j <= nbTimes; j++) {
      if (nbSearchTime [j] >= (7 * (i + 1)) / 10)
	fprintf (stderr, "%f %f\n", t, sumValueAfterTime [j] / nbSearchTime [j]);
      t *= 2;
    }
  }
  sprintf (s, "time.Level.%d.Beam.%d.%d.%d.%d.%d.nbSearches.%d.plot", level, beam [0], beam [1], beam [2], beam [3], beam [4], nbSearches);
  FILE * fp = fopen (s, "w");
  if (fp != NULL) {
    fprintf (fp, "# %d searches\n", nbSearches);
    double t = firstTime;
    for (int i = 0; i <= nbTimes; i++) {
      if (nbSearchTime [i] >= (7 * nbSearches) / 10)
	fprintf (fp, "%f %f\n", t, sumValueAfterTime [i] / nbSearchTime [i]);
      t *= 2;
    }
  }
  fclose (fp);
}

void plotAverageScore (int level, int beam [5]) {
  char s [1000];
  float sumScores [MaxBeam + 1];
  int maxBeam = beam [level];
  for (int b = 1; b <= MaxBeam; b *= 2)
    sumScores [b] = 0;
  for (int i = 0; i < nbSearches; i++) {
    for (int b = 1; b <= maxBeam; b *= 2) {
      beam [level] = b;
      Problem p;
      p.init ();
      int score = p.nestedBeamRollout (level, beam);
      sumScores [b] += score;
    }
    sprintf (s, "averageScore.Level.%d.Beam.%d.%d.%d.%d.%d.nbSearches.%d.plot", level, beam [0], beam [1], beam [2], beam [3], beam [4], nbSearches);
    FILE * fp = fopen (s, "w");
    if (fp != NULL) {
      fprintf (fp, "# %d searches\n", i + 1);
      for (int b = 1; b <= maxBeam; b *= 2) {
	fprintf (fp, "%d %f\n", b, sumScores [b] / (i + 1));
      }
    }
    fclose (fp);
  }
}

#ifdef USEMPI
int rang, nbProc;

int Problem::nestedBeamRolloutMPI (int n, int sizeBeam [MaxLevel]) {
  int nbBeam = 1;
 
  for (int b = 0; b < nbBeam; b++) {
    beam [n] [b] = getProblem ();
    *beam [n] [b] = *this;
    beam [n] [b]->lengthBestRollout = 0;
    beam [n] [b]->scoreBestRollout = 0;
  }

  int best = 0;
  while (true) {
    int nbNextBeam = 0;
    bool oneNewPosition = false;
      
    for (int b = 0; b < nbBeam; b++) {
      // insert the best problem in the next beam
      bool inserted = false;
      for (int i = 0; i < nbNextBeam; i++)
	if (nextBeam [n] [i]->scoreBestRollout < beam [n] [b]->scoreBestRollout) {
	  inserted = true;
	  if (nbNextBeam < sizeBeam [n])
	    nbNextBeam = nbNextBeam + 1;
	  else
	    putProblem (nextBeam [n] [nbNextBeam - 1]);
	  for (int j = nbNextBeam - 1; j > i; j--)
	    nextBeam [n] [j] = nextBeam [n] [j-1];
	  nextBeam [n] [i] = getProblem ();
	  *nextBeam [n] [i] = *beam [n] [b];
	  if (beam [n] [b]->scoreBestRollout > 0)
	    if (beam [n] [b]->lengthVariation < beam [n] [b]->lengthBestRollout)
	      nextBeam [n] [i]->playMove (beam [n] [b]->bestRollout [beam [n] [b]->lengthVariation]);
	  break;
	}
      if (!inserted && (nbNextBeam < sizeBeam [n])) {
	nextBeam [n] [nbNextBeam] = getProblem ();
	*nextBeam [n] [nbNextBeam] = *beam [n] [b];
	if (beam [n] [b]->scoreBestRollout > 0)
	  if (beam [n] [b]->lengthVariation < beam [n] [b]->lengthBestRollout)
	    nextBeam [n] [nbNextBeam]->playMove (beam [n] [b]->bestRollout [beam [n] [b]->lengthVariation]);
 	nbNextBeam = nbNextBeam + 1;
      }
    }
     
    int nbSent = 0;
    int client = 1;
    for (int b = 0; b < nbBeam; b++) {
      //beam [b].findMoves ();
      if (beam [n] [b]->moves.size () != 0)
	oneNewPosition = true;
      for (list<Move>::iterator it = beam [n] [b]->moves.begin (); it != beam [n] [b]->moves.end (); ++it) {
	Problem p = *beam [n] [b];
	p.playMove (*it);
	Message m;
	p.get (m);
	m.currentLengthVariation = p.lengthVariation;
	MPI::COMM_WORLD.Send(&m, sizeof (m), MPI::CHAR, client, 0);
	client++;
	if (client == nbProc)
	  client = 1;
	nbSent++;
	fprintf (stderr, "s=%d,", nbSent);
     }
    }

    fprintf (stderr, "nbSent = %d\n", nbSent);
    
    for (int c = 0; c < nbSent; c++) {
      Message m;
      MPI::Status status;
      MPI::COMM_WORLD.Recv (&m, sizeof (m), MPI::CHAR, MPI_ANY_SOURCE, 0, status);
      fprintf (stderr, "c=%d,", c);
      Problem p;
      p.set (m);
      // insert the  problem in the next beam
      bool inserted = false;
      for (int i = 0; i < nbNextBeam; i++)
	if (nextBeam [n] [i]->scoreBestRollout < p.score) {
	  inserted = true;
	  if (nbNextBeam < sizeBeam [n])
	    nbNextBeam = nbNextBeam + 1;
	  else
	    putProblem (nextBeam [n] [nbNextBeam - 1]);
	  for (int j = nbNextBeam - 1; j > i; j--)
	    nextBeam [n] [j] = nextBeam [n] [j-1];
	  nextBeam [n] [i] = getProblem ();
	  *nextBeam [n] [i] = p;
	  nextBeam [n] [i]->lengthVariation = m.currentLengthVariation;
	  nextBeam [n] [i]->scoreBestRollout = p.score;
	  nextBeam [n] [i]->lengthBestRollout = p.lengthVariation;
	  for (int j = 0; j < p.lengthVariation; j++)
	    nextBeam [n] [i]->bestRollout [j] = p.variation [j];
	  break;
	}
      if (!inserted && (nbNextBeam < sizeBeam [n])) {
	nextBeam [n] [nbNextBeam] = getProblem ();
	*nextBeam [n] [nbNextBeam] = p;
	nextBeam [n] [nbNextBeam]->scoreBestRollout = p.score;
	nextBeam [n] [nbNextBeam]->lengthBestRollout = p.lengthVariation;
	for (int j = 0; j < p.lengthVariation; j++)
	  nextBeam [n] [nbNextBeam]->bestRollout [j] = p.variation [j];
	nbNextBeam = nbNextBeam + 1;
      }
    }
    fprintf (stderr, "allReceived : lengthVariation = %d, best = %d\n", nextBeam [n] [0]->lengthVariation, nextBeam [n] [0]->lengthBestRollout);
    if (!oneNewPosition) {
      for (int b = 0; b < nbNextBeam; b++)
	putProblem (nextBeam [n] [b]);
      break;
    }
    for (int b = 0; b < nbBeam; b++)
      putProblem (beam [n] [b]);
    for (int b = 0; b < nbNextBeam; b++)
      beam [n] [b] = nextBeam [n] [b];
    nbBeam = nbNextBeam ;
    if (beam [n] [0]->scoreBestRollout > best) {
      best = beam [n] [0]->scoreBestRollout;
      if ((n >= 2)  || (n == level)) {
	fprintf (stderr, "\n\n");
	for (int t = 0; t < n - 1; t++)
	  fprintf (stderr, "\t");
	fprintf (stderr, "n = %d, beam = %d, progress = %d, length = %d, score = %d\n", n, sizeBeam [n], nextBeam [n] [0]->lengthVariation, nextBeam [n] [0]->lengthBestRollout, nextBeam [n] [0]->scoreBestRollout);
	if (nextBeam [n] [0]->lengthBestRollout >= 80)
	  if (n == level) {
	    char s [1000];
	    sprintf (s, "ms.Beam%d%d%d%d%d.Progress%d.Length.%d.ps", sizeBeam [0], sizeBeam [1], sizeBeam [2], sizeBeam [3], sizeBeam [4], nextBeam [n] [0]->lengthVariation, nextBeam [n] [0]->lengthBestRollout);
	    nextBeam [n] [0]->printMovesPS (s);
	  }
	fprintf (stderr, "\n\n");
      }
    }
  }
  score = beam [n] [0]->score;
  if (plot) {
    if (score > bestBeam)
      bestBeam = score;
    gettimeofday (&timeEnd, NULL);
    double time = ((float)(timeEnd.tv_sec - timeStart.tv_sec)) + 
      ((float)(timeEnd.tv_usec - timeStart.tv_usec))/1000000.0;
    if (time > nextTimeBeam) {
      while (time > 2 * nextTimeBeam) {
	indexTimeBeam++;
	nextTimeBeam *= 2;
      }
      sumValueAfterTime [indexTimeBeam] += bestBeam;
      nbSearchTime [indexTimeBeam]++;
      indexTimeBeam++;
      nextTimeBeam *= 2;
    }
  }
  lengthVariation = beam [n] [0]->lengthVariation;
  for (int j = 0; j < beam [n] [0]->lengthVariation; j++)
    variation [j] = beam [n] [0]->variation [j];
  for (int b = 0; b < nbBeam; b++)
    putProblem (beam [n] [b]);
  return score;
}

void sendRemoteAverageScore (int level, int maxBeam1, int maxBeam2) {
  int beam [5] = {1, 1, 1, 1, 1};
  while (true)
    for (int b1 = 1; b1 <= maxBeam1; b1 *= 2)
      for (int b2 = 1; b2 <= maxBeam2; b2 *= 2) 
	if (b1 * b2 < 64) {
	  beam [level - 1] = b1;
	  beam [level] = b2;
	  Problem p;
	  p.init ();
	  int score = p.nestedBeamRollout (level, beam);
	  int res [3];
	  res [0] = b1;
	  res [1] = b2;
	  res [2] = score;
	  MPI::COMM_WORLD.Send(res, sizeof (res), MPI::CHAR, 0, 0);  
	}
}

void getRemoteAverageScore (int level, int maxBeam1, int maxBeam2) {
  char s [1000];
  float sumScores [maxBeam1 + 1] [maxBeam2 + 1];
  int nbScores [maxBeam1 + 1] [maxBeam2 + 1];
  for (int b1 = 1; b1 <= maxBeam1; b1 *= 2) 
    for (int b2 = 1; b2 <= maxBeam2; b2 *= 2) {
      sumScores [b1] [b2] = 0;
      nbScores [b1] [b2] = 0;
    }
  while (true) {
    int res [3];
    MPI::Status status;
    MPI::COMM_WORLD.Recv (res, sizeof (res), MPI::CHAR, MPI_ANY_SOURCE, 0, status);  
    sumScores [res [0]] [res [1]] += res [2];
    nbScores [res [0]] [res [1]]++;
    fprintf (stderr, "level %d, beam %d,%d, score %d\n", level, res [0], res [1], res [2]);
    sprintf (s, "averageScore.Level.%d.Beam.%d.%d.plot", level, res [0], maxBeam2);
    FILE * fp = fopen (s, "w");
    if (fp != NULL) {
      int b1 = res [0];
      fprintf (fp, "# ");
      for (int b2 = 1; b2 <= maxBeam2; b2 *= 2)
	fprintf (fp, "%d searches beam %d, ", nbScores [b1] [b2], b2);
      fprintf (fp, "\n");
      for (int b2 = 1; b2 <= maxBeam2; b2 *= 2) {
	fprintf (fp, "%d %f\n", b2, sumScores [b1] [b2] / nbScores [b1] [b2]);
      }
    }
    fclose (fp);
  }
}
#endif

void usage () {
  fprintf (stderr, "morpionBeam -level 2 -beam1 16 -beam2 4 -nbTimes 17 -nbSearches 20\n");
}

int main (int argc, char ** argv) {
  initStackProblem ();
  initHash (); 

  level = 2;
  sizeBeam [1] = 1;
  sizeBeam [2] = 1;
  sizeBeam [3] = 1;
  sizeBeam [4] = 1;
  srand (time(NULL));

  for (int i = 1; i < argc; i++)
    if (!strcmp (argv [i], "-level")) {
      if (sscanf (argv [++i], "%d", & level) != 1)
	usage();
    } 
    else if (!strcmp (argv [i], "-N")) {
      if (sscanf (argv [++i], "%d", &N) != 1)
	usage();
    }
    else if (!strcmp (argv [i], "-beam1")) {
      if (sscanf (argv [++i], "%d", & sizeBeam [1]) != 1)
	usage();
    }
    else if (!strcmp (argv [i], "-beam2")) {
      if (sscanf (argv [++i], "%d", & sizeBeam [2]) != 1)
        usage();
    }
    else if (!strcmp (argv [i], "-beam3")) {
      if (sscanf (argv [++i], "%d", & sizeBeam [3]) != 1)
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
    else if (!strcmp (argv [i], "-touching")) {
      touching = true;
    }
    else if (!strcmp (argv [i], "-loop")) {
      loop = true;
    }
    else if (!strcmp (argv [i], "-plot")) {
      loop = false;
    }
    else if (!strcmp (argv [i], "-average")) {
      average = true;
    }

  Problem p;
  p.init ();

  
  if (average) {
    plotAverageScore (level, sizeBeam);
    exit (0);
  }

  beam [level] [0] = getProblem ();
  beam [level] [0]->init ();
  beam [level] [0]->initPolicy ();
  int score=p.beamNRPA (level, sizeBeam);
  //int score = p.NRPA (level);
  //int score = p.nestedRollout(level);
  cout << "Score: " << score << endl;

  exit (0);
 
  if (loop)
    while (true) {
      p.init ();
      int s = p.nestedBeamRollout (level, sizeBeam);
      fprintf (stderr, "%d,", s);
    }
  
  plotTime (level, sizeBeam);
  
/*   sizeBeam [1] = 1; */
/*   sizeBeam [2] = 1; */
/*   plotTime (2, sizeBeam); */

/*   sizeBeam [1] = 2; */
/*   sizeBeam [2] = 1; */
/*   plotTime (2, sizeBeam); */

/*   sizeBeam [1] = 3; */
/*   sizeBeam [2] = 1; */
/*   plotTime (2, sizeBeam); */

/*  sizeBeam [1] = 1;
  sizeBeam [2] = 1;
  plotTime (2, sizeBeam);
*/

/*   sizeBeam [1] = 1; */
/*   sizeBeam [2] = 2; */
/*   plotTime (2, sizeBeam); */

/*   sizeBeam [1] = 1; */
/*   sizeBeam [2] = 3; */
/*   plotTime (2, sizeBeam); */

/*   sizeBeam [1] = 1; */
/*   sizeBeam [2] = 4; */
/*   plotTime (2, sizeBeam); */

/*   sizeBeam [1] = 2; */
/*   sizeBeam [2] = 2; */
/*   plotTime (2, sizeBeam); */

//  exit (1);

/*   for (int i = 0; i < 20; i++) { */
/*     problem [i].print (stderr); */
/*     fprintf (stderr, "\n"); */
/*     problem [i].findMoves (); */
/*     fprintf (stderr, "\n\n\n"); */
/*   } */
  int nbSample = 10;
  float sum = 0;
  int maxi = 0;
/*   for (int i = 0; i < nbSample; i++) { */
/*     Problem p = problem [0]; */
/*     p.playout (); */
/*     fprintf (stderr, "%d ", p.score); */
/*     if (p.score > maxi) */
/*       maxi = p.score; */
/*     sum += p.score; */
/*   } */
/*   fprintf (stderr, "\nmean = %2.2f\nmaxi = %d\n", sum / nbSample, maxi); */
/*   sum = 0; */
/*   maxi = 0; */
/*   for (int i = 0; i < nbSample; i++) { */
/*     Problem p = problem [0]; */
/*     nestedRollout (p, 0); */
/*     fprintf (stderr, "%d ", p.score); */
/*     if (p.score > maxi) */
/*       maxi = p.score; */
/*     sum += p.score; */
/*   } */
/*   fprintf (stderr, "\nmean = %2.2f\nmaxi = %d\n", sum / nbSample, maxi); */
/*   nbSample = 1; */
/*   sum = 0; */
/*   maxi = 0; */
/*   for (int i = 0; i < nbSample; i++) { */
/*     Problem p = problem [0]; */
/*     nestedRollout (p, 1); */
/*     fprintf (stderr, "%d ", p.score); */
/*     if (p.score > maxi) */
/*       maxi = p.score; */
/*     sum += p.score; */
/*   } */
/*   fprintf (stderr, "\nmean = %2.2f\nmaxi = %d\n", sum / nbSample, maxi); */

  //srand (time(NULL));
  /* int pb = 0; */
  /* int best = 0; */
  /* for (int i = 0; i < 5000; i++) { */
  /*   Problem p = problem [pb]; */
  /*   //sizeBeam [1] = 4; */
  /*   p.nestedBeamRollout (2, sizeBeam); */
  /*   fprintf (stderr, "\n"); */
  /*   fprintf (stderr, "score (%d) = %d\n\n", pb, p.score); */
  /*   if (p.score > best) */
  /*     best = p.score; */
  /*   fprintf (stderr, "best = %d\n", best); */
  /*   if (p.score > highScore [pb].score) { */
  /*     highScore [pb].score = p.score; */
  /*     highScore [pb].lengthVariation = p.lengthVariation; */
  /*     for (int j = 0; j < p.lengthVariation; j++) */
  /* 	highScore [pb].variation [j] = p.variation [j]; */
  /*     printHighScore (pb); */
  /*   } */
  /* } */
}

