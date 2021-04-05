### Generating a Voronoi Diagram on the Surface of a Unit Sphere
<img src=Demo.png>

#### Credits
- http://www.math.kent.edu/~zheng/papers/voronoi_paper_updated.pdf The paper I followed for the algorithm 
- https://github.com/kelvin13/voronoi A swift reference implementation which I relied on specifically for the intersection of arcs on a sphere, and how to search through the beachline in log(n) time for a given point

#### What I encountered
There are a few ways to create a voronoi diagram on the surface of a sphere, namely
1. A version of [Fortunes Algorithm](https://en.wikipedia.org/wiki/Fortune%27s_algorithm) modified to run on the surface of a sphere
2. A regular fortunes algorithm run on a plane, then projected onto the surface of the sphere using [Stereographic Projection](https://en.wikipedia.org/wiki/Stereographic_projection)
3. Generate the [Delanuay Triangulation](https://en.wikipedia.org/wiki/Delaunay_triangulation) of the sphere which is the dual of the Voronoi diagram, through generating the 3D convex hull of the points.

This is an example of the first implementation, it runs in nlogn time, and has no significant deviations to the paper, however I used a RB-Tree instead of a Skiplist to represent the beachline, as I found it simplified the algorithm-facing insertion code when adding new Arcs into the tree. This implementation of the algorithm does not need to handle the case of non-bounded grids, as the surface of the sphere is finitely bound. However, care needs to be taken for the final two edges left by the algorithm, as they will not be connected by the end of the algorithm;

#### Issues / Maybe Todos for this project
**Algorithm**
- [ ] There are a lot of duplicated triangles drawn for the delanuay triangulation, this could be improved by counting the amount of times an edge has been used to draw a triangle (and limiting it to 2), or improving the representation by replacing the messy DCEL data structure currently used
- [ ] There are floating point issues seemingly inherent with the algorithm, and its not something I am very familiar with improving;
  - [ ] Currently causes hard crashes (should add safety to handle the errors)
  - [ ] Move to using doubles to see whether it improves the numerical stability (or optimally templating it)
  - [ ] Finding a more robust way to predict when numerical stabiity is the cause of issues (currently breaks the beachline)
- [ ] There are edge cases pointed out in the original paper which are not handled by my code
- [ ] The visualisation for the delanuay triangulation doesn't draw the triangles in the correct order for larger point sets, leading to gaps in the 'shell' of the sphere 
- [ ] I could improve the efficiency of the Lookup for a BeachArcs predecessor/Successor (they currently hold ptr's to their Left and Right arcs, but this is not equivalent to the pred/succ), which would improve the performance of `FindArcOnBeach`
**Renderer**
- [ ] I could probably pack all of the buffers into a single VkBuffer given they have the same vertex attributes, and I can use Eulers polyhedron formula in order to determine a 'worst case' vertex size for delanuay / voronoi edges (as described in the aforementioned paper, section 10), removing the need to store all vertices because the size of the buffer could need to be changed.
- [ ] Currently I (not as I want to) duplicate resources for every frame in flight, yet in this example I have single un-synchronised buffers.
**Enhancements**
- [ ] Draw the beachline as a series of arcs between the points, instead of straight lines between points
- [ ] Threading? I think the algorithm could be run on disjoint sets of points on the sphere in parallel, then stitched together afterwards. However, currently it performs quite well for the small (relatively) data sets it can operate on given the numerical stability issues I've yet to solve, so I am not sure the point.


