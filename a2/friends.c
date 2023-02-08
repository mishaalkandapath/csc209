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
    
    if (strlen(name) > MAX_NAME - 1){
        return 2;
    }
    strncpy(new_user -> name, name, strlen(name));
    (new_user -> name)[strlen(name)] = '\0';

    User *curr_user = *user_ptr_add; //pointer to first user in the list
    User *prev_user = NULL;


    while((*curr_user) != NULL){
        if (strcmp(curr_user -> name, name) == 0){
            return 1;
        }
        prev_user = curr_user;
        curr_user = (curr_user -> next);
    }
    // no such user exists and we are clear to add
    (prev_user -> next) = &new_user;
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
    User *curr_usr_ptr = head;
    while (curr_usr_ptr != NULL){
        if (strcmp((curr_usr_ptr -> name), name) == 0){
            return curr_usr_ptr;
        }
        curr_user_ptr = (curr_usr_ptr -> next);
    }
    return NULL;
}


/*
 * Print the usernames of all users in the list starting at curr.
 * Names should be printed to standard output, one per line.
 */
void list_users(const User *curr) {
    User *curr_usr_ptr = head;
    while (curr_usr_ptr != NULL){
        printf("%s\n", curr_usr_ptr -> name);
        curr_user_ptr = (curr_usr_ptr -> next);
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
        (user -> profile_pic) = filename;
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
    if (strcmp(name1, name2) == 0){
        return 3
    } //checking if the name passed is the same
    User *user1 = NULL;
    User *user2 = NULL;

    while(head != NULL){
        if (strcmp((head -> name), name1) == 0){
            user1 = head;
        }else if (strcmp((head -> name), name2) == 0){
            user2 = head;
        }
    }
    if (user1 == NULL || user2 == NULL){
        return 4;
    }//checked if each user was found

    User *curr_users_1 = (user1 -> friends);
    User *curr_users_2 = (user2 -> friends);
     User *prev_users_1 = NULL;
    User *prev_users_2 = NULL;
    int count1 = 0;
    int count2 = 0;

    while (curr_users_1 != NULL){
        if (strcmp(curr_users_1 -> name, name2) == 0){
            return 1;
        }
        curr_users_1 = (curr_users_1 -> next);
        count1++;
    }

    while (curr_users_2 != NULL){
        if (strcmp(curr_users_2 -> name, name1) == 0){
            return 1;
        }
        curr_users_2 = (curr_users_2 -> next);
        count2++;
    }

    if (count1 >= MAX_FRIENDS || count2 >= MAX_FRIENDS){
        return 2;
    }

    (prev_users_1 -> next) = user2;
    (prev_users_2 -> next) = user1;

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
    return -1;
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
    return -1;
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
    return -1;
}
