/* Detect a cycle in a linked list */
#include <stdio.h>
#include <stdlib.h>

struct Node {
    int data;
    struct Node* next;
};

int detectCycle(struct Node* head) {
    struct Node* slow = head;
    struct Node* fast = head;

    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;

        if (slow == fast) {
            return 1; // Cycle detected
        }
    }

    return 0; // No cycle
}
// Helper to build a list quickly for testing
struct Node* createList(int arr[], int n) {
    struct Node *head = NULL, *tail = NULL;
    for (int i = 0; i < n; i++) {
        struct Node *newNode = (struct Node*)malloc(sizeof(struct Node));
        newNode->data = arr[i];
        newNode->next = NULL;
        if (head == NULL) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
    }
    return head;
}
int main() {
    // Create a simple linked list with a cycle for testing
    int arr[] = {1, 2, 3, 4};
    struct Node* head = createList(arr, 4);
    // Create a cycle by connecting the last node to the second node
    struct Node* current = head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = head->next;

    if (detectCycle(head)) {
        printf("Cycle detected in the linked list.\n");
    } else {
        printf("No cycle found in the linked list.\n");
    }

    return 0;
}