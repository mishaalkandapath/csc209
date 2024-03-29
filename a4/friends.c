#include "friends.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


/*
 * Create a new user with the given name.  Insert it at the tail of the list
 * of users whose head is pointed to by *user_ptr_add.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if a user by this name already exists in this list.
 *   - 2 if the given name cannot fit in the 'name' array
 *       (don't forget about the null terminator).
 */
int create_user(const char *name, User **user_ptr_add) {
    if (strlen(name) >= MAX_NAME) {
        return 2;
    }

    User *new_user = malloc(sizeof(User));
    if (new_user == NULL) {
        perror("malloc");
        exit(1);
    }
    strncpy(new_user->name, name, MAX_NAME); // name has max length MAX_NAME - 1

    for (int i = 0; i < MAX_NAME; i++) {
        new_user->profile_pic[i] = '\0';
    }

    new_user->first_post = NULL;
    new_user->next = NULL;
    for (int i = 0; i < MAX_FRIENDS; i++) {
        new_user->friends[i] = NULL;
    }

    // Add user to list
    User *prev = NULL;
    User *curr = *user_ptr_add;
    while (curr != NULL && strcmp(curr->name, name) != 0) {
        prev = curr;
        curr = curr->next;
    }

    if (*user_ptr_add == NULL) {
        *user_ptr_add = new_user;
        return 0;
    } else if (curr != NULL) {
        free(new_user);
        return 1;
    } else {
        prev->next = new_user;
        return 0;
    }
}


/*
 * Return a pointer to the user with this name in
 * the list starting with head. Return NULL if no such user exists.
 *
 * NOTE: You'll likely need to cast a (const User *) to a (User *)
 * to satisfy the prototype without warnings.
 */
User *find_user(const char *name, const User *head) {
    while (head != NULL && strcmp(name, head->name) != 0) {
        head = head->next;
    }

    return (User *)head;
}

int list_users_compute_length(const User *curr) {
    int length = 0;
    while (curr != NULL) {
        length += snprintf(NULL, 0, "%s\r\n",curr->name);
        curr = curr->next;
    }
    return length;
}

/*
 * Print the usernames of all users in the list starting at curr.
 * Names should be printed to standard output, one per line.
 */
char * list_users(const User *curr) {
    const User * temp = curr;
    int length = list_users_compute_length(temp) +  3;
    char * buffer = malloc(length); // +3 for \0 and \r\n
    if (buffer == NULL) {
        perror("malloc");
        exit(1);
    }
    strcpy(buffer, "\0"); // Initialize buffer to empty string
    while (curr != NULL) {
        snprintf(buffer + strlen(buffer), length - strlen(buffer), "%s\r\n",curr->name);
        curr = curr->next;
    }

    return buffer;
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

    if (user1 == NULL || user2 == NULL) {
        return 4;
    } else if (user1 == user2) { // Same user
        return 3;
    }

    int i, j;
    for (i = 0; i < MAX_FRIENDS; i++) {
        if (user1->friends[i] == NULL) { // Empty spot
            break;
        } else if (user1->friends[i] == user2) { // Already friends.
            return 1;
        }
    }

    for (j = 0; j < MAX_FRIENDS; j++) {
        if (user2->friends[j] == NULL) { // Empty spot
            break;
        }
    }

    if (i == MAX_FRIENDS || j == MAX_FRIENDS) { // Too many friends.
        return 2;
    }

    user1->friends[i] = user2;
    user2->friends[j] = user1;
    return 0;
}

/*
 *  compute the length of a string to print out a post
 *  Use localtime to print the time and date.
 */
int print_post_compute_length(const Post *post) {
    int length = 0;
    if (post == NULL) {
        return 0;
    }
    // Print author
    length += snprintf(NULL, 0, "From: %s\r\n", post->author);

    // Print date
    length += snprintf(NULL, 0, "Date: %s\r\n", asctime(localtime(post->date)));

    // Print message
    length += snprintf(NULL, 0, "%s\r\n", post->contents);

    return length;
}

/* DELETEE KLJDSKLFJKLSDFJDKLSJFDKLSFJDKLSJLJDSLKJFLKSJ
 *  Print a post.
 *  Use localtime to print the time and date.
 */
void print_post(const Post *post, char *output, int output_length) {
    if (post == NULL) {
        return;
    }
    // Print author
    snprintf(output + strlen(output), output_length - strlen(output), "From: %s\r\n", post->author);

    // Print date
    snprintf(output + strlen(output), output_length - strlen(output), "Date: %s\r\n", asctime(localtime(post->date)));

    // Print message
    snprintf(output + strlen(output), output_length - strlen(output), "%s\r\n", post->contents);

    return;
}

/*
Compute the string length required for storing the output of printing a user profile in a string*/
int print_user_compute_length(const User *user) {
    int length = 0;
    if (user == NULL) {
        return 0;
    }

    // Print name
    length += snprintf(NULL, 0, "Name: %s\r\n\r\n", user->name);
    length += strlen("------------------------------------------\r\n");

    // Print friend list.
    length += strlen("Friends:\n");
    for (int i = 0; i < MAX_FRIENDS && user->friends[i] != NULL; i++) {
        length += snprintf(NULL, 0, "%s\n", user->friends[i]->name);
    }
    length += strlen("------------------------------------------\r\n");

    // Print post list.
    length += strlen("Posts:\r\n");
    const Post *curr = user->first_post;
    while (curr != NULL) {\
        const Post * curr_dup = curr;
        length += print_post_compute_length(curr_dup);
        curr = curr->next;
        if (curr != NULL) {
            length += strlen("\r\n===\r\n\r\n");
        }
    }
    length += strlen("------------------------------------------\r\n");

    return length;
}


/*
 * Print a user profile.
 * For an example of the required output format, see the example output
 * linked from the handout.
 * Return:
 *   - 0 on success.
 *   - 1 if the user is NULL.
 */
char * print_user(const User *user) {
    if (user == NULL) {
        return NULL;
    }
    const User *user_dup = user;
    int base_length = print_user_compute_length(user_dup) + 3;  
    char * output = malloc(base_length); //1 for null terminator, 2 \r\n

    // Print name
    snprintf(output, base_length, "Name: %s\r\n\r\n", user->name);
    snprintf(output + strlen(output), base_length - strlen(output), "%s", "------------------------------------------\r\n");

    // Print friend list.
    snprintf(output + strlen(output), base_length - strlen(output), "%s", "Friends:\n");
    for (int i = 0; i < MAX_FRIENDS && user->friends[i] != NULL; i++) {
        snprintf(output + strlen(output), base_length - strlen(output), "%s\r\n", user->friends[i]->name);
    }
    snprintf(output + strlen(output), base_length - strlen(output), "%s", "------------------------------------------\r\n");

    // Print post list.
    snprintf(output + strlen(output), base_length - strlen(output), "%s", "Posts:\r\n");
    const Post *curr = user->first_post;
    while (curr != NULL) {
        print_post(curr, output, base_length);
        curr = curr->next;
        if (curr != NULL) {
            snprintf(output + strlen(output), base_length - strlen(output), "%s", "\r\n===\r\n\r\n");
        }
    }
    snprintf(output + strlen(output), base_length - strlen(output), "%s", "------------------------------------------\r\n");
    snprintf(output + strlen(output), base_length - strlen(output), "%s", "\r\n");
    // printf("------------------------------------------\n");

    return output;
}


/*
 * Make a new post from 'author' to the 'target' user,
 * containing the given contents, IF the users are friends.
 *
 * Insert the new post at the *front* of the user's list of posts.
 *
 * Use the 'time' function to store the current time.
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
    if (target == NULL || author == NULL) {
        return 2;
    }

    int friends = 0;
    for (int i = 0; i < MAX_FRIENDS && target->friends[i] != NULL; i++) {
        if (strcmp(target->friends[i]->name, author->name) == 0) {
            friends = 1;
            break;
        }
    }

    if (friends == 0) {
        return 1;
    }

    // Create post
    Post *new_post = malloc(sizeof(Post));
    if (new_post == NULL) {
        perror("malloc");
        exit(1);
    }
    strncpy(new_post->author, author->name, MAX_NAME);
    new_post->contents = contents;
    new_post->date = malloc(sizeof(time_t));
    if (new_post->date == NULL) {
        perror("malloc");
        exit(1);
    }
    time(new_post->date);
    new_post->next = target->first_post;
    target->first_post = new_post;

    return 0;
}

