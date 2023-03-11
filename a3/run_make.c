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

void run_actions(Action* action){

  while (action){
    int result = fork();
    if (result < 0){
      perror("fork");
      exit(1);
    }else if (result == 0){
      char action_line[MAXLINE];
      args_to_string(action -> args, action_line, MAXLINE);
      printf("%s\n", action_line);
      execvp((action -> args)[0] ,action -> args);
    }else{
      int status;
      status = wait(&status);
      if (WIFEXITED(status)){
        status = WEXITSTATUS(status);
        if (status != 0){
          exit(status);
        }        
      }
      action = action -> next_act;
    }
  }
}

  int evaluate_rules(Rule* rule, int pflag){

    if (rule -> dependencies == NULL && rule -> actions == NULL){
      //nothing to do here, return some random shet
      return 0;
    }else if (rule -> dependencies == NULL){
      run_actions(rule -> actions);
    }else{
      // this rule has dependencies
      Dependency *curr_dep = rule -> dependencies;
      int update_reqd = 0;
      int forked = 1;
      int children = 0;
      while (curr_dep){
        children++;
        curr_dep = curr_dep -> next_dep;
      }
      //resetting
      curr_dep = rule -> dependencies;
      int pipfd[children][2];

      children = 0;
      while (curr_dep){
        if (pflag && forked){
          pipe(pipfd[children]); //pass in the right file directories
          close(pipfd[children][1]); //they dont need to write. 
          //create child if necessary 
          forked = pflag ? fork() : 0;
          children += pflag ? 1 : 0;
        }
    
        if (!forked || !pflag){
          update_reqd = evaluate_rules(curr_dep -> rule, pflag);
          if (!forked){
            close(pipfd[children - 1][0]);// children are not reading
            //close before forks reading ends
            for (int j = 0; j < children -1 ; j++){
              close(pipfd[j][0]);
            }
            write(pipfd[children - 1][1], &update_reqd, sizeof(int));
            close(pipfd[children - 1][1]);
            break; //coz one dependency per child policy
          }
        }
        if (!pflag || forked) {
          curr_dep = curr_dep -> next_dep;
        }
      }

      if (children && pflag && forked){
        int temp_upg = 0;
        for (int i = 0; i < children; i++){
          int status;
          wait(&status); // wait for each child.
          read(pipfd[i][0], &temp_upg, sizeof(int));
          close(pipfd[i][0]);
          update_reqd += temp_upg;
        }
      }

      update_reqd = forked ? update_reqd : -1; //if this is a child return -1;
      // printf("upgrade status for %s %d %d\n",rule -> target, update_reqd, forked);

      if (update_reqd >= 1){//coz it can be -1 
        //evaluate all the actions, if any
        Action *curr_action = rule -> actions;
        if (curr_action != NULL){
          run_actions(curr_action);
        }
        return 1;
      }else if (update_reqd != -1){
        //check the dependency creation times;
        //if the current target does not exist, or any of its dependencies do not exist, or modification times of its dependencies more than its modification, then run actions

        //check if the target file exists:
        struct stat stat_info;
        stat(rule -> target, &stat_info);
        if (errno != 0){
          //no such file
          Action *curr_action = rule -> actions;
          if (curr_action != NULL){
            run_actions(curr_action);
          }
          return 1;
        }else if (errno == 0){
          struct timespec stat_time = stat_info.st_mtim;
          long seconds = (stat_info.st_mtim.tv_sec);
          long nanosecs = (stat_time.tv_nsec);
          //check the dependencies:
          curr_dep = rule -> dependencies;
          while(curr_dep){
            stat(curr_dep -> rule -> target, &stat_info);
            if (errno != 0){
              update_reqd = 1;
            }else{
              //modification time of the dependency file
              long dep_secs = ((stat_info.st_mtim).tv_sec);
              long dep_nsecs = ((stat_info.st_mtim).tv_nsec);
              if (dep_secs > seconds || (dep_secs == seconds && dep_nsecs >= nanosecs)){
                update_reqd = 1;
              }
            }
            curr_dep = curr_dep -> next_dep;
          }
          if (update_reqd){
            Action *curr_action = rule -> actions;
            if (curr_action != NULL){
              run_actions(curr_action);
            }
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

  evaluate_rules(rel_rule, pflag);

}

