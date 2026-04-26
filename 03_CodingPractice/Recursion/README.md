# Basic Recursion

- Function calling itself directly or indirectly is called recursion.
- It is used when the solution to big problem can be found using smaller and similiar sub  problems
- Every recursion function must have:
    1. A base case
    2. Recursive case
- Commonly used in finding solutions to problems like factorials, Fibonacci, tree traversal, back tracking etc.
- Basically, recursion is: Function calling itself until a stopping conditions is met.
- Most used suntax include if conditions (for base case) and calling functions (usually by decrementing the value been passed).

# Advance Recursion

- Every recursive function calls adds a new stack frame
- Stack frame stores functions parameters, local variables and return address. i.e. FP, LV, RA.
- Whenever base case is reached, the function calls start returning in reverse order.
- Recursive calls follow FILO or LIFO order in the stack i.e. Last in First Out or First in Last Out.
- If base case is missing, it causes stack ovrerflow.
- MORE Recursive calls = MORE Stack /memory usage.
- It is usually avoided in embedded systems as the stack size is limited.
- If recursion deth becomes too high, it causes stack overflow. This causes crash, reset, corrupted memory and unpredictable behaviour.
- Realtime code must have deterministic timing. Hence we avoid in practice.
- It makes worse case stack usage analysis more difficult.
- Instead we use the for/while loops in embedded firmware.


