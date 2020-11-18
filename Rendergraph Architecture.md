# RenderGraph
Represents a frame of work through a collection of passes.
#### Fields:
- passes
- resources
- framesInFlight
- queues

#### Methods:
- Build
- Clear
- Execute

- CreateGraph
This method creates the directed acyclic graph which represents the work that will be executed per frame. It will order the input passes with regards to all synchronisation, and will add information about dependency levels (I.e. Which passes can be executed in paralell). It has three primary steps:

1. Create Adjacency List of Pass Dependencies
2. Topologically Sort Pass List using Adjacency List
3. Build Dependency Levels based on information from the sorted list/
  
- ValidateGraph, CreateResources

#### Questions
- How to thread command buffer recording, when the order in which they are submitted must be linear? Is the order they are inside the array passed to the main buffer indicative of the order they will be begun in?
- What determines if two resources can be aliased? - What actually is aliasing, is it sharing memory, or sharing a VkImage/Buffer
  * If two resources are used in disjoint parts of the graph, and the dimensions line up
  * To note, orienting around minimal overlap will likely effect the opportunities for resource aliasing
- Which stages of graph construction could be multi-threaded?  
  * The physical creation of renderpasses and resources, this would require a thread safe memory allocator.
- How is the renderpass and frame buffer found for a given pass, and what determines if two passes could be merged.
- Where could implicit renderpass synchronisation replace a pipeline barrier
- Could timeline semaphores be used for cross queue interaction ? (Pack bits into an integer depending on what has signalled it)
- How does a bindless architecture impact the creation and use of a rendergraph



# PassDesc
The PassDesc represents a logical grouping of work that must be executed within a frame. Expressing dependencies between passes is done via explicit mention of the resources which are read and written to by each pass.

Passes will be re-ordered by the rendergraph after submission, passes which can run in parallel will be, and commands can be recorded over multiple threads.

#### Fields:
- passId
- queueIndex
- readResources
- writtenResources
- feedbackResources
- syncObjects
- execute ( lambda )

#### Methods:
- AddRead(Buffer/Image)
- AddWritten(Buffer/Image)
- AddFeedback(Buffer/Image)
- SetRecordFunc

#### Questions:


# Resources

## SyncResource

#### Types:
- Timeline Semaphore (GPU-GPU), esp. Cross queue
- Fence (GPU - CPU)
- Pipeline Barrier (Resource Transitions)


#### Methods:
- InsertBarrier


## Usage
Provides information about the use of a resource.

#### Fields:
- passId
- stageFlags
- accessFlags
- queueFamilyIndex
- (optional) imgLayout

## Resource

#### Fields:
- name
- readResources
- writtenResources

#### Questions:
- How do I implement feedback resources, I.e. a resource which requires information from a previous frame, for example, cellular automata