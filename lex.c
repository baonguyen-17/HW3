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


typedef enum {
    skipsym = 1, // Skip/ignore token
    identsym , // Identifier
    numbersym , // Number
    plussym , // +
    minussym , // -
    multsym , // *
    slashsym , // /
    eqsym , // =
    neqsym , // <>
    lessym , // <
    leqsym , // <=
    gtrsym , // >
    geqsym , // >=
    lparentsym , // (
    rparentsym , // )
    commasym , // ,
    semicolonsym , // ;
    periodsym , // .
    becomessym , // :=
    beginsym , // begin
    endsym , // end
    ifsym , // if
    fisym , // fi
    thensym , // then
    whilesym , // while
    dosym , // do
    callsym , // call
    constsym , // const
    varsym , // var
    procsym , // procedure
    writesym , // write
    readsym , // read
    elsesym , // else
    evensym // even
} TokenType;

int checkNumber(const char *s)
{
    size_t n = strlen(s);

    // Returns 0 if number is too long or empty
    if (n == 0 || n > 5)
    {
        return 0;
    }

    //otherwise return number string
    return strspn(s, "0123456789") == n;
}

int checkAlphabet(const char *s)
{
    size_t n = strlen(s);

    // Returns 0 if too long or empty
    if (n == 0 || n > 11)
    {
        return 0;
    }

    if (!(isalpha((unsigned char)s[0])))
    {
        return 0;
    }

    return strspn(s + 1, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_")
           == (n - 1);
}

TokenType lexemeToToken(const char *lex) 
{
    // if String is found return token
    if      (strcmp(lex, "+")  == 0) return plussym;
    else if (strcmp(lex, "-")  == 0) return minussym;
    else if (strcmp(lex, "*")  == 0) return multsym;
    else if (strcmp(lex, "/")  == 0) return slashsym;
    else if (strcmp(lex, "=")  == 0) return eqsym;
    else if (strcmp(lex, "<=") == 0) return leqsym;
    else if (strcmp(lex, "<>") == 0) return neqsym;
    else if (strcmp(lex, "<")  == 0) return lessym;
    else if (strcmp(lex, ">=") == 0) return geqsym;
    else if (strcmp(lex, ">")  == 0) return gtrsym;
    else if (strcmp(lex, "(")  == 0) return lparentsym;
    else if (strcmp(lex, ")")  == 0) return rparentsym;
    else if (strcmp(lex, ",")  == 0) return commasym;
    else if (strcmp(lex, ";")  == 0) return semicolonsym;
    else if (strcmp(lex, ".")  == 0) return periodsym;
    else if (strcmp(lex, ":=") == 0) return becomessym;
    else if (strcmp(lex, "begin") == 0) return beginsym;
    else if (strcmp(lex, "end")   == 0) return endsym;
    else if (strcmp(lex, "if")    == 0) return ifsym;
    else if (strcmp(lex, "fi")    == 0) return fisym;
    else if (strcmp(lex, "then")  == 0) return thensym;
    else if (strcmp(lex, "while") == 0) return whilesym;
    else if (strcmp(lex, "do")    == 0) return dosym;
    else if (strcmp(lex, "call")  == 0) return callsym;
    else if (strcmp(lex, "const") == 0) return constsym;
    else if (strcmp(lex, "var")   == 0) return varsym;
    else if (strcmp(lex, "procedure") == 0) return procsym;
    else if (strcmp(lex, "write") == 0) return writesym;
    else if (strcmp(lex, "read")  == 0) return readsym;
    else if (strcmp(lex, "else")  == 0) return elsesym;
    else if (strcmp(lex, "even")  == 0) return evensym;

    else if (checkNumber(lex))
    {
        return numbersym;
    }

    else if (checkAlphabet(lex))
    {
        return identsym;
    }

    // return if error
    return skipsym;
}

int main(int argc, char **argv){
    FILE* ptr;
    ptr = fopen(argv[1], "r");
    if (ptr == NULL) {
        perror("Error reading the file!");
        return 1;
    }

    int fileLen = 0;
    char ch;
    
    while (ch = fgetc(ptr) != EOF) {
        fileLen++;
    }

    fseek(ptr, 0, SEEK_SET);
    char* buffer = (char*)malloc(sizeof(char) * fileLen + 1);

    if (buffer) {
        int read = fread(buffer, 1, fileLen, ptr);
    }
    
    int i = 0, j = 0, tokenCount = 0, countSaved = 0;
    char lexeme[500];
    TokenType tokens[500];  
    char *saved[600] = {0};

    while (buffer[i] != '\0')
    {
        // If character is a space move to next spot in array
        if (isspace((unsigned char)buffer[i]))
        {
            i++;
            continue;
        }

        // Checks for double character symbols
        if ((buffer[i] == '<' && (buffer[i + 1] == '=' || buffer[i + 1] == '>')) ||
            (buffer[i] == '>' && buffer[i + 1] == '=') ||
            (buffer[i] == ':' && buffer[i + 1] == '='))
        {
            lexeme[0] = buffer[i];
            lexeme[1] = buffer[i+1];
            lexeme[2] = '\0';
            i += 2;

            TokenType t = lexemeToToken(lexeme);

            if (t != skipsym)
            {
                tokens[tokenCount] = t;
                saved[tokenCount] = NULL;
                tokenCount++;
            }
            continue;
        }

        // Checks for comment blocks
        if (buffer[i] == '/' && buffer[i+1] == '*')
        {
            i += 2;  // past "/*"
            while (buffer[i] != '\0')
            {
                if (buffer[i] == '*' && buffer[i+1] == '/')  // found the closing */
                {
                    i += 2;    // move past "*/"
                    break;     // done skipping
                }
                i++;
            }
            continue;          // go scan the next token
        }
        
        // Checks for just ":" if alone
        if (buffer[i] == ':') {
            i++;
            continue;
        }

        // Checks for single character symbols
        if (buffer[i]=='+'||buffer[i]=='-'||buffer[i]=='*'||buffer[i]=='/'||
            buffer[i]=='='||buffer[i]=='('||buffer[i]==')'||buffer[i]==','||
            buffer[i]==';'||buffer[i]=='.'||buffer[i]=='<'||buffer[i]=='>') {
            char s[2]={buffer[i++],'\0'};
            TokenType t = lexemeToToken(s);
            tokens[tokenCount] = t;
            saved[tokenCount] = NULL;
            tokenCount++;
            continue;
        }


        // Checks for integers
        if (isdigit((unsigned char)buffer[i])) 
        {
            int j = 0;
            while (isdigit((unsigned char)buffer[i]) && j + 1 < (int)sizeof(lexeme)) 
            {
                lexeme[j++] = buffer[i++];
            }
            lexeme[j] = '\0';

            // If number too long, skip
            if (!checkNumber(lexeme)) 
            {
                continue;
            }

            // otherwise, valid number
            tokens[tokenCount] = numbersym;
            saved[tokenCount] = (char*)malloc(strlen(lexeme) + 1);
            strcpy(saved[tokenCount], lexeme);
            tokenCount++;
            continue;
        }
        
        // Checks for identifiers
        if (isalpha((unsigned char)buffer[i])) 
        {
            int j = 0;
            while ((isalpha((unsigned char)buffer[i]) || isdigit((unsigned char)buffer[i]) || buffer[i]=='_') &&
                j + 1 < (int)sizeof(lexeme)) {
                lexeme[j++] = buffer[i++];
            }
            lexeme[j] = '\0';

            // If too long, skip
            if (strlen(lexeme) > 11) {
                continue;
            }

            /* now it's safe to ask if it is a reserved word vs identifier */
            TokenType t = lexemeToToken(lexeme);
            tokens[tokenCount] = t;

            if (t == identsym) 
            {
                saved[tokenCount] = (char*)malloc(strlen(lexeme) + 1);
                strcpy(saved[tokenCount], lexeme);
            } else 
            {
                saved[tokenCount] = NULL;
            }
            tokenCount++;
            continue;
        }

        i++; //skip invalid
        
    }
    
    // Creates tokens text file
    FILE *out = fopen("tokens.txt", "w");
    if (!out) {
        fprintf(stderr, "Error: cannot create tokens.txt\n");
        return 1;
    }

    for (int j = 0; j < tokenCount; j++) {
        if (tokens[j] == identsym || tokens[j] == numbersym)
            fprintf(out, "%d %s\n", (int)tokens[j], saved[j]);
        else
            fprintf(out, "%d\n", (int)tokens[j]);
    }
    fclose(out);

    // Memory cleanup
    for (int j = 0; j < tokenCount; j++)
    {
        free(saved[j]);
    }
    free(buffer);
    fclose(ptr);
    return 0;
}


