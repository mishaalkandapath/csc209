#include <stdlib.h>
#include <stdio.h>

/*
 * Define a function void fib(...) below. This function takes parameter n
 * and generates the first n values in the Fibonacci sequence.  Recall that this
 * sequence is defined as:
 *         0, 1, 1, 2, 3, 5, ... , fib[n] = fib[n-2] + fib[n-1], ...
 * The values should be stored in a dynamically-allocated array composed of
 * exactly the correct number of integers. The values should be returned
 * through a pointer parameter passed in as the first argument.
 *
 * See the main function for an example call to fib.
 * Pay attention to the expected type of fib's parameters.
 */

/* Write your solution here */

void fib(int **fibo_list, int n){
    int curr1 = 0;
    int curr2 = 0;
    int *fibos = malloc(sizeof(int) * n);
    

    for (int i =  0; i < n; i++){
        // printf("%d %d %d \n", curr1, curr2, i);
        if (i == 0){
            fibos[i] = 0;
        }else if (i == 1){
            fibos[i] = 1;
            curr1 = 1;
        }else{
            fibos[i] = curr1 + curr2;
            curr2 = curr1;
            curr1 = fibos[i];
        }
        
    }

    *fibo_list = fibos;

}


int main(int argc, char **argv) {
    /* do not change this main function */
    int count = strtol(argv[1], NULL, 10);
    int *fib_sequence;

    fib(&fib_sequence, count);
    for (int i = 0; i < count; i++) {
        printf("%d ", fib_sequence[i]);
    }
    free(fib_sequence);
    return 0;
    
}
