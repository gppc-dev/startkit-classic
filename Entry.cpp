#include <algorithm>
#include <map>
#include "ThetaStar.h"
#include "Entry.h"


/**
 * User code used during preprocessing of a map.  Can be left blank if no pre-processing is required.
 * It will not be called in the same program execution as `PrepareForSearch` is called,
 * all data must be shared through file.
 * 
 * Called with command below:
 * ./run -pre file.map
 * 
 * @param[in] bits Array of 2D table.  (0,0) is located at top-left corner.  bits.size() = height * width
 *                 Packed as 1D array, row-by-ray, i.e. first width bool's give row y=0, next width y=1
 *                 bits[i] returns `true` if (x,y) is traversable, `false` otherwise
 * @param[in] width Give the map's width
 * @param[in] height Give the map's height
 * @param[in] filename The filename you write the preprocessing data to.  Open in write mode.
 */
void PreprocessMap(vector<bool> &bits, int width, int height, const string filename) {}

/**
 * User code used to setup search before queries.  Can also load pre-processing data from file to speed load.
 * It will not be called in the same program execution as `PreprocessMap` is called,
 * all data must be shared through file.
 * 
 * Called with any commands below:
 * ./run -run file.map file.map.scen
 * ./run -check file.map file.map.scen
 * 
 * @param[in] bits Array of 2D table.  (0,0) is located at top-left corner.  bits.size() = height * width
 *                 Packed as 1D array, row-by-ray, i.e. first width bool's give row y=0, next width y=1
 *                 bits[i] returns `true` if (x,y) is traversable, `false` otherwise
 * @param[in] width Give the map's width
 * @param[in] height Give the map's height
 * @param[in] filename The filename you write the preprocessing data to.  Open in write mode.
 * @returns Pointer to data-structure used for search.  Memory should be stored on heap, not stack.
 */
void *PrepareForSearch(vector<bool> &bits, int width, int height, const string filename) {
  ThetaStar* astar = new ThetaStar(&bits, width, height);
  return astar;
}

/**
 * User code used to setup search before queries.  Can also load pre-processing data from file to speed load.
 * It will not be called in the same program execution as `PreprocessMap` is called,
 * all data must be shared through file.
 * 
 * Called with any commands below:
 * ./run -run file.map file.map.scen
 * ./run -check file.map file.map.scen
 * 
 * @param[in,out] data Pointer to data returned from `PrepareForSearch`.  Can static_cast to correct data type.
 * @param[in] s The start (x,y) coordinate of search query
 * @param[in] g The goal (x,y) coordinate of search query
 * @param[out] path The points that forms the shortest path from `s` to `g` computed by search algorithm.
 *                  Shortest path length will calculated by summation of Euclidean distance
 *                  between consecutive pairs path[i]--path[i+1].  Collinear points are allowed.
 *                  Return an empty path if no shortest path exists.
 * @returns `true` if search is complete, including if no-path-exists.  `false` if search only partially completed.
 *          if `false` then `GetPath` will be called again until search is complete.
 */
bool GetPath(void *data, xyLoc s, xyLoc g, vector<xyLoc> &path) {

  ThetaStar* astar = (ThetaStar*)(data);
  int16_t w = astar->width;

  vector<int> pa(astar->bits->size(), -1);
  double d = astar->run(s.x, s.y, g.x, g.y, pa);
  if (d > 0) {
    int16_t x = static_cast<int16_t>(g.x), y = static_cast<int16_t>(g.y);
    while (true) {
      path.push_back({static_cast<double>(x), static_cast<double>(y)});
      if (x == s.x && y == s.y) break;
      int cid = y * w + x;
      x = pa[cid] % w;
      y = pa[cid] / w;
    }
    reverse(path.begin(), path.end());
  }
  return true;
}

/**
 * The algorithm name.  Please update string and ensure name is immutable.
 * 
 * @returns the name of the algorithm
 */
const string GetName() { return "example-Theta*"; }
