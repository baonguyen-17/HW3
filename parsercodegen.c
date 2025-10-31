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

enum { 
    LIT = 1, 
    OPR = 2, 
    LOD = 3, 
    STO = 4, 
    CAL = 5, 
    INC = 6, 
    JMP = 7, 
    JPC = 8, 
    SYS = 9 
};

typedef struct
{
    int op;
    int l;
    int m;
} instruction;

#define MAX_CODE 2000
instruction pmCodes[MAX_CODE];
int cx = 0;

void emit(int op, int l, int m)
{
    pmCodes[cx].op = op;
    pmCodes[cx].l  = l;
    pmCodes[cx].m  = m;
    cx++;
}

const char* mnem(int op)
{
    switch (op)
    {
        case LIT: return "LIT";
        case OPR: return "OPR";
        case LOD: return "LOD";
        case STO: return "STO";
        case CAL: return "CAL";
        case INC: return "INC";
        case JMP: return "JMP";
        case JPC: return "JPC";
        case SYS: return "SYS";
        default:  return "???";
    }
}

#define MAX_SYMBOL_TABLE_SIZE 500

typedef struct {
int kind; // const = 1, var = 2, proc = 3
char name[12]; // name up to 11 chars
int val; // number (ASCII value)
int level; // L level
int addr; // M address
int mark; // to indicate unavailable or deleted
} symbol;

symbol symtab[MAX_SYMBOL_TABLE_SIZE];
int symcount = 0;
int nextAddr = 3; // RA, SL, DL

// Prints error to terminal and elf.txt
void printError(const char *msg)
{
    printf("Error: %s\n", msg);

    FILE *elf = fopen("elf.txt", "w");
    if (elf != NULL)
    {
        fprintf(elf, "Error: %s", msg);
        fclose(elf);
    }

    exit(0);
}

// Prototypes
void program  (int tokens[], char lexemes[][12], int count, int *pos);
void block    (int tokens[], char lexemes[][12], int count, int *pos);
void const_declaration(int tokens[], char lexemes[][12], int count, int *pos);
int  var_declaration  (int tokens[], char lexemes[][12], int count, int *pos);
void statement        (int tokens[], char lexemes[][12], int count, int *pos);
void condition        (int tokens[], char lexemes[][12], int count, int *pos);
void expression       (int tokens[], char lexemes[][12], int count, int *pos);
void term             (int tokens[], char lexemes[][12], int count, int *pos);
void factor           (int tokens[], char lexemes[][12], int count, int *pos);

void program(int tokens[], char lexemes[][12], int count, int *pos)
{

    emit(JMP, 0, 3); // First instruction must be JMP 0 3

    block(tokens, lexemes, count, pos);

    // End . check
    if (*pos >= count || tokens[*pos] != periodsym)
    {
        printError("program must end with period");
    }

    (*pos)++;

    emit(SYS, 0, 3); // end of program halt
}

void block(int tokens[], char lexemes[][12], int count, int *pos)
{
    int numVars;

    const_declaration(tokens, lexemes, count, pos);
    numVars = var_declaration(tokens, lexemes, count, pos);

    // Allocate local variables
    emit(INC, 0, 3 + numVars);

    
    statement(tokens, lexemes, count, pos);
}

void const_declaration(int tokens[], char lexemes[][12], int count, int *pos)
{
    if (*pos < count && tokens[*pos] == constsym)
    {
        // Increment from constant
        (*pos)++;

        // Identifier error check
        if (*pos >= count || tokens[*pos] != identsym)
        {
            printError("const, var, and read keywords must be followed by identifier");
        }

        do
        {
            char namebuf[12];

            // Check symbol table if symbol was already declared
            for (int i = symcount - 1; i >= 0; i--)
            {
                if (symtab[i].mark == 0 && strcmp(symtab[i].name, lexemes[*pos]) == 0)
                {
                    printError("symbol name has already been declared");
                }
            }

            // Save symbol name
            strncpy(namebuf, lexemes[*pos], 11);
            namebuf[11] = '\0';

            (*pos)++;

            // Const must be with = error check
            if (*pos >= count || tokens[*pos] != eqsym)
            {
                printError("constants must be assigned with =");
            }
            (*pos)++;

            // Const must be assigned an integer
            if (*pos >= count || tokens[*pos] != numbersym)
            {
                printError("constants must be assigned an integer value");
            }

            // Insert symbol into symbol table
            if (symcount >= MAX_SYMBOL_TABLE_SIZE)
            {
                printError("internal symbol table overflow");
            }
            symtab[symcount].kind  = 1;
            strncpy(symtab[symcount].name, namebuf, 11);
            symtab[symcount].name[11] = '\0';
            symtab[symcount].val   = atoi(lexemes[*pos]);
            symtab[symcount].level = 0;
            symtab[symcount].addr  = 0;
            symtab[symcount].mark  = 0;
            symcount++;

            (*pos)++;

            // If comma is next, increment again and loop
            if (*pos < count && tokens[*pos] == commasym)
            {
                (*pos)++;

                if (*pos >= count || tokens[*pos] != identsym)
                {
                    printError("const, var, and read keywords must be followed by identifier");
                }
            }
            
        }
        while (tokens[*pos - 1] == commasym);

        // Declaration must end with ; error check
        if (*pos >= count || tokens[*pos] != semicolonsym)
        {
            printError("constant and variable declarations must be followed by a semicolon");
        }
        (*pos)++;
    }
}


int var_declaration(int tokens[], char lexemes[][12], int count, int *pos)
{
    int numVars = 0;
    int i;

    if (*pos >= count)
    {
        return 0;
    }

    if (tokens[*pos] != varsym)
    {
        return 0;
    }

    // Increment
    (*pos)++;

    // Identifier error check
    if (*pos >= count || tokens[*pos] != identsym)
    {
        printError("const, var, and read keywords must be followed by identifier");
    }

    do
    {
        
        // Check symbol table if symbol was already declared
        for (i = symcount - 1; i >= 0; i--)
        {
                if (strcmp(symtab[i].name, lexemes[*pos]) == 0)
                {
                    printError("symbol name has already been declared");
                }
        }

        // Insert variable into table

        symtab[symcount].kind  = 2;
        strncpy(symtab[symcount].name, lexemes[*pos], 11);
        symtab[symcount].name[11] = '\0';
        symtab[symcount].val   = 0;
        symtab[symcount].level = 0;
        symtab[symcount].addr  = nextAddr;
        symtab[symcount].mark  = 0;

        symcount++;
        nextAddr++;
        numVars++;

        (*pos)++;

        // If next token is comma, increment again and loop
        if (*pos < count && tokens[*pos] == commasym)
        {
            (*pos)++;
            if (*pos >= count || tokens[*pos] != identsym)
            {
                printError("const, var, and read keywords must be followed by identifier");
            }
        }

    } while (*pos < count && tokens[*pos - 1] == commasym);

    // Declaration must end with ; error check
    if (*pos >= count || tokens[*pos] != semicolonsym)
    {
        printError("constant and variable declarations must be followed by a semicolon");
    }

    (*pos)++;

    return numVars;
}

void statement(int tokens[], char lexemes[][12], int count, int *pos)
{
    int symIndex;
    int jpcIndex;
    int loopIndex;

    if (*pos >= count)
    {
        return;
    }

    // Identifier grammer check
    if (tokens[*pos] == identsym)
    {
        symIndex = -1; // -1 means not found

        // Checks if identifier exists
        for (int i = symcount - 1; i >= 0; i--)
        {
                // Compares symbol entry to identifier from tokens.txt
                if (strcmp(symtab[i].name, lexemes[*pos]) == 0)
                {
                    symIndex = i;
                    break;
                }
        }

        // If not found, undeclared identifier error check
        if (symIndex == -1)
        {
            printError("undeclared identifier");
        }

        // If found but a variable, variable VALUE error check
        if (symtab[symIndex].kind != 2)
        {
            printError("only variable values may be altered");
        }

        (*pos)++;

        // Variable needs := for assignment, := error check
        if (tokens[*pos] != becomessym)
        {
            printError("assignment statements must use :=");
        }

        (*pos)++;
        expression(tokens, lexemes, count, pos);

        // If made it here, mark variable as used for
        emit(STO, 0, symtab[symIndex].addr);
        symtab[symIndex].mark = 1;

        return;
    }

    // begin-end grammar check
    if (tokens[*pos] == beginsym)
    {
        (*pos)++;

        // Reads everything in between begin and end
        statement(tokens, lexemes, count, pos);

        // keep checking the in between if semicolon found
        while (*pos < count && tokens[*pos] == semicolonsym)
        {
            (*pos)++;
            statement(tokens, lexemes, count, pos);
        }

        // begin-end error check
        if (*pos >= count || tokens[*pos] != endsym)
        {
            printError("begin must be followed by end");
        }

        (*pos)++;
        return;
    }

    // if-fi grammar check
    if (tokens[*pos] == ifsym)
    {
        (*pos)++;

        // Check in between if-fi statement
        condition(tokens, lexemes, count, pos);

        // Emit now in case we don't jump
        int jpcIndex = cx;
        emit(JPC, 0, 0);

        // if-then error check
        if (*pos >= count || tokens[*pos] != thensym)
        {
            printError("if must be followed by then");
        }
        (*pos)++;

        statement(tokens, lexemes, count, pos);

        // if-fi error check
        if (*pos >= count || tokens[*pos] != fisym)
        {
            printError("if must be followed by fi");
        }
        (*pos)++;

        // Now we know where to jump so we change M
        pmCodes[jpcIndex].m = cx * 3;
        return;
    }

    // while-do grammar check
    if (tokens[*pos] == whilesym)
    {
        (*pos)++;

        loopIndex = cx;

        // Check what's in between the while-do statement
        condition(tokens, lexemes, count, pos);

        // while-do error check
        if (*pos >= count || tokens[*pos] != dosym)
        {
            printError("while must be followed by do");
        }

        (*pos)++;

        jpcIndex = cx;
        emit(JPC, 0, 0); // jump if false

        statement(tokens, lexemes, count, pos); // check statement

        emit(JMP, 0, loopIndex); // loop again

        pmCodes[jpcIndex].m = cx * 3; // Rewrite M for JPC
        return;
    }

    // read grammar check
    if (tokens[*pos] == readsym)
    {
        (*pos)++;

        // const, var, read identifier error check
        if (*pos >= count || tokens[*pos] != identsym)
        {
            printError("const, var, and read keywords must be followed by identifier");
        }

        symIndex = -1; // -1 for not found

        // Checks if identifier exists
        for (int i = symcount - 1; i >= 0; i--)
        {
                // Compares symbol entry to identifier from tokens.txt
                if (strcmp(symtab[i].name, lexemes[*pos]) == 0)
                {
                    symIndex = i;
                    break;
                }
        }

        // If not found, undeclared identifier error check
        if (symIndex == -1)
        {
            printError("undeclared identifier");
        }

        // If found but a variable, variable VALUE error check
        if (symtab[symIndex].kind != 2)
        {
            printError("only variable values may be altered");
        }

        emit(SYS, 0, 2); // Read integer
        emit(STO, 0, symtab[symIndex].addr); // pop from stack and store into variable
        symtab[symIndex].mark = 1; // mark as used

        (*pos)++;
        return;
    }

    // write grammar check
    if (tokens[*pos] == writesym)
    {
        (*pos)++;

        expression(tokens, lexemes, count, pos);
        emit(SYS, 0, 1); // write integer

        return;
    }
}

void condition(int tokens[], char lexemes[][12], int count, int *pos)
{
    int opToken;

    // Condition error check
    if (*pos >= count)
    {
        printError("condition must contain comparison operator");
    }

    //"even" <expression> 
    if (tokens[*pos] == evensym)
    {
        (*pos)++;

        expression(tokens, lexemes, count, pos);
        emit(OPR, 0, 11);
        return;
    }

    //<expression> <rel-op> <expression>
    expression(tokens, lexemes, count, pos);

    if (*pos >= count)
    {
        printError("condition must contain comparison operator");
    }

    opToken = tokens[*pos];

    if (opToken != eqsym &&
        opToken != neqsym &&
        opToken != lessym &&
        opToken != leqsym &&
        opToken != gtrsym &&
        opToken != geqsym)
    {
        printError("condition must contain comparison operator");
    }

    (*pos)++;

    expression(tokens, lexemes, count, pos);

    if (opToken == eqsym)
    {
        emit(OPR, 0, 5);
    }
    else if (opToken == neqsym)
    {
        emit(OPR, 0, 6);
    }
    else if (opToken == lessym)
    {
        emit(OPR, 0, 7);
    }
    else if (opToken == leqsym)
    {
        emit(OPR, 0, 8);
    }
    else if (opToken == gtrsym)
    {
        emit(OPR, 0, 9);
    }
    else
    {
        emit(OPR, 0, 10);
    }
}

void expression(int tokens[], char lexemes[][12], int count, int *pos)
{
    int hadLeadingMinus;

    hadLeadingMinus = 0;

    // + / - <term> grammar check
    if (*pos < count && tokens[*pos] == plussym)
    {
        (*pos)++;
    }
    else if (*pos < count && tokens[*pos] == minussym)
    {
        (*pos)++;
        hadLeadingMinus = 1;

        emit(LIT, 0, 0);
        if (*pos >= count)
        {
            printError("arithmetic equations must contain operands, parentheses, numbers, or symbols");
        }
        term(tokens, lexemes, count, pos);
        emit(OPR, 0, 2);
    }

    // If we didn’t already parse a term via unary -, parse the first term now
    if (!hadLeadingMinus)
    {
        if (*pos >= count)
        {
            printError("arithmetic equations must contain operands, parentheses, numbers, or symbols");
        }
        term(tokens, lexemes, count, pos);
    }

    while (tokens[*pos] == plussym || tokens[*pos] == minussym)
    {
        int op;

        op = tokens[*pos];
        (*pos)++;

        if (*pos >= count)
        {
            printError("arithmetic equations must contain operands, parentheses, numbers, or symbols");
        }

        term(tokens, lexemes, count, pos);

        if (op == plussym)
        {
            emit(OPR, 0, 1);
        }
        else
        {
            emit(OPR, 0, 2); 
        }
    }
}

void term(int tokens[], char lexemes[][12], int count, int *pos)
{
    if (*pos >= count)
    {
        printError("arithmetic equations must contain operands, parentheses, numbers, or symbols");
    }

    factor(tokens, lexemes, count, pos);

    while (*pos < count && (tokens[*pos] == multsym || tokens[*pos] == slashsym))
    {
        int op;

        op = tokens[*pos];
        (*pos)++;

        if (*pos >= count)
        {
            printError("arithmetic equations must contain operands, parentheses, numbers, or symbols");
        }

        factor(tokens, lexemes, count, pos);

        if (op == multsym)
        {
            emit(OPR, 0, 3);
        }
        else
        {
            emit(OPR, 0, 4);
        }
    }
}

void factor(int tokens[], char lexemes[][12], int count, int *pos)
{
    int i;

    if (*pos >= count)
    {
        printError("arithmetic equations must contain operands, parentheses, numbers, or symbols");
    }

    if (tokens[*pos] == identsym)
    {

        int idx = -1; // -1 for not found

        // Checks if identifier exists
        for (int i = symcount - 1; i >= 0; i--)
        {
                // Compares symbol entry to identifier from tokens.txt
                if (strcmp(symtab[i].name, lexemes[*pos]) == 0)
                {
                    idx = i;
                    break;
                }
        }

        // If not found, undeclared identifier error check
        if (idx == -1)
        {
            printError("undeclared identifier");
        }

        if (symtab[idx].kind == 1)
        {
            emit(LIT, 0, symtab[idx].val);
            symtab[idx].mark = 1;
        }
        else if (symtab[idx].kind == 2)
        {
            emit(LOD, 0, symtab[idx].addr);
            symtab[idx].mark = 1;
        }

        (*pos)++;
        return;
    }

    if (tokens[*pos] == numbersym)
    {
        int value;

        value = atoi(lexemes[*pos]);
        emit(LIT, 0, value);

        (*pos)++;
        return;
    }

    // parantheses grammar check
    if (tokens[*pos] == lparentsym)
    {
        (*pos)++;

        // Get the expression inbetween parantheses
        expression(tokens, lexemes, count, pos);

        // parantheses error check
        if (*pos >= count)
        {
            printError("right parenthesis must follow left parenthesis");
        }

        if (tokens[*pos] != rparentsym)
        {
            printError("right parenthesis must follow left parenthesis");
        }

        (*pos)++;
        return;
    }

    // else error
    printError("arithmetic equations must contain operands, parentheses, numbers, or symbols");
}

int main() {
    FILE* ptr;
    ptr = fopen("tokens.txt", "r");
    if (ptr == NULL) {
        perror("Error reading the file!");
        return 1;
    }

    int tokens[600];
    char lexemes[600][12];
    int count = 0;
    int pos = 0;

    // Arrays for declared identifiers
    char declaredIdent[500][12];
    int declaredCount = 0;
    
    while (count < 600 && fscanf(ptr, "%d", &tokens[count]) == 1) 
    {

        lexemes[count][0] = '\0';

        // If 2 or 3, read identifier next to it
        if (tokens[count] == identsym || tokens[count] == numbersym)
        {
            if (fscanf(ptr, "%11s", lexemes[count]) != 1)
            {
                printError("Scanning error detected by lexer (skipsym present)");

            }

        }

        if (tokens[count] == skipsym)
        {
            printError("Scanning error detected by lexer (skipsym present)");
        }

        count++;
    }
    fclose(ptr);

    // Start translating tokens
    program(tokens, lexemes, count, &pos);

    // Prints out Aseembly Code to terminal
    printf("Assembly Code: \n\n");

    printf("Line\t%4s %4s %4s \n", "OP", "L", "M");
    for (int i = 0; i < cx; i++)
    {
        printf("%3d %8s %4d %4d\n", i , mnem(pmCodes[i].op) , pmCodes[i].l , pmCodes[i].m);
    }

    // Write OP L M to elf.txt
    FILE *elf = fopen("elf.txt", "w");
    if (elf == NULL)
    {
        perror("elf.txt");
        return 1;
    }
    for (int i = 0; i < cx; i++)
    {
        fprintf(elf, "%d %d %d\n", pmCodes[i].op, pmCodes[i].l, pmCodes[i].m);
    }
    fclose(elf);

    printf("Symbol Table:\n\n");

    printf("Kind | Name       | Value | Level | Address | Mark\n");
    printf("---------------------------------------------------\n");

    for (int i = 0; i < symcount; i++)
    {

        printf("%4d | %10s | %5d | %5d | %7d | %4d\n", symtab[i].kind, symtab[i].name,symtab[i].val,
               symtab[i].level,
               symtab[i].addr,
               symtab[i].mark);
    }

    return 0;
}