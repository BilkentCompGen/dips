#include "clique.h"
#include "cluster.h"
#include <assert.h>
#define INITIAL_CLIQUE_COUNT 12


int clique_plateau_move( clique_t *c, graph_t *sv_graph, adjlist_t *al, int TABU){
	double max;
	int added, removed;
	double score;
	sv_t *best_sv = NULL;
	size_t best_index = -1;
	max = INT_MIN;
	int i;
	added = 1;
	removed = 1;
	sv_t *tmp;
	sv_t **etmp;
//	printf("\rClique Size: %8zu \tGraph # of items: %8zu\tIteration %9d",c->items->size,sv_graph->number_of_items,ccc++);

	//ADD
	for(i=0;i< al->size;i++){
		tmp = al_get_value(al,i);
		if(tmp->inactive){continue;}
		if( tmp->tabu > 0){
			tmp->tabu--;
		}
		if( tmp->tabu == 0 && tmp->dv > c->gamma * (c->v_prime - 1) &&
			(c->e_prime + tmp->dv >= (c->lambda * (c->v_prime * (c->v_prime-1)))/2.0)){
			score = (2.0* (c->e_prime + tmp->dv)) / (c->v_prime * (c->v_prime +1));
			if(max < score){
				best_sv = tmp;
				max = score;
				best_index = i;
			}
		}
	}
	if( max == INT_MIN){ added = 0;}
	else{
		clique_add_node(c,sv_graph, best_sv);

		best_sv->inactive = 1;
		vector_t *edges = al_get_edges(al,best_index);
		for(i=0;i<edges->size;i++){
			etmp = vector_get(edges,i);
			(*etmp)->dv++;
		}
		best_sv->tabu = TABU;
	}

	//REMOVE
	max = INT_MIN;
	for(i=0;i< c->items->size;i++){
		tmp = *(sv_t **)vector_get(c->items,i);
		if( tmp->tabu > 0){
			tmp->tabu--;
		}

		if( tmp->tabu == 0 && tmp->dv < c->gamma * (c->v_prime - 1) &&
			(c->e_prime - tmp->dv > (c->lambda * ((c->v_prime-2) * (c->v_prime-1)))/2.0)){
			score = (2.0* (c->e_prime - tmp->dv)) / ((c->v_prime-1) * (c->v_prime -2));
			if(max < score){
				best_sv = tmp;
				max = score;
				best_index = i;
			}
		}
	}
	//MAKE RE-edging faster, efficient

	if(max==INT_MIN){removed = 0;}
	else{
		clique_rem_node(c,sv_graph, best_sv);
		best_sv->tabu = TABU;
		best_sv->inactive = 0;
		vector_t *rem_edges = graph_get_edges(sv_graph,best_sv);
		for(i=0;i<rem_edges->size;i++){
			sv_t **_tmp = vector_get(rem_edges,i);
			(*_tmp)->dv--;
		}
	}

	return added||removed;
}

int add_entr,rem_entr,add_ext,rem_ext;
clique_t *clique_find_clique(graph_t *sv_graph,int seed, float lambda, float gamma){

	//Sort nodes with degree
	//qsort(sv_graph->nodes->items, sv_graph->nodes->size, sizeof(void *),graph_node_cmp);
	adjlist_t *adjg = graph_to_al(sv_graph);
	al_sortbydegree(adjg);
	sv_t *initial = al_get_value(adjg,0);	

	//graph_node *initial = vector_head(sv_graph->nodes);
	clique_t *clique = clique_init(lambda,gamma,initial);
	if(clique==NULL){ 
//		fprintf(stderr,"Null Clique");
		return NULL;
	}


	vector_t *edges = graph_get_edges(sv_graph,initial);
	if(edges ==NULL){
//		fprintf(stderr,"Null edges");
		return clique;
	}
	if(edges->size==0){
//		fprintf(stderr,"No edges\n");
		return clique;
	}

	int i;
	for(i=0;i<edges->size;i++){
		sv_t **temp = vector_get(edges,i);
		(*temp)->dv = 1;
	}
	initial->inactive = 1;


//	vector_remove(clique->items,0);
//	printf("\n");
	while(clique_plateau_move(clique,sv_graph,adjg,3+adjg->size/1000));
//	fprintf(stderr, "Add entry %d, Add exit %d\nRem entry %d, Rem exit %d\n",add_entr,add_ext,rem_entr,rem_ext);
//	printf("\n");

	vector_free(adjg);
	return clique;
}


//TODO test methods
clique_t *clique_init(float lambda, float gamma, sv_t *node){
	if( node==NULL){ return NULL;}
	clique_t *new_clique = getMem(sizeof(clique_t));
	new_clique->items = vector_init(sizeof(sv_t*),INITIAL_CLIQUE_COUNT);
	new_clique->e_prime = 0;
	new_clique->v_prime = 1;
	new_clique->lambda = lambda;
	new_clique->gamma = gamma;
	vector_put(new_clique->items,&node);

	add_entr = 0;
	rem_entr = 0;
	add_ext = 0;
	rem_ext = 0;
	return new_clique;
}

void clique_free(clique_t *c){
	if(c==NULL){return;}
	vector_free(c->items);
	freeMem(c,sizeof(c));
}
int clique_add_node(clique_t *c, graph_t *g, sv_t *node){
	add_entr++;
	if(!graph_have_node(g,node)){return -1;}
	vector_t *edges = graph_get_edges(g,node);
	vector_put(c->items,&node);
	c->v_prime++;
	int i;
	sv_t **adj_item;
	for(i=0;i<edges->size;i++){
		adj_item = vector_get(edges,i);
		if(vector_contains(c->items,adj_item)!=-1){
			c->e_prime++;
		}
	}
	add_ext++;
	return c->e_prime;
}
int clique_rem_node(clique_t *c, graph_t *g, sv_t *node){
	rem_entr++;
	int i = 0;
	assert(graph_have_node(g,node));
	assert(vector_contains(c->items,&node)!=-1);
	if(!graph_have_node(g,node)){return -1;}

	size_t index = vector_contains(c->items, &node);
	if(index!=-1){
		vector_remove(c->items,i);
	}else{
		return -1;
	}
	c->v_prime--;
	vector_t *edges = graph_get_edges(g,node);
	sv_t **adj_item;
	for(i=0;i<edges->size;i++){
		adj_item = vector_get(edges,i);
		if(vector_contains(c->items,adj_item)!=-1){
			c->e_prime--;
		}
	}
	rem_ext++;

	return c->e_prime;
}

