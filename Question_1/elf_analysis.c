#include <stdio.h>
#include <stdlib.h>


int global_multiplier = 3;

// Memory allocation and memory write
int* allocate_and_populate(int size) {
    int *array = (int *)malloc(size * sizeof(int));
    if (array != NULL) {
        for (int i = 0; i < size; i++) {
            array[i] = i + 1; 
        }
    }
    return array;
}

// Loop and conditional branch 
void process_data(int *array, int size) {
    for (int i = 0; i < size; i++) {
        if (array[i] % 2 != 0) {
            array[i] = array[i] * global_multiplier;
        } else {
            array[i] = array[i] + 10;
        }
    }
}

// Standard library usage (printf) 
void print_results(int *array, int size) {
    for (int i = 0; i < size; i++) {
        printf("Index %d: %d\n", i, array[i]);
    }
}

// Main function
int main() {
    int elements = 4;
    
    int *my_data = allocate_and_populate(elements);
    if (my_data == NULL) {
        return 1;
    }
    
    process_data(my_data, elements);
    print_results(my_data, elements);
    
    free(my_data);
    return 0;
}
