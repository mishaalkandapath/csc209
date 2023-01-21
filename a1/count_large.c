#include <stdio.h>
#include <stdlib.h>


// TODO: Implement a helper named check_permissions that matches the prototype below.
int check_permissions(char * given_perm, char * standard){
    char empty = '-';
    for (int i = 0; i < 9; i++){
        if (!(given_perm[i] == standard[i] || standard[i] == empty)){
            return 1;
        };
    }
    return 0;
}


int main(int argc, char** argv) {
    if (!(argc == 2 || argc == 3)) {
        fprintf(stderr, "USAGE: count_large size [permissions]\n");
        return 1;
    }

    // TODO: Process command line arguments.
    char *required_perm;
    int size_bound = strtol(argv[1], NULL, 10);
    if (argc == 3){
        required_perm = argv[2];
    }else{
        required_perm = "---------";
    }

    // printf("done loading arguments\n");
    // TODO: Call check_permissions as part of your solution to count the files to
    // compute and print the correct value.
    //defining ls variables
    char permissions[11];
    int node_type;
    char username[32];
    char group[32];
    int file_size;
    char month[4];
    int day;
    char time[6];
    char filename[32];

    scanf("%s %d", permissions, &day); //disregarding the first line by reading it    
    // printf("first line %s %d\n", permissions, day);

    int count = 0;

    while(scanf("%s %d %s %s %d %s %d %s %s", permissions, &node_type, username, group, &file_size, month, &day, time, filename) == 9){
        //load the 9 characters into an array:
        // printf("processing lines %s %d %d\n", filename, file_size, count);
        char perm[9];
        for (int i = 1; i < 10; i++){
            perm[i-1] = permissions[i];
        }
        // printf("permissions %d %d %d\n", permissions[0] != 'd', check_permissions(perm, required_perm), file_size > size_bound);
        if (permissions[0] != 'd' && !check_permissions(perm, required_perm) && file_size > size_bound){
            count = count + 1;
        };
    };

    printf("%d\n", count);

    return 0;
}
