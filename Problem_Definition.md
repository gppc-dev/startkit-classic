# Problem statement: any-angle setting
Your task is to find a path on a given graph, and the solution is evaluated based on optimality, time performance and space cost.

We use following concept and notations in this document.

* term *location `(x, y)`* means the location in **char-map** from input, (non-traversable: `location(x, y)=0`, traversable: `location(x, y)=1`)
* term *point `(x, y)`* means a point in **euclidean plane**

## Map
Instead of using polygons, the map is represented by characters.

  1. the input map is a `h*w` char map, i.e. the map is represented by `h` row strings, each row has `w` characters;

  2. traversable char (e.g. `.`) at location `(x, y)` (row y, column x) means `[x, x+1] * [y, y+1]` area in euclidean plane is traversable

  3. non-traversable chars (`SWT@O`) at location `(x, y)` means `(x, x+1) * (y, y+1)` area in plane is non-traversable
 
## No Double Corner Cutting

* point `(x, y)` is non-traversable if both diagonal quadrants are non-traversable, e.g.

  * In euclidean plane: <img src="./figs/corner-cutting.png" width="250" height="125">
  * In char-map
    ```
      .#      #.
      #.  or  .#
    ```

## Agent

  1. agent at `(x, y)` is a point on the euclidean plane

  2. agent start and target position is an intersection point <img src="figs/intersection-example.png" height="100" width="100"> -- the top-left corner of an area that represented by the char-map, 
  and it must be a traversable area, i.e. `location(x, y)=1`

  3. agent cannot pass through double-cutting corner; when agent start or target is a double-cutting corner,
  **the only case is**: <img src="figs/corner-cutting-case.png" height="100" width="100">

  * then outgoing / incoming direction must between `EAST` and `SOUTH`, i.e. shifting point `(x, y)` to `(x', y')` in `SOUTHEAST` with a small distance.

**When start and target are same node, the path must be empty, the length must be `0`.**
