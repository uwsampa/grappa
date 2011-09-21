
void swap(int* arr, int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

#define LOCAL_STEAL_RETRIES 1
#define MAX_STEALS_IN_FLIGHT 1

void run(int core_index, int num_threads, global_array* ibt, int myNode, int num_cores_per_node, int num_nodes) {
    
    while (1) {
        gasnet_AMPoll();

        bool gotWork = false;
        TreeNode_ptr work;

        if (CWSD_OK == local_wsqs[core_index].popBottom(&work)) {
            gotWork = true;
        } else {
            int perm[num_cores_per_node];
            for (int i=0; i<num_cores_per_node; i++) {
                perm[i] = i;
            }

            // exlcude self for local stealing
            swap(perm, core_index, 0);
        
            // try to steal locally in the random order
            // TODO: should a remote steal be started first if non in progress?
            for (int i=1; i<num_cores_per_node; i++) {
                swap(perm, i, random()%(num_cores_per_node-i)+i);

                int victim_queue = perm[i];

                if (CWSD_OK == local_wsqs[victim_queue].steal(&work)) {
                    gotWork = true;
                    break;
                }
            }

            if (!gotWork) {

                // steal remote
                if (num_steals_in_flight < MAX_STEALS_IN_FLIGHT) {
                    int victim_node = random()%num_nodes;
                    
                    //printf("p%d: sending steal request to p%d queue%d\n", myNode, victim_node, i % num_cores_per_node);
                    gasnet_AMRequestShort3 (victim_node, WORKSTEAL_REQUEST_HANDLER, i % num_cores_per_node, core_index, current_data->index);
                    
                }





        

