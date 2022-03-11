#pragma once
#include <cstdio>
#include <limits>
#include <queue>
#include <vector>
#include <math.h>
#include <map>
using namespace std;

class ThetaStar {

const double SQRT2 = 1.4141;

typedef enum
{
	NORTH = 1,
	SOUTH = 2,
	EAST = 4,
	WEST = 8,
	NORTHEAST = 16,
	NORTHWEST = 32, 
	SOUTHEAST = 64,
	SOUTHWEST = 128,
} dir;

struct Node {
  int x, y;
  double g = 0;
  double h = 0;

  inline double f() const { return g + h; }

  bool operator< (const Node& rhs) const {
    if (f() == rhs.f()) return g > rhs.g;
    else return f() > rhs.f();
  }
};

public:
  const vector<bool>* bits; // grid map
  int width, height;
  vector<double> dist;

  ThetaStar(const vector<bool>* mapData, int w, int h): 
    bits(mapData), width(w), height(h) {};

  inline int id(const Node&loc) const {
    return loc.y * width + loc.x;
  }
    

/*

current point is `(x, y)`

| direction   | to point     | condition                                       |
|-------------|--------------|-------------------------------------------------|
| `north`     | `(x, y-1)`   | `loc[y-1, x-1] = loc[y-1, x] = 1`               |
| `south`     | `(x, y+1)`   | `loc[y, x-1] = loc[y, x] = 1`                   |
| `west`      | `(x+1, y)`   | `loc[y-1, x] = loc[y, x] = 1`                   |
| `east`      | `(x-1, y)`   | `loc[y-1, x-1] = loc[y-1, x] = 1`               |
| `northwest` | `(x-1, y-1)` | `loc[y-1, x-1] = loc[y-1, x] = loc[y-1, x] = 1` |
| `northeast` | `(x+1, y-1)` | `loc[y-1, x] = loc[y-1, x-1] = loc[y, x] = 1`   |
| `southwest` | `(x-1, y+1)` | `loc[y, x-1] = loc[x-1, y-1] = loc[y, x] = 1`   |
| `southeast` | `(x+1, y+1)` | `loc[y, x] = loc[y-1, x] = loc[y, x-1] = 1`     |
*/

  inline bool validMove(int x, int y, dir d) const {
    // for cardinal moves, one of side must be empty
    // for diagonal move, the start point must be traversable (no double corner)
    // and the region must be "emptyLoc"
    switch (d) {
      case NORTH:
        if (emptyLoc(x-1, y-1) && emptyLoc(x, y-1)) return true;
        break;
      case SOUTH:
        if (emptyLoc(x-1, y) && emptyLoc(x, y)) return true;
        break;
      case WEST:
        if (emptyLoc(x-1, y-1) && emptyLoc(x, y)) return true;
        break;
      case EAST:
        if (emptyLoc(x-1, y-1) && emptyLoc(x, y-1)) return true;
        break;
      case NORTHWEST:
        if (emptyLoc(x-1, y-1) && emptyPoint(x, y)) return true;
        break;
      case NORTHEAST:
        if (emptyLoc(x, y-1) && emptyPoint(x, y)) return true;
        break;
      case SOUTHWEST:
        if (emptyLoc(x-1, y) && emptyPoint(x, y)) return true;
        break;
      case SOUTHEAST:
        if (emptyLoc(x, y) && emptyPoint(x, y)) return true;
        break;
      default:
        break;
    }
    return false;
  }

  inline bool emptyLoc(int x, int y) const {
    if (x<0 || x>=width || y<0 || y>=height) return false;
    return bits->at(id({x, y}));
  }

  inline bool emptyPoint(int x, int y) const {
    // no double corner cutting
    if (!emptyLoc(x-1, y-1) && !emptyLoc(x, y)) return false;
    if (!emptyLoc(x-1, y) && !emptyLoc(x, y-1)) return false;
    return true;
  }

  double hVal(const Node& a, const Node& b) {
    int diag = min(abs(a.x - b.x), abs(a.y - b.y));
    int card = abs(a.x - b.x) + abs(a.y - b.y) - 2*diag;
    return card + diag * SQRT2;
  }

  double euclidean(int x0, int y0, int x1, int y1) {
    int dx = (x1 - x0);
    int dy = (y1 - y0);
    return sqrt(dx*dx + dy*dy);
  }

  bool segXvisible(int y, int xl, int xu) {
    // char [xl, xr-1] * [y-1, y] must be traversable
    for (int i=y-1; i<=y; i++) 
    for (int x=xl; x<xu; x++) {
      if (!emptyLoc(x, i)) return false;
    }
    return true;
  }

  bool segYvisible(int x, int yl, int yu) {
    // char [x-1, x] * [yl, yu]
    for (int i=x-1; i<=x; i++)
    for (int y=yl; y<yu; y++) {
      if (!emptyLoc(i, y)) return false;
    }
    return true;
  }

  bool visible(int px, int py, int cx, int cy) {
    int dx = cx - px;
    int dy = cy - py;
    // parent to current is following cardinal or diagonal moves.
    if (dx == 0 || dy == 0 || abs(dx) == abs(dy)) return true;
    double r;
    r = (double)dy / (double)dx;
    for (int i=1; i<abs(dx); i++) {
      int x = px + i*dx;
      int yl = floor((double)py + (double)(i*r));
      int yu = ceil((double)py + (double)(i*r));
      if (!segYvisible(x, yl, yu)) return false;
    }

    r = (double)dx / (double)dy;
    for (int i=1; i<abs(dy); i++) {
      int y = py + i*dy;
      int xl = floor((double)px + (double)(i*r));
      int xu = ceil((double)px + (double)(i*r));
      if (!segXvisible(y, xl, xu)) return false;
    }
    return true;
  }

  void update_vert(int& pid, Node& nxt, vector<int>& pa) {
    while (pa[pid] != -1) {
      int px = pa[pid] % width;
      int py = pa[pid] / width;
      if (visible(px, py, nxt.x, nxt.y)) {
        nxt.g = dist[pa[pid]] + euclidean(px, py, nxt.x, nxt.y);
        pid = pa[pid];
      }
      else break;
    }
  }

  double run(int sx, int sy, int gx, int gy, vector<int>& pa) {
    priority_queue<Node, vector<Node>, less<Node>> q;
    dist = vector<double>(bits->size(), numeric_limits<double>::max());
    Node g{gx, gy, 0, 0};
    Node s{sx, sy, 0, 0};
    s.h = hVal(s, g);

    dist[id(s)] = 0;
    pa[id(s)] = -1;
    q.push(s);

    const int dx[] = {0, 0, 1, -1, 1, -1, 1, -1};
    const int dy[] = {-1, 1, 0, 0, -1, -1, 1, 1};
    while (!q.empty()) {
      Node c = q.top(); q.pop();
      if (c.g != dist[id(c)]) continue;
      if (c.x == g.x && c.y == g.y) return c.g;
      for (int i=0; i<8; i++) 
      if (validMove(c.x, c.y, (dir)(1<<i))) {
        int x = c.x + dx[i];
        int y = c.y + dy[i];
        if (0 <= x && x < width && 0 <= y && y < height) {
          double w = (c.x == x || c.y == y)? 1: SQRT2;
          int pid = id(c);
          Node nxt = {x, y, c.g+w, 0};
          update_vert(pid, nxt, pa);
          if (dist[id({x, y})] > nxt.g) {
            dist[id({x, y})] = nxt.g;
            pa[id({x, y})] = pid;
            nxt.h = hVal(nxt, g);
            q.push(nxt);
          }
        }
      }
    }
    return -1;
  }
};
