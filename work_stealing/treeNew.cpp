
void swap(int* arr, int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

#define LOCAL_STEAL_RETRIES 1

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
            for (int i=1; i<num_cores_per_node; i++) {
                swap(perm, i, random()%(num_cores_per_node-i)+i);

                int victim_node = perm[i];

                if (CWSD_OK == local_wsqs[victim_node].steal(&work)) {
                    gotWork = true;
                    break;
                }
            }

            if (!gotWork) {




        

