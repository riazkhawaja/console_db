#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
#define TABLE_MAX_PAGES 100

// Row with id, username, and email to identify a user.
typedef struct {
  uint32_t id;
  char username[COLUMN_USERNAME_SIZE];
  char email[COLUMN_EMAIL_SIZE];
} Row;

// Get size of columns as well as offsets
const uint32_t ID_SIZE = sizeof(((Row*)0)->id);
const uint32_t USERNAME_SIZE = sizeof(((Row*)0)->username);
const uint32_t EMAIL_SIZE = sizeof(((Row*)0)->email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t PAGE_SIZE = 4096; // 4 KB
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct {
  uint32_t num_rows;
  void* pages[TABLE_MAX_PAGES];
} Table;

typedef struct {
  char * buffer;
  size_t buffer_length;
  ssize_t input_length;
} InputBuffer;

typedef enum {
  COMMAND_SUCCESS,
  COMMAND_UNRECOGNIZED
} CommandResult;

typedef enum { 
  STATEMENT_INSERT, 
  STATEMENT_SELECT 
} StatementType;

typedef struct {
  StatementType type;
  Row row_to_insert;
} Statement;

typedef enum {
  PREPARE_STATEMENT_SUCCESS,
  PREPARE_SYNTAX_INCORRECT,
  PREPARE_STATEMENT_UNRECOGNIZED
} PrepareStatementResult;

typedef enum { 
  EXECUTE_SUCCESS, 
  EXECUTE_TABLE_FULL,
  EXECUTE_ERROR
}  ExecuteResult;

// convert row to serialized representation
void serialize_row(Row* source, void* dest) {
  memcpy(dest + ID_OFFSET, &(source->id), ID_SIZE);
  memcpy(dest + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
  memcpy(dest + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

// convert row to deserialized representation
void deserialize_row(void* source, Row* dest) {
  memcpy(&(dest->id), source + ID_OFFSET, ID_SIZE);
  memcpy(&(dest->username), source + USERNAME_OFFSET, USERNAME_SIZE);
  memcpy(&(dest->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

// fetch a pointer to the row from memory
void* row_slot(Table* table, uint32_t row_num) {
  uint32_t page_num = row_num / ROWS_PER_PAGE;
  void* page = table->pages[page_num];
  if (page == NULL) { // allocate memory for page if it doesn't exist
    page = table->pages[page_num] = malloc(PAGE_SIZE);
  }
  uint32_t row_offset = row_num % ROWS_PER_PAGE;
  uint32_t byte_offset = row_offset * ROW_SIZE;
  return page + byte_offset;
}
  
InputBuffer * create_input_buffer() {
  InputBuffer * input_buffer = (InputBuffer * ) malloc(sizeof(InputBuffer));
  input_buffer -> buffer = NULL;
  input_buffer -> buffer_length = 0;
  input_buffer -> input_length = 0;
  return input_buffer;
}

// Read from input stream (standard input). Remove the last byte from stdin ("\n" newline character)
void get_input(InputBuffer * input_buffer) {
  ssize_t bytes_read = getline( & (input_buffer -> buffer), & (input_buffer -> buffer_length), stdin);
  if (bytes_read <= 0) {
    printf("Error when reading input!\n");
    exit(EXIT_FAILURE);
  }
  input_buffer -> input_length = bytes_read - 1;
  input_buffer -> buffer[bytes_read - 1] = 0;
}

// Free the memory used by the input buffer
void close_input_buffer(InputBuffer * input_buffer) {
  free(input_buffer -> buffer);
  free(input_buffer);
}

// !exit or !quit can be used to terminate the program.
CommandResult command(InputBuffer * input_buffer) {
  if (strcmp(input_buffer -> buffer, "!exit") == 0 || strcmp(input_buffer -> buffer, "!quit") == 0) {
    exit(EXIT_SUCCESS);
  } else {
    return COMMAND_UNRECOGNIZED;
  }
}

PrepareStatementResult prepare_statement(InputBuffer * input_buffer,
  Statement * statement) {
  if (strncmp(input_buffer -> buffer, "insert", 6) == 0) {
    statement -> type = STATEMENT_INSERT;
    int no_of_args = sscanf(input_buffer->buffer, "insert %d %s %s", &(statement->row_to_insert.id), statement->row_to_insert.username, statement->row_to_insert.email);
    if (no_of_args < 3) return PREPARE_SYNTAX_INCORRECT;
    return PREPARE_STATEMENT_SUCCESS;
  }
  if (strcmp(input_buffer -> buffer, "select") == 0) {
    statement -> type = STATEMENT_SELECT;
    return PREPARE_STATEMENT_SUCCESS;
  }
  return PREPARE_STATEMENT_UNRECOGNIZED;
}

ExecuteResult execute_insert(Statement* statement, Table* table) {
  if (table->num_rows >= TABLE_MAX_ROWS) return EXECUTE_TABLE_FULL;
  Row* row_to_insert = &(statement->row_to_insert);
  
  serialize_row(row_to_insert, row_slot(table, table->num_rows));
  table->num_rows += 1;
  
  return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement* statement, Table* table) {
  Row row;
  for (uint32_t i = 0; i < table->num_rows; i++) {
    deserialize_row(row_slot(table, i), &row);
    print_row(&row);
  }
  return EXECUTE_SUCCESS;
}
  
void execute_statement(Statement * statement, Table *table) {
  switch (statement -> type) {
  case (STATEMENT_INSERT):
    return execute_insert(statement, table);
  case (STATEMENT_SELECT):
    return execute_select(statement, table);
  }
}

Table* create_table() {
  Table* table = (Table*)malloc(sizeof(Table));
  table->num_rows = 0;
  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
     table->pages[i] = NULL;
  }
  return table;
}

void free_table(Table* table) {
  for (int i = 0; table->pages[i]; i++) {
	  free(table->pages[i]);
  }
  free(table);
}

// Main program loop. 
int main(int argc, char * argv[]) {
  Table* table = create_table();
  InputBuffer * input_buffer = create_input_buffer();
  while (true) {
    printf("db > ");
    get_input(input_buffer);

    // Non-SQL commands should start with an exclamation mark "!".
    if (input_buffer -> buffer[0] == '!') {
      switch (command(input_buffer)) {
      case (COMMAND_SUCCESS):
        continue;
      case (COMMAND_UNRECOGNIZED):
        printf("Unrecognized command '%s'\n", input_buffer -> buffer);
        continue;
      }
    }

    Statement statement;
    switch (prepare_statement(input_buffer, & statement)) {
    case (PREPARE_STATEMENT_SUCCESS):
      break;
    case (PREPARE_SYNTAX_INCORRECT):
      printf("Syntax error.\n");
      continue;
    case (PREPARE_STATEMENT_UNRECOGNIZED):
      printf("Unrecognized keyword at start of command: '%s'.\n", input_buffer -> buffer);
      continue;
    }


    switch (execute_statement(&statement, table)) {
      case (EXECUTE_SUCCESS):
        printf("Executed.\n");
        break;
      case (EXECUTE_TABLE_FULL):
        printf("Error: Table full.\n");
        break;
      case (EXECUTE_ERROR):
        printf("Error.\n");
        break;
    }
  }
}
