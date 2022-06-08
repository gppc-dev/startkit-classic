#pragma once
#include <cstdio>
#include <limits>
#include <queue>
#include <vector>
#include <math.h>
#include <map>
using namespace std;

class Astar {

const double SQRT2 = sqrt(2);

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

  Astar(const vector<bool>* mapData, int w, int h): 
    bits(mapData), width(w), height(h) {};

  inline int id(const Node&loc) const {
    return loc.y * width + loc.x;
  }
    
  inline bool traversable(const Node& loc) const {
    return bits->at(id(loc));
  }

  double hVal(const Node& a, const Node& b) {
    int diag = min(abs(a.x - b.x), abs(a.y - b.y));
    int card = abs(a.x - b.x) + abs(a.y - b.y) - 2*diag;
    return card + diag * SQRT2;
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
    const int dy[] = {1, -1, 0, 0, 1, -1, -1, 1};
    while (!q.empty()) {
      Node c = q.top(); q.pop();
      if (c.g != dist[id(c)]) continue;
      if (c.x == g.x && c.y == g.y) return c.g;
      for (int i=0; i<8; i++) {
        int x = c.x + dx[i];
        int y = c.y + dy[i];
        if (0 <= x && x < width && 0 <= y && y < height) {
          // no corner cutting
          if (!traversable({c.x, y}) || 
              !traversable({x, c.y}) ||
              !traversable({x, y})) 
            continue;
          double w = (c.x == x || c.y == y)? 1: SQRT2;
          if (dist[id({x, y})] > c.g + w) {
            dist[id({x, y})] = c.g + w;
            pa[id({x, y})] = id({c.x, c.y});
            Node nxt = {x, y, c.g+w};
            nxt.h = hVal(nxt, g);
            q.push(nxt);
          }
        }
      }
    }
    return -1;
  }
};
