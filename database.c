#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char* buffer;
  size_t buffer_length;
  ssize_t input_length;
} InputBuffer;

typedef struct {
  StatementType type;
} Statement;

typedef enum {
  COMMAND_SUCCESS,
  COMMAND_UNRECOGNIZED
} CommandResult;

typedef enum { 
  STATEMENT_INSERT, 
  STATEMENT_SELECT 
} StatementType;

typedef enum { 
  STATEMENT_SUCCESS,
  STATEMENT_UNRECOGNIZED 
} PrepareStatementResult;

InputBuffer* create_input_buffer() {
  InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));
  input_buffer->buffer = NULL;
  input_buffer->buffer_length = 0;
  input_buffer->input_length = 0;
  return input_buffer;
}

// Read from input stream (standard input). Remove the last byte from stdin ("\n" newline character)
void read_input(InputBuffer* input_buffer) {
  ssize_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);
  if (bytes_read <= 0) {
    printf("Error when reading input!");
    exit(EXIT_FAILURE);
  }
  input_buffer->input_length = bytes_read - 1;
  input_buffer->buffer[bytes_read - 1] = 0;
}

// Free the memory used by the input buffer
void close_input_buffer(InputBuffer* input_buffer) {
    free(input_buffer->buffer);
    free(input_buffer);
}

// !exit or !quit can be used to terminate the program.
CommandResult command(InputBuffer* input_buffer) {
  if (strcmp(input_buffer->buffer, "!exit") == 0 || strcmp(input_buffer->buffer, "!quit") == 0) {
    exit(EXIT_SUCCESS);
  } else {
    return COMMAND_UNRECOGNIZED;
  }
}

PrepareStatementResult prepare_statement(InputBuffer* input_buffer,
                                Statement* statement) {
  if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
    statement->type = STATEMENT_INSERT;
    return PREPARE_SUCCESS;
  }
  if (strcmp(input_buffer->buffer, "select") == 0) {
    statement->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }
  return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(Statement* statement) {
  switch (statement->type) {
    case (STATEMENT_INSERT):
      break;
    case (STATEMENT_SELECT):
      break;
  }
}

// Main program loop. 
int main(int argc, char* argv[]) {
  InputBuffer* input_buffer = create_input_buffer();
  while (true) {
    printf("db > ");
    get_input(input_buffer);

    // Non-SQL commands should start with an exclamation mark "!".
    if (input_buffer->buffer[0] == '!') {
+      switch (command(input_buffer)) {
+        case (COMMAND_SUCCESS):
+          continue;
+        case (COMMAND_UNRECOGNIZED):
+          printf("Unrecognized command '%s'\n", input_buffer->buffer);
+          continue;
+      }
     }
+
+    Statement statement;
+    switch (prepare_statement(input_buffer, &statement)) {
+      case (STATEMENT_SUCCESS):
+        break;
+      case (STATEMENT_UNRECOGNIZED):
+        printf("Unrecognized keyword at start of '%s'.\n",
+               input_buffer->buffer);
+        continue;
+    }
+
+    execute_statement(&statement);
+    printf("Executed.\n");
  }
}
