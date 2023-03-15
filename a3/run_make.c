/*
 * Created on Sat Mar 11 2023
 *
 * Copyright (c) 2023 Mishaal Kandapath
 * Starter code does not belong to Mishaal Kandapath
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "pmake.h"
 
extern Rule * get_rule(Rule *curr_rule, char * target_name);// externally defined function to get a rule of a certain name

/*
Handle errors from a fork call using its return value.
If result < 0, perror is called for an appropriate sterr message, and the program quits with a non-zero exit code
Nothing is done otherwise, as is indicative of a successfull fork call
*/
void fork_err_handle(int result){
  if (result < 0){
      perror("fork");
      exit(1);
    }
}

/*
Run all actions in a linkedlist of actions that are NULL terminated.
Each action is handled by one child process.
*/
void run_actions(Action* action){

  while (action){//until the end of a list
    int result = fork(); //create a child process to execute execvp
    fork_err_handle(result); //handle any errors if any 
    if (result == 0){ //if child process
      char action_line[MAXLINE]; //array to store action string in
      args_to_string(action -> args, action_line, MAXLINE);
      printf("%s\n", action_line); //print the action 
      execvp((action -> args)[0] ,action -> args);//execute the action
    }else{ //parent process
      int status; 
      status = wait(&status);//wait for the child process to terminate
      if (WIFEXITED(status)){//if child exited with a status
        status = WEXITSTATUS(status); //get the value
        if (status != 0){
          exit(status); //exit if non-zero value with the specified value. 
        }        
      }
      action = action -> next_act; //move on to the next action.
    }
  }
}

/*
helper method for evaluate_rules
For this given rule, check if the current target does not exist, 
or any of its dependencies do not exist, 
or modification times of its dependencies more than its modification
return 1 if parent is outdated
else return 0
*/
int compare_mod_times(Rule * rule, struct stat *stat_info){
  //main target update_times:
  struct timespec stat_time = (*stat_info).st_mtim;
  long seconds = (*stat_info).st_mtim.tv_sec;
  long nanosecs = stat_time.tv_nsec;
  //check the dependencies:
  Dependency *curr_dep = rule -> dependencies;
  while(curr_dep){
    if (access(curr_dep -> rule -> target, F_OK) == -1){//file was not found
      return 1;
    }else{
      stat(curr_dep -> rule -> target, stat_info); //dependency mod times
      //modification time of the dependency file
      long dep_secs = ((*stat_info).st_mtim.tv_sec);
      long dep_nsecs = ((*stat_info).st_mtim.tv_nsec);
      if (dep_secs > seconds || (dep_secs == seconds && dep_nsecs >= nanosecs)){//dependency is newer
       return 1;
      }
    }
    curr_dep = curr_dep -> next_dep;
  }
  return 0;
}

int evaluate_rules(Rule* rule, int pflag); //inserting this header here. main function to evaluate rules of a target

/*
Helper method for evaluate_rules
go through all the dependencies, and recursively (indirect) call evaluate_rule
*/
void eval_dependencies(Dependency * curr_dep, int pflag, int* forked, int* children){

  while (curr_dep){// run until 
    if (pflag && forked){ 
      //create child if necessary 
      *forked = pflag ? fork() : 0;
    }

    if (!(*forked) || !pflag){//if in child process or we are not parallelizing
      evaluate_rules(curr_dep -> rule, pflag);
      if (!(*forked)){
        break; //one dependency per child;
      }
    }

    if (!pflag || *forked) {// only if not parallelizing or parent process
      curr_dep = curr_dep -> next_dep;
    }
  }

  if (pflag && forked){ // if parallelizing and there are children to read from
    for (int i = 0; i < *children; i++){
      int status;
      wait(&status); // wait for each child.
      if (WIFEXITED(status)){//if child exited with a status
        status = WEXITSTATUS(status); //get the value
        if (status != 0){
          exit(status); //exit if non-zero value with the specified value. 
        }        
      }
    }
  }
}

/*
Evaluate all the rules of this current rule @rule recursively.
If @pflag is 1, then each rule is recursed into by a new child process. 
The function returns:
-1 return to finish program
*/
int evaluate_rules(Rule* rule, int pflag){
  if (rule -> dependencies == NULL){
    //no dependencies to check here, run all actions
    run_actions(rule -> actions);
    return 0; //parent process will determine whether to update or not 
  }else{
    // this rule has dependencies
    Dependency *curr_dep = rule -> dependencies;
    int forked = 1; // non-zero initialization to signify parent process
    int children = 0; //number of dependencies

    while (curr_dep){ //count the number of children
      children++;
      curr_dep = curr_dep -> next_dep;
    }
    //resetting to the head of the dependency list
    curr_dep = rule -> dependencies;

    eval_dependencies(curr_dep, pflag, &forked, &children); //evaluate all the dependencies as reqd 

    if (!forked){
      return -1; //end the program if in a child forked process...
    }

    Action *curr_action = rule -> actions; //first action in rule
    //checking creation times;
    struct stat stat_info;
    if (access(rule ->target, F_OK) == -1 && curr_action){
      //no such file, file needs to be created
      run_actions(curr_action); //run actions to create file
      return 1;
    }else if (curr_action){
      stat(rule -> target, &stat_info);
      int update_reqd = compare_mod_times(rule, &stat_info); //compare the modification times of target and dependencies
      if (update_reqd){
        run_actions(curr_action); //run actions if required.
      }
      return update_reqd;
    }else{
      //unknown error occurred
      perror(rule -> target);
      exit(1);
    } 
  }

  return -1; //undefined behaviour
}


/* Evaluate the rule in rules corresponding to the given target.
   If target is NULL, evaluate the first rule instead.
   If pflag is 0, evaluate each dependency in sequence.
   If pflag is 1, then evaluate each dependency in parallel (by creating one 
   new process per dependency). In this case, the parent process will wait until
   all child processes have terminated before checking dependency modified times
   to decide whether to execute the actions.
 */
void run_make(char *target, Rule *rules, int pflag) {
  if (target == NULL){
    target = rules -> target;
  }

  Rule *rel_rule = get_rule(rules, target);
  
  if (rel_rule == NULL){
    //no such target exists
    fprintf(stderr, "No such target (%s) found\n", target);
    exit(1);
  }
  //evaluate the rule provided by user
  evaluate_rules(rel_rule, pflag);

}

