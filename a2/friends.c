#include "friends.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Create a new user with the given name.  Insert it at the tail of the list
 * of users whose head is pointed to by *user_ptr_add.
 *
 * Return:
 *   - 0 if successful
 *   - 1 if a user by this name already exists in this list
 *   - 2 if the given name cannot fit in the 'name' array
 *       (don't forget about the null terminator)
 */
int create_user(const char *name, User **user_ptr_add) {
    User *new_user = malloc(sizeof(User));
    
    if (new_user == NULL){
        exit(-1);
    }
    
    if (strlen(name) > MAX_NAME - 1){
        return 2;
    }

    strncpy(new_user -> name, name, strlen(name));
    (new_user -> name)[strlen(name)] = '\0';
    (new_user -> profile_pic)[0] = '\0'; //setting to NULL if no pfp set for future use. 

    User *curr_user = *user_ptr_add; //pointer to first user in the list
    User *prev_user = NULL;

    if (curr_user == NULL){
        *user_ptr_add = new_user;
    }else{
        while((curr_user) != NULL){
            if (strcmp(curr_user -> name, name) == 0){
                return 1;
            }
            prev_user = curr_user;
            curr_user = (curr_user -> next);
        }
        // no such user exists and we are clear to add
        (prev_user -> next) = new_user;
    }
    (new_user ->first_post) = NULL;
    (new_user -> next) = NULL;
    (new_user -> friends)[0] = NULL;
    
    return 0;

}


/*
 * Return a pointer to the user with this name in
 * the list starting with head. Return NULL if no such user exists.
 *
 * NOTE: You'll likely need to cast a (const User *) to a (User *)
 * to satisfy the prototype without warnings.
 */
User *find_user(const char *name, const User *head) {
    User *curr_usr_ptr = (User *) head;
    while (curr_usr_ptr != NULL){
        if (strcmp((curr_usr_ptr -> name), name) == 0){
            return curr_usr_ptr;
        }
        curr_usr_ptr = (curr_usr_ptr -> next);
    }
    return NULL;
}


/*
 * Print the usernames of all users in the list starting at curr.
 * Names should be printed to standard output, one per line.
 */
void list_users(const User *curr) {
    User *curr_usr_ptr = (User *) curr;
    printf("User List\n");
    while (curr_usr_ptr != NULL){
        printf("\t%s\n", curr_usr_ptr -> name);
        curr_usr_ptr = (curr_usr_ptr -> next);
    }
}


/*
 * Change the filename for the profile pic of the given user.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if the file does not exist or cannot be opened.
 *   - 2 if the filename is too long.
 */
int update_pic(User *user, const char *filename) {
    if (strlen(filename) > MAX_NAME -1){
        return 2;
    }else{
        FILE *pfp_file;
        pfp_file = fopen(filename, "r");
        if (pfp_file == NULL){
            return 1;
        }
        for (int i = 0; i<strlen(filename); i++){
            (user -> profile_pic)[i] = filename[i];
        }
        (user ->profile_pic)[strlen(filename)] = '\0';
        fclose(pfp_file);
        return 0;
    }
}


/*
 * Make two users friends with each other.  This is symmetric - a pointer to
 * each user must be stored in the 'friends' array of the other.
 *
 * New friends must be added in the first empty spot in the 'friends' array.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if the two users are already friends.
 *   - 2 if the users are not already friends, but at least one already has
 *     MAX_FRIENDS friends.
 *   - 3 if the same user is passed in twice.
 *   - 4 if at least one user does not exist.
 *
 * Do not modify either user if the result is a failure.
 * NOTE: If multiple errors apply, return the *largest* error code that applies.
 */
int make_friends(const char *name1, const char *name2, User *head) {

    User *user1 = find_user(name1, head);
    User *user2 = find_user(name2, head);
    if (user1 == NULL || user2 == NULL){
        return 4;
    }//checked if each user was found
    else if (strcmp(name1, name2) == 0){
        return 3;
    } //checking if the name passed is the same

    User *curr_users_1 = (user1 -> friends)[0];
    User *curr_users_2 = (user2 -> friends)[0];
    int count1 = 0;
    int count2 = 0;

    while (curr_users_1 != NULL && count1 < MAX_FRIENDS){
        if (strcmp(curr_users_1 -> name, name2) == 0){
            return 1;
        }
        count1++;
        if (count1 != MAX_FRIENDS){
            curr_users_1 = (user1 -> friends)[count1];
        }
        
    }

    // if (curr_users_1 != NULL && strcmp(curr_users_1 -> name, name2) == 0){
    //     return 1; // if the list hasnt terminated yet and the last element is the new friend
    // }else{// the list contains max elements already
    //     count1++;
    // }

    while (curr_users_2 != NULL && count2 < MAX_FRIENDS -1){
        printf("%d", count1);
        if (strcmp(curr_users_2 -> name, name1) == 0){
            return 1;
        }
        count2++;
        if (MAX_FRIENDS != count2){
            curr_users_2 = (user2 -> friends)[count2];
        }
    }

    // if (curr_users_2 != NULL && strcmp(curr_users_2 -> name, name1) == 0){
    //     return 1; // if the list hasnt terminated yet and the last element is the new friend
    // }else{// the list contains max elements already
    //     count2++;
    // }

    if (count1 >= MAX_FRIENDS || count2 >= MAX_FRIENDS){
        return 2;
    }

    if (count1 == 0){
        ((user1 -> friends)[0]) = user2;
        ((user1 -> friends)[1]) = NULL;
        count1++;
    }else{
        ((user1 -> friends)[count1]) = user2;
         count1++;
         //if the list of friends hasnt filled up yet, termninate it at the terminate point
        if (count1 != MAX_FRIENDS){
            ((user1 -> friends)[count1]) = NULL;
        }
        if (count2 != MAX_FRIENDS){
            ((user2-> friends)[count2]) = NULL;
        }
    }
    if (count2 == 0){
        ((user2 -> friends)[0]) = user1;
        ((user2 -> friends)[1]) = NULL;
        count2++;
    }else{
        ((user2 -> friends)[count2]) = user1; //add elements to corresponding indices
        count2++;\
        //if the list of friends hasnt filled up yet, termninate it at the terminate point
        if (count1 != MAX_FRIENDS){
            ((user1 -> friends)[count1]) = NULL;
        }
        if (count2 != MAX_FRIENDS){
            ((user2-> friends)[count2]) = NULL;
        }
    }

    return 0;

}


/*
 * Print a user profile.
 * For an example of the required output format, see the example output
 * linked from the handout.
 * Return:
 *   - 0 on success.
 *   - 1 if the user is NULL.
 */
int print_user(const User *user) {
    if (user == NULL){
        return 1;
    }
    //print the profile pic if its exists
    if ((user -> profile_pic)[0] != '\0'){
        FILE *pfp = fopen((user -> profile_pic), "rb");
        char c = fgetc(pfp);
        while (c != EOF){
            printf("%c", c);
            c = fgetc(pfp);
        }
        fclose(pfp);
        printf("\n");
    }

    printf("Name: %s", (user -> name));
    printf("\n------------------------------------------\n");
    printf("Friends: \n");
    User *curr_frend = (User *) (user -> friends)[0];
    int count = 0;
    while (curr_frend != NULL && count < MAX_FRIENDS - 1){
        printf("%s\n", (curr_frend -> name));
        count++;
        curr_frend = (User *) (user -> friends)[count];
    }
    if (curr_frend != NULL){
        printf("%s\n", (((User *) (user -> friends)[count]) -> name));
    }
    printf("------------------------------------------\n");
    printf("Posts:\n");
    Post *curr_post = (user -> first_post);
    
    int hours, minutes, seconds, day, month, year, weekday;
    char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char *days[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    while (curr_post != NULL){
        printf("From: %s\n", curr_post -> author);

        struct tm *local = localtime((curr_post -> date));

        day = local ->tm_mday;
        weekday = local -> tm_wday;
        month = local -> tm_mon;
        year = local -> tm_year + 1900;
        hours = local -> tm_hour;
        minutes = local -> tm_min;
        seconds = local -> tm_sec;

        printf("Date: %s %s %d %02d:%02d:%02d %d\n\n", days[weekday],months[month],day, hours, minutes, seconds, year);
        printf("%s\n", curr_post -> contents);
        if (curr_post -> next != NULL){
            printf("\n==\n\n");
        }
        curr_post = curr_post -> next;
    }

    printf("------------------------------------------\n");

    return 0;
}


/*
 * Make a new post from 'author' to the 'target' user,
 * containing the given contents, IF the users are friends.
 *
 * Insert the new post at the *front* of the user's list of posts.
 *
 * 'contents' is a pointer to heap-allocated memory - you do not need
 * to allocate more memory to store the contents of the post.
 *
 * Return:
 *   - 0 on success
 *   - 1 if users exist but are not friends
 *   - 2 if either User pointer is NULL
 */
int make_post(const User *author, User *target, char *contents) {
    if (author == NULL || target == NULL){
        return 2;
    }
    User *curr_user = (((User *) author) -> friends)[0];
    int not_found = 1;
    int count = 0;
    while (curr_user != NULL && count < MAX_FRIENDS - 1){
        if ((*curr_user).name  == (*target).name){
            not_found = 0;
            break;
        }
        count++;
        curr_user = (((User *) author) -> friends)[count];
    }

    if (curr_user != NULL){
        curr_user = (((User *) author) -> friends)[count];
        if ((*curr_user).name  == (*target).name){
            not_found = 0;
        }//checking if the leftover user is a friend
        count++;
    }

    if (not_found){
        return 1;
    }

    Post *new_post = malloc(sizeof(Post)); //create a new post pointer in heap space
    //copy in the author name:
    strncpy((new_post -> author), (author -> name), strlen(author -> name));
    (new_post -> author)[strlen(author -> name)] = '\0';

    (new_post -> contents) = contents;//already heap allocated so direct assignment. 

    time_t *now_time = malloc(sizeof(time_t));
    *now_time = time(NULL);

    (new_post -> date) = now_time;

    if (target -> first_post == NULL){ //no posts for this user yet
        User *mainuser = (User *) target;
        (mainuser -> first_post) = new_post;
        new_post -> next = NULL;
    }else{
        // Post *curr_node = new_post;
        // Post *prev_node = (target -> first_post);
        // while(prev_node != NULL){
        //     (curr_node -> next) = prev_node;
        //     prev_node = (prev_node -> next);
        // }
        (new_post -> next) = (target -> first_post);
        User *mainuser = (User *) target;
        (mainuser -> first_post) = new_post;
    }
    return 0;
    
}


/*
 * From the list pointed to by *user_ptr_del, delete the user
 * with the given name.
 * Remove the deleted user from any lists of friends.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if a user with this name does not exist.
 */
int delete_user(const char *name, User **user_ptr_del) {
    User *curr_user = *user_ptr_del;
    User *inter_user = NULL;
    User *prev_user = NULL;
    if (find_user(name, curr_user) == NULL){
        return 1;
    }
    //go through all users and remove friends where necessary
    while (curr_user != NULL){
        inter_user = curr_user -> next;
        int curr_frnd_idx = 0;
        int found = 0;
        User **user_friends = (curr_user -> friends);
        for (int i=0; i<MAX_FRIENDS; i++){
            //can be at the beginning, in between , or at the end
            if (user_friends[curr_frnd_idx] == NULL){
                break;
            }//there are no more users
            if (strcmp(user_friends[curr_frnd_idx] -> name, name) == 0){
                //user found at this index. 
                found++;
            }
            if (found){//shift elements if found;
                if (curr_frnd_idx == MAX_FRIENDS - 1 || user_friends[curr_frnd_idx+1] == NULL){
                    user_friends[curr_frnd_idx] = NULL; //terminate the list
                    break; //exit loop as mo more modifications
                }else{ //there is a user next to it
                    user_friends[curr_frnd_idx] = user_friends[curr_frnd_idx+1];
                }
            }
            curr_frnd_idx++;
        }
        prev_user = curr_user;
        curr_user = inter_user;
    }
    curr_user = *user_ptr_del;
    inter_user = NULL;
    prev_user = NULL;
    while (curr_user != NULL){
        if (strcmp(curr_user -> name, name) == 0){
            if (prev_user == NULL){
                *user_ptr_del = curr_user -> next;
            }else{
                prev_user -> next = curr_user -> next;
            }//delete accordingly on position at list
            //releasing memory in order:
            //first go through all the posts and clear their memory;
            Post * curr_post = curr_user -> first_post;
            Post* inter_post = NULL;
            while (curr_post != NULL){
                free(curr_post -> date);
                free(curr_post -> contents);
                inter_post = curr_post -> next; //intermediate pointer to save the next pointer
                free(curr_post);
                curr_post = inter_post;
            }//freed all posts;
            inter_user = curr_user -> next;
            // printf("%p\n", curr_user);
            free(curr_user); //free curr_user pointer
            curr_user = inter_user;
            // printf("%s\n", inter_user -> name);
            break;

        }else{
            prev_user = curr_user;
            curr_user = curr_user -> next;
        }
    }
    return 0;
}
