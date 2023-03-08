#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pmake.h"


void remove_character_return(char *line){
    int len = strlen(line);
    for (int i =0; i<len; i++){
        if (line[i] == 13){
            for (int j = i; j < len - 1; j++) {
                line[j] = line[j + 1];
            }
            len--;
            i--;
        }
    }

    line[len] = '\0';
}

Rule * get_rule(Rule *head, char * target_name){
    Rule *curr_rule = head;
    while (curr_rule != NULL){
        // printf("Searching %s %s\n", target_name, curr_rule -> target);
        if (!strcmp(curr_rule -> target, target_name)){
            return curr_rule;
        }
        curr_rule = curr_rule -> next_rule;
    }
    return NULL;
}

Rule * link_dependencies(char * targets, Dependency **curr_dependency, Rule *head, Rule *curr_node, Rule** tail_node){
    
    //set the dependency links, create rules if they dont exist:
    Rule *dep_rule = get_rule(head, targets);
    int created = 0;
    if (dep_rule == NULL){
        dep_rule = malloc(sizeof(Rule));
        dep_rule -> target = malloc(sizeof(char) * (strlen(targets) + 1));
        strncpy(dep_rule -> target, targets, strlen(targets));
        (dep_rule -> target)[strlen(targets)] = '\0';
        dep_rule -> next_rule = NULL;
        (*tail_node) -> next_rule = dep_rule;
        (*tail_node) = dep_rule;
        (*tail_node) -> next_rule = NULL;
        (*tail_node) -> dependencies = NULL;
        (*tail_node) -> actions = NULL;
        created = 1;
    }

    Dependency *new_dep = malloc(sizeof(Dependency));

    if (*curr_dependency != NULL){
        (*curr_dependency) ->  next_dep = new_dep;
    }else{
        curr_node -> dependencies = new_dep;
    }
    *curr_dependency = new_dep;
    (*curr_dependency) -> next_dep = NULL;

    (*curr_dependency) -> rule = dep_rule;

    if (created){
        return dep_rule;
    }
    return NULL;
}

int line_type(char * line){
    for (int i = 0; i < strlen(line); i++){
        if ((!((line[i] >= 'a' && line[i] <= 'z' )|| (line[i] >= 'A' && line[i] <= 'Z'))) && !(line[i] == '\t' && i == 0)){//if there is a space or if there is more than one tab or if there is a pound in the sentence
            return 2;
        }else if (line[i] == '\t' && i == 0){
            continue;
        }else if (i == 1){
            //this is now an action line
            return 1;
        }else{
            //this is a target line 
            return 0;
        }
    }
    return 2; //undefined output, should never occur
}

/* Read from the open file fp, and create the linked data structure
   that represents the Makefile contained in the file.
   See the top of pmake.h for the specification of Makefile contents.
 */
Rule *parse_file(FILE *fp) {
    // Implement this function and remove the stubbed return statement below.
    char line[MAXLINE]; //store a line in the file
    Rule *curr_rule = NULL; //the last created rule, guaranteed to be intialized. 
    Rule *head = NULL;//the first created rule, guaranteed to be initialized
    Rule *curr_tail = NULL;
    Action *curr_action = NULL; //last created action
    Dependency *curr_dependency = NULL; //last created dependency

    while(fgets(line, MAXLINE, fp) != NULL){ //go through the file line by line
        int type = line_type(line); //get the type of the line
        line[strlen(line) - 1] = '\0'; //remove the newline character
        remove_character_return(line);

        if (type == 0){// it is a rule line:
            char *targets; //where all the tokens will be filled

            // get the name of the rule:
            targets = strtok(line, ":");
            targets[strlen(targets) - 1] = '\0'; //remove the space character

            //create the rule
            if (head == NULL){
                head = malloc(sizeof(Rule));
                curr_rule = head;
                curr_tail = head;
                head -> next_rule = NULL;
                head ->dependencies = NULL;
                head -> actions = NULL;
            }else{
                //if a rule of this name does not exist, create it
                Rule *new_rule = get_rule(head, targets);
                if (new_rule == NULL){
                    new_rule = malloc(sizeof(Rule));
                    curr_tail -> next_rule = new_rule;
                    curr_tail = new_rule;
                    curr_tail-> actions = NULL;
                    curr_tail -> dependencies = NULL;
                    curr_tail -> next_rule = NULL;
                }
                curr_rule = new_rule; //set it to be the most upd to date node.   
            }

            //copy in the name of the target
            curr_rule -> target = malloc(sizeof(char) * (strlen(targets) + 1));
            strncpy(curr_rule -> target, targets, strlen(targets));
            (curr_rule -> target)[strlen(targets)] = '\0';

            //link the dependencies
            while(targets != NULL){
                targets = strtok(NULL, " ");
                if (targets != NULL && (((int) targets[0]) != 13)){
                    link_dependencies(targets, &curr_dependency, head, curr_rule, &curr_tail);
                }
            }

            //reset dependencies and actions
            curr_dependency = NULL;
            curr_action = NULL; 

            // printf("at the end %s\n", curr_rule -> target);
            // printf("at the tail %s\n", curr_tail -> target);
            // printf("at the head %s\n", head -> target);
        }else if (type == 1){
            //create a new actions
            // printf("%s\n", line);
            Action * new_action = malloc(sizeof(Action));

            if (curr_action != NULL){
                //not first action for last created rule
                curr_action -> next_act = new_action;
            }else{
                curr_rule -> actions = new_action;
            }
            curr_action = new_action;
            new_action -> next_act = NULL;

            //count the number of spaces in the action
            int space_count = 0;
            for (int j = 0; j < strlen(line); j++){
                space_count += (line[j] == ' ');
            }

            char **all_words = malloc(sizeof(char *) * (space_count + 2)); //there are enuf words for the number of spaces + 1+ null
            char *actions;
            actions = strtok(line+1, " "); //tokenize by the space, +1 to skip the tab character
            int count = 0; //to keep a tally on which word is being processed right now
            while(actions != NULL && (((int) actions[0]) != 13)){
                all_words[count] = malloc(sizeof(char) * (strlen(actions) + 1)); //allocate space for this string
                strncpy(all_words[count], actions, strlen(actions));
                all_words[count][strlen(actions)] = '\0';

                actions = strtok(NULL, " ");
                count++;
            }
            
            all_words[count] = NULL; //final NULL instacne at the tail of the list 

            curr_action -> args = all_words;

        }
    }
    return head;
}


/******************************************************************************
 * These helper functions are provided for you. Do not modify them.
 *****************************************************************************/
/* Print the list of actions */
void print_actions(Action *act) {
    while(act != NULL) {
        if(act->args == NULL) {
            fprintf(stderr, "ERROR: action with NULL args\n");
            act = act->next_act;
            continue;
        }
        printf("\t");

        int i = 0;
        while(act->args[i] != NULL) {
            printf("%s ", act->args[i]) ;
            i++;
        }
        printf("\n");
        act = act->next_act;
    }
}

/* Print the list of rules to stdout in makefile format. If the output
   of print_rules is saved to a file, it should be possible to use it to
   run make correctly.
 */
void print_rules(Rule *rules){
    Rule *cur = rules;

    while (cur != NULL) {
        // printf("%s\n", cur->target);
        if (cur->dependencies || cur->actions) {
            // Print target
            printf("%s: ", cur->target);

            // Print dependencies
            Dependency *dep = cur->dependencies;
            while (dep != NULL){
                if(dep->rule->target == NULL) {
                    fprintf(stderr, "ERROR: dependency with NULL rule\n");
                }
                printf("%s ", dep->rule->target);
                dep = dep->next_dep;
            }
            printf("\n");

            // Print actions
            print_actions(cur->actions);
        }
        cur = cur->next_rule;
    }
}


/* Return 1 if the line is a comment line, as defined on the assignment handout.
   Return 0 otherwise.
 */
int is_comment_or_empty(const char *line) {
    for (int i = 0; i < strlen(line); i++){
        if (line[i] == '#') {
            return 1;
        }
        if (line[i] != '\t' && line[i] != ' ') {
            return 0;
        }
    }
    return 1;
}

/* Convert an array of args to a single space-separated string in buffer.
   Returns buffer.  Note that memory for args and buffer should be allocted
   by the caller.
 */
char *args_to_string(char **args, char *buffer, int size) {
    buffer[0] = '\0';
    int i = 0;
    while (args[i] != NULL) {
        strncat(buffer, args[i], size - strlen(buffer));
        strncat(buffer, " ", size - strlen(buffer));
        i++;
    }
    return buffer;
}
