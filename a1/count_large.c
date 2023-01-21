#include <stdio.h>
#include <stdlib.h>


// TODO: Implement a helper named check_permissions that matches the prototype below.
int check_permissions(char *, char *);


int main(int argc, char** argv) {
    if (!(argc == 2 || argc == 3)) {
        fprintf(stderr, "USAGE: count_large size [permissions]\n");
        return 1;
    }

    // TODO: Process command line arguments.

    // TODO: Call check_permissions as part of your solution to count the files to
    // compute and print the correct value.

    return 0;
}
