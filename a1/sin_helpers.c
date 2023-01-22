// TODO: Implement populate_array
/*
 * Convert a 9 digit int to a 9 element int array.
 */
int populate_array(int sin, int *sin_array) {
    int count = 0;
    int reversed_array[9]; 
    while(sin != 0){
        reversed_array[count] = sin % 10;
        sin = sin / 10;
        count += 1;
    }
    if (count != 9){
        return 1;
    }else{
        for (int i = 0; i < 9; i++){
            sin_array[i] = reversed_array[8-i];
        }
    }
    return 0;
}

// TODO: Implement check_sin
/*
 * Return 0 if the given sin_array is a valid SIN, and 1 otherwise.
 */
int check_sin(int *sin_array) {

    if (sin_array[0] == 0){
        return 1;
    }

    int sum = 0;
    for (int i = 0; i < 9; i++){
        // printf("%d", i%2);
        if (i % 2 != 0){
            sum += (2*sin_array[i])%10 + (2*sin_array[i]/10);
        }else{
            sum += sin_array[i];
        }
    }

    if (sum % 10 == 0){
        return 0;
    }
    return 1;
}
