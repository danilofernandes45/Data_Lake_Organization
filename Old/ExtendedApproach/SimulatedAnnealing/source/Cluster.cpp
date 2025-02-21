#include "Cluster.hpp"

Cluster* Cluster::init_clusters(Instance * inst)
{
    Cluster* active_clusters = NULL;

    State *state;
    Cluster *current, *previous;
    //Leaf nodes (columns representation) creation
    for (int i = 0; i < inst->total_num_topics; i++)
    {
        state = new State;
        state->update_id = -1;
        state->topic_id = i;
        state->sum_vector = inst->topic_vectors[i];
        state->similarities = new float[inst->total_num_topics];
        state->reach_probs = new double[inst->total_num_topics];
        state->domain = new int[inst->total_num_topics];
        state->domain[i] = 1;

        current = new Cluster;
        current->state = state;
        current->id = i;
        current->cardinality = 1;

        if(active_clusters == NULL)
            active_clusters = current;
        else
            previous->next = current;

        previous = current;
    }
    return active_clusters;
}

float** Cluster::init_dist_matrix(Cluster* active_clusters, int total_num_topics, int embedding_dim)
{

    Cluster *cluster_i = active_clusters;
    Cluster *cluster_j;

    int size = 2 * total_num_topics - 1;
    float **dist_matrix = new float*[size];
    float similarity;

    //Init dist_matrix with zeros
    for(int i = 0; i < size; i++)
        dist_matrix[i] = new float[size];

    for(int i = 0; i < total_num_topics; i++)
    {
        cluster_j = cluster_i->next;
        for(int j = i+1; j < total_num_topics; j++)
        {
            similarity = cossine_similarity(cluster_i->state->sum_vector, cluster_j->state->sum_vector, embedding_dim);
            cluster_i->state->similarities[ cluster_j->state->topic_id ] = similarity;
            cluster_j->state->similarities[ cluster_i->state->topic_id ] = similarity;

            dist_matrix[i][j] = 1 - similarity;
            dist_matrix[j][i] = dist_matrix[i][j];
            cluster_j = cluster_j->next;
        }
        cluster_i->state->similarities[ cluster_i->state->topic_id ] = 1.0;

        // <TEST>
        // printf("ID: %d\n", cluster_i->state->abs_column_id);
        // for (int z = 0; z < total_num_columns; z++)
        //     printf("%.2f ", cluster_i->state->similarities[z]);
        // printf("\n\n");
        // </TEST>

        cluster_i = cluster_i->next;
    }
    return dist_matrix;
}

//MERGE THE LAST TWO CLUSTER OF NN CHAIN AND ADD THE NEW CLUSTER INTO UNMERGED CLUSTERS LIST 
Cluster* Cluster::merge_clusters(Cluster *stack, float **dist_matrix, int cluster_id, Instance *inst)
{
    int id_1 = stack->id;
    int id_2 = stack->is_NN_of->id;

    int card_1 = stack->cardinality;
    int card_2 = stack->is_NN_of->cardinality;

    State *state_1 = stack->state;
    State *state_2 = stack->is_NN_of->state;

    //CREATE A NEW STATE IN THE ORGANIZATION WHICH WILL BE PARENT OF RNN STATES
    State *new_state = new State;

    new_state->sum_vector = new float[inst->embedding_dim];
    for (int i = 0; i < inst->embedding_dim; i++)
        new_state->sum_vector[i] = state_1->sum_vector[i] + state_2->sum_vector[i];

    new_state->similarities = new float[inst->total_num_topics];    

    for (int i = 0; i < inst->total_num_topics; i++)
        new_state->similarities[i] = cossine_similarity(new_state->sum_vector, inst->topic_vectors[i], inst->embedding_dim);

    new_state->update_id = -1;
    new_state->topic_id = -cluster_id; //NEGATIVE ID: A NON-LEAF STATE
    new_state->reach_probs = new double[inst->total_num_topics];
    new_state->children.push_back(state_1);
    new_state->children.push_back(state_2);

    state_1->parents.push_back(new_state);
    state_2->parents.push_back(new_state);

    new_state->domain = new int[inst->total_num_topics];
    for (int i = 0; i < inst->total_num_topics; i++)
        new_state->domain[i] = state_1->domain[i] | state_2->domain[i];
    

    //CREATE A NEW CLUSTER CONTAINING THE NEW STATE
    Cluster *new_cluster = new Cluster;
    new_cluster->state = new_state;
    new_cluster->id = cluster_id;
    new_cluster->cardinality = card_1 + card_2;

    //UPDATE DISTANCES MATRIX
    for (int i = 0; i < cluster_id; i++)
    {
        dist_matrix[cluster_id][i] = ( card_1 * dist_matrix[id_1][i] + card_2 * dist_matrix[id_2][i] ) / ( card_1 + card_2 );
        dist_matrix[i][cluster_id] = dist_matrix[cluster_id][i];
    }

    //TEST!
    // for(int i=0; i <= cluster_id; i++){
    //     for(int j =0; j <= cluster_id; j++)
    //         printf("%.2f ", dist_matrix[i][j]);
    //     printf("\n");
    // }

    return new_cluster;
}
