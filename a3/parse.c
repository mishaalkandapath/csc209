#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pmake.h"


/* Read from the open file fp, and create the linked data structure
   that represents the Makefile contained in the file.
   See the top of pmake.h for the specification of Makefile contents.
 */
Rule *parse_file(FILE *fp) {
    // Implement this function and remove the stubbed return statement below.
    char line[MAXLINE]; //store a line in the file
    Rule *curr_rule = NULL; //the last created rule, guaranteed to be intialized. 
    Rule *head = NULL;//the first created rule, guaranteed to be initialized
    Action *curr_action = NULL; //last created action
    Dependency *curr_dependency = NULL; //last created dependency

    while(fgets(line, MAXLINE, fp) != NULL){ //go through the file line by line
        int type = line_type(line);//0 if target line, 1 if action line, 2 if comment or empty lines
        
        //the type of line has been identified and it is a target line
        if (type == 0){
            //process a target line;
            const char colon[2] = ':';
            const char space[2] = ' ';
            char *targets;

            //get the target name
            targets = strtok(line, ":");

            //create the Rule datatype
            if (curr_rule == NULL){
                Rule *curr_rule = malloc(sizeof(Rule));
                Rule *head = curr_rule;
            }else{
                Rule * new_rule = malloc(sizeof(Rule));
                (curr_rule -> next_rule) = new_rule;
                curr_rule = new_rule;
            }
            curr_rule -> next_rule = NULL;

            //set the name properly
            (curr_rule -> target) = malloc(sizeof(char) * (strlen(targets) + 1)); 
            strncpy(curr_rule -> target, targets, sizeof(char) * (strlen(targets) + 1));
            (curr_rule -> target)[strlen(curr_rule -> target)] = '\0';

            //link the dependencies
            while (targets != NULL){
                targets = strtok(NULL, " ");
                link_dependencies(targets, curr_dependency, head);
            }
            //link the last leftover dependency
            targets = strtok(NULL, "");
            link_dependencies(targets, curr_dependency, head);

            //reset curr_dependency
            curr_dependency = NULL;

        }else if (type == 1){
            Action *action = malloc(sizeof(Action));

            if (action == NULL){
                curr_rule -> actions = action;
            }else{
                curr_action -> next_act = action;
            }
            curr_action = action;
            curr_action -> next_act = NULL;

            int space_count = 0;
            for (int j = 0; j < strlen(line); j++){
                space_count += (line[j] == ' ');
            }

            char **all_words = malloc(sizeof(char *) * (space_count + 2)); //there are enuf words for the number of spaces + 1+ null
            char *targets;
            targets = strtok(line, " ");
            int count = 0;
            while(targets != NULL){
                count++;
                all_words[count - 1] = malloc(sizeof(char)*(strlen(targets) + 1));
                strncpy(all_words[count - 1], targets, strlen(targets));
                all_words[count - 1][strlen(targets)] = '\0';

                targets = strtok(NULL, "  ");
            }
            //put in the last word and NULL
            targets = strtok(NULL, "");
            all_words[count] = malloc(sizeof(char)*(strlen(targets) + 1));
            strncpy(all_words[count], targets, strlen(targets));
            all_words[count][strlen(targets)] = '\0';
            all_words[count + 1] = NULL;

            curr_action -> args = all_words;
        }
    }

    return head;
}

Rule * get_rule(Rule *curr_rule, char * target_name){
    while (curr_rule != NULL){
        if (strcmp(curr_rule -> target, target_name)){
            return curr_rule;
        }
        curr_rule = curr_rule -> next_rule;
    }
    return curr_rule;
}

void link_dependencies(char * targets, Dependency *curr_dependency, Rule *head){
    
    //set the dependency links, create rules if they dont exist:
    targets = strtok(NULL, " ");
    Rule *dep_rule = get_rule(head, targets);
    if (dep_rule == NULL){
        dep_rule = malloc(sizeof(Rule));
        dep_rule -> target = malloc(sizeof(char) * (strlen(targets) + 1));
        strncpy(dep_rule -> target, targets, strlen(targets));
        (dep_rule -> target)[strlen(targets)] = '\0';
    }

    Dependency *new_dep = malloc(sizeof(Dependency));

    if (curr_dependency != NULL){
        curr_dependency ->  next_dep = new_dep;
    }
    curr_dependency = new_dep;
    new_dep -> next_dep = NULL;

    new_dep -> rule = dep_rule;
}

int line_type(char * line){
    for (int i = 0; i < strlen(line); i++){
        if (line[i] == ' '  || (line[i] == '\t' && i >=1) || line[i] == '#'){//if there is a space or if there is more than one tab or if there is a pound in the sentence
            return 2;
        }else if (line[i] == '\t' && i == 0){
            continue;
        }else if (i == 1 && line_type == 1){
            //this is now an action line
            return 1;
        }else{
            //this is a target line 
            return 0;
        }
    }
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
        if (cur->dependencies || cur->actions) {
            // Print target
            printf("%s : ", cur->target);

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
