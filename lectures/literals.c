#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {

    // Read the first 2 command-line arguments into rodata_offset & rodata_size

    int rodata_offset = strtol(argv[1], NULL, 16);
    int rodata_size = strtol(argv[2], NULL, 10);  // replace

   // Open the file named in the third argument
    FILE *file = fopen(argv[3], "rb");



    // Allocate space for the .rodata section
    char *strings = malloc(sizeof(char) * rodata_size);

    if (strings == NULL) {
        perror("malloc");
        exit(1);
    }

    // Read the .rodata section
    fseek(file, rodata_offset, SEEK_SET);
    fread(strings, sizeof(char), rodata_size, file); 


    // Format it for printing
    for (int i =0; i<rodata_size; i++){
        if (strings[i] == '\0'){
            strings[i] = '\n';
        }
    }
    
    // Add a null terminating character to the end of the output so it is
    // safe to pass to printf
    strings[rodata_size - 1] = '\0';

    printf("%s\n", strings);
    free(strings);

    // close the file

    return 0;
}
