# RenderGraph


#### Sorting
When information about the render passes which create a RenderGraph has been collected, it is then required to find an order for the nodes whereby no Pass (A) that reads a resource that is written to by a Pass (B) should be prior to (B) in the final ordering. Speaking in a more abstract way (applicable to any DAG); For every directed edge (U, V), U must come before V in the final ordering.

This can be done by creating an adjacency list from the passes, where the adjacent indices are passes in the graph which consume a resource which is written to by the current pass.

I then use a Topological sort to order the remaining nodes based on the information provided by the adjacency list, I use a modified version of a DFS in order to recursively traverse the dependencies. 

However, while this creates an ordered list which satisfies the above predicate, it is not potentially the best ordering, and does not offer any information about potential parallel execution which modern hardware supports. To further improve this, 