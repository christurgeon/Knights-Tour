#include "includes.h"

int max_squares = 0;
static char*** dead_end_boards;
static int dead_boards_index = 0;
static int m;
static int n;
static int x = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Finds the available moves given a coordinate on the board
int* find_available_moves(char** board, int* moves, int x, int y) {

  // 16 total moves for a knight
  int* locations = calloc(16, sizeof(int));
  *moves = 0;
  if (y-2 >= 0 && x-1 >= 0) { // Try left and up
    if (board[x-1][y-2] == '.') {
      locations[(*moves)++] = x-1;
      locations[(*moves)++] = y-2;
    }
  }
  if (x-2 >= 0 && y-1 >= 0) { // Try up and left
    if (board[x-2][y-1] == '.') {
      locations[(*moves)++] = x-2;
      locations[(*moves)++] = y-1;
    }
  }
  if (x-2 >= 0 && y+1 < n) { // Try up and right
    if (board[x-2][y+1] == '.') {
      locations[(*moves)++] = x-2;
      locations[(*moves)++] = y+1;
    }
  }
  if (y+2 < n && x-1 >= 0) { // Try right and up
    if (board[x-1][y+2] == '.') {
      locations[(*moves)++] = x-1;
      locations[(*moves)++] = y+2;
    }
  }
  if (y+2 < n && x+1 < m) { // Try right and down
    if (board[x+1][y+2] == '.') {
      locations[(*moves)++] = x+1;
      locations[(*moves)++] = y+2;
    }
  }
  if (x+2 < m && y+1 < n) { // Try down and right
    if (board[x+2][y+1] == '.') {
      locations[(*moves)++] = x+2;
      locations[(*moves)++] = y+1;
    }
  }
  if (x+2 < m && y-1 >= 0) { // Try down and left
    if (board[x+2][y-1] == '.') {
      locations[(*moves)++] = x+2;
      locations[(*moves)++] = y-1;
    }
  }
  if (y-2 >= 0 && x+1 < m) { // Try left and down
    if (board[x+1][y-2] == '.') {
      locations[(*moves)++] = x+1;
      locations[(*moves)++] = y-2;
    }
  }
  return locations;
}

// =============================================================================
// =============================================================================

// Deallocate memory for a board
void free_board(char** board) {
  for (int i = 0; i < m; i++) {
    if (board[i] != NULL) {
      free(board[i]);
    }
  }
  free(board);
}

// =============================================================================
// =============================================================================

// Returns the number of empty spots available
int completed_board(char** board) {
  int count = 0;
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      if (board[i][j] == '.') {
        count++;
      }
    }
  }
  return count;
}

// =============================================================================
// =============================================================================

// Prints the board for testing purposes
void print_board(char** board) {
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      printf("%c", board[i][j]);
    }
    printf("\n");
  }
}

// =============================================================================
// =============================================================================

// Returns allocated memory for a new board initialized to all periods
char** make_board() {
  char** board = calloc(m, sizeof(char *));
  for (int i = 0; i < m; i++) {
    board[i] = calloc(n, sizeof(char));
    for (int j = 0; j < n; j++) {
      board[i][j] = '.';
    }
  }
  board[0][0] = 'S';
  return board;
}

// =============================================================================
// =============================================================================

// Makes a copy of the board provided
char** copy_board(char** board) {
  char** new_board = calloc(m, sizeof(char *));
  for (int i = 0; i < m; i++) {
    new_board[i] = calloc(n, sizeof(char));
    for (int j = 0; j < n; j++) {
      new_board[i][j] = board[i][j];
    }
  }
  return new_board;
}

// =============================================================================
// =============================================================================

// Allocates all of the data for a struct and returns the memory address of it
struct thread_args* build(int row, int col, char** board, long move) {
  struct thread_args* new_struct = calloc(1, sizeof(struct thread_args));
  if (new_struct == NULL)
    return NULL;
  new_struct->board = board;
  new_struct->move = move;
  new_struct->r = row;
  new_struct->c = col;
  return new_struct;
};

// =============================================================================
// =============================================================================

// Main execution of the knight's tour
void* tour(void* arguments) {

  // Copy the arguments and free the struct
  struct thread_args* args = (struct thread_args*) arguments;
  char** board = copy_board( (char **)args->board );
  long move = (long) args->move;
  long next_move = move + 1;
  int r = (int) args->r;
  int c = (int) args->c;
  free(args);

  // Find the available moves starting from the top left
  int size, rc;
  int* available_moves = find_available_moves(board, &size, r, c);

  // Check to see if there is a deadlock or if the game is done
  if (size == 0) {
    int count = completed_board(board);
    if (count == 0) { // Found a completed tour
      printf("THREAD %ld: Sonny found a full knight's tour!\n", pthread_self());
    }
    else { // Dead end tour, add it to the array
      printf("THREAD %ld: Dead end after move #%ld\n", pthread_self(), move);
      if (move >= x) { // Lock dead end boards access
        pthread_mutex_lock(&mutex);
        dead_end_boards[dead_boards_index++] = copy_board(board);
        pthread_mutex_unlock(&mutex);
      }
    }
    int squares = m*n - count;
    if (squares > max_squares) max_squares = squares;
    free(available_moves);
    pthread_exit( (void *)move );
  }

  // If one move, take that move, no need to allocate extra threads
  else if (size == 2) {
    int row = available_moves[0];
    int col = available_moves[1];
    board[row][col] = 'S';
    struct thread_args* new_struct = build(row, col, board, next_move);
    free(available_moves);
    return tour( (void *)new_struct );
  }

  else {
    printf("THREAD %ld: %d moves possible after move #%ld; creating threads...\n", pthread_self(), size / 2, move);

    // Loop through available moves and allocate threads to continue the tour
    #ifndef NO_PARALLEL
      pthread_t tids[CHILDREN];
      int j = 0;
    #endif
    #ifdef NO_PARALLEL
      pthread_t tid;
    #endif

    long* thread_rc;
    long max_thread_rc = 0;
    for (int i = 0; i < size; i += 2) {
      int row = available_moves[i];
      int col = available_moves[i+1];
      char** new_board = copy_board(board);
      new_board[row][col] = 'S';
      struct thread_args* new_struct = build(row, col, new_board, next_move);

      #ifndef NO_PARALLEL
        rc = pthread_create(&tids[j++], NULL, tour, (void *)new_struct);
      #endif
      #ifdef NO_PARALLEL
        rc = pthread_create(&tid, NULL, tour, (void *)new_struct);
      #endif
      if (rc != 0) {
        fprintf(stderr, "ERROR: <could not create thread>\n");
        exit(1);
      }

      #ifdef NO_PARALLEL
        rc = pthread_join(tid, (void *)&thread_rc);
        if (rc != 0) {
          fprintf(stderr, "ERROR: <could not join thread %ld (rc %d)>\n", tid, rc);
        }
        if ((long)thread_rc > max_thread_rc) max_thread_rc = (long)thread_rc;
        printf("THREAD %ld: Thread [%ld] joined (returned %ld)\n", pthread_self(), tid, (long)thread_rc);
      #endif
    }
    free(available_moves);

    #ifndef NO_PARALLEL
      // Join the threads and check error codes
      for (int i = 0; i < j; i++) {
        rc = pthread_join(tids[i], (void *)&thread_rc);
        if (rc != 0) {
          fprintf(stderr, "ERROR: <could not join thread %ld (rc %d)>\n", tids[i], rc);
        }
        if ((long)thread_rc > max_thread_rc) max_thread_rc = (long)thread_rc;
        printf("THREAD %ld: Thread [%ld] joined (returned %ld)\n", pthread_self(), tids[i], (long)thread_rc);
      }
    #endif
    return (void *)max_thread_rc;
  }
  return NULL;
}

int main(int argc, char** argv) {

  // Flush output buffer for grading purposes and validate input
  setvbuf(stdout, NULL, _IONBF, 0);
  if (argc < 3) {
    fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: a.out <m> <n> [<x>]\n");
    return EXIT_FAILURE;
  }

  // Read in arguments and validate them
  m = atoi(argv[1]);
  n = atoi(argv[2]);
  if (m < 3 || n < 3) {
    fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: a.out <m> <n> [<x>]\n");
    return EXIT_FAILURE;
  }
  if (argc == 4) {
    x = atoi(argv[3]);
    if (x < 0 || x > m*n) {
      fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: a.out <m> <n> [<x>]\n");
      return EXIT_FAILURE;
    }
  }

  // Allocate memory for the initial board and dead end boards array
  dead_end_boards = calloc(1000, sizeof(char **));
  char** initial_board = make_board();
  printf("THREAD %ld: Solving Sonny's knight's tour problem for a %dx%d board\n", pthread_self(), m, n);

  // Create the arguments for the first move and begin the tour
  struct thread_args* args = build(0, 0, initial_board, 1);
  tour( (void *)args );

  // Print statistics output and dead boards
  printf("THREAD %ld: Best solution(s) found visit %d squares (out of %i)\n", pthread_self(), max_squares, m*n);
  printf("THREAD %ld: Dead end boards:\n", pthread_self());

  // Print the dead boards, row by row
  for (int i = 0; i < dead_boards_index; i++) {
    for (int j = 0; j < m; j++) {
      if (j == 0) {
        printf("THREAD %ld: > %s\n", pthread_self(), dead_end_boards[i][j]);
      } else {
        printf("THREAD %ld:   %s\n", pthread_self(), dead_end_boards[i][j]);
      }
    }
    free_board(dead_end_boards[i]);
  }
  free_board(initial_board);
  free(dead_end_boards);

  // Return exit success
  return EXIT_SUCCESS;
}
