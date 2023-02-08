#include <stdio.h>
#define HEADERSIZE 64

int main(int argc, char **argv) {

    // Open the file to read
    FILE *input = fopen(argv[1], "rb");

    // Find the starting address of the section headers
    long header_loc;  

    // Read the starter address of section headers


    // Uncomment the following to check your work
    /*
     printf("Address of section headers: %#lx (offset %ld bytes)\n", 
             header_loc, header_loc);
    */

    // Compute address of the .rodata section header



    // then uncomment the following to check your work
    /*
    printf("Address of .rodata section header: %#lx (offset %ld bytes)\n", 
            header_loc, header_loc);
    */

    // Go to .rodata section header and read the address 
    // and size of the .rodata section

    long rodata_offset;  // Read the .rodata section address into here
    long rodata_size;    // Read the .rodata section size into here





   
    // Uncomment the following to check your work
    /*
    printf("Address of .rodata section: %#lx\n", rodata_offset);
    printf("Size of .rodata section: %ld\n", rodata_size);
    printf("%#lx %ld\n", rodata_offset, rodata_size);
    */

    // Close input
    fclose(input);

    return 0;
}
