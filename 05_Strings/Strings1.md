## Strings Basics  

- Collection of characters terminated by a NULL character '\0'.  
- To store strings, we use character array.  
- This NULL character is automatically added by the compiler at the end of the string.  
- It is represented in double quotes " ".  
- The same is stored in the continous memory locations.  
- String length = #of char + 1 byte (for NULL character).  
- We use strings in UART, SPI, I2C for logging purposes.  

### Constant Strings  

- AKA String Literals  
- Can't be modified  
- Stored in Read only Memory.  i.e. Code or text segment.  
- Constant strings are shared, E.g if same string literal is used multiple times in a program. Compiler automatically points them to the same memory address (shared).  
- This is called String Pooling.  

### Modifiable Strings  

- These strings can be Modified.  
- They are stored in the Stack/Data segment.  
- They are stored in the read/write memory segments.  
- Hence, we can change the elements hold by char array during runtime.  
- The Modifiable strings are Not Shared.  
- Hence, every time we define a new array, a new unique copy is created in the memory.  

### Reverse the order of the words in a sentence  

Example:-  I/P = "string to be reversed", O/P = "reversed be to string"  

#include <stdio.h>  
#include <string.h>  

int main()  
{  
&emsp;char str[100];  
&emsp;int i, end, start;  
&emsp;fgets(str, sizeof(str), stdin);  
&emsp;str[strcspn(str, "\n")] = '\0';  
&emsp;end = strlen(str) - 1;  

&emsp;while(end >= 0)  
&emsp;{
&emsp;&emsp;while( end >= 0 && str[end] == ' ')  
&emsp;&emsp;&emsp;end--;  
&emsp;&emsp;if ( end < 0)  
&emsp;&emsp;&emsp;break;  
&emsp;&emsp;start = end;  
&emsp;&emsp;while(start >= 0 && str[start] != ' ')  
&emsp;&emsp;&emsp;start--;  
&emsp;&emsp;for(i = start+1; i <= end; i++)  
&emsp;&emsp;&emsp;printf("%c",str[i]);  
&emsp;&emsp;printf(" ");  
&emsp;&emsp;end = start -1;  
&emsp;}
return 0;  

}  

1. Read the full string using fgets().  
2. Remove the newline using str[strcspn(strcspn(str, "\n"))] = '\n';  
3. Set end to the last character using strlen(str) - 1  
4. Traverse the string from right to left.  
5. Skip trailing or extra spaces(" ")  
6. Mark the current from start + 1 to end.  
9. Print a space a word end using end variable.  
7. Move start backward until space or beginning of string.  
8. Print charactersfter each word  
10. Move end to the previous word and repeat.  

### Reverse each character of the word of a sentence  

I/P - string to be reversed  
O/P - gnirts ot eb desrever  

#include <stdio.h>  
#include <string.h>  

int main()  
{  
&emsp;char str[100];  
&emsp;int start = 0, end, i;  
&emsp;fgets(str, sizeof(str), stdin);  
&emsp;str[strcspn(str, "\n")] = '\0';  
&emsp;printf("Reversed string: ");  
&emsp;while (str[start] != '\0')  
&emsp;{  

&emsp;&emsp;while(str[start] == " ")  
&emsp;&emsp;{  

&emsp;&emsp;&emsp;printf(" ");  
&emsp;&emsp;&emsp;start++;  

&emsp;&emsp;}  

&emsp;&emsp;end = start;  
&emsp;&emsp;while(str[end] != " " && str[end] != '\0')  
&emsp;&emsp;{  
&emsp;&emsp;&emsp;end++;  
&emsp;&emsp;}  
&emsp;&emsp;for (i = end - 1; i >= start; i--)  
&emsp;&emsp;{  
&emsp;&emsp;&emsp;printf("%c", str[i]);  
&emsp;&emsp;}  

&emsp;}

}  

1] Read the full string using fgets()  
2] Remove the newline using str[strcspn(str, "\n")] = '\0';  
3] Start from the first character using start = 0  
4] Skip spaces if present  
5] Mark the beginning of the current word using start  
6] Move end forward until space or '\0'  
7] Print the current word in reverse from end - 1 to start  
8] Update start = end  
9] Repeat until the full string is processed  
