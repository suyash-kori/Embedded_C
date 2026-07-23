/* Find the middle node in a singly linked list
Given the head of a singly linked list, write a C function that returns a pointer to the middle node.
If there are two middle nodes (even length), return the second one.
Example: For the list 1->2->3->4->5, the middle node is 3
Example: For the list 1->2->3->4, the middle node is 3
Constraints:
- Do it in a single pass (no counting the length first, then traversing again).
- Handle empty list (head == NULL) and single-node list.*/

#include <stdio.h>
#include <stdlib.h>

struct Node {
    int data;
    struct Node *next;
};

struct Node* findMiddle(struct Node *head) {
    if (head == NULL) return NULL;

    struct Node *slow = head;
    struct Node *fast = head;

    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }

    return slow;
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
    struct Node *head = NULL;
    int arr1[] = {1, 2, 3, 4, 5};
    struct Node *list1 = createList(arr1, 5);
    printf("Middle of odd list: %d\n", findMiddle(list1)->data); // 3

    int arr2[] = {1, 2, 3, 4};
    struct Node *list2 = createList(arr2, 4);
    printf("Middle of even list: %d\n", findMiddle(list2)->data); // 3

    return 0;
}