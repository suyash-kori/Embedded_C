/* Count the number of set bits (1s) in an integer with using (n&(n-1))*/
int countSetBits(int n) {
    int count = 0;
    while (n) {
        n = n & (n - 1);
        count++;
    }
    return count;
}
int main() {
    int num = 29; // Example number
    printf("Number of set bits in %d is %d\n", num, countSetBits(num));
    return 0;
}