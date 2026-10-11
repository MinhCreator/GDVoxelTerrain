#ifndef MESH_COMPUTE_SCHEDULER_H
#define MESH_COMPUTE_SCHEDULER_H

#include "voxel_octree_node.h"
#include <atomic>
#include <functional>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <thread>
#include "utility/thread_pool.h"
#include "utility/concurrent_priority_queue.h"

using namespace godot;

class JarVoxelTerrain;


struct ChunkComparator {
    bool operator()(const VoxelOctreeNode *a, const VoxelOctreeNode *b) const {
        return a->get_lod() > b->get_lod();
    }
};
// struct ChunkComparator2 {
//     bool operator()(const std::pair<VoxelOctreeNode*, ChunkMeshData*> a, const std::pair<VoxelOctreeNode*, ChunkMeshData*> b) const {
//         return a.first->get_lod() > b.first->get_lod();
//     }
// };

class MeshComputeScheduler
{
  private:
    ConcurrentPriorityQueue<VoxelOctreeNode*, ChunkComparator> ChunksToAdd;
    ConcurrentPriorityQueue<std::pair<VoxelOctreeNode*, ChunkMeshData*>> ChunksToProcess;

    std::atomic<int> _activeTasks;
    int _maxConcurrentTasks;

    ThreadPool threadPool;

    // Debug variables
    int _totalTris;
    int _prevTris;

    void process_queue(JarVoxelTerrain &terrain);
    void run_task(const JarVoxelTerrain &terrain, VoxelOctreeNode &chunk);

  public:
    MeshComputeScheduler(int maxConcurrentTasks);
    void enqueue(VoxelOctreeNode &node);
    void process(JarVoxelTerrain &terrain);
    void clear_queue();

    // True while any mesh task holds octree node pointers: queued, running on
    // the pool, or finished but not yet applied on the main thread. The octree
    // must not be mutated (build / edit) while this is true.
    bool is_meshing()
    {
        return _activeTasks > 0 || !ChunksToAdd.empty() || !ChunksToProcess.empty();
    }
};

#endif // MESH_COMPUTE_SCHEDULER_H
