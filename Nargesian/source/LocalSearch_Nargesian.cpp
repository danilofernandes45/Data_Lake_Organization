#include <stdio.h>
#include <bits/stdc++.h>
#include <vector>
#include <iostream>     // std::cout
#include <random>       // std::default_random_engine
#include <time.h>
#include <map>
using namespace std;

//PROJECT FILES
#include "Instance.hpp"
#include "Organization.hpp"

#define best_neighbor_func Organization* (*)(Organization*, int)


void print_organization(Organization *org)
{
    for (int i = 0; i < org->all_states.size(); i++)
    {
        for (State * state : org->all_states[i])
        {
            cout << state->abs_column_id << " (" << state->level << ")\nParents => ";
            for(State * parent : state->parents)
                cout << parent->abs_column_id << " ";
            
            cout << "\nChildren => ";
            for(State * child : state->children)
                cout << child->abs_column_id << " ";
            cout << "\n";
        }
        
    }
    cout << "\n";

    // for (int i = 0; i < org->all_states.size(); i++)
    // {
    //     for (State * state : org->all_states[i])
    //     {
    //         cout << "~ " << state->abs_column_id << " ~" << endl;
    //         for(int p = 0; p < org->instance->total_num_columns; p++)
    //             cout << state->reach_probs[p] << " ";
    //         cout << "\n" << state->overall_reach_prob << endl;
    //     }
        
    // }
    // cout << "\n";
}

void compare_orgs(Organization *org, Organization *new_org)
{

    set<State*, CompareID>::iterator iter_1, iter_2;
    set<State*, CompareLevel>::iterator state_1, state_2;

    assert( org->all_states.size() == new_org->all_states.size() );

    for (int i = 0; i < org->all_states.size(); i++)
    {
        assert(org->all_states[i].size() == new_org->all_states[i].size());

        iter_1 = org->all_states[i].begin();
        iter_2 = new_org->all_states[i].begin();

        for(int j = 0; j < org->all_states[i].size(); j++)
        {
            assert( (*iter_1)->abs_column_id == (*iter_2)->abs_column_id );
            assert( (*iter_1)->parents.size() == (*iter_2)->parents.size() );
            assert( (*iter_1)->children.size() == (*iter_2)->children.size() );

            // cout << "Parents" << endl;
            // state_1 = (*iter_1)->parents.begin();
            // state_2 = (*iter_2)->parents.begin();
            // for (int s = 0; s < (*iter_1)->parents.size(); s++)
            // {
            //     cout << (*state_1)->level << " " << (*state_1)->abs_column_id << " " << (*state_2)->level << " " << (*state_2)->abs_column_id << endl;
            //     state_1++;
            //     state_2++;
            // }
            // cout << endl;

            state_1 = (*iter_1)->parents.begin();
            state_2 = (*iter_2)->parents.begin();
            for (int s = 0; s < (*iter_1)->parents.size(); s++)
            {
                assert( (*state_1)->abs_column_id == (*state_2)->abs_column_id );
                state_1++;
                state_2++;
            }

            // cout << "Children" << endl;
            // state_1 = (*iter_1)->children.begin();
            // state_2 = (*iter_2)->children.begin();
            // for (int s = 0; s < (*iter_1)->children.size(); s++)
            // {
            //     cout << (*state_1)->level << " " << (*state_1)->abs_column_id << " " << (*state_2)->level << " " << (*state_2)->abs_column_id << endl;
            //     state_1++;
            //     state_2++;
            // }

            // cout << endl;


            state_1 = (*iter_1)->children.begin();
            state_2 = (*iter_2)->children.begin();
            for (int s = 0; s < (*iter_1)->children.size(); s++)
            {
                assert( (*state_1)->abs_column_id == (*state_2)->abs_column_id );
                state_1++;
                state_2++;
            }
            
            iter_1++;
            iter_2++;
        }
    }
}

Organization* modify_organization(Organization *org, int level, int level_id, int update_id)
{
    Organization *new_org_add = org->copy();
    Organization *new_org_del = org->copy();

    #if DEBUG
    cout << "ORIGINAL: " << org->effectiveness << "\n\n";
    print_organization(org);
    compare_orgs(org, new_org_add);
    compare_orgs(org, new_org_del);
    #endif

    new_org_add->add_parent(level, level_id, update_id);

    #if DEBUG
    cout << "ADD: " << new_org_add->effectiveness << "\n\n";
    print_organization(new_org_add);
    #endif

    new_org_del->delete_parent(level, level_id, update_id);

    #if DEBUG
    cout << "REMOVE: " << new_org_del->effectiveness << "\n\n";
    print_organization(new_org_del);
    #endif

    if(new_org_add->effectiveness > new_org_del->effectiveness)
        return new_org_add;
    return new_org_del;
}

Organization* local_search(Organization *org, int plateau_iters, float eps, double timeout, float target)
{
    Organization *new_org;
    int level = 2, level_id = 0;
    int count = 0, update_id = 1;
    float prob_accept, increse_perc;
    //GENERATOR OF RANDOM NUMBERS
    random_device rand_dev;
    mt19937 generator(rand_dev());
    uniform_real_distribution<float> distribution(0.0, 1.0);

    while (count < plateau_iters && org->all_states.size() >= 3 && difftime(org->t_end, org->t_start) < timeout && org->effectiveness < target)
    {
        new_org = modify_organization(org, level, level_id, update_id);
        increse_perc = ( new_org->effectiveness - org->effectiveness ) / org->effectiveness;

        cout << new_org->effectiveness / org->effectiveness << endl;

        if( increse_perc >= 0 ) {
            if( increse_perc < eps )
                count++;
            else
                count = 0;
            //UPDATE ORGANIZATION
            org = new_org;
            #if DEBUG
            print_organization(new_org);
            print_organization(org);
            compare_orgs(new_org, org);
            #endif
            level = 2;
            level_id = 0;
            update_id = update_id + 2; 
        } else {
            prob_accept = new_org->effectiveness / org->effectiveness;
            // cout << distribution(generator) << ", " << prob_accept << endl;
            if(  distribution(generator) < prob_accept )
            {
                org = new_org;
                #if DEBUG
                print_organization(new_org);
                print_organization(org);
                compare_orgs(new_org, org);
                #endif
                level = 2;
                level_id = 0;
                update_id = update_id + 2;
            } else {
                level_id = ( level_id + 1 ) % org->all_states[level].size();
                if( level_id == 0 ) {
                    level++; 
                    if( level == org->all_states.size() )
                        level = 2;
                }
            }
            count++;
        }
        time(&org->t_end);
    }
    return org;
}

Organization* multistart_ls(Instance *instance, float gamma, int num_restarts, int plateau_iters, float eps, double timeout, float target) {
    
    time_t t_start;
    time(&t_start);

    Organization *best_org, *new_org;
    Organization *org = Organization::generate_organization_by_clustering(instance, gamma);
    org->t_start = t_start;
    time(&org->t_end);

    best_org = org;

    int i = 0;
    // while ( i < num_restarts && difftime(best_org->t_end, t_start) < timeout )
    while ( difftime(best_org->t_end, t_start) < timeout && best_org->effectiveness < target)
    {
        new_org = local_search(org, plateau_iters, eps, timeout, target);
        if( new_org->effectiveness > best_org->effectiveness )
            best_org = new_org;
        else 
            time(&best_org->t_end);
        i++;
    }
    // cout << i << ", ";
    return best_org;    
}

// ILS ---------------------------------------------------------------------------------

Organization* best_neighbor(Organization *org, int update_id, int neighborhood_struct)
{
    Organization *aux; 
    Organization *best_org = org->copy();
    Organization *new_org;
    // int best_level, best_level_id, min_level;
    // int best_min_level = -1;
    for(int level = 2; level < org->all_states.size(); level++)
    {
        for(int level_id = 0; level_id < org->all_states[level].size(); level_id++)
        { 
            new_org = org->copy();
            if(neighborhood_struct == 0){
                new_org->add_parent(level, level_id, update_id);
            } else {
                new_org->delete_parent(level, level_id, update_id);
            }
            if( new_org->effectiveness > best_org->effectiveness ) {
                delete best_org;
                best_org = new_org;
            } else {
                delete new_org;
            }
        }
    }
    return best_org;
}

pair<Organization*, int> rvnd_local_search(Organization *org, int update_id)
{
    Organization *new_org;
    srand(std::chrono::system_clock::now().time_since_epoch().count());
    int neighborhood_struct = rand() % 2;
    unsigned k = 0;

    while (k < 2)
    {
        new_org = best_neighbor(org, update_id, neighborhood_struct);
        // cout << new_org->effectiveness << endl;
        if( new_org->effectiveness > org->effectiveness ){
            srand(std::chrono::system_clock::now().time_since_epoch().count());
            neighborhood_struct = rand() % 2;
            k = 0;
            org = new_org;
            update_id++;
            // cout << "Accepted" << endl;
        } else {
            k++;
            // cout << "Rejected" << endl;
        }
    }
    return {org, update_id};
}

Organization* perturbation_ils(Organization *org, int update_id)
{
    Organization *new_org = org->copy();
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    srand(seed);

    int level = rand() % org->all_states.size();
    level = max(level, 2);
    int level_id = rand() % org->all_states[level].size();

    if( rand() % 2 == 0 )
        new_org->add_parent(level, level_id, update_id);
    else
        new_org->delete_parent(level, level_id, update_id);
    
    return new_org;
}

Organization* iterated_local_search(Instance *instance, float gamma, double timeout, float target)
{
    int update_id = 1;
    Organization *new_org;
    pair<Organization*, int> local_opt;

    time_t t_start;
    time(&t_start);

    Organization *best_org = Organization::generate_organization_by_clustering(instance, gamma);
    best_org->t_start = t_start;
    best_org->t_end = t_start;
    local_opt = rvnd_local_search(best_org, update_id);
    best_org = local_opt.first;
    update_id = local_opt.second;

    while ( difftime(best_org->t_end, t_start) < timeout && best_org->effectiveness < target)
    {
        new_org = perturbation_ils(best_org, update_id);
        local_opt = rvnd_local_search(new_org, update_id + 1);
        if( local_opt.first->effectiveness > best_org->effectiveness ){
            best_org = local_opt.first;
            update_id = local_opt.second;
            best_org->t_start = t_start;
        }
        time(&best_org->t_end);
    }
    return best_org;
}

// ILS ---------------------------------------------------------------------------------

map<string, string> parseCommandline(int argc, char* argv[]) {
    map<string, string> args;
    for(int i=1; i < argc; i = i + 2)
        args.insert(pair<string, string>(argv[i], argv[i+1]));
    return args;
}

int main(int argc, char* argv[]) {

    // Parses command line arguments.
    auto args = parseCommandline(argc, argv);

    Instance * instance = Instance::read_instance(args["-i"]);
    instance->num_tags = 0;

    // float gamma = 25.0;
    float gamma = stof(args["--gamma"]);
    double timeout = stod(args["--time"]);
    // int num_restarts = stoi(args["--Kr"]);
    int plateau_iters = stoi(args["--Kp"]);
    float eps = stof(args["--eps"]);

    float target = stof(args["--target"]);

    // float gamma = 25.0;
    // int K_max = 10;

    // Organization *best_org = multistart_ls(instance, gamma, -1, plateau_iters, eps, timeout, target);
    Organization *best_org = iterated_local_search(instance, gamma, timeout, target);


    // org = Organization::generate_organization_by_clustering(instance, gamma);
    // for (int i = 0; i < K_max; i++)
    // {
    //     // new_org = local_search(org, 40, 0.05); // 100
    //     new_org = local_search(org, 20, 0.05);
    //     if( best_org == NULL || new_org->effectiveness > best_org->effectiveness )
    //         best_org = new_org;
    // }
    // best_org = local_search(org, 40, 0.05); //500
    // best_org = local_search(org, 15, 0.05); //500

    // cout <<  difftime(best_org->t_end, best_org->t_start) << endl;
    cout << best_org->effectiveness << ", " << best_org->all_states.size() << ", " << difftime(best_org->t_end, best_org->t_start) << "\n";
    // cout << -best_org->effectiveness << endl;

    // best_org->success_probabilities();


    return 0;
}