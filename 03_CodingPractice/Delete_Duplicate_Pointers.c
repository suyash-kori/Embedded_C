#include <stdio.h>

int removeDuplecate(int* arr, int size) {
    if (size == 0) return 0;

    int uniqueIndex = 0;

    for (int i = 1; i < size; i++) {
        if (arr[i] != arr[uniqueIndex]) {
            uniqueIndex++;
            arr[uniqueIndex] = arr[i];
        }
    }

    return uniqueIndex + 1; // Return the new size of the array
}

int main() {
    int arr[] = {1, 1, 2, 3, 3, 4, 5, 5};
    int size = sizeof(arr) / sizeof(arr[0]);

    int newSize = removeDuplecate(arr, size);

    printf("Array after removing duplicates: ");
    for (int i = 0; i < newSize; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    return 0;
}