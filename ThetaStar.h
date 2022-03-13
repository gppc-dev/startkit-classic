#pragma once
#include <cstdio>
#include <iostream>
#include <limits>
#include <queue>
#include <string>
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

  string to_str() {
    return "(" + to_string(x) + ", " + to_string(y) + 
      ") f:" + to_string(g+h) + " g:" + to_string(g) + ", h: " + to_string(h); 
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
        if (emptyLoc(x-1, y-1) || emptyLoc(x, y-1)) return true;
        break;
      case SOUTH:
        if (emptyLoc(x-1, y) || emptyLoc(x, y)) return true;
        break;
      case WEST:
        if (emptyLoc(x-1, y-1) || emptyLoc(x-1, y)) return true;
        break;
      case EAST:
        if (emptyLoc(x, y-1) || emptyLoc(x, y)) return true;
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
    // check visibility from (px, py) to (cx, cy)
    int dx = cx - px;
    int dy = cy - py;
    // it is possible moving back and forth since no pruning
    if (dx == 0 && dy == 0) return false;
    const double eps = 1e-6;
    if (dx == 0) {
      int fromY = dy>0?py: py-1;
      int toY = dy>0?cy-1: cy;
      dy /= abs(cy-py);
      for (int y=fromY; y != toY+dy; y+=dy) {
        if ((!emptyLoc(px-1, y) && !emptyLoc(px, y))) return false;
      }
    }
    else if (dy == 0) {
      int fromX = dx>0?px: px-1;
      int toX = dx>0?cx-1: cx;
      dx /= abs(cx-px);
      for (int x=fromX; x != toX+dx; x+=dx) {
        if ((!emptyLoc(x, py-1) && !emptyLoc(x, py))) return false;
      }
    }
    else {
      int x0, y0, x1, y1;
      if (px < cx) {x0 = px, y0 = py, x1 = cx, y1 = cy;}
      else {x0 = cx, y0 = cy, x1 = px, y1 = py;}
      double r = (double)(y1 - y0) / (x1 - x0);
      int dx = (x1 - x0) / abs(x1 - x0);
      int dy = (y1 - y0) / abs(y1 - y0);
      auto f = [&](double x) {
        return r*x-r*x0+y0;
      };
      for (int x=x0; x<x1; x++) {
        int fromY = (int)floor(f((double)x+eps));
        int toY = (int)floor(f((double)x+1-eps));
        for (int y=fromY; y!=toY+dy; y+=dy) {
          if (!emptyLoc(x, y)) return false;
        }
      }
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
      // cerr << "Pop out node: " << c.to_str() << " parent: ("
      //      << pa[id(c)] % width << ", " << pa[id(c)] / width << ")" << endl;
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
            // cerr << "Push node: " << nxt.to_str() << endl;
            q.push(nxt);
          }
        }
      }
    }
    return -1;
  }
};
