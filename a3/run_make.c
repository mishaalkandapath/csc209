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
#include <time.h>
#include <errno.h>

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
update_reqd is passed in from evaluate_rules
*/
void compare_mod_times(Rule * rule, int* update_reqd, struct stat *stat_info){
  //main target update_times:
  struct timespec stat_time = (*stat_info).st_mtim;
  long seconds = (*stat_info).st_mtim.tv_sec;
  long nanosecs = stat_time.tv_nsec;
  //check the dependencies:
  Dependency *curr_dep = rule -> dependencies;
  while(curr_dep){
    stat(curr_dep -> rule -> target, stat_info); //dependency mod times
    if (errno != 0){//file was not found
      *update_reqd = 1;
    }else{
      //modification time of the dependency file
      long dep_secs = ((*stat_info).st_mtim.tv_sec);
      long dep_nsecs = ((*stat_info).st_mtim.tv_nsec);
      if (dep_secs > seconds || (dep_secs == seconds && dep_nsecs >= nanosecs)){//dependency is newer
        *update_reqd = 1;
      }
    }
    curr_dep = curr_dep -> next_dep;
  }
}

int evaluate_rules(Rule* rule, int pflag); //inserting this header here. main function to evaluate rules of a target

/*
Helper method for evaluate_rules
go through all the dependencies, and recursively (indirect) call evaluate_rule
*/
void eval_dependencies(Dependency * curr_dep, int pflag, int *children, int* update_reqd, int* forked){
  //create a pipe if needed, else create a placeholder pipe
  int nos_pipes = (*children && pflag) ? *children : 1; //since 0 size arrays are not permitted
  int pipfd[nos_pipes][2];

  while (curr_dep){// run until 
    if (pflag && forked){
      pipe(pipfd[*children]); //pass in the right file directories
      close(pipfd[*children][1]); //they dont need to write. 
      //create child if necessary 
      *forked = pflag ? fork() : 0;
      *children += pflag ? 1 : 0;
    }

    if (!(*forked) || !pflag){
      *update_reqd = evaluate_rules(curr_dep -> rule, pflag);
      if (!(*forked)){
        close(pipfd[*children - 1][0]);// children are not reading
        //close before forks reading ends
        for (int j = 0; j < *children -1 ; j++){
          close(pipfd[j][0]);
        }
        write(pipfd[*children - 1][1], update_reqd, sizeof(int)); //write value of update_reqd to pipe
        close(pipfd[*children - 1][1]); //close the write end
        break; //one dependency per child policy
      }
    }

    if (!pflag || *forked) {// only if not parallelizing or parent process
      curr_dep = curr_dep -> next_dep;
    }
  }

  if (children && pflag && forked){ // if parallelizing and there are children to read from
    int temp_upg = 0;
    for (int i = 0; i < *children; i++){
      int status;
      wait(&status); // wait for each child.
      read(pipfd[i][0], &temp_upg, sizeof(int)); //read an integer
      close(pipfd[i][0]); //close the reading pipe end
      *update_reqd += temp_upg; //update update_reqd
    }
  }
}

/*
Evaluate all the rules of this current rule @rule recursively.
If @pflag is 1, then each rule is recursed into by a new child process. 
The function returns:
0, if no updates are to be made
1, if updated are to be made
-1, for parallel mode, a return for the sake of return 
*/
int evaluate_rules(Rule* rule, int pflag){

  if (rule -> dependencies == NULL){
    //no dependencies to check here, run all actions
    run_actions(rule -> actions);
    return 0; //parent process will determine whether to update or not 
  }else{
    // this rule has dependencies
    Dependency *curr_dep = rule -> dependencies;
    int update_reqd = 0; //whether this rule will require a run or not
    int forked = 1; // non-zero initialization to signify parent process
    int children = 0; //number of dependencies

    while (curr_dep){ //count the number of children
      children++;
      curr_dep = curr_dep -> next_dep;
    }
    //resetting to the head of the dependency list
    curr_dep = rule -> dependencies;
    children = 0;

    eval_dependencies(curr_dep, pflag, &children, &update_reqd, &forked); //evaluate all the dependencies as reqd

    if (!forked){
      return -1; //end the program if in a child forked process...
    }

    Action *curr_action = rule -> actions; //first action in rule
    if (update_reqd >= 1 && curr_action){ //pflag situation with clear requirement for updation
      //evaluate all the actions, if any
      run_actions(curr_action);
      return 1;
    }else if (update_reqd == 0){ //no dependencies needed to be updated
      //checking creation times;
      struct stat stat_info;
      stat(rule -> target, &stat_info);
      if (errno != 0 && curr_action){
        //no such file, file needs to be created
        run_actions(curr_action); //run actions to create file
        return 1;
      }else if (errno == 0 && curr_action){
        compare_mod_times(rule, &update_reqd, &stat_info); //compare the modification times of target and dependencies
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

