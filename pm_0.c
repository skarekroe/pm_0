//Benjamin Rogers
//COP3402

//a virtual machine interpreter for PM/0
//note only accepts filenames "mcode.txt" or "mcode-arithmetic.txt"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STACK_HEIGHT 2000
#define MAX_CODE_LENGTH 500
#define MAX_LEXI_LEVELS 3


//stack pointer, base pointer, program counter
int SP = 0, BP = 1, PC = 0;
//flag that returns 1 upon reading an instruction to halt
int HALT = 0;

//the code store will be an array of Instruction structs
//each instruction will contain an opcode, a lexigraphical level, and data 'm'
typedef struct Instruction
{
	int op;
	int l;
	int m;

}Instruction;
Instruction *code = NULL;

//IR = instruction register, will contain the current instruction
Instruction IR;

//global array to represent the stack
int *stack;




//returns the input file if it can be found, or prints an error and terminates otherwise
FILE *getFile()
{
	char *filename = "mcode.txt";
	FILE *file = NULL;
	if((file = fopen(filename, "r")) == NULL)
	{
		if((file = fopen("mcode-arithmetic.txt", "r")) == NULL)
		{
			fprintf(stderr, "Could not open %s in main()\n", filename);
			exit(1);
		}	
	}
	return file;
}

//used to determine which stack frame we are in
int base(int l, int bp)
{
	if(l > MAX_LEXI_LEVELS)
		l = MAX_LEXI_LEVELS;

	int b1 = bp;//find base L levels down
	while (l > 0)
	{
		b1 = stack[b1+1];
		l--;
	}

	return b1;
}

//arithmetic logic unit, M determines operation to be performed
void ALU(int M)
{
	switch(M)
	{
		//NEG: returns the negative of the value at the top of the stack
		case 1:
			stack[SP] = stack[SP] * -1;
			break;
		//ADD: two values popped off stack and added.
		//sum is assigned to top of the stack, which is now 1 unit shorter.
		case 2:
			SP--;
			stack[SP] = stack[SP] + stack[SP+1];
			stack[SP+1] = 0;
			break;
		//SUB: 2nd value - top value
		case 3:
			SP--;
			stack[SP] = stack[SP] - stack[SP+1];
			stack[SP+1] = 0;
			break;
		//MUL: returns product of top two values of stack
		case 4:
			SP--;
			stack[SP] = stack[SP]*stack[SP+1];
			stack[SP+1] = 0;
			break;
		//DIV: 2nd value/top value
		case 5:
			SP--;
			stack[SP] = stack[SP]/stack[SP+1];
			stack[SP+1] = 0;
			break;
		//ODD: 1 if odd, 0 otherwise
		case 6:
			if (stack[SP] % 2 != 0)
				stack[SP] = 1;
			else
				stack[SP] = 0;
			break;
		//MOD: remainder of 2nd value/top value
		case 7:
			SP--;
			stack[SP] = stack[SP] % stack[SP+1];
			stack[SP+1] = 0;
			break;
		//EQL: 1 if top two values are equal, 0 otherwise
		case 8:
			SP--;
			if (stack[SP] == stack[SP+1])
				stack[SP] = 1;
			else
				stack[SP] = 0;
			stack[SP+1] = 0;
			break;
		//NEQ: 1 if top two values are NOT equal, 0 if they are
		case 9:
			SP--;
			if (stack[SP] != stack[SP+1])
				stack[SP] = 1;
			else
				stack[SP] = 0;
			stack[SP+1] = 0;
			break;
		//LSS: 1 if 2nd value is less than top, 0 otherwise
		case 10:
			SP--;
			if(stack[SP] < stack[SP+1])
				stack[SP] = 1;
			else
				stack[SP] = 0;
			stack[SP+1] = 0;
			break;
		//LEQ: 1 if 2nd value is less than OR equal to top value, 0 otherwise
		case 11:
			SP--;
			if (stack[SP] <= stack[SP+1])
				stack[SP] = 1;
			else
				stack[SP] = 0;
			stack[SP+1] = 0;
			break;
		//GTR: 1 if 2nd value is greater than top, 0 otherwise
		case 12:
			SP--;
			if(stack[SP] > stack[SP+1])
				stack[SP] = 1;
			else
				stack[SP] = 0;
			stack[SP+1] = 0;
			break;
		//GEQ: 1 if 2nd value is greater than OR equal to top, 0 otherwise
		case 13:
			SP--;
			if(stack[SP] >= stack[SP+1])
				stack[SP] = 1;
			else
				stack[SP] = 0;
			stack[SP+1] = 0;
			break;
		//if the instruction's M value is not 1-13, nothing happens.
		default:
			break;
	}
}

//occurs in the fetch function AFTER 
//the instruction register gets instruction from code[PC] and PC is incremented
void execute(Instruction IR)
{
	//The Instruction Set
	switch(IR.op)
		{
			//LIT: Pushes literal value of IR.m onto the stack
			case 1:
				SP++;
				stack[SP] = IR.m;
				break;
			//OPR: if IR.m is 0, returns from procedure call
			case 2:
				if(IR.m == 0)
				{
					//new top of stack is 1 below the base of the current stack frame
					SP = BP - 1;
					//see case 5 --> stack[SP+4] contains the return address
					PC = stack[SP+4];
					//see case 5 --> stack[SP+3] contains dynamic link 
					//(base of stack frame where the procedure was called from)
					BP = stack[SP+3];	
				}
				//if IR.m is not 0, an ALU operation is performed	
				else
					ALU(IR.m);
				break;
			//LOD: go IR.l levels down, read value at offset IR.m
			case 3:
				SP++;
				stack[SP] = stack[base(IR.l, BP) + IR.m];
				break;
			//STO: go IR.l levels down, pop stack and store value at offset IR.m
			case 4:
				stack[base(IR.l, BP)+IR.m] = stack[SP];
				SP--;
				break;
			//CAL: call procedure at IR.m
			//Generate Activation Record
			case 5:
				//Functional Value
				stack[SP+1] = 0;
				//Static Link (points to the frame of the current procedure's parent procedure)
				stack[SP+2] = base(IR.l, BP);
				//Dynamic Link (points to the caller's frame)
				stack[SP+3] = BP;
				//Return Address (instruction to be executed after current procedure ends)
				stack[SP+4] = PC;
				//new base of stack frame = 1 above the top of current stack frame
				BP = SP+1;
				//next instruction is a procedure at IR.m 
				PC = IR.m;
				break;
			//INC: allocate space for IR.m local variables
			case 6:
				SP = SP + IR.m;
				break;
			//JMP: Next instruction will be at IR.m
			case 7:
				PC = IR.m;
				break;
			//JPC: pop, and if value is 0 --> branch to IR.m
			case 8:
				if(stack[SP] == 0)
					PC = IR.m;
				SP--;
				break;
			//SIO pop and write to screen
			case 9:
				printf("%d\n", stack[SP]);
				SP--;
				break;
			//SIO push value input from user
			case 10:
				SP++;
				scanf("%d", &stack[SP]);
				break;
			//SIO stop the machine (halt)
			case 11:
				PC = 0;
				SP = 0;
				BP = 0;
				HALT = 1;
				break;
			default:
				break;
		}
}

//mneumonics will be printed instead of numerical opcode values
void getMneumonic(int op, FILE *out)
{
	switch(op)
		{
			case 1:
				fprintf(out, "lit ");
				break;
			case 2:
				fprintf(out, "opr ");
				break;
			case 3:
				fprintf(out, "lod ");
				break;
			case 4:
				fprintf(out, "sto ");
				break;
			case 5:
				fprintf(out, "cal ");
				break;
			case 6:
				fprintf(out, "inc ");
				break;
			case 7:
				fprintf(out, "jmp ");
				break;
			case 8:
				fprintf(out, "jpc ");
				break;
			case 9:
				fprintf(out, "sio ");
				break;
			case 10:
				fprintf(out, "sio ");
				break;
			case 11:
				fprintf(out, "sio ");
				break;
			//if opcode is not 1-11
			default:
				fprintf(out, "invalid instruction ");
				break;
		}
}

//First step
void fetch(int size, FILE *out)
{
	int i, j;
	int *flag = calloc(size, sizeof(int));

	fprintf(out, "                  pc  bp  sp stack\n");
	fprintf(out, "Initial values");
	fprintf(out, "   %2d  %2d  %2d\n", PC, BP, SP);

	//size = number of instructions
	for(i=0; i < size-1; i++)
	{
		//FETCH INSTRUCTION FROM CODE STORE
		IR = code[PC];
		fprintf(out, "%2d  ", PC);
		//INCREMENT PROGRAM COUNTER
		PC++;

		//this is the actual end of the fetch step

		//BEGIN EXECUTE CYCLE
		execute(IR);

		//fetch and execute are complete, the rest is just printing.

		//OP
		getMneumonic(IR.op, out);
		//L, M
		fprintf(out, " %2d  %2d", IR.l, IR.m);
		//PC, BP, SP will not print if instruction is to halt
		if(!HALT)
			fprintf(out, "  %2d  %2d  %2d  ", PC, BP, SP);

		//stack
		for(j=1; j <= SP; j++)
		{
			fprintf(out, "%d ", stack[j]);
			//Since this condition depends on BP, we need to set a flag for value j
			//if we enter a stack frame higher than 2, we need to remember where the bar line should be printed
			//for the lower lexigraphical levels (stack frames)
			if (j == BP-1 && BP > 1)
				flag[j] = 1;
			//j != SP prevents a bar line from being printed when 
			//there's nothing above the end of a stack frame
			if (flag[j] == 1 && j != SP)
				fprintf(out, " | ");
		}	
		fprintf(out, "\n");
		//we halt here in order to continue printing after the 
		//halt instruction is read
		if (HALT)
			exit(1);
	}
}
//reads in a line of input, then prints a line
//size = number of instructions
int printInput(FILE *file, FILE *out, int size)
{
	fprintf(out, "line  OP   L  M\n");
	while(fscanf(file, "%d", &code[size].op) != EOF)
	{
		fscanf(file, "%d", &code[size].l);
		fscanf(file, "%d", &code[size].m);
		fprintf(out, "%2d    ", size);
		getMneumonic(code[size].op, out);
		fprintf(out, "%2d %2d\n", code[size].l, code[size].m);
		size++;
	}
	fprintf(out, "\n");
	return size;
}

int main()
{
	int input;
	FILE *file = getFile();
	FILE *out = fopen("stacktrace.txt", "w");
	code = malloc(sizeof(Instruction)*MAX_CODE_LENGTH);
	stack = calloc(MAX_STACK_HEIGHT, sizeof(int));
	int size = 0;
	//size will be calculated as each line is read in and printed out
	size = printInput(file, out, size);
	fetch(size, out);
	//execute cycle is called inside of fetch()

	fclose(file);
	fclose(out);
	return 0;
}