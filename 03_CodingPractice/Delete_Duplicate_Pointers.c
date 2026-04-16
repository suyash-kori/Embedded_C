#include <stdio.h>

int removeDuplecate(int *arr, int n) {
    int *p, *q, *end;

    for(p = arr; p < arr + n; p++) {
        for(q = p + 1; q < arr + n; q++) {
            if(*p == *q) {
                end = q;
                while(end < arr + n - 1) {
                    *end = *(end + 1);
                    end++;
                }
                n--; // Decrease the size of the array
                q--; // Move back the pointer to check the new value at this position
            }
        }
    }
    return n;
}

int main() {
    int arr[] = {1, 6, 2, 3, 3, 1, 5, 5};
    int size = sizeof(arr) / sizeof(arr[0]);

    int newSize = removeDuplecate(arr, size);

    printf("Array after removing duplicates: ");
    for (int i = 0; i < newSize; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    return 0;
}