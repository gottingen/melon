//

#ifndef ABEL_SYNCHRONIZATION_INTERNAL_GRAPHCYCLES_H_
#define ABEL_SYNCHRONIZATION_INTERNAL_GRAPHCYCLES_H_

// graph_cycles detects the introduction of a cycle into a directed
// graph that is being built up incrementally.
//
// Nodes are identified by small integers.  It is not possible to
// record multiple edges with the same (source, destination) pair;
// requests to add an edge where one already exists are silently
// ignored.
//
// It is also not possible to introduce a cycle; an attempt to insert
// an edge that would introduce a cycle fails and returns false.
//
// graph_cycles uses no internal locking; calls into it should be
// serialized externally.

// Performance considerations:
//   Works well on sparse graphs, poorly on dense graphs.
//   Extra information is maintained incrementally to detect cycles quickly.
//   insert_edge() is very fast when the edge already exists, and reasonably fast
//   otherwise.
//   find_path() is linear in the size of the graph.
// The current implementation uses O(|V|+|E|) space.

#include <cstdint>

#include <abel/base/profile.h>

namespace abel {

    namespace thread_internal {

// Opaque identifier for a graph node.
        struct graph_id {
            uint64_t handle;

            bool operator==(const graph_id &x) const { return handle == x.handle; }

            bool operator!=(const graph_id &x) const { return handle != x.handle; }
        };

// Return an invalid graph id that will never be assigned by graph_cycles.
        ABEL_FORCE_INLINE graph_id invalid_graphId() {
            return graph_id{0};
        }

        class graph_cycles {
        public:
            graph_cycles();

            ~graph_cycles();

            // Return the id to use for ptr, assigning one if necessary.
            // Subsequent calls with the same ptr value will return the same id
            // until Remove().
            graph_id get_id(void *ptr);

            // Remove "ptr" from the graph.  Its corresponding node and all
            // edges to and from it are removed.
            void remove_node(void *ptr);

            // Return the pointer associated with id, or nullptr if id is not
            // currently in the graph.
            void *ptr(graph_id id);

            // Attempt to insert an edge from source_node to dest_node.  If the
            // edge would introduce a cycle, return false without making any
            // changes. Otherwise add the edge and return true.
            bool insert_edge(graph_id source_node, graph_id dest_node);

            // Remove any edge that exists from source_node to dest_node.
            void remove_edge(graph_id source_node, graph_id dest_node);

            // Return whether node exists in the graph.
            bool has_node(graph_id node);

            // Return whether there is an edge directly from source_node to dest_node.
            bool has_edge(graph_id source_node, graph_id dest_node) const;

            // Return whether dest_node is reachable from source_node
            // by following edges.
            bool is_reachable(graph_id source_node, graph_id dest_node) const;

            // Find a path from "source" to "dest".  If such a path exists,
            // place the nodes on the path in the array path[], and return
            // the number of nodes on the path.  If the path is longer than
            // max_path_len nodes, only the first max_path_len nodes are placed
            // in path[].  The client should compare the return value with
            // max_path_len" to see when this occurs.  If no path exists, return
            // 0.  Any valid path stored in path[] will start with "source" and
            // end with "dest".  There is no guarantee that the path is the
            // shortest, but no node will appear twice in the path, except the
            // source and destination node if they are identical; therefore, the
            // return value is at most one greater than the number of nodes in
            // the graph.
            int find_path(graph_id source, graph_id dest, int max_path_len,
                          graph_id path[]) const;

            // Update the stack trace recorded for id with the current stack
            // trace if the last time it was updated had a smaller priority
            // than the priority passed on this call.
            //
            // *get_stack_trace is called to get the stack trace.
            void update_stack_trace(graph_id id, int priority,
                                    int (*get_stack_trace)(void **, int));

            // Set *ptr to the beginning of the array that holds the recorded
            // stack trace for id and return the depth of the stack trace.
            int get_stack_trace(graph_id id, void ***ptr);

            // Check internal invariants. Crashes on failure, returns true on success.
            // Expensive: should only be called from graphcycles_test.cc.
            bool check_invariants() const;

            // ----------------------------------------------------
            struct Rep;
        private:
            Rep *rep_;      // opaque representation
            graph_cycles(const graph_cycles &) = delete;

            graph_cycles &operator=(const graph_cycles &) = delete;
        };

    }  // namespace thread_internal

}  // namespace abel

#endif
