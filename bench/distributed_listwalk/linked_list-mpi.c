
#include <mpi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>

#define DEBUG 0

//typedef uint64_t int_t;
typedef int int_t;

struct msg_t {
  int_t dest;
  int_t val;
};


typedef struct vertex_t {
  int_t id;
  int_t source;
  int_t next;
} vertex_t;

int compare_vertices_by_id( const vertex_t* a, const vertex_t* b ) {
  if (a->id > b->id) {
    return 1;
  } else if (a->id < b->id) {
    return -1;
  } else {
    return 0;
  }
}

int compare_vertices_by_next( const vertex_t* a, const vertex_t* b ) {
  if (a->next > b->next) {
    return 1;
  } else if (a->next < b->next) {
    return -1;
  } else {
    return 0;
  }
}

vertex_t* local_vertices;

int main( int argc, char* argv[] ) {

  int i;
  int myrank, numprocs;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs); 

  //srand ( time(NULL) + myrank * numprocs );
  srand ( myrank * numprocs );

  const int_t local_vertex_count = (1 << 3);
  const int_t total_vertex_count = local_vertex_count * numprocs;
  const int_t num_lists = numprocs;

  //if (DEBUG) printf("Hello from vertex %d of %d.\n", myrank+1, numprocs);
  local_vertices = (vertex_t*) malloc( sizeof(vertex_t) * local_vertex_count );

  int bufsize = sizeof(char) * sizeof(struct vertex_t);

  for( i = 0; i < local_vertex_count; ++i ) {
    local_vertices[i].id = local_vertex_count * myrank + i;
    local_vertices[i].next = rand() % numprocs;
  }
  const int shuffle_tag = 1234;

  // print some stuff
  if (DEBUG) {
    for (int i = 0; i < numprocs; ++i) {
      if (i == myrank) {
	for (int j = 0; j < local_vertex_count; ++j) {
	  if (DEBUG) printf("proc %d id %d next %d\n", i, local_vertices[j].id, local_vertices[j].next);
	}
      }
      //sleep(1);
      MPI_Barrier( MPI_COMM_WORLD );
    }

    sleep(1);
    if (myrank == 0) {
      if (DEBUG) printf("-----\n");
    }
    MPI_Barrier( MPI_COMM_WORLD );
  }

  if (1 || DEBUG) {
    for (int i = 0; i < numprocs; ++i) {
      if (i == myrank) {
	for (int j = 0; j < local_vertex_count; ++j) {
	  if (1 || DEBUG) printf("id 0%3d: proc %3d. %3d: id %3d next %3d %3d //// %3d: %3d\n", local_vertices[j].id, i, j + myrank * local_vertex_count, local_vertices[j].id, local_vertices[j].next, j + myrank * local_vertex_count, j + myrank * local_vertex_count, 0);
	}
      }
      //sleep(1);
      MPI_Barrier( MPI_COMM_WORLD );
    }
  }
  MPI_Barrier( MPI_COMM_WORLD );

  int* locs = malloc( sizeof(int) * local_vertex_count );
  if (1) {
    int* transfer_counts = malloc( sizeof(int) * numprocs );
    int* destinations = malloc( sizeof(int) * local_vertex_count );
    int* values = malloc( sizeof(int) * local_vertex_count );

    // each processor shuffles its vertices
    for (int i = numprocs-1; i >= 0; i--) {
      if (i == myrank) {

	if (DEBUG) printf("a: processor %d's turn\n", i);

	// this is a two-step process. 
	//   first, notify each processor of how many messages it will receive
	//   second, swap vertices

	// initialize destination and transfer count arrays
	for (int j = 0; j < numprocs; ++j) {
	  transfer_counts[j] = 0;
	}
	for (int j = 0; j < local_vertex_count; ++j) {
	  destinations[j] = 0;
	}

	// compute shuffle destinations
	for (int j = myrank * local_vertex_count + local_vertex_count-1; j >= myrank * local_vertex_count; --j) {
	  if (j != 0) {
	    int_t k = random() % j;
	    destinations[j % local_vertex_count] = k;
	    int_t dest_node = k / local_vertex_count;
	    if (DEBUG) printf("b: swapping %d with %d on node %d\n", j, k, dest_node);
	    transfer_counts[dest_node]++;
	  }
	}

	// send transfer counts
	for (int j = 0; j < numprocs; ++j) {
	  if (j != myrank) {
	    if (DEBUG) printf("c: node %d will receive %d\n", j, transfer_counts[j]);
	    MPI_Send( &transfer_counts[j], 1, MPI_INT, j, shuffle_tag, MPI_COMM_WORLD );
	  }
	}

	// do the swaps
	for (int j = myrank * local_vertex_count + local_vertex_count-1; j >= myrank * local_vertex_count; --j) {
	  if (DEBUG) printf("e%da: checking on id %d\n", j, j);
	  int_t destination = destinations[j % local_vertex_count];
	  int_t dest_node = destination / local_vertex_count;

	  if (j != destination) {
	    if (dest_node == myrank) {
	      if (DEBUG) printf("e%db: swapping id %d with id %d locally\n", j, j, destination);
	      vertex_t temp = local_vertices[j % local_vertex_count];
	      local_vertices[j % local_vertex_count] = local_vertices[destination % local_vertex_count];
	      local_vertices[destination % local_vertex_count] = temp;
	    } else {
	      if (DEBUG) printf("e%db: node %d sending id %d to node %d\n", j, myrank, j, dest_node);
	      local_vertices[j % local_vertex_count].source = j;
	      local_vertices[j % local_vertex_count].next = destination;
	      MPI_Send( &local_vertices[j % local_vertex_count], sizeof(vertex_t), MPI_CHAR, 
			dest_node, shuffle_tag, MPI_COMM_WORLD );
	      MPI_Status status;
	      MPI_Recv( &local_vertices[j % local_vertex_count], sizeof(vertex_t), MPI_CHAR, 
			MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
	      if (DEBUG) printf("e%de: node %d got id %d from node %d\n", 
				j, myrank, local_vertices[j % local_vertex_count].source, dest_node);
	    }
	  } else { 
	    if (DEBUG) printf("e%db: skipping id %d since it has no work to do\n", j, j);
	  }
	  if (DEBUG) printf("e%df: finished considering id %d\n", j, j);
	}
	
	if (DEBUG) printf("f: processor %d's turn is done\n", myrank);

      } else {
	//sleep(1);
	int transfer_count = 0;
	MPI_Status status;
	MPI_Recv( &transfer_count, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
	if (DEBUG) printf("d: node %d: waiting for %d\n", myrank, transfer_count);

	while (transfer_count--) {
	  vertex_t vertex;
	  MPI_Status status;
	  MPI_Recv( &vertex, sizeof(vertex_t), MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
	  int_t source = vertex.source;
	  int_t destination = vertex.next % local_vertex_count;
	  int_t source_node = vertex.source / local_vertex_count;
	  local_vertices[destination].source = vertex.next;
	  if (DEBUG) printf("e%dc: node %d got id %d from node %d\n", source, myrank, source, source_node);
	  if (DEBUG) printf("e%dd: node %d sending id %d to node %d\n", source, myrank, vertex.next, source_node);
	  MPI_Send( &local_vertices[destination], sizeof(vertex_t), MPI_CHAR, source_node, shuffle_tag, MPI_COMM_WORLD );
	  local_vertices[destination] = vertex;
	}
      }

      MPI_Barrier( MPI_COMM_WORLD );
      if (DEBUG) {
	sleep(1);
	if (myrank == 0) {
	  if (DEBUG) printf("-----\n");
	}
	MPI_Barrier( MPI_COMM_WORLD );
      }
    }

  if (1 || DEBUG) {
    for (int i = 0; i < numprocs; ++i) {
      if (i == myrank) {
	for (int j = 0; j < local_vertex_count; ++j) {
	  if (1 || DEBUG) printf("id 1%3d: proc %3d. %3d: id %3d next %3d %3d //// %3d: %3d\n", local_vertices[j].id, i, j + myrank * local_vertex_count, local_vertices[j].id, local_vertices[j].next, j + myrank * local_vertex_count, j + myrank * local_vertex_count, 0);
	}
      }
      //sleep(1);
      MPI_Barrier( MPI_COMM_WORLD );
    }
  }
  MPI_Barrier( MPI_COMM_WORLD );

    // extract locs
    for (int j = myrank * local_vertex_count + local_vertex_count-1; j >= myrank * local_vertex_count; --j) {
      struct msg_t send_msg;
      send_msg.dest = local_vertices[j % local_vertex_count].id;
      int dest_node = send_msg.dest / local_vertex_count;
      send_msg.val = j;
      MPI_Request send_req;
      MPI_Isend( &send_msg, sizeof(struct msg_t), MPI_CHAR, dest_node, shuffle_tag, MPI_COMM_WORLD, &send_req );

      struct msg_t recv_msg;
      MPI_Status recv_status;
      MPI_Recv( &recv_msg, sizeof(struct msg_t), MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &recv_status );
      locs[recv_msg.dest % local_vertex_count] = recv_msg.val;

      MPI_Status send_status;
      MPI_Wait(&send_req, &send_status);
    }

    MPI_Barrier( MPI_COMM_WORLD );

    //
    // initialize pointers
    //
    for (int j = myrank * local_vertex_count + local_vertex_count-1; j >= myrank * local_vertex_count; --j) {
      struct msg_t send_msg;
	
      int id = j;
      int current = locs[j % local_vertex_count];
      int thread_size = total_vertex_count / num_lists;
      int base = id / thread_size;
      int nextid = id + 1;
      int wrapped_nextid = ((nextid % thread_size) + (base * thread_size)) % total_vertex_count;
      send_msg.dest = wrapped_nextid;
      send_msg.val = current;
      int dest_node = send_msg.dest / local_vertex_count;
      if (DEBUG) printf("vertex %d sending %d to vertex  %d\n", id, send_msg.val, send_msg.dest);
      MPI_Request send_req;
      MPI_Isend( &send_msg, sizeof(struct msg_t), MPI_CHAR, dest_node, shuffle_tag, MPI_COMM_WORLD, &send_req );

      struct msg_t recv_msg;
      MPI_Status recv_status;
      MPI_Recv( &recv_msg, sizeof(struct msg_t), MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &recv_status );
      if (DEBUG) printf("vertex %d received %d at id %d\n", recv_msg.dest, recv_msg.val, j);
      int wrapped_nextid_loc = locs[recv_msg.dest % local_vertex_count];
      destinations[recv_msg.dest % local_vertex_count] = recv_msg.val;
      //local_vertices[recv_msg.dest % local_vertex_count].next = wrapped_nextid_loc;
      values[recv_msg.dest % local_vertex_count] = wrapped_nextid_loc;

      MPI_Status send_status;
      MPI_Wait(&send_req, &send_status);
    }


  if (1 || DEBUG) {
    for (int i = 0; i < numprocs; ++i) {
      if (i == myrank) {
	for (int j = 0; j < local_vertex_count; ++j) {
	  if (1 || DEBUG) printf("id 7%3d: proc %3d. %3d: id %3d next %3d %3d //// %3d: %3d\n", local_vertices[j].id, i, j + myrank * local_vertex_count, local_vertices[j].id, local_vertices[j].next, j + myrank * local_vertex_count, j + myrank * local_vertex_count, 0);
	}
      }
      //sleep(1);
      MPI_Barrier( MPI_COMM_WORLD );
    }
  }
  MPI_Barrier( MPI_COMM_WORLD );

    // necessary?
    MPI_Barrier( MPI_COMM_WORLD );
    if (DEBUG) {
      sleep(1);
      if (myrank == 0) {
	if (DEBUG) printf("-----\n");
      }
      MPI_Barrier( MPI_COMM_WORLD );
    }

    // move next pointer to node and store it
    for (int j = myrank * local_vertex_count + local_vertex_count-1; j >= myrank * local_vertex_count; --j) {
      struct msg_t send_msg;
      send_msg.dest = destinations[j % local_vertex_count];
      //send_msg.val = local_vertices[j % local_vertex_count].next;
      send_msg.val = values[j % local_vertex_count];
      int dest_node = send_msg.dest / local_vertex_count;
      if (DEBUG) printf("vertex %d sending %d to vertex  %d\n", j, send_msg.val, send_msg.dest);

      MPI_Request send_req;
      MPI_Isend( &send_msg, sizeof(struct msg_t), MPI_CHAR, dest_node, shuffle_tag, MPI_COMM_WORLD, &send_req );
      
      // receive a message
      struct msg_t recv_msg;
      MPI_Status recv_status;
      MPI_Recv( &recv_msg, sizeof(struct msg_t), MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &recv_status );
      if (DEBUG) printf("vertex %d received %d at id %d\n", recv_msg.dest, recv_msg.val, j);

      // update 
      local_vertices[recv_msg.dest % local_vertex_count].next = recv_msg.val;

      // block until message is sent
      MPI_Status send_status;
      MPI_Wait(&send_req, &send_status);
    }

      MPI_Barrier( MPI_COMM_WORLD );
    if (DEBUG) {
      sleep(1);
      if (myrank == 0) {
	if (DEBUG) printf("-----\n");
      }
      MPI_Barrier( MPI_COMM_WORLD );
    }

    free(transfer_counts);
    free(destinations);
    free(values);
  }

  if (1 || DEBUG) {
    for (int i = 0; i < numprocs; ++i) {
      if (i == myrank) {
	for (int j = 0; j < local_vertex_count; ++j) {
	  if (1 || DEBUG) printf("id 9%3d: proc %3d. %3d: id %3d next %3d %3d //// %3d: %3d\n", local_vertices[j].id, i, j + myrank * local_vertex_count, local_vertices[j].id, local_vertices[j].next, j + myrank * local_vertex_count, j + myrank * local_vertex_count, locs[j]);
	}
      }
      //sleep(1);
      MPI_Barrier( MPI_COMM_WORLD );
    }
  }

  free(locs);
  free(local_vertices);
  MPI_Finalize();
  return 0;
}
