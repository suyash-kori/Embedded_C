/* Check if a string is a palindrome */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int isPalindrome(char *str) {
    int len = strlen(str);
    int start = 0;
    int end = len - 1;

    while (start < end) {
        // Skip non-alphanumeric characters from the start
        while (!isalnum(str[start]) && start < end) {
            start++;
        }
        // Skip non-alphanumeric characters from the end
        while (!isalnum(str[end]) && start < end) {
            end--;
        }
        // Compare characters (case-insensitive)
        if (tolower(str[start]) != tolower(str[end])) {
            return 0; // Not a palindrome
        }
        start++;
        end--;
    }
    return 1; // Is a palindrome
}

int main() {
    char str[100];

    printf("Enter a string: ");
    fgets(str, sizeof(str), stdin);

    // Remove newline character if present
    str[strcspn(str, "\n")] = 0;

    if (isPalindrome(str)) {
        printf("The string is a palindrome.\n");
    } else {
        printf("The string is not a palindrome.\n");
    }

    return 0;
}