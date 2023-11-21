/*
    COP 3402 Systems Software
    Homework 4 - PL/0 Compiler
    Authored by Caleb Rivera and Matthew Labrada
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#define MAX_IDENTIFIER_LENGTH 11
#define MAX_NUMBER_LENGTH 5
#define MAX_BUFFER_LENGTH 1000
#define MAX_SYMBOL_TABLE_SIZE 500
#define MAX_INSTRUCTION_LENGTH MAX_SYMBOL_TABLE_SIZE

typedef enum
{
  oddsym = 1,   // "odd"
  identsym,     // Identifier
  numbersym,    // Number
  plussym,      // +
  minussym,     // -
  multsym,      // *
  slashsym,     // /
  eqsym,        // =
  neqsym,       // <>
  lessym,       // <
  leqsym,       // <=
  gtrsym,       // >
  geqsym,       // >=
  lparentsym,   // (
  rparentsym,   // )
  commasym,     // ,
  semicolonsym, // ;
  periodsym,    // .
  becomessym,   // :=
  beginsym,     // "begin"
  endsym,       // "end"
  ifsym,        // "if"
  thensym,      // "then"
  whilesym,     // "while"
  dosym,        // "do"
  constsym,     // "const"
  varsym,       // "var"
  writesym,     // "write"
  readsym,      // "read"
  callsym,      // "call"
  procsym,      // "procedure"

} token_type;

typedef struct
{
  char lexeme[MAX_BUFFER_LENGTH + 1]; // String representation of token (Ex: "+", "-", "end")
  char value[MAX_BUFFER_LENGTH + 1];  // Value/Type of token
} token;

typedef struct
{
  token *tokens; // Array of tokens
  int size;      // Current size of list
  int capacity;  // Capacity of list
} list;

typedef struct
{
  int kind;      // const = 1, var = 2, proc = 3
  char name[10]; // name up to 11 chars
  int val;       // number (ASCII value)
  int level;     // L level
  int addr;      // M address
  int mark;      // to indicate unavailable or deleted
} symbol;

typedef struct
{
  int op; // opcode
  int l;  // L
  int m;  // M
} instruction;

list *token_list;                           // Global pointer to list that holds all tokens
FILE *input_file;                           // Input file pointer
FILE *output_file;                          // Output file pointer
symbol symbol_table[MAX_SYMBOL_TABLE_SIZE]; // Global symbol table
instruction code[MAX_INSTRUCTION_LENGTH];   // Global code array
int cx = 0;                                 // Code index
int tx = 0;                                 // Symbol table index
int level = -1;                             // Current level
int dx = 4;                                 // Space for variables
int prev_tx = 0;                            // Previous symbol table index

// Function prototypes
char peekc();
void print_both(const char *format, ...);
void print_source_code();
void clear_to_index(char *str, int index);
int handle_reserved_word(char *buffer);
int handle_special_symbol(char *buffer);
int is_special_symbol(char c);
list *create_list();
list *destroy_list(list *l);
list *append_token(list *l, token t);
void add_token(list *l, token t);
void print_lexeme_table(list *l);
void print_tokens(list *l);

// Parser/Codegen function prototypes
void get_next_token();
void emit(int op, int l, int m);
void error(int error_code);
int check_symbol_table(char *string, int to_add);
void add_symbol(int kind, char *name, int val, int level, int addr, int mark);
void program();
void block();
void const_declaration();
int var_declaration();
void statement();
void condition();
void expression();
void term();
void factor();
void print_symbol_table();
void print_instructions();
void get_op_name(int op, char *name);

// PL/0 Compiler function prototypes
void procedure();
void print_elf_file();

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    print_both("Usage: %s <input file> <output file>\n", argv[0]);
    return 1;
  }

  input_file = fopen(argv[1], "r");
  output_file = fopen(argv[2], "w");

  if (input_file == NULL)
  {
    print_both("Error: Could not open input file %s\n", argv[1]);
    exit(1);
  }

  if (output_file == NULL)
  {
    print_both("Error: Could not open output file %s\n", argv[2]);
    exit(1);
  }

  token_list = create_list();

  char c;
  char buffer[MAX_BUFFER_LENGTH + 1] = {0};
  int buffer_index = 0;

  while ((c = fgetc(input_file)) != EOF)
  {
    if (iscntrl(c) || isspace(c)) // Skip control characters and whitespace
    {
      c = fgetc(input_file);
    }
    if (isdigit(c)) // Handle numbers
    {
      buffer[buffer_index++] = c;
      while (1)
      {
        char nextc = peekc();
        if (isspace(nextc) || is_special_symbol(nextc)) // If next character is a space or special symbol, we've reached the end of the number
        {
          token t;
          if (buffer_index > MAX_NUMBER_LENGTH)
          {
            exit(1);
            // Number is too long
            // print_both("%10s %20s\n", buffer, "ERROR: NUMBER TOO LONG");
          }
          else
          {
            // Number is valid
            // print_both("%10s %20d\n", buffer, numbersym);
            sprintf(t.value, "%d", numbersym);
            strcpy(t.lexeme, buffer);
            append_token(token_list, t);
          }

          // Clear buffer and break out of loop
          clear_to_index(buffer, buffer_index);
          buffer_index = 0;
          break;
        }
        else if (isdigit(nextc))
        {
          // If next character is a digit, add it to the buffer
          c = getc(input_file);
          buffer[buffer_index++] = c;
        }
        else if (nextc == EOF) // This is the last character in the file
          break;
        else if (isalpha(nextc))
        {
          // Invalid number
          token t;
          // print_both("%10s %20d\n", buffer, numbersym);
          sprintf(t.value, "%d", numbersym);
          strcpy(t.lexeme, buffer);
          append_token(token_list, t);
          clear_to_index(buffer, buffer_index);
          buffer_index = 0;
          break;
        }
      }
    }
    else if (isalpha(c)) // Handle identifiers and reserved words
    {
      buffer[buffer_index++] = c;
      while (1)
      {
        char nextc = peekc();
        if (isspace(nextc) || is_special_symbol(nextc) || nextc == EOF) // If next character is a space or special symbol, we've reached the end of the identifier
        {
          // Check reserved words
          int token_value = handle_reserved_word(buffer);
          if (token_value)
          {
            token t;
            // print_both("%10s %20d\n", buffer, token_value);
            sprintf(t.value, "%d", token_value);
            strcpy(t.lexeme, buffer);
            append_token(token_list, t);
            clear_to_index(buffer, buffer_index);
            buffer_index = 0;
            break;
          }
          else
          {
            // Identifier
            token t;
            if (buffer_index > MAX_IDENTIFIER_LENGTH) // Check if identifier is too long
            {
              exit(1);
              // print_both("%10s %20s\n", buffer, "ERROR: IDENTIFIER TOO LONG");
            }
            else
            {
              // Valid identifier
              // print_both("%10s %20d\n", buffer, identsym);
              sprintf(t.value, "%d", identsym);
              strcpy(t.lexeme, buffer);
              append_token(token_list, t);
            }

            clear_to_index(buffer, buffer_index);
            buffer_index = 0;
            break;
          }
          break;
        }
        else if (isalnum(nextc)) // If next character is a letter or digit, add it to the buffer
        {
          c = getc(input_file);
          buffer[buffer_index++] = c;
        }
      }
    }
    else if (is_special_symbol(c)) // Handle special symbols
    {
      buffer[buffer_index++] = c;
      char nextc = peekc();

      if (is_special_symbol(nextc)) // Current character is special and so is the next
      {
        // Handle block comments
        if (c == '/' && nextc == '*')
        {
          // Clear buffer
          clear_to_index(buffer, buffer_index);
          buffer_index = 0;
          while (1) // Consume characters until we reach the end of the block comment
          {
            c = getc(input_file);
            nextc = peekc();
            if (c == '*' && nextc == '/')
            {
              c = getc(input_file);
              break;
            }
          }
          continue;
        }

        // Handle single line comments
        if (c == '/' && nextc == '/')
        {
          // Clear buffer
          clear_to_index(buffer, buffer_index);
          buffer_index = 0;
          while (1) // Consume characters until we reach the end of the line
          {
            c = getc(input_file);
            nextc = peekc();
            if (c == '\n')
            {
              break;
            }
          }
          continue;
        }

        // Handle special case where the second symbol is a semicolon
        if (nextc == ';')
        {
          // Check if first symbol is a valid symbol
          token t;
          int token_value = handle_special_symbol(buffer);
          if (!token_value)
          {
            exit(1);
          }

          // Append first symbol to token list
          sprintf(t.value, "%d", token_value);
          strcpy(t.lexeme, buffer);
          append_token(token_list, t);

          // Append semicolon to token list
          sprintf(t.value, "%d", semicolonsym);
          strcpy(t.lexeme, ";");
          append_token(token_list, t);

          clear_to_index(buffer, buffer_index);
          buffer_index = 0;

          continue;
        }

        // We have two pontentially valid symbols, so we need to check if they make a valid symbol
        c = getc(input_file);
        buffer[buffer_index++] = c;

        token t;
        int token_value = handle_special_symbol(buffer);
        if (!token_value)
        {
          // All symbols are invalid
          // for (int i = 0; i < buffer_index; i++)
          //   print_both("%10c %20s\n", buffer[i], "ERROR: INVALID SYMBOL");
          exit(1);
        }
        else
        {
          // Both symbols make a valid symbol
          // print_both("%10s %20d\n", buffer, token_value);
          sprintf(t.value, "%d", token_value);
          strcpy(t.lexeme, buffer);
          append_token(token_list, t);
        }

        clear_to_index(buffer, buffer_index);
        buffer_index = 0;
      }
      else
      {
        // Handle single special symbol
        token t;
        int token_value = handle_special_symbol(buffer);
        if (!token_value)
        {
          exit(1);
          // print_both("%10c %20s\n", c, "ERROR: INVALID SYMBOL");
        }
        else
        {
          // print_both("%10s %20d\n", buffer, token_value);
          sprintf(t.value, "%d", token_value);
          strcpy(t.lexeme, buffer);
          append_token(token_list, t);
        }

        clear_to_index(buffer, buffer_index);
        buffer_index = 0;
      }
    }
  }

  // Read in tokens in the tokens list and generate code
  program();

  destroy_list(token_list); // Free memory used by token list
  fclose(input_file);       // Close input file
  fclose(output_file);      // Close output file
  return 0;
}

// Peek at the next character from the input file without consuming it
char peekc()
{
  int c = getc(input_file);
  ungetc(c, input_file);
  return (char)c;
}

// Print formatted output to both the console and the output file
void print_both(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);

  va_start(args, format);
  vfprintf(output_file, format, args);
  va_end(args);
}

// Print the entire source code from the input file to both the console and the output file
void print_source_code()
{
  rewind(input_file); // Reset file pointer to the beginning of the file
  char c;
  char lastChar = 0; // To keep track of the last character printed
  while ((c = fgetc(input_file)) != EOF)
  {
    print_both("%c", c);
    lastChar = c;
  }
  if (lastChar != '\n') // If the last character wasn't a newline, print one
    print_both("\n");
  rewind(input_file); // Reset file pointer to the beginning of the file
}

// Clear a string up to a specified index by setting characters to null
void clear_to_index(char *str, int index)
{
  for (int i = 0; i < index; i++)
    str[i] = '\0';
}

// Check if given buffer matches any reserved word, return its corresponding token value
int handle_reserved_word(char *buffer)
{
  if (strcmp(buffer, "const") == 0)
    return constsym;
  else if (strcmp(buffer, "var") == 0)
    return varsym;
  else if (strcmp(buffer, "begin") == 0)
    return beginsym;
  else if (strcmp(buffer, "end") == 0)
    return endsym;
  else if (strcmp(buffer, "if") == 0)
    return ifsym;
  else if (strcmp(buffer, "then") == 0)
    return thensym;
  else if (strcmp(buffer, "while") == 0)
    return whilesym;
  else if (strcmp(buffer, "do") == 0)
    return dosym;
  else if (strcmp(buffer, "read") == 0)
    return readsym;
  else if (strcmp(buffer, "write") == 0)
    return writesym;
  else if (strcmp(buffer, "procedure") == 0)
    return procsym;
  else if (strcmp(buffer, "call") == 0)
    return callsym;
  else if (strcmp(buffer, "odd") == 0)
    return oddsym;
  return 0; // invalid reserved word
}

// Check if given buffer matches any special symbol, return its corresponding token value
int handle_special_symbol(char *buffer)
{
  if (strcmp(buffer, "+") == 0)
    return plussym;
  else if (strcmp(buffer, "-") == 0)
    return minussym;
  else if (strcmp(buffer, "*") == 0)
    return multsym;
  else if (strcmp(buffer, "/") == 0)
    return slashsym;
  else if (strcmp(buffer, "(") == 0)
    return lparentsym;
  else if (strcmp(buffer, ")") == 0)
    return rparentsym;
  else if (strcmp(buffer, ",") == 0)
    return commasym;
  else if (strcmp(buffer, ";") == 0)
    return semicolonsym;
  else if (strcmp(buffer, ".") == 0)
    return periodsym;
  else if (strcmp(buffer, "=") == 0)
    return eqsym;
  else if (strcmp(buffer, "<") == 0)
    return lessym;
  else if (strcmp(buffer, ">") == 0)
    return gtrsym;
  else if (strcmp(buffer, ":=") == 0)
    return becomessym;
  else if (strcmp(buffer, "<=") == 0)
    return leqsym;
  else if (strcmp(buffer, ">=") == 0)
    return geqsym;
  else if (strcmp(buffer, "<>") == 0)
    return neqsym;
  return 0; // invalid special symbol
}

// Check if a given character is a special symbol
int is_special_symbol(char c)
{
  // If character matches, return 1 else return 0
  return (c == '+' ||
          c == '-' ||
          c == '*' ||
          c == '/' ||
          c == '(' ||
          c == ')' ||
          c == '=' ||
          c == ',' ||
          c == '.' ||
          c == '<' ||
          c == '>' ||
          c == ':' ||
          c == ';' ||
          c == '&' ||
          c == '%' ||
          c == '!' ||
          c == '@' ||
          c == '#' ||
          c == '$' ||
          c == '?' ||
          c == '^' ||
          c == '`' ||
          c == '~' ||
          c == '|');
}

// Create and initialize new list for storing tokens
list *create_list()
{
  list *l = malloc(sizeof(list));
  l->size = 0;
  l->capacity = 10;
  l->tokens = malloc(sizeof(token) * l->capacity);
  return l;
}

// Free the memory used by a list and its tokens
list *destroy_list(list *l)
{
  free(l->tokens);
  free(l);
  return NULL;
}

// Append a token to a list, resizing the list if necessary
list *append_token(list *l, token t)
{
  if (l->size == l->capacity)
  {
    l->capacity *= 2;
    l->tokens = realloc(l->tokens, sizeof(token) * l->capacity);
  }
  l->tokens[l->size++] = t;
  return l;
}

// Add a token to a list, resizing the list if necessary
void add_token(list *l, token t)
{
  if (l->size == l->capacity)
  {
    l->capacity *= 2;
    l->tokens = realloc(l->tokens, sizeof(token) * l->capacity);
  }
  l->tokens[l->size++] = t;
}

// Print the lexeme table to both the console and output file
void print_lexeme_table(list *l)
{
  for (int i = 0; i < l->size; i++)
    print_both("%10s %20s\n", l->tokens[i].lexeme, l->tokens[i].value);
}

// Print the tokens to both the console and output file
void print_tokens(list *l)
{
  int counter; // Counter to keep track of the number of tokens printed for sake of ommitting extra new line character at end of file

  for (int i = 0; i < l->size; i++)
  {
    print_both("%s ", l->tokens[i].value);
    char identifier_value[3] = {0}, number_value[3] = {0};
    sprintf(identifier_value, "%d", identsym);
    sprintf(number_value, "%d", numbersym);

    // Check if the token value matches identifier or number and print its lexeme
    if (strcmp(l->tokens[i].value, identifier_value) == 0 || strcmp(l->tokens[i].value, number_value) == 0)
      print_both("%s ", l->tokens[i].lexeme);
    counter++;
  }

  // Print a newline if we haven't reached the last token
  if (counter < l->size - 1)
  {
    print_both("\n");
  }
}

// Parser/Codegen stuff
token current_token; // Keep track of current token

// Get next token from token list
void get_next_token()
{
  current_token = token_list->tokens[0];
  for (int i = 0; i < token_list->size - 1; i++)
  {
    token_list->tokens[i] = token_list->tokens[i + 1];
  }
  token_list->size--;
}

// Emit an instruction to the code array
void emit(int op, int l, int m)
{
  if (cx > MAX_INSTRUCTION_LENGTH)
  {
    error(16);
  }
  else
  {
    code[cx].op = op;
    code[cx].l = l;
    code[cx].m = m;
    cx++;
  }
}

// Print an error message and exit
void error(int error_code)
{
  print_both("Error: ");
  switch (error_code)
  {
  case 1:
    print_both("program must end with a period\n");
    break;
  case 2:
    print_both("const, var, and procedure keywords must be followed by identifier\n");
    break;
  case 3:
    print_both("symbol name has already been declared\n");
    break;
  case 4:
    print_both("constants must be assigned with =\n");
    break;
  case 5:
    print_both("constants must be assigned an integer value\n");
    break;
  case 6:
    print_both("constant, variables, and procedure declarations must be followed by a semicolon\n");
    break;
  case 7:
    print_both("undeclared or out of scope identifier %s\n", current_token.lexeme);
    break;
  case 8:
    print_both("only variable values may be altered\n");
    break;
  case 9:
    print_both("assignment statements must use :=\n");
    break;
  case 10:
    print_both("begin must be followed by end\n");
    break;
  case 11:
    print_both("if must be followed by then\n");
    break;
  case 12:
    print_both("while must be followed by do\n");
    break;
  case 13:
    print_both("condition must contain comparison operator\n");
    break;
  case 14:
    print_both("right parenthesis must follow left parenthesis\n");
    break;
  case 15:
    print_both("arithmetic equations must contain operands, parenthesis, numbers, or symbols\n");
    break;
  case 16:
    print_both("program too long\n");
    break;
  case 17:
    print_both("call must be followed by an identifier\n");
    break;
  case 18:
    print_both("cannot call variable or constant\n");
    break;
  }

  exit(1);
}

// Find a symbol in the symbol table
int check_symbol_table(char *string, int to_add)
{
  for (int i = tx - 1; i >= 0; i--)
  {
    if (to_add && strcmp(string, symbol_table[i].name) == 0 && level == symbol_table[i].level)
    {
      return i;
    }
    else if (!to_add && strcmp(string, symbol_table[i].name) == 0 && symbol_table[i].level <= level)
    {
      return i;
    }
  }
  return -1;
}

// Add a symbol to the symbol table
void add_symbol(int kind, char *name, int val, int level, int addr, int mark)
{
  symbol_table[tx].kind = kind;
  strcpy(symbol_table[tx].name, name);
  symbol_table[tx].val = val;
  symbol_table[tx].level = level;
  symbol_table[tx].addr = addr;
  symbol_table[tx].mark = mark;
  tx++;
}

// Parse the program
void program()
{
  get_next_token();
  block();                                    // Parse block
  if (atoi(current_token.value) != periodsym) // Check if program ends with a period
  {
    error(1); // Error if it doesn't
  }
  emit(9, 0, 3); // Emit halt instruction
  print_source_code();
  printf("No errors, program is syntactically correct.\n");
  print_elf_file();
}

void block()
{
  level++;      // Increment level
  prev_tx = tx; // Save previous symbol table index
  dx = 4;       // Reserve space for return value, static link, dynamic link, and return address
  int jx = cx;  // Save current code index to jump to

  emit(7, 0, 0); // Emit JMP instruction

  if (atoi(current_token.value) == constsym)
    const_declaration(); // Parse constants

  if (atoi(current_token.value) == varsym)
    dx += var_declaration(); // Parse variables

  while (atoi(current_token.value) == procsym)
    procedure(); // Parse procedures

  code[jx].m = cx * 3; // Set JMP instruction's M to current code index
  emit(6, 0, dx);      // Emit INC instruction

  statement(); // Parse statement

  if (level > 0)
  {
    emit(2, 0, 0); // Emit RTN instruction
  }

  tx = prev_tx; // Reset symbol table index
  level--;      // Decrement level
}

void procedure()
{
  while (atoi(current_token.value) == procsym)
  {
    get_next_token();
    if (atoi(current_token.value) != identsym) // Check if next token is an identifier
    {
      error(2); // Error if it isn't
    }

    add_symbol(3, current_token.lexeme, 0, level, cx * 3, 0); // Add procedure to symbol table
    get_next_token();
    if (atoi(current_token.value) != semicolonsym) // Check if next token is a semicolon
    {
      error(6); // Error if it isn't
    }

    get_next_token();

    block();                                       // Parse block
    if (atoi(current_token.value) != semicolonsym) // Check if next token is a semicolon
    {
      error(6); // Error if it isn't
    }
    get_next_token();
  }
}

// Parse constants
void const_declaration()
{
  char name[MAX_IDENTIFIER_LENGTH + 1]; // Track name of constant
                                        // Check if current token is a const
  do
  {
    get_next_token();
    if (atoi(current_token.value) != identsym) // Check if next token is an identifier
    {
      error(2); // Error if it isn't
    }
    strcpy(name, current_token.lexeme);                    // Save name of constant
    if (check_symbol_table(current_token.lexeme, 1) != -1) // Check if constant has already been declared
    {
      error(3); // Error if it has
    }
    get_next_token();
    if (atoi(current_token.value) != eqsym) // Check if next token is an equals sign
    {
      error(4); // Error if it isn't
    }
    get_next_token();
    if (atoi(current_token.value) != numbersym) // Check if next token is a number
    {
      error(5); // Error if it isn't
    }
    add_symbol(1, name, atoi(current_token.lexeme), level, 0, 0); // Add constant to symbol table
    get_next_token();
  } while (atoi(current_token.value) == commasym); // Continue parsing constants if next token is a comma
  if (atoi(current_token.value) != semicolonsym)   // Check if next token is a semicolon
  {
    error(6); // Error if it isn't
  }
  get_next_token();
}

// Parse variables
int var_declaration()
{
  int num_vars = 0; // Track number of variables
  do
  {
    num_vars++; // Increment number of variables
    get_next_token();
    if (atoi(current_token.value) != identsym) // Check if next token is an identifier
    {
      error(2);
    }
    if (check_symbol_table(current_token.lexeme, 1) != -1) // Check if variable has already been declared
    {
      error(3); // Error if it has
    }
    add_symbol(2, current_token.lexeme, 0, level, num_vars + 2, 0); // Add variable to symbol table

    get_next_token();
  } while (atoi(current_token.value) == commasym); // Continue parsing variables if next token is a comma
  if (atoi(current_token.value) != semicolonsym)   // Check if next token is a semicolon
  {
    error(6); // Error if it isn't
  }
  get_next_token();

  return num_vars; // Return number of variables
}

// Parse statements
void statement()
{
  if (atoi(current_token.value) == identsym) // Check if current token is an identifier
  {
    int sx = check_symbol_table(current_token.lexeme, 0); // Check if identifier is in symbol table
    if (sx == -1)
    {
      error(7); // Error if it isn't
    }
    if (symbol_table[sx].kind != 2) // Check if identifier is a variable
    {
      error(8); // Error if it isn't
    }
    get_next_token();
    if (atoi(current_token.value) != becomessym) // Check if next token is a becomes symbol (:=)
    {
      error(9); // Error if it isn't
    }
    get_next_token();
    expression();                                                   // Parse expression
    emit(4, level - symbol_table[sx].level, symbol_table[sx].addr); // Emit STO instruction
  }
  else if (atoi(current_token.value) == callsym)
  {
    get_next_token();
    if (atoi(current_token.value) != identsym) // Check if next token is an identifier
    {
      error(17); // Error if it isn't
    }
    int i = check_symbol_table(current_token.lexeme, 0); // Check if identifier is in symbol table
    if (i == -1)
    {
      error(7); // Error if it isn't
    }
    if (symbol_table[i].kind != 3) // Check if identifier is a procedure
    {
      error(18); // Error if it isn't
    }
    emit(5, level - symbol_table[i].level, symbol_table[i].addr); // Emit CAL instruction
    get_next_token();
  }
  else if (atoi(current_token.value) == beginsym) // Check if current token is a begin
  {
    do
    {
      get_next_token();
      statement();                                       // Parse statement
    } while (atoi(current_token.value) == semicolonsym); // Continue parsing statements if next token is a semicolon
    if (atoi(current_token.value) != endsym)             // Check if next token is an end
    {
      error(10); // Error if it isn't
    }
    get_next_token();
  }
  else if (atoi(current_token.value) == ifsym) // Check if current token is an if
  {
    get_next_token();
    condition(); // Parse condition
    int jx = cx;
    emit(8, 0, 0);                            // Emit JPC instruction
    if (atoi(current_token.value) != thensym) // Check if next token is a then
    {
      error(11); // Error if it isn't
    }
    get_next_token();
    statement();         // Parse statement
    code[jx].m = cx * 3; // Set JPC instruction's M to current code index
  }
  else if (atoi(current_token.value) == whilesym) // Check if current token is a while
  {
    get_next_token();
    int lx = cx;
    condition();                            // Parse condition
    if (atoi(current_token.value) != dosym) // Check if next token is a do
    {
      error(12); // Error if it isn't
    }
    get_next_token();
    int jx = cx;         // Save current code index to jump to
    emit(8, 0, 0);       // Emit JPC instruction
    statement();         // Parse statement
    emit(7, 0, lx * 3);  // Emit JMP instruction
    code[jx].m = cx * 3; // Set JPC instruction's M to current code index
  }
  else if (atoi(current_token.value) == readsym) // Check if current token is a read
  {
    get_next_token();
    if (atoi(current_token.value) != identsym) // Check if current token is an identifier
    {
      error(2); // Error if it isn't
    }
    int sx = check_symbol_table(current_token.lexeme, 0); // Check if identifier is in symbol table
    if (sx == -1)
    {
      error(7); // Error if it isn't
    }
    if (symbol_table[sx].kind != 2) // Check if identifier is a variable
    {
      error(8); // Error if it isn't
    }
    get_next_token();
    emit(9, 0, 2);      // Emit SIO instruction
    emit(4, level, sx); // Emit STO instruction
  }
  else if (atoi(current_token.value) == writesym) // Check if current token is a write
  {
    get_next_token();
    expression();  // Parse expression
    emit(9, 0, 1); // Emit SIO instruction
  }
}

// Parse condition
void condition()
{
  if (atoi(current_token.value) == oddsym) // Check if current token is odd
  {
    get_next_token();
    expression();   // Parse expression
    emit(2, 0, 11); // Emit ODD instruction
  }
  else
  {
    expression();                      // Parse expression
    switch (atoi(current_token.value)) // Check if current token is a comparison operator
    {
    case eqsym:
      get_next_token();
      expression();
      emit(2, 0, 5); // Emit EQL instruction
      break;
    case neqsym:
      get_next_token();
      expression();
      emit(2, 0, 6); // Emit NEQ instruction
      break;
    case lessym:
      get_next_token();
      expression();
      emit(2, 0, 7); // Emit LSS instruction
      break;
    case leqsym:
      get_next_token();
      expression();
      emit(2, 0, 8); // Emit LEQ instruction
      break;
    case gtrsym:
      get_next_token();
      expression();
      emit(2, 0, 9); // Emit GTR instruction
      break;
    case geqsym:
      get_next_token();
      expression();
      emit(2, 0, 10); // Emit GEQ instruction
      break;
    default:
      error(13); // Error if it isn't
      break;
    }
  }
}

// Parse expression
void expression()
{
  term(); // Parse term
  // Check if current token is a plus or minus
  while (atoi(current_token.value) == plussym || atoi(current_token.value) == minussym)
  {
    if (atoi(current_token.value) == plussym) // Check if current token is a plus
    {
      get_next_token();
      term();
      emit(2, 0, 1); // Emit ADD instruction
    }
    else
    {
      get_next_token();
      term();
      emit(2, 0, 2); // Emit SUB instruction
    }
  }
}

// Parse term
void term()
{
  factor(); // Parse factor
  while (atoi(current_token.value) == multsym || atoi(current_token.value) == slashsym)
  {
    if (atoi(current_token.value) == multsym) // Check if current token is a multiply
    {
      get_next_token();
      factor();      // Parse factor
      emit(2, 0, 3); // Emit MUL
    }
    else
    {
      get_next_token();
      factor();      // Parse factor
      emit(2, 0, 4); // Emit DIV
    }
  }
}

// Parse factor
void factor()
{
  if (atoi(current_token.value) == identsym) // Check if current token is an identifier
  {
    int sx = check_symbol_table(current_token.lexeme, 0); // Check if identifier is in symbol table
    if (sx == -1)
    {
      error(7); // Error if it isn't
    }
    if (symbol_table[sx].kind == 1) // Check if identifier is a constant
    {
      emit(1, 0, symbol_table[sx].val); // Emit LIT instruction
    }
    else
    {
      emit(3, level - symbol_table[sx].level, symbol_table[sx].addr); // Emit LOD instruction
    }
    get_next_token();
  }
  else if (atoi(current_token.value) == numbersym) // Check if current token is a number
  {
    emit(1, 0, atoi(current_token.lexeme)); // Emit LIT instruction
    get_next_token();
  }
  else if (atoi(current_token.value) == lparentsym) // Check if current token is a left parenthesis
  {
    get_next_token();
    expression();                                // Parse expression
    if (atoi(current_token.value) != rparentsym) // Check if currenet token is right parenthesis
    {
      error(14); // Error if it isn't
    }
    get_next_token();
  }
  else
  {
    error(15); // Error if current token is none of the above
  }
}

// Print symbol table
void print_symbol_table()
{
  print_both("\nSymbol Table:\n");
  print_both("%10s | %10s | %10s | %10s | %10s | %10s\n", "Kind", "Name", "Value", "Level", "Address", "Mark", "\n");
  print_both("    -----------------------------------------------------------------------\n");

  for (int i = 0; i < MAX_SYMBOL_TABLE_SIZE; i++)
  {
    if (symbol_table[i].kind == 0)
      continue;

    symbol_table[i].mark = 1;
    if (symbol_table[i].kind == 1)
      print_both("%10d | %10s | %10d | %10s | %10s | %10d\n", symbol_table[i].kind, symbol_table[i].name, symbol_table[i].val, "-", "-", symbol_table[i].mark);
    else
      print_both("%10d | %10s | %10d | %10d | %10d | %10d\n", symbol_table[i].kind, symbol_table[i].name, symbol_table[i].val, symbol_table[i].level, symbol_table[i].addr, symbol_table[i].mark);
  }
}

// Print assmebly code
void print_instructions()
{

  print_both("Assembly Code:\n");
  print_both("%10s %10s %10s %10s\n", "Line", "OP", "L", "M");
  for (int i = 0; i < cx; i++)
  {
    char name[4];
    get_op_name(code[i].op, name);
    print_both("%10d %10s %10d %10d\n", i, name, code[i].l, code[i].m);
  }
}

// Get op name from op code
void get_op_name(int op, char *name)
{
  switch (op)
  {
  case 1:
    strcpy(name, "LIT");
    break;
  case 2:
    strcpy(name, "OPR");
    break;
  case 3:
    strcpy(name, "LOD");
    break;
  case 4:
    strcpy(name, "STO");
    break;
  case 5:
    strcpy(name, "CAL");
    break;
  case 6:
    strcpy(name, "INC");
    break;
  case 7:
    strcpy(name, "JMP");
    break;
  case 8:
    strcpy(name, "JPC");
    break;
  case 9:
    strcpy(name, "SYS");
    break;
  }
}

void print_elf_file()
{
  FILE *elf_file = fopen("elf.txt", "w");
  for (int i = 0; i < cx; i++)
  {
    fprintf(elf_file, "%d %d %d\n", code[i].op, code[i].l, code[i].m);
  }

  fclose(elf_file);

  for (int i = 0; i < cx; i++)
  {
    printf("%d %d %d\n", code[i].op, code[i].l, code[i].m);
  }
}
