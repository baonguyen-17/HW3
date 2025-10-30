/*
Assignment:
HW3 - Parser and Code Generator for PL/0
Author(s): Heidreb Tahamid, Bao Nguyen
Language: C (only)
To Compile:
Scanner:
gcc -O2 -std=c11 -o lex lex.c
Parser/Code Generator:
gcc -O2 -std=c11 -o parsercodegen parsercodegen.c
To Execute (on Eustis):
./lex <input_file.txt>
./parsercodegen
where:
<input_file.txt> is the path to the PL/0 source program
Notes:
- lex.c accepts ONE command-line argument (input PL/0 source file)
- parsercodegen.c accepts NO command-line arguments
- Input filename is hard-coded in parsercodegen.c
- Implements recursive-descent parser for PL/0 grammar
- Generates PM/0 assembly code (see Appendix A for ISA)
- All development and testing performed on Eustis
Class: COP3402 - System Software - Fall 2025
Instructor: Dr. Jie Lin
Due Date: Friday, October 31, 2025 at 11:59 PM ET
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_SYMBOL_TABLE_SIZE 500
typedef struct {
int kind; // const = 1, var = 2, proc = 3
char name[12]; // name up to 11 chars
int val; // number (ASCII value)
int level; // L level
int addr; // M address
int mark; // to indicate unavailable or deleted
} symbol;

int main() {
    FILE* ptr;
    ptr = fopen("tokens.txt", "r");
    if (ptr == NULL) {
        perror("Error reading the file!");
        return 1;
    }

    int tokens[600];
    char lexemes[600][12];
    int token = 0;
    int count = 0;
    
    while (count < 600 && fscanf(ptr, "%d", &token) == 1) 
    {
        tokens[count] = token;

        lexemes[count][0] = '\0';
        
        // If 2 or 3, read identifier next to it
        if (tokens[count] == 2 || tokens[count] == 3)
        {
            if (fscanf(ptr, "%11s", lexemes[count]) != 1)
            {
                // Prints to terminal
                printf("Error: Scanning error detected by lexer (skipsym present)\n");

                // Prints to file
                FILE *elf = fopen("elf.txt", "w");
                if (elf != NULL)
                {
                    fprintf(elf, "Error: Scanning error detected by lexer (skipsym present)\n");
                    fclose(elf);
                }

                fclose(ptr);
                return 0;
            }
        }

        // Error for skipsym
        if (tokens[count] == 1) 
        {
            // Prints to terminal
            printf("Error: Scanning error detected by lexer (skipsym present)\n");

            // Prints to text file
            FILE *elf = fopen("elf.txt", "w");
            if (elf != NULL)
            {
                fprintf(elf, "Error: Scanning error detected by lexer (skipsym present)\n");
                fclose(elf);
            }

            fclose(ptr);
            return 0;

        }

        // Error for const, var, and read keywords
        if (token == 28 || token == 29 || token == 32)
        {
            int nextToken;
            fscanf(ptr, "%d", &nextToken);

            // If the next token is not an identifier
            if (nextToken != 2 || nextToken != 1)
            {
                printf("Error: const, var, and read keywords must be followed by identifier\n");

                FILE *elf = fopen("elf.txt", "w");
                if (elf != NULL)
                {
                    fprintf(elf, "Error: const, var, and read keywords must be followed by identifier\n");
                    fclose(elf);
                }

                fclose(ptr);
                return 0;
            }

            // Store the identifier token
            tokens[++count] = nextToken;

            // If identifier, read its lexeme
            if (nextToken == 2)
            {
                fscanf(ptr, "%11s", lexemes[count]);
            }
        }
        
        count++;
    }

    int last = tokens[count - 1];
    int prev = tokens[count - 2];

    // End . program check
    if (!(prev == 21 && last == 18))
    {
        if (last == 21)
        {
            printf("Error: program must end with period\n");
            FILE *elf = fopen("elf.txt", "w");
                if (elf != NULL)
                {
                    fprintf(elf, "Error: program must end with period\n");
                    fclose(elf);
                }
        }
        fclose(ptr);
        return 0;
    }

    fclose(ptr);
    return 0;
}