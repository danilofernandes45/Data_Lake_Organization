#include "Organization.hpp"

void Organization::destroy()
{
    this->instance = NULL;

    for(int i = 0; i < this->all_states.size(); i++)
    {
        for(int j = 0; j < this->all_states[i].size(); j++)
            this->all_states[i][j]->destroy();
        vector<State*>().swap(this->all_states[i]);
    } 
    vector< vector<State*> >().swap(this->all_states);  
    delete this;
}

void Organization::update_effectiveness()
{
    State *leaf;
    int table_id;
    float *tables_discover_probs = new float[this->instance->num_tables];

    for (int i = 0; i < this->instance->num_tables; i++)
        tables_discover_probs[i] = 1.0;

    #if DEBUG
    cout << "Columns Overall Reach Probs: ";
    for (int i = 0; i < this->leaves.size(); i++)
    {
        leaf = this->leaves[i];
        cout << leaf->overall_reach_prob << " (" << leaf->abs_column_id << ") ";
    }

    cout << "\nColumns Discover Probs: ";
    #endif

    for (int i = 0; i < this->leaves.size(); i++)
    {   
        leaf = this->leaves[i];
        table_id = this->instance->map[leaf->abs_column_id][0];
        tables_discover_probs[table_id] *= ( 1 - leaf->reach_probs[leaf->abs_column_id] );

        #if DEBUG
        cout << leaf->reach_probs[leaf->abs_column_id] << " (" << leaf->abs_column_id << ") ";
        #endif
    }
    #if DEBUG
    cout << "\nTables Discover Probs: ";
    #endif

    this->effectiveness = 0.0;
    for (int i = 0; i < this->instance->num_tables; i++){
        this->effectiveness += 1 - tables_discover_probs[i];

        #if DEBUG
        cout << 1 - tables_discover_probs[i] << " ";
        #endif
    }
    #if DEBUG
    cout << "\n\n";
    #endif

    this->effectiveness /= this->instance->num_tables; 
}

void Organization::compute_all_reach_probs()
{
    this->root->level = 0;
    this->root->overall_reach_prob = 1.0;
    this->root->reach_probs = new float[this->instance->total_num_columns];
    for (int i = 0; i < this->instance->total_num_columns; i++)
        this->root->reach_probs[i] = 1.0;

    Organization::update_descendants(this->root, this->gamma, this->instance->total_num_columns, 0);
}

//INITIALIZE all_states, CONSIDERING THAT ALL STATES HAVE ONLY ONE PARENT
void Organization::init_all_states()
{
    queue<State*> queue;
    queue.push(this->root);
    State* current;
    int current_level = 0;
    vector<State*> *states_level = new vector<State*>; //STATES ADDED IN THE CURRENT LEVEL
    while( !queue.empty() ){
        //REMOVE FROM QUEUE
        current = queue.front();
        queue.pop();
        if( current->children.size() > 0) {
            //ADD ITS CHILDREN IN THE QUEUE
            for (int i = 0; i < current->children.size(); i++)
                queue.push(current->children[i]);
        } else {
            this->leaves.push_back(current);
        }
        //ADD states_level IN all_states, IF IT'S NECESSARY
        if( current_level < current->level ){

            // for (int i = 0; i < states_level->size(); i++)
            //     cout << (*states_level)[i]->abs_column_id << " ";
            // cout << "\n";

            //Unnecessary in a local search
            //sort( states_level->begin(), states_level->end(), State::compare );
            this->all_states.push_back(*states_level);
            states_level = new vector<State*>;
            current_level++;
        }
        //ADD CURRENT STATE IN states_level
        states_level->push_back(current);           
    }

    // for (int i = 0; i < states_level->size(); i++)
    //     cout << (*states_level)[i]->abs_column_id << " ";
    // cout << "\n";

    //Unnecessary in a local search
    // sort( states_level->begin(), states_level->end(), State::compare );
    this->all_states.push_back(*states_level);
      
}

void Organization::update_all_states(int level)
{
    for (int i = level; i < this->all_states.size(); i++)
        sort(this->all_states[i].begin(), this->all_states[i].end(), State::compare);
    
}

Organization* Organization::copy()
{
    Organization *copy = new Organization;
    copy->instance = this->instance;
    copy->gamma = this->gamma;
    copy->effectiveness = this->effectiveness;

    State *copied_state;
    for(int i = 0; i < this->all_states.size(); i++)
    {
        vector<State*> states;
        for (int j = 0; j < this->all_states[i].size(); j++){
            copied_state = this->all_states[i][j]->copy(this->instance->total_num_columns, this->instance->embedding_dim);
            //ADD TRANSITIONS PARENT-CHILD
            //UNDER CONSIDERATION THAT ALL PARENTS ARE IN PREVIOUS LEVEL
            for (int k = 0; k < this->all_states[i][j]->parents.size(); k++)
            {
                for (int l = 0; l < copy->all_states[i-1].size(); l++)
                {
                    if( this->all_states[i][j]->parents[k]->abs_column_id == copy->all_states[i-1][l]->abs_column_id ) {
                        copied_state->parents.push_back( copy->all_states[i-1][l] );
                        copy->all_states[i-1][l]->children.push_back( copied_state );
                        break;
                    }
                }   
            }
            states.push_back( copied_state );
            if( this->all_states[i][j]->children.size() == 0 )
                copy->leaves.push_back(copied_state);
        }
        copy->all_states.push_back(states);
    }
    copy->root = copy->all_states[0][0];

    return copy; 
}

vector<State*> Organization::delete_parent(int level, int level_id, int update_id)
{
    State *current, *parent;
    queue<State*> outdated_states;
    vector<State*> *new_parents, *grandpa_children, deleted_states;
    vector<State*>::iterator iter;
    for (int i = 0; i < this->all_states[level].size(); i++)
    {
        current = this->all_states[level][i];
        new_parents = new vector<State*>;
        for (int j = 0; j < current->parents.size(); j++)
        {
            parent = current->parents[j];
            for (int k = 0; k < parent->parents.size(); k++)
            {
                if( find(new_parents->begin(), new_parents->end(), parent->parents[k]) == new_parents->end() )
                {
                    grandpa_children = &parent->parents[k]->children;

                    // cout << "Parent " << parent->abs_column_id << "\n";
                    // for (int t = 0; t < grandpa_children->size(); t++)
                    //     cout << (*grandpa_children)[t]->abs_column_id << " ";
                    // cout << "\n";

                    iter = find(grandpa_children->begin(), grandpa_children->end(), parent);
                    if( iter != grandpa_children->end() )
                        grandpa_children->erase(iter);

                    // for (int t = 0; t < grandpa_children->size(); t++)
                    //     cout << (*grandpa_children)[t]->abs_column_id << " ";
                    // cout << "\n";
                    
                    grandpa_children->push_back(current);
                    new_parents->push_back(parent->parents[k]);
                    outdated_states.push(parent->parents[k]);
                }
            }
        }
        current->parents = *new_parents;
    }
    while ( !outdated_states.empty() )
    {
        current = outdated_states.front();
        outdated_states.pop();
        Organization::update_descendants(current, this->gamma, this->instance->total_num_columns, update_id);
    }
    //UPDATE all_states
    // AVOID LOST LEAF STATES
    for(int i=0; i < this->all_states[level-1].size(); i++)
    {
        if( this->all_states[level-1][i]->children.size() == 0 )
            this->all_states[level].push_back( this->all_states[level-1][i] );
    }

    deleted_states = this->all_states[level-1];
    this->all_states.erase(this->all_states.begin() + level - 1);
    this->update_effectiveness();

    // Unnecessary in a local search
    // for (int i = level-1; i < this->all_states.size(); i++)
    //     sort(this->all_states[i].begin(), this->all_states[i].end(), State::compare);
    
    this->last_modification = new Modification;
    this->last_modification->operation = 1;
    this->last_modification->level = level;
    this->last_modification->level_id = level_id;
    this->last_modification->deleted_states = deleted_states;

    return deleted_states;
}

//IT CONSIDERS THAT THE LAST PERFORMED OPERATION WAS A delete_parent IN THE NODES LOCATED IN level
void Organization::undo_delete_parent(Organization* prev_org, vector<State*> deleted_states, int level)
{
    State *parent, *child, *current, *previous;
    int *map, max_id;

    //REMOVE THE LEAF NODES INSERTED DURING delete_parent
    //THESE NODES ARE FROM THE REMOVED LEVEL
    this->all_states[level-1].resize( prev_org->all_states[level].size() );

    //REMOVE ALL PARENTHOOD RELATIONSHIP BETWEEN level-1 and level-2
    for(int i = 0; i < this->all_states[level-2].size(); i++)
        this->all_states[level-2][i]->children.clear();
    
    for(int i = 0; i < this->all_states[level-1].size(); i++)
        this->all_states[level-1][i]->parents.clear();

    //ADD BACK deleted_states IN ITS PREVIOUS POSITION IN all_states
    this->all_states.insert(this->all_states.begin() + level-1, deleted_states);
    //GENERATE MAP: abs_column_id -> id_{deleted_states}
    //THE GOAL IS TO ENSURE THE SAME ORDER IN BETWEEN this->all_states[i][j]->parents AND prev_org->all_states[i][j]->parents
    //THE ANALOGOUS IS CONSIDERED FOR all_states[i][j]->children
    max_id = 0;
    for (int i = 0; i < deleted_states.size(); i++)
        max_id = max( max_id, abs( deleted_states[i]->abs_column_id ) );

    map = new int[ 2 * (max_id+1) ];
    for (int i = 0; i < deleted_states.size(); i++) {
        max_id = deleted_states[i]->abs_column_id;
        if( max_id < 0 )
            max_id = 2 * abs(max_id);
        else
            max_id = 2 * abs(max_id) + 1;
        map[max_id] = i;
    }
    
    //ADD PARENTHOOD RELATIONSHIPS BETWEEN deleted_states AND ITS PREVIOUS PARENTS
    for (int i = 0; i < prev_org->all_states[level-2].size(); i++)
    {
        parent = prev_org->all_states[level-2][i];
        for (int j = 0; j < parent->children.size(); j++)
        {
            max_id = parent->children[j]->abs_column_id;
            if( max_id < 0 )
                max_id = 2 * abs(max_id);
            else
                max_id = 2 * abs(max_id) + 1;

            current = deleted_states[ map[max_id] ];
            this->all_states[level-2][i]->children.push_back(current);
        }
    }
    //ADD PARENTHOOD RELATIONSHIPS BETWEEN deleted_states AND ITS PREVIOUS CHILDREN
    for (int i = 0; i < prev_org->all_states[level].size(); i++)
    {
        child = prev_org->all_states[level][i];
        for (int j = 0; j < child->parents.size(); j++)
        {
            max_id = child->parents[j]->abs_column_id;
            if( max_id < 0 )
                max_id = 2 * abs(max_id);
            else
                max_id = 2 * abs(max_id) + 1;

            current = deleted_states[ map[max_id] ];
            this->all_states[level][i]->parents.push_back(current);
        }
    }

    //RECOVER PREVIOUS STATES' PROBABILITIES
    for(int i = level-1; i < this->all_states.size(); i++)
    {
        for(int j = 0; j < this->all_states[i].size(); j++)
        {
            previous = prev_org->all_states[i][j];
            current = this->all_states[i][j];
            if( previous->update_id != current->update_id )
            {
                current->update_id = previous->update_id;
                current->level = previous->level;
                current->overall_reach_prob = previous->overall_reach_prob;
                for(int r = 0; r < this->instance->total_num_columns; r++)
                    current->reach_probs[r] = previous->reach_probs[r];
            }
        }
    }
    this->effectiveness = prev_org->effectiveness;
}

int Organization::add_parent(int level, int level_id, int update_id)
{
    State *current = this->all_states[level][level_id];
    vector <State*> *candidates = &this->all_states[level-1];
    State *best_candidate = NULL;
    int min_level = -1;

    for( int i = candidates->size()-1; i >= 0; i--)
    {
        if( find(current->parents.begin(), current->parents.end(), (*candidates)[i]) ==  current->parents.end() ){
            if( best_candidate == NULL )
                best_candidate = (*candidates)[i];
            else if ( (*candidates)[i]->overall_reach_prob > best_candidate->overall_reach_prob )
                best_candidate = (*candidates)[i];
        } 
    }

    if( best_candidate != NULL ){
        current->parents.push_back( best_candidate );
        best_candidate->children.push_back(current);
        min_level = Organization::update_ancestors(current, this->instance, this->gamma, update_id);

        // Unnecessary in a local search
        // for (int i = min_level; i < this->all_states.size(); i++)
        //     sort(this->all_states[i].begin(), this->all_states[i].end(), State::compare);

        this->update_effectiveness();
    }

    this->last_modification = new Modification;
    this->last_modification->operation = 0;
    this->last_modification->level = level;
    this->last_modification->level_id = level_id;
    this->last_modification->min_level = min_level;

    return min_level;
}

//IT CONSIDERS THAT THE LAST PERFORMED OPERATION WAS A add_parent IN THE NODE LOCATED IN (level, level_id)
void Organization::undo_add_parent(Organization* prev_org, int level, int level_id, int min_level)
{

    //Remove the last parenthood relationship
    State *previous;
    State *current = this->all_states[level][level_id];
    State *parent = current->parents.back();
    parent->children.pop_back();
    current->parents.pop_back();

    for(int i = 0; i < this->all_states[min_level-1].size(); i++)
        this->all_states[min_level-1][i]->update_id = prev_org->all_states[min_level-1][i]->update_id;

    for(int l = min_level; l < this->all_states.size(); l++)
    {
        for(int l_id = 0; l_id < this->all_states[l].size(); l_id++)
        {
            current = this->all_states[l][l_id];
            previous = prev_org->all_states[l][l_id];
            if( current->update_id != previous->update_id )
            {
                current->update_id = previous->update_id;
                current->overall_reach_prob = previous->overall_reach_prob;

                for (int r = 0; r < this->instance->total_num_columns; r++)
                    current->reach_probs[r] = previous->reach_probs[r];
                
                if( l < level )
                {
                    current->sample_size = previous->sample_size;

                    for (int r = 0; r < this->instance->embedding_dim; r++)
                        current->sum_vector[r] = previous->sum_vector[r];

                    for (int r = 0; r < this->instance->total_num_columns; r++)
                        current->similarities[r] = previous->similarities[r];

                    for (int r = 0; r < this->instance->total_num_columns; r++)
                        current->domain[r] = current->domain[r];
                }
            }
        }
    }
    this->effectiveness = prev_org->effectiveness;
}

void Organization::undo_last_operation(Organization* prev_org)
{
    if( this->last_modification != NULL ) {
        if( this->last_modification->operation == 0 ) {
            this->undo_add_parent(prev_org, this->last_modification->level, this->last_modification->level_id, this->last_modification->min_level);
        } else {
            this->undo_delete_parent(prev_org, this->last_modification->deleted_states, this->last_modification->level);
        }
    }

    this->last_modification = NULL;
}

//IMPLEMENTATION CONSIDERING THAT ALL PARENTS OF A STATE ARE IN THE SAME LEVEL
//UNDER THIS CONSIDERATION, A BREADTH-FIRST SEARCH CAN UPDATE ALL PARENTS BEFORE ITS CHILDREN
void Organization::update_descendants(State *patriarch, float gamma, int total_num_columns, int update_id)
{    
    queue<State*> queue; //QUEUE USED IN THE BREADTH-FIRST SEARCH
    queue.push(patriarch);
    State *current;

    while( !queue.empty() ){
        //GET THE FIRST STATE FROM THE QUEUE
        current = queue.front();
        queue.pop();
        //VERIFY IF THE CURRENT STATE HAS ALREADY BEEN UPDATED
        if( current->update_id != update_id )
        {
            //COMPUTE ITS REACHABILITY PROBABILITIES
            //UPDATE LEVEL OF CURRENT NODE. OBS.: ALL PARENTS ARE IN THE SAME LEVEL
            if( current != patriarch )
                current->update_reach_probs(gamma, total_num_columns);

            //<TEST!>
            // printf("\nLevel: %d\nID: %d\n", current->level, current->abs_column_id);
            // printf("Reach Probs\n");
            // for (int t = 0; t < total_num_columns; t++)
            //     printf("%.3f ", current->reach_probs[t]);
            
            // printf("\nOverall reach prob: %.3f\n", current->overall_reach_prob);
            // if(current->abs_column_id >= 0)
            //     printf("Discover probability: %.3f\n", current->reach_probs[current->abs_column_id]);
            //</TEST!>

            //ADD ITS CHILDREN TO THE QUEUE
            for(int i=0; i < current->children.size(); i++ )
                queue.push(current->children[i]);
            current->update_id = update_id;
        }
    }
}

int Organization::update_ancestors(State *descendant, Instance *inst, float gamma, int update_id)
{
    queue<State*> queue; //QUEUE USED IN THE BREADTH-FIRST SEARCH
    queue.push(descendant);
    //PATRIARCHS WHOSE DESCENDANTS NEED TO UPDATE REACH_PROBS
    priority_queue<State*, vector<State*>, CompareLevel> outdated_states;  
    //ITERATORS
    State *current;
    int table_id, col_id, min_level;
    int has_changed = 1;
    float *sum_vector_i;

    while( !queue.empty() ){
        //GET THE FIRST STATE FROM THE QUEUE 
        current = queue.front();
        queue.pop();

        if( current != descendant )
        {
            has_changed = 0;
            //CHECK WHICH TOPICS NEED TO BE ADDED IN current's DOMAIN
            for(int i = 0; i < inst->total_num_columns; i++)
            {
                if(descendant->domain[i] == 1 && current->domain[i] == 0)
                {
                    has_changed = 1;
                    current->domain[i] = 1;
                    table_id = inst->map[i][0];
                    col_id = inst->map[i][1];
                    for(int d = 0; d < inst->embedding_dim; d++)
                        current->sum_vector[d] += inst->tables[table_id]->sum_vectors[col_id][d];
                }
            }
        }

        if( has_changed == 1 )
        {
            //UPDATE SIMILARITIES
            for (int i = 0; i < inst->total_num_columns; i++)
            {
                table_id = inst->map[i][0];
                col_id = inst->map[i][1];
                sum_vector_i = inst->tables[table_id]->sum_vectors[col_id];
                current->similarities[i] = cossine_similarity(current->sum_vector, sum_vector_i, inst->embedding_dim);
            }
            
            //ITS PARENTS NEED TO BE VERIFIED
            for(int i = 0; i < current->parents.size(); i++)
                queue.push(current->parents[i]);
        } else {
            //ITS SIBILINGS NEED UPDATE THIER reach_probs
            outdated_states.push(current);
        }
        
    }
    //UPDATE AFFECTED SUBGRAPHS WITH outdated_states AS PATRIARCHS
    min_level = outdated_states.top()->level;
    while( !outdated_states.empty() )
    {
        current = outdated_states.top();
        outdated_states.pop();
        if( current->update_id != update_id )
        {
            current->update_reach_probs(gamma, inst->total_num_columns);
            current->update_id = update_id;
            for(int i = 0; i < current->children.size(); i++)
                outdated_states.push(current->children[i]);
        } 
    }
    return min_level + 1;
}

//GENERATE THE BASELINE ORGANIZATION
Organization* Organization::generate_basic_organization(Instance * inst, float gamma)
{

    //Organizatio setup
    Organization *org = new Organization;
    org->instance = inst;
    org->gamma = gamma;
    //Root node creation
    org->root = new State;
    org->root->update_id = -1;
    org->root->abs_column_id = -inst->total_num_columns;
    org->root->sum_vector = new float[inst->embedding_dim]; // VECTOR INITIALIZED WITH ZEROS
    org->root->sample_size = 0;
    org->root->domain = new int[inst->total_num_columns];
    for (int i = 0; i < inst->total_num_columns; i++)
        org->root->domain[i] = 1;

    State *state;
    int count = 0;

    //Leaf nodes (columns representation) creation
    for (int i = 0; i < inst->num_tables; i++)
    {
        org->root->sample_size += inst->tables[i]->nrows * inst->tables[i]->ncols;

        for (int j = 0; j < inst->tables[i]->ncols; j++)
        {
            state = new State;

            state->abs_column_id = count;
            state->update_id = -1;
            state->reach_probs = new float[inst->total_num_columns];
            state->domain = new int[inst->total_num_columns];
            state->domain[count] = 1;
            count++;

            state->parents.push_back( org->root );
            state->sum_vector = inst->tables[i]->sum_vectors[j];
            state->sample_size = inst->tables[i]->nrows;
            //root sum vector incrementation 
            for (int d = 0; d < inst->embedding_dim; d++)
                org->root->sum_vector[d] += state->sum_vector[d];
            //Adding the leaf to the organization as child of the root
            org->root->children.push_back(state);

            state->similarities = new float[inst->total_num_columns];
            for(int s=0; s < inst->total_num_columns; s++)
                state->similarities[s] = cossine_similarity(state->sum_vector, inst->tables[inst->map[s][0]]->sum_vectors[inst->map[s][1]], inst->embedding_dim);
        }   
    }
    org->compute_all_reach_probs();
    org->init_all_states();
    org->update_effectiveness();
    return org;
}


//GENERATE A ORGANIZATION BY HIERARQUICAL CLUSTERING
//ALGORITHM: NEAREST NEIGHBORS CHAIN -> O(N²)
//DISTANCE BETWEEN CLUSTERS: UNWEIGHTED PAIR-GROUP METHOD WITH ARITHMETIC MEAN (UPGMA)
Organization* Organization::generate_organization_by_clustering(Instance * inst, float gamma, bool compute_probs)
{
    //Organization setup
    Organization *org = new Organization;
    org->instance = inst;
    org->gamma = gamma;

    Cluster* active_clusters = Cluster::init_clusters(inst); // CHAINED LIST OF CLUSTERS AVAILABLE TO BE ADDED TO NN CHAIN
    float** dist_matrix = Cluster::init_dist_matrix(active_clusters, inst->total_num_columns, inst->embedding_dim); // DISTANCE BETWEEN ALL CLUSTERS

    Cluster *stack = active_clusters; //NEAREST NEIGHBORS CHAIN
    active_clusters = active_clusters->next; // REMOVE THE HEAD FROM CHAINED LIST AND ADD TO NN CHAIN

    Cluster *prev_nn, *nn; // PREVIOUS NN CLUSTER AND NN CLUSTER IN CHAINED LIST
    Cluster *previous, *current; // ITERATORS
    Cluster *new_cluster;

    int cluster_id = inst->total_num_columns; // MATRIX ID OF THE NEXT CLUSTER THAT WILL BE CREATED
    float diff_dists;

    //WHILE THERE ARE CLUSTERS TO MERGE 
    while(stack != NULL)
    {
        // printf("Iteration\n\n");
        // printf("%d ", stack->id);
        prev_nn = NULL;
        nn = active_clusters;
        previous = active_clusters;
        current = active_clusters->next;

        //FIND NEAREST NEIGHBOR TO TOP STACK CLUSTER
        while(current != NULL)
        {
            diff_dists = dist_matrix[stack->id][current->id] - dist_matrix[stack->id][nn->id];
            if( diff_dists < 0 || ( diff_dists == 0 && current->id < nn->id ) )
            {
                prev_nn = previous;
                nn = current;
            }
            previous = current;
            current = current->next;
        }

        //CHECK IF A PAIR OF RNN WERE FOUND
        diff_dists = 1.0;
        if( stack->is_NN_of != NULL ){
            diff_dists = dist_matrix[stack->id][stack->is_NN_of->id] - dist_matrix[stack->id][nn->id];
        }
        if( diff_dists < 0 || ( diff_dists == 0 && stack->is_NN_of->id < nn->id ) )
        {
            // printf("%d\n", stack->is_NN_of->id);
            //MERGE THE RNN INTO A NEW CLUSTER
            new_cluster = Cluster::merge_clusters(stack, dist_matrix, cluster_id, inst);
            //ADD THE NEW CLUSTER INTO UNMERGED CLUSTER LIST
            new_cluster->next = active_clusters;
            active_clusters = new_cluster;
            cluster_id++;

            //REMOVE THE RNN FROM NN CHAIN
            stack = stack->is_NN_of->is_NN_of;
            if(stack == NULL && active_clusters->next != NULL){
                stack = active_clusters;
                active_clusters = active_clusters->next;
            }
        } else {
            //REMOVE NN FROM THE CHAINED LIST
            if(prev_nn == NULL)
                active_clusters = nn->next;
            else
                prev_nn->next = nn->next;
            
            //ADD NN TO THE NN CHAIN
            // printf("%d\n", nn->id);
            nn->is_NN_of = stack;
            stack = nn;

            if( active_clusters == NULL ) {
                active_clusters = Cluster::merge_clusters(stack, dist_matrix, cluster_id, inst);
                cluster_id++;
                stack = stack->is_NN_of->is_NN_of;
            }
        }
        //Test!
        current = stack;
        // printf("Stack: ");
        // while(current != NULL){
        //     printf("%d ", current->id);
        //     current = current->is_NN_of;
        // }
        // printf("\n");

        // current = active_clusters;
        // printf("Active Clusters: ");
        // while(current != NULL){
        //     printf("%d ", current->id);
        //     current = current->next;
        // }vector<State*> states_level;
        // printf("\n\n");
    }

    org->root = active_clusters->state;
    if( compute_probs ){
        org->compute_all_reach_probs();
        org->init_all_states();
        org->update_effectiveness();
    }
    return org;
}

Organization* Organization::generate_organization_by_heuristic(Instance * inst, float gamma)
{
    Organization *org = Organization::generate_organization_by_clustering(inst, gamma, false);
    int num_merges = ceil( 0.1 * ( abs(org->root->abs_column_id) - inst->total_num_columns ) );

    //GET NON-LEAF NODES IN THE ORGANIZATION
    State *current, *child;
    queue<State*> queue;
    vector<State*> internal_states;
    queue.push(org->root);
    internal_states.push_back(org->root);
    //O(N)
    while( !queue.empty() ){
        current = queue.front();
        queue.pop();
        for(int i=0; i < current->children.size(); i++) {
            child = current->children[i];
            if( child->children.size() > 0 ){
                if( child->children[0]->children.size() > 0 || child->children[1]->children.size() > 0 ) {
                    queue.push(child);
                    internal_states.push_back(child);
                }
            }
        }
    }


    int id;
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    srand(seed);
    vector<State*> new_children;
    bool flag;

    for(int i = 0; i < num_merges; i++)
    {
        id = rand() % internal_states.size();
        current = internal_states[id];
        if( current->children.size() > 0 ){
            flag = false;
            for(int j = 0; j < current->children.size(); j++) {
                child = current->children[j];
                if( child->children.size() == 0 )
                    new_children.push_back(child);
                else {
                    flag = true;
                    // cout << child->abs_column_id << " " << child->children.size() <<  endl;
                    for(int k = 0; k < child->children.size(); k++) {
                        child->children[k]->parents[0] = current; //CHANGE THE PARENT
                        new_children.push_back(child->children[k]);
                    }
                    child->children.clear();
                }
            }
            if(flag)
                current->children = new_children;
            else {
                internal_states.erase(internal_states.begin() + id);
                i--;
            }
            new_children.clear();
        } else {
            internal_states.erase(internal_states.begin() + id);
            i--;
        }
    }

    org->compute_all_reach_probs();
    org->init_all_states();
    org->update_effectiveness();

    return org;
}