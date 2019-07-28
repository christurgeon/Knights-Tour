# Knight's Tour

This program is written in C and it solves the Knight's Tour for any size board. It provides the option to run multithreaded with pthreads or single threaded by adding ```-D NO_PARALLEL``` during compilation.

## How to Run 
The ```m``` argument denotes the number of rows in the board, the ```n``` argument denotes the number of columns, and the ```x``` argument is optional; if provided, it indicates that the main thread should
display all “dead end” boards with at least x squares covered.
* Parallel:  ```gcc -Werror -Wall -pthread tour.c```
  * ```./a.out m n x```
* Not Parallel:   ```gcc -Werror -Wall -D NO_PARALLEL -pthread tour.c```
  * ```./a.out m n x```

## Additional Information
The program will print out all dead end boards and provide information of when threads are started and joined.
