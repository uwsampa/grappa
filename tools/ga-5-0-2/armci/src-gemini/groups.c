#if HAVE_CONFIG_H
#   include "config.h"
#endif

/* $Id: groups.c,v 1.4.6.2 2007-08-15 08:37:16 manoj Exp $ */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef MPI
#  define MPI
#endif
#include "armcip.h"
#include "message.h"

#define DEBUG_ 0

ARMCI_iGroup ARMCI_Default_Proc_Group;
ARMCI_iGroup ARMCI_World_Proc_Group;

#ifdef ARMCI_GROUP
void ARMCI_Bcast_(void *buffer, int len, int root, ARMCI_Group *group);
#else
void ARMCI_Bcast_(void *buffer, int len, int root, ARMCI_Comm comm);
#endif
void ARMCI_Group_create(int n, int *pid_list, ARMCI_Group *group);
int  ARMCI_Group_rank(ARMCI_Group *group, int *rank);
void ARMCI_Group_size(ARMCI_Group *group, int *size);

static void get_group_clus_id(ARMCI_iGroup *igroup, int grp_nproc, 
                              int *grp_clus_id) {
    int i, *ranks1, *ranks2;
#ifdef ARMCI_GROUP
    assert(grp_nproc<=igroup->grp_attr.nproc);
    for(i=0; i<grp_nproc; i++) {
      grp_clus_id[i] = armci_clus_id(igroup->grp_attr.proc_list[i]);
    }
#else
    MPI_Group group2;
    
    /* Takes the list of processes from one group and attempts to determine
     * the corresponding ranks in a second group (here, MPI_COMM_WORLD) */

    ranks1 = (int *)malloc(2*grp_nproc*sizeof(int));
    ranks2 = ranks1 + grp_nproc;
    for(i=0; i<grp_nproc; i++) ranks1[i] = i;
    MPI_Comm_group(MPI_COMM_WORLD, &group2);
    MPI_Group_translate_ranks(igroup->igroup, grp_nproc, ranks1, group2, ranks2);
    
    /* get the clus_id of processes */
    for(i=0; i<grp_nproc; i++) grp_clus_id[i] = armci_clus_id(ranks2[i]);
    free(ranks1);
#endif
}

/**
 * Construct the armci_clus_t arrays of structs for a given group. 
 *
 * @param grp_nclus_nodes OUT #clus_info objects returned
 * @param igroup IN Group whose clus_info needs to be constructed
 * @return array of armci_clus_t objects
 */
static armci_clus_t* group_construct_clusinfo(int *grp_nclus_nodes, ARMCI_iGroup *igroup) {
  armci_clus_t *grp_clus_info=NULL;
  int i, *grp_clus_id, cluster, clus_id, grp_nproc, grp_nclus;

  ARMCI_Group_size((ARMCI_Group *)igroup, &grp_nproc);

  /* get the cluster_id of processes in the group */
  grp_clus_id = (int *)malloc(grp_nproc*sizeof(int));
  get_group_clus_id(igroup, grp_nproc, grp_clus_id);
       
  /* first find out how many cluster nodes we got for this group */
  grp_nclus=1;
  for(i=0; i<grp_nproc-1; i++) {
    if(grp_clus_id[i] != grp_clus_id[i+1]) ++grp_nclus;
  }
  *grp_nclus_nodes = grp_nclus;
  grp_clus_info = (armci_clus_t*)malloc(grp_nclus*sizeof(armci_clus_t));
  if(!grp_clus_info)armci_die("malloc failed for grp_clusinfo",grp_nclus);

  cluster = 1;
  clus_id = grp_clus_id[0];
  grp_clus_info[0].nslave = 1;
  grp_clus_info[0].master = 0;
  strcpy(grp_clus_info[0].hostname, armci_clus_info[clus_id].hostname);

  for(i=1; i<grp_nproc; i++) {
    if(grp_clus_id[i-1] == grp_clus_id[i]) 
      ++grp_clus_info[cluster-1].nslave;
    else {
      clus_id = grp_clus_id[i];
      grp_clus_info[cluster].nslave = 1;
      grp_clus_info[cluster].master = i;
      strcpy(grp_clus_info[cluster].hostname, 
	     armci_clus_info[clus_id].hostname);
      ++cluster;
    }
  }

  free(grp_clus_id);
  if(grp_nclus != cluster)
    armci_die("inconsistency processing group clusterinfo", grp_nclus);

#   if DEBUG_
  {
    int i,j;
    for(i=0; i<cluster;i++) {
      printf("%d: Cluster %d: Master=%d, #Slaves=%d, HostName=%s\n", 
	     grp_nclus, i, grp_clus_info[i].master, 
	     grp_clus_info[i].nslave, grp_clus_info[i].hostname);
      fflush(stdout);
    }
  }
#   endif  
  return grp_clus_info;
}

/**
 * Group cluster information "grp_clus_info" (similar to armci_clus_info)
 */
static void group_process_list(ARMCI_Group *group, 
                               armci_grp_attr_t *grp_attr) {
    ARMCI_iGroup *igroup = (ARMCI_iGroup *)group;
#ifndef ARMCI_GROUP
    ARMCI_Comm comm = igroup->icomm;
#endif

    int grp_me, grp_nproc, grp_nclus, *grp_clus_id, grp_clus_me;
    armci_clus_t *grp_clus_info=NULL;
#ifdef CLUSTER
    int i, len, clus_id, cluster=0, root=0;
#endif
    
#ifndef ARMCI_GROUP
    if(comm==MPI_COMM_NULL || igroup->igroup==MPI_GROUP_NULL) 
       armci_die("group_process_list: NULL COMMUNICATOR",0);
#endif
    
    ARMCI_Group_rank(group, &grp_me);
    ARMCI_Group_size(group, &grp_nproc);
    
#ifdef CLUSTER
#ifdef ARMCI_GROUP
    /*all processes construct the clus_info structure in parallel*/
    grp_clus_info = group_construct_clusinfo(&grp_nclus, igroup);
#else
    /* process 0 gets group cluster information: grp_nclus, grp_clus_info */
    if(grp_me == 0) {
      grp_clus_info = group_construct_clusinfo(&grp_nclus, igroup);
    }

    /* process 0 broadcasts group cluster information */
    len = sizeof(int);
    ARMCI_Bcast_(&grp_nclus, len, root, comm);
    
    if(grp_me){
       /* allocate memory */
       grp_clus_info = (armci_clus_t*)malloc(grp_nclus*sizeof(armci_clus_t));
       if(!armci_clus_info)armci_die("malloc failed for clusinfo",armci_nclus);
    }
    
    len = sizeof(armci_clus_t)*grp_nclus;
    ARMCI_Bcast_(grp_clus_info, len, root, comm);
#endif
    /* determine current group cluster node id by comparing me to master */
    grp_clus_me =  grp_nclus-1;
    for(i =0; i< grp_nclus-1; i++) {
       if(grp_me < grp_clus_info[i+1].master){
          grp_clus_me=i;
          break;
       }
    }

#else
    grp_clus_me = 0;
    grp_nclus = 1;
    grp_clus_info = (armci_clus_t*)malloc(grp_nclus*sizeof(armci_clus_t));
    if(!grp_clus_info)armci_die("malloc failed for clusinfo",grp_nclus);
    strcpy(grp_clus_info[0].hostname, armci_clus_info[0].hostname);
    grp_clus_info[0].master=0;
    grp_clus_info[0].nslave=grp_nproc;
#endif
#ifdef ARMCI_GROUP
    /*Set in ARMCI_Group_create. ARMCI_Group_rank is used before
      setting this field. So moving it there in the generic
      implementation.*/
#else
    grp_attr->grp_me        = grp_me;
#endif
    grp_attr->grp_clus_info = grp_clus_info;
    grp_attr->grp_nclus     = grp_nclus;
    grp_attr->grp_clus_me   = grp_clus_me;
}

/* attribute caching: group_cluster_information and memory_offset should 
   be cached in group data structure */
static void armci_cache_attr(ARMCI_Group *group) {
    armci_grp_attr_t *grp_attr;
    ARMCI_iGroup *igroup = (ARMCI_iGroup *)group;

    /* allocate storage for the attribute content. Note: Attribute contents 
       should be stored in persistent memory */
    grp_attr = &(igroup->grp_attr); 
    
    /* get group cluster information and  grp_attr */
    group_process_list(group, grp_attr);
}

armci_grp_attr_t *ARMCI_Group_getattr(ARMCI_Group *group)
{
    ARMCI_iGroup *igroup = (ARMCI_iGroup *)group;
    return(&(igroup->grp_attr));

}

#ifdef ARMCI_GROUP
void ARMCI_Bcast_(void *buffer, int len, int root, ARMCI_Group *group) {
  armci_msg_group_bcast_scope(SCOPE_ALL, buffer, len, 
			      ARMCI_Absolute_id(group, root), 
			      group);
}
#else
void ARMCI_Bcast_(void *buffer, int len, int root, ARMCI_Comm comm) {
    int result;
    MPI_Comm_compare(comm, MPI_COMM_WORLD, &result);
    if(result == MPI_IDENT)  armci_msg_brdcst(buffer, len, root); 
    else MPI_Bcast(buffer, len, MPI_BYTE, root, (MPI_Comm)comm);
}
#endif

void ARMCI_Group_free(ARMCI_Group *group) {

    int rv;
    
    ARMCI_iGroup *igroup = (ARMCI_iGroup *)group;
    
    free(igroup->grp_attr.grp_clus_info);
#ifdef ARMCI_GROUP
    free(igroup->grp_attr.proc_list);
    igroup->grp_attr.nproc = 0;
#else

    rv=MPI_Group_free(&(igroup->igroup));
    if(rv != MPI_SUCCESS) armci_die("MPI_Group_free: Failed ",armci_me);
    
    rv = MPI_Comm_free( (MPI_Comm*)&(igroup->icomm) );
    if(rv != MPI_SUCCESS) armci_die("MPI_Comm_free: Failed ",armci_me);
#endif
}

/*
  Create a child group for to the given group.
  @param n IN #procs in this group (<= that in group_parent)
  @param pid_list IN The list of proc ids (w.r.t. group_parent)
  @param group_out OUT Handle to store the created group
  @param group_parent IN Parent group 
 */
void ARMCI_Group_create_child(int n, int *pid_list, ARMCI_Group *group_out,
			      ARMCI_Group *grp_parent) {
    int i,grp_me, world_me;
    int rv;
    
    ARMCI_iGroup *igroup = (ARMCI_iGroup *)group_out;
    ARMCI_iGroup *igroup_parent = (ARMCI_iGroup *)grp_parent;
    armci_grp_attr_t *grp_attr = &igroup->grp_attr;
#ifndef ARMCI_GROUP
    MPI_Group *group_parent;
    MPI_Comm *comm_parent;
#endif
    
    for(i=0; i<n-1;i++) {
       if(pid_list[i] > pid_list[i+1]){
         armci_die("ARMCI_Group_create: Process ids are not sorted ",armci_me);
         break;
       }
    }
    
#ifdef ARMCI_GROUP
    grp_attr->grp_clus_info = NULL;
    grp_attr->nproc = n;
    grp_attr->proc_list = (int *)malloc(n*sizeof(int));
    assert(grp_attr->proc_list!=NULL);
    for(i=0; i<n; i++)  {
      grp_attr->proc_list[i] = ARMCI_Absolute_id(grp_parent,pid_list[i]); 
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &world_me);
    grp_attr->grp_me = grp_me = MPI_UNDEFINED;
    for(i=0; i<n; i++) {
      if(igroup->grp_attr.proc_list[i] == world_me) {
	grp_attr->grp_me = grp_me = i;
	break;
      }
    }
    if(grp_me != MPI_UNDEFINED) armci_cache_attr(group_out);
      
#else
    /* NOTE: default group is the parent group */
    group_parent = &(igroup_parent->igroup);
    comm_parent  = &(igroup_parent->icomm);

    rv=MPI_Group_incl(*group_parent, n, pid_list, &(igroup->igroup));
    if(rv != MPI_SUCCESS) armci_die("MPI_Group_incl: Failed ",armci_me);
    
    rv = MPI_Comm_create(*comm_parent, (MPI_Group)(igroup->igroup), 
                         (MPI_Comm*)&(igroup->icomm));
    if(rv != MPI_SUCCESS) armci_die("MPI_Comm_create: Failed ",armci_me);
    
    /* processes belong to this group should cache attributes */
    MPI_Group_rank((MPI_Group)(igroup->igroup), &grp_me);
    if(grp_me != MPI_UNDEFINED) armci_cache_attr(group_out);
#endif
}

void ARMCI_Group_create(int n, int *pid_list, ARMCI_Group *group_out) {
  ARMCI_Group_create_child(n, pid_list, group_out, (ARMCI_Group *)&ARMCI_Default_Proc_Group);
}

int ARMCI_Group_rank(ARMCI_Group *group, int *rank) {
    ARMCI_iGroup *igroup = (ARMCI_iGroup *)group;
#ifdef ARMCI_GROUP
    if(!igroup) return MPI_ERR_GROUP;
    *rank = igroup->grp_attr.grp_me;
    return MPI_SUCCESS;
#else
    return MPI_Group_rank((MPI_Group)(igroup->igroup), rank);
#endif
}

void ARMCI_Group_size(ARMCI_Group *group, int *size) {
    ARMCI_iGroup *igroup = (ARMCI_iGroup *)group;
#ifdef ARMCI_GROUP
    *size = igroup->grp_attr.nproc;
#else
    MPI_Group_size((MPI_Group)(igroup->igroup), size);
#endif
}

int ARMCI_Absolute_id(ARMCI_Group *group,int group_rank)
{
    int abs_rank,status;
    ARMCI_iGroup *igroup = (ARMCI_iGroup *)group;
#ifdef ARMCI_GROUP
    assert(group_rank < igroup->grp_attr.nproc);
    return igroup->grp_attr.proc_list[group_rank];
#else
    MPI_Group grp;
    status = MPI_Comm_group(MPI_COMM_WORLD,&grp);
    MPI_Group_translate_ranks(igroup->igroup,1,&group_rank,grp,&abs_rank);
    return(abs_rank);
#endif
}

void ARMCI_Group_set_default(ARMCI_Group *group) 
{
    ARMCI_iGroup *igroup = (ARMCI_iGroup *)group;
    ARMCI_Default_Proc_Group = *igroup;
}

void ARMCI_Group_get_default(ARMCI_Group *group_out)
{
    ARMCI_iGroup *igroup = (ARMCI_iGroup *)group_out;
    *igroup = ARMCI_Default_Proc_Group;
}

void ARMCI_Group_get_world(ARMCI_Group *group_out)
{
    ARMCI_iGroup *igroup = (ARMCI_iGroup *)group_out;
    *igroup = ARMCI_World_Proc_Group;
}

void armci_group_init() 
{
    int grp_me, i;
    ARMCI_iGroup *igroup = (ARMCI_iGroup *)&ARMCI_World_Proc_Group;

#ifdef ARMCI_GROUP
    /*setup the world proc group*/
    MPI_Comm_size(MPI_COMM_WORLD, &igroup->grp_attr.nproc); 
    MPI_Comm_rank(MPI_COMM_WORLD, &igroup->grp_attr.grp_me); 
    igroup->grp_attr.proc_list = (int *)malloc(igroup->grp_attr.nproc*sizeof(int));
    assert(igroup->grp_attr.proc_list != NULL);
    for(i=0; i<igroup->grp_attr.nproc; i++) {
      igroup->grp_attr.proc_list[i] = i;
    } 
    igroup->grp_attr.grp_clus_info = NULL;
    armci_cache_attr((ARMCI_Group*)&ARMCI_World_Proc_Group);
#else
    /* save MPI world group and communicatior in ARMCI_World_Proc_Group */
    igroup->icomm = MPI_COMM_WORLD;
    MPI_Comm_group(MPI_COMM_WORLD, &(igroup->igroup));

    /* processes belong to this group should cache attributes */
    MPI_Group_rank((MPI_Group)(igroup->igroup), &grp_me);
    if(grp_me != MPI_UNDEFINED) 
    {
       armci_cache_attr((ARMCI_Group*)&ARMCI_World_Proc_Group);
    }
#endif    

    /* Initially, World group is the default group */
    ARMCI_Default_Proc_Group = ARMCI_World_Proc_Group;
}

void armci_group_finalize() {
#ifdef ARMCI_GROUP
  ARMCI_Group_free((ARMCI_Group *)&ARMCI_World_Proc_Group);
#endif
}

/*
  ISSUES:
  1. Make sure ARMCI_Group_free frees the attribute data structures 
  2. replace malloc with, kr_malloc using local_context.
*/
