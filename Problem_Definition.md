# Problem statement: any-angle setting
You are given a query with a start/target point, and must find a path on a Euclidean plane represented by a grid.
The solution is then evaluated based on optimality, time performance and space cost.

## From Grid to Euclidean Plane
Input grid format is specified at [MovingAI](https://movingai.com/benchmarks/formats.html).

The grid is mapped to the Euclidean plane, details below:

  1. Euclidean plane `(x,y)` coordinates increases to the right (east) and downwards (south) respectively.

  2. Each grid cell maps to the Euclidean plane with a zero-based integer coordinate `(x,y)`.

  3. The grid cell `(x,y)` translates to a square on the Euclidean plane, area `(x,y)` to `(x+1,y+1)`, i.e. a grid coordinate is the top-left corner.

  4. The blocked grid cell squares translates to non-traversable space on the Euclidean plane.

### Example Map
Grid:

    ...#.
    #.#..
    #..#.
    ##...

Euclidean palne:

<p align="center">
<img src="./figs/grid_plane.png" height="200">
</p>

## Agent

1. Agent at `(x, y)` is a point on the Euclidean plane

2. Agents are provided as start and target positions by the query, these are given as **Integer Coordinates** located at non-blocked grid cell,
which as stated earlier translates to the top-left corner of a cell on the Euclidean plane.
For example, all purple points in the following plane are possible start and target positions (as they are open on the grid), while all red points will never be given.
  <p align="center">
    <img src="figs/grid_plane_start_target.png" height="200" > 
  </p>

## Path
For a query `<s, t>`, a valid path is a sequence of **Real Coordinates** `p=<s,v1,...vn,t>`, where any two consecutive coordinates `a` and `b` on the path must be a **valid path segment** (see details in the next section).

The following example has the query `<(0,0), (1,3)>`, and shows a valid path (which is also the shortest path for the query), `<(0,0), (3,3), (3,4), (2,4), (1,3)>`: 
  <p align="center">
    <img src="figs/grid_plane_path.png" height="200" > 
  </p>


**When start and target are the same position, the path must be empty, thus the length will be `0`.** TODO: would not empty be no-solution, should it just be a single point?

## Valid Path Segments

For any given two consecutive coordinates `a` and `b` in a path, where the agent moves from position `a` to position `b`, the following constraints must be followed in general:
  
1. The Euclidean distance from `a` to `b` must be of length at least 0.01, to alleviate ambiguities with epsilons.

2. Can't pass through blocked grid cell square (non-traversable area), excluding squares corners/edges.

3. Can't pass through boundaries shared by two adjacent blocked grid cell edges.

4. Can't pass through any cell corner shared by two diagonal adjacent blocked grid cell squares (No Double Corner Cutting).

5. Any boundary of a blocked grid cell square that is also the boundary of the map (assume the map's boundary is surrounded by blocked grid cells).

In the following example, path segment `(a, b)` is invalid if `b` is located on red points, and is valid if `b` is located on purple points:
  <p align="center">
    <img src="figs/invalid_segments.png" height="200" width="200">
  </p>


### When `a` is a start position on a double conner

A special case is that `a` is a start position located on a double corner. In this case, the outgoing direction must between `EAST` and  `SOUTH`.

In the following example, path segment `(a, b)` is invalid if `b` is located on red points, and is valid if `b` is located on purple points:
  <p align="center">
    <img src="figs/invalid_segments_start.png" height="200" width="200">
  </p>

### When `b` is a target position on a double conner

Another special case is that `b` is a target position located on a double corner. In this case, the incoming direction must come between `EAST` and  `SOUTH`.

In the following example, path segment `(a, b)` is invalid if `a` is located on red points, and is valid if `a` is located on purple points:
  <p align="center">
    <img src="figs/invalid_segments_target.png" height="200" width="200">
  </p>



