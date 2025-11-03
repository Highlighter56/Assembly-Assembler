// a1shell.c
// Patrick Hughes
#include <stdio.h>    // for I/O functions
#include <stdlib.h>   // for exit()
#include <string.h>   // for string functions
#include <ctype.h>    // for isspace(), tolower()
#include <time.h>     // for time functions

FILE *infile, *outfile;
short pcoffset9, pcoffset11, imm5, imm9, offset6;
unsigned short symadd[500], macword, dr, sr, sr1, sr2, baser, trapvec, eopcode;
char outfilename[100], linesave[100], buf[100], *symbol[500], *p1, *p2, *cp, *mnemonic, *o1, *o2, *o3, *label;
int stsize, linenum, rc, loc_ctr, num;
time_t timer;
// Leaving the file name as null temproarily. When the main method runs, this var will be assigned, and the error function can print the name of the function, same as the LCC
char *fileName = NULL;
char *end;

// Case insensitive string compare
// Returns 0 if two strings are equal.
short int mystrcmpi(const char *p, const char *q) 
{
	char a, b;
	while (1) 
	{
		a = tolower(*p);           // dereference the pointer, and just grab the first character
		b = tolower(*q);
		if (a != b) return a-b;    // if the letters your currently at are not the same, return that their not the same
		if (a == '\0') return 0;   // meaing both would have to be at the end of the array because you already checked if the symbols were different
		p++; q++;                  // incriment to the next character
	}
	return 0;                     // this will never be reached, so why is it here?
}

// Case insensitive string compare
// Compares up to a maximum of n characters from each string.
// Returns 0 if characters compared are equal.
short int mystrncmpi(const char *p, const char *q, int n)   // Same as mystrcmpi, but with a limit to how many chars you check
{
	char a, b;
	int i;
	for (i = 1; i <= n; i++) 
	{
		a = tolower(*p); 
		b = tolower(*q);
		if (a != b) return a-b;
		if (a == '\0') return 0;
		p++; q++;
	}
	return 0;
}
// Error
// Takes in an error message and displays it to the console
void error(char *p)
{
	// Displays error message p points to, line number in linenum, and line in linesave.
	// Error message format is the same as the LCC
	printf("Error on line %d of %s:\n%s\n%s\n",linenum,fileName,linesave,p);
	exit(1); // On error, exit program
}
// checks if a string is a valid register name or not
int isreg(char *p)
{
	// Returns 1 if p points to a register name. Otherwise, returns 0.  
	if(strlen(p)==2) {               // checks if length of p is 2
		if(p[0]=='r') {               // Checks for r0-7
			int num = p[1]-48;
			if(num>=0 && num<=7) {
				return 1;
			}
		} else if(strcmp(p,"fp")==0) {   // Checks for fp
			return 1;
		} else if(strcmp(p,"sp")==0) {   // Check for sp
			return 1;
		} else if(strcmp(p,"lr")==0) {   // Check for lr
			return 1;
		}
	}
	return 0;
}
// get register number
// returns the number associated with a register : ex r5 = 5, lr = 7,...
unsigned short getreg(char *p)              
{
   // Returns register number of the register whose name p points to. Calls error() if not passed a register name.
	if(isreg(p)) {                      // If p is a valid register
		if(p[0]=='r'){                   // Checks for r0-7
			return p[1]-48; // converts the character number into a number
		} else if(strcmp(p,"fp")==0) {   // Checks for fp
			return 5;
		} else if(strcmp(p,"sp")==0) {   // Check for sp
			return 6;
		} else if(strcmp(p,"lr")==0) {   // Check for lr
			return 7;
		}
	}
   error("Bad register");  // If not a register, call error
}
// Gets the adress associated with a symbol, if its in the symbol table
unsigned short getadd(char *p)
{
	// Returns address of symbol p points to accessed from the symbol table. Calls error() if symbol not in symbol table.
	for(int i=0; i<stsize; i++) {			// stsize is the known size of the table, so loop for just that size
		if(strcmp(symbol[i],p)==0) {		// if symbol and p are the same
			return symadd[i];				// return adress of symbol
		}
	}
	error("Symbol not in symbol table");	// If there is no match, call error
}



int main(int argc,char *argv[])  // Main Method
{
	if (argc != 2)                // Checking num of arguments passed in when program is run
	{
		printf("Wrong number of command line arguments\n");
		printf("Usage: a1 <input filename>\n");
		exit(1);
	}

	// This assigns gets the name of the file passed in and saves it as a global var, so the error function can display it same as the LCC
	fileName = argv[1];

	// display your name, command line args, time
	time(&timer);      // get time
	printf("Patrick Hughes   %s %s   %s", 
			argv[0], argv[1], asctime(localtime(&timer)));

	infile = fopen(argv[1], "r");
	if (!infile)
	{
		printf("Cannot open input file %s\n", argv[1]);
		exit(1);
	}

	// construct output file name
	strcpy(outfilename, argv[1]);        // copy input file name
	p1 = strrchr(outfilename, '.');      // search for period in extension
	if (p1)                              // name has period
	{
#ifdef _WIN32                           // defined only on Windows systems
      	p2 = strrchr(outfilename, '\\' ); // compiled if _WIN32 is defined
#else
      	p2 = strrchr(outfilename, '/');   // compiled if _WIN32 not defined
#endif
		// can p2 < p1 in following if statement be eliminated by 
		// using strchr(p1, ...) instead of strrchr(outfilename, ...) 
		// in the preceding assignments to p2?
		if (!p2 || p2 < p1)               // input file name has extension?
			*p1 = '\0';                    // null out extension
	}
	strcat(outfilename, ".e");           // append ".e" extension

	outfile = fopen(outfilename, "wb");
	if (!outfile)
	{
		printf("Cannot open output file %s\n", outfilename);
		exit(1);
	}

	loc_ctr = linenum = 0;       // initialize, not required because global
	fwrite("oC", 2, 1, outfile); // output empty header

	// Pass 1
	printf("Starting Pass 1\n");                    // OMG OMG THIS IS WHAT THE LCC MEANS WHEN IT PRINTS Starting Pass 1
	while (fgets(buf, sizeof(buf), infile)) {
		linenum++;                                   // incriment line number
		cp = buf;                                    // cp is becomes an array, with each character of a single line of code in a different index
		while (isspace(*cp))                         // itterates past the white space
			cp++;
		if (*cp == '\0' || *cp ==';')                // if line is blank OR commented, go to next line
			continue;                                 // go to next pass - next loop of while loop
		strcpy(linesave, buf);                       // save buf in linesave
		if (!isspace(buf[0])) {                // If line has label: Break it up into 3 parts
			label = strdup(strtok(buf, " \r\n\t:"));  // get the label - strtok breaks a string based on the entered string - so this is breaking up buf, by searching for the first occurence of  \r\n\t:
			// Check for duplicate labels
			int i=0;
			while(symbol[i]!=0) {                     // loop the symbol table until you reach the end of inputed symbols. (The null value for a char pointer is 0)
				if(strcmp(symbol[i],label)==0)         // If theres a match
				error("Duplicate label");           // There is a duplicate label, and call error
				i++;                                   // Incriment i
			}
			symbol[stsize] = strdup(label);                   // stsize is a bad name, but its an int that keeps in index inside of symbol and symadd, to make sure were alwasu at the same place
			symadd[stsize++] = loc_ctr;               // adding the label and its adress to symbol and symadd
			mnemonic = strtok(NULL," \r\n\t:");       // get ptr to mnemonic/directive - if on subsequent calls you done enter a string and instead enter null as the string to tokenize, strtok will remember the last string you called on, and contiue to tokenize from where it last left off
			o1 = strtok(NULL, " \r\n\t,");            // get ptr to first operand
		} else {                               // If the line does not start with a label
			mnemonic = strtok(buf, " \r\n\t");        // opcode of current line   - get ptr to mnemonic
			o1 = strtok(NULL, " \r\n\t,");            // rest of the line of code - get ptr to first operand
		}
		if (mnemonic == NULL)    // check for mnemonic or directive - i think this accounts for whitespace?
			continue;
		if (!mystrcmpi(mnemonic, ".zero"))    // case insensitive compare - if the line/opcode/mnumonic is .zero
		{  // If we find a .zero, add the nessecary lines of blank space
			if (o1)
				rc = sscanf(o1, "%d", &num);    // get size of block from o1
			else
				error("Missing operand");
			if (rc != 1 || num > (65536 - loc_ctr) || num < 1)
				error("Invalid operand");
			loc_ctr = loc_ctr + num;
		} else 
			loc_ctr++; // if the line is anything other than .zero, than incriment normaly
		if (loc_ctr > 65536)
			error("Program too big");
	}
   	rewind(infile);
	loc_ctr = linenum = 0;                          // reinitialize - after pass 1, reset to start pass 2

	// Pass 2
	printf("Starting Pass 2\n");                    // starting pass 2
	while (fgets(buf, sizeof(buf), infile)) {
		linenum++;                                   // incriment line number
		cp = buf;                                    // cp is becomes an array, with each character of a single line of code in a different index
		while (isspace(*cp))                         // itterates past the white space
			cp++;
		if (*cp == '\0' || *cp ==';')                // if line is blank OR commented, go to next line
			continue;                                // go to next pass - next loop of while loop
		strcpy(linesave, buf);                       // save buf in linesave
		// Tokenize entire current line
		if (!isspace(buf[0])) {                // If line has label: Break it up into 3 parts
			label = strdup(strtok(buf, " \r\n\t:"));  // get the label - strtok breaks a string based on the entered string - so this is breaking up buf, by searching for the first occurence of  \r\n\t:
			mnemonic = strtok(NULL," \r\n\t:");       // get opcode/mnemonic - if on subsequent calls you done enter a string and instead enter null as the string to tokenize, strtok will remember the last string you called on, and contiue to tokenize from where it last left off
			o1 = strtok(NULL, " \r\n\t,");            // get next part
			o2 = strtok(NULL, " \r\n\t,");            // get next part
         	o3 = strtok(NULL, " \r\n\t,");            // get next part
			// printf("%s:\t%s\t%s\t%s\t%s\n",label,mnemonic,o1,o2,o3); // Debugging
		} else {                               // If the line does not start with a label
			mnemonic = strtok(buf, " \r\n\t");        // opcode of current line   - get ptr to mnemonic
			o1 = strtok(NULL, " \r\n\t,");            // get next part
			o2 = strtok(NULL, " \r\n\t,");            // get next part
         	o3 = strtok(NULL, " \r\n\t,");            // get next part
			// printf("\t%s\t%s\t%s\t%s\n", mnemonic, o1, o2, o3);      // Debugging
		}

		// Building Lines of Assembly
		// Blank Lines
		if (mnemonic == NULL)
			continue;
		
		// --Branching--
		if (!mystrncmpi(mnemonic, "br", 2)) {        		// case sensitive compares - if br
			if (!mystrcmpi(mnemonic, "br" ))
				macword = 0x0e00;
			else
			if (!mystrcmpi(mnemonic, "brz" ))
				macword = 0x0000;
			else
			if (!mystrcmpi(mnemonic, "brnz" ))
				macword = 0x0200;
			else
			if (!mystrcmpi(mnemonic, "brn" ))
				macword = 0x0400;
			else
			if (!mystrcmpi(mnemonic, "brp" ))
				macword = 0x0600;
			else
			if (!mystrcmpi(mnemonic, "brlt" ))
				macword = 0x0800;
			else
			if (!mystrcmpi(mnemonic, "brgt" ))
				macword = 0x0a00;
			else
			if (!mystrcmpi(mnemonic, "brc" ))
				macword = 0x0c00;
			else
				error("Invalid branch mnemonic");

			pcoffset9 = (getadd(o1) - loc_ctr - 1);    		// compute pcoffset9
			if (pcoffset9 > 255 || pcoffset9 < -256)
				error("pcoffset9 out of range");
			macword = macword | (pcoffset9 & 0x01ff);  		// assemble inst
			fwrite(&macword, 2, 1, outfile);           		// write instruction

		// Add
		} else if (!mystrcmpi(mnemonic, "add" )) {
			if (!o3)
				error("Missing operand");
			dr = getreg(o1) << 9;                     		// get and position dest reg number
			sr1 = getreg(o2) << 6;                    		// get and position srce reg number
			if (isreg(o3)) {                          		// is 3rd operand a reg?
				sr2 = getreg(o3);                      		// get third reg number
				macword = 0x1000 | dr | sr1 | sr2;     		// assemble inst
			} else {
				if (sscanf(o3,"%d", &num) != 1)        		// convert imm5 field
					error("Bad imm5");
				if (num > 15 || num < -16)
					error("imm5 out of range");
				macword = 0x1000 | dr | sr1 | 0x0020 | (num & 0x1f);
			}
			fwrite(&macword, 2, 1, outfile);          		// write out instruction

		// ld
		} else if (!mystrcmpi(mnemonic, "ld" )) {
			dr = getreg(o1) << 9;                     		// get and position destination reg number
			pcoffset9 = (getadd(o2) - loc_ctr - 1);			// **Calculates the pcoffset9**
			if (pcoffset9 > 255 || pcoffset9 < -256)		// Checks if the pcoffset9 is within the valid range
				error("pcoffset9 out of range");
			macword = 0x2000 | dr | (pcoffset9 & 0x1ff);	// assemble inst
			fwrite(&macword, 2, 1, outfile);          		// write out instruction
		}

		// st
		else if (!mystrcmpi(mnemonic, "st" )) {
			sr = getreg(o1) << 9;                     		// get and position destination reg number
			pcoffset9 = (getadd(o2) - loc_ctr - 1);			// **Calculates the pcoffset9**
			if (pcoffset9 > 255 || pcoffset9 < -256)		// Checks if the pcoffset9 is within the valid range
				error("pcoffset9 out of range");
			macword = 0x3000 | sr | (pcoffset9 & 0x1ff);	// assemble inst
			fwrite(&macword, 2, 1, outfile);          		// write out instruction
		}

		// bl
		else if (!mystrcmpi(mnemonic, "bl") || !mystrcmpi(mnemonic, "call") || !mystrcmpi(mnemonic, "jsr")) {
			sscanf(o1, "%d", &num);							// convert string to num
			if(num > 511 || num < 512) 						// if num out of range
				error("offset6 out of range");
			pcoffset11 = (num & 0x7ff);						// get pcoffset11
			macword = 0x4800 | pcoffset11;					// assembly macword
			fwrite(&macword, 2, 1, outfile);          		// write out instruction
		}

		// blr
		else if (!mystrcmpi(mnemonic, "blr" )) {
			// code missing here
		}

		// and
		// note to self, my 'and' code is a coppied from the 'add' code, but opcode 5 instead of 1
		else if (!mystrcmpi(mnemonic, "and" )) {
			if (!o3)
				error("Missing operand");
			dr = getreg(o1) << 9;                     		// get and position dest reg number
			sr1 = getreg(o2) << 6;                    		// get and position srce reg number
			if (isreg(o3)) {                          		// is 3rd operand a reg?
				sr2 = getreg(o3);                      		// get third reg number
				macword = 0x5000 | dr | sr1 | sr2;     		// assemble inst
			} else {
				if (sscanf(o3,"%d", &num) != 1)        		// convert imm5 field
					error("Bad imm5");
				if (num > 15 || num < -16)
					error("imm5 out of range");
				macword = 0x5000 | dr | sr1 | 0x0020 | (num & 0x1f);
			}
			fwrite(&macword, 2, 1, outfile);          		// write out instruction
		}

		// ldr
		else if (!mystrcmpi(mnemonic, "ldr" )) {
			dr = getreg(o1) << 9;							// get dr
			baser = getreg(o2) << 6;						// get baser
			sscanf(o3, "%d", &num);							// convert string to num
			if(num > 63 || num < -64) 						// if num out of range
				error("offset6 out of range");
			offset6 = (num & 0x3f);							// get offset6
			macword = 0x6000 | dr | baser | offset6;		// assembly macword
			fwrite(&macword, 2, 1, outfile);          		// write out instruction
		}

		// str
		else if (!mystrcmpi(mnemonic, "str" )) {
			sr = getreg(o1) << 9;							// get sr
			baser = getreg(o2) << 6;						// get baser
			sscanf(o3, "%d", &num);							// convert string to num
			if(num > 63 || num < -64) 						// if num out of range
				error("offset6 out of range");
			offset6 = (num & 0x3f);							// get offset6
			macword = 0x7000 | sr | baser | offset6;		// assembly macword
			fwrite(&macword, 2, 1, outfile);          		// write out instruction
		}
		
		// not
		else if (!mystrcmpi(mnemonic, "not" )) {
			dr = getreg(o1) << 9;							// get dr
			sr1 = getreg(o2) << 6;							// get sr1
			macword = 0x9000 | dr | sr1;					// assembly macword
			fwrite(&macword, 2, 1, outfile);          		// write out instruction
		}

		// jmp
		else if (!mystrcmpi(mnemonic, "jmp" )) {     		// also ret instruction
			baser = getreg(o1) << 6;                  		// get reg number and position it
			if (o2) {                                 		// offset6 specified?
				if (sscanf(o2,"%d", &num) != 1)        		// convert offset6 field
					error("Bad offset6");
				if (num > 31 || num < -32)
					error("offset6 out of range");
			} else
				num = 0;                               		// offset6 defaults to 0
			// combine opcode, reg number, and offset6
			macword = 0xc000 | baser | num;       
			fwrite(&macword, 2, 1, outfile);          		// write out instruction
		
		// ret
		} else if (!mystrcmpi(mnemonic, "ret" )) {   		// also ret instruction
			// code missing here is similar to code for jmp except baser
			// is always 7 and optional offset6 is pointed to by
			// o1, not by o2 as in jmp
		}

		// lea
		else if (!mystrcmpi(mnemonic, "lea" )) {
			dr = getreg(o1) << 9;
			sscanf(o2, "%d", &num);							// convert string to num
			if(num > 255 || num < -256) 					// if num out of range
				error("offset out of range");
			pcoffset9 = (num & 0x1ff);						// get pcoffset9
			macword = 0xe000 | dr | pcoffset9;				// assembly macword
			fwrite(&macword, 2, 1, outfile);          		// write out instruction
		}

		// --Trap--
		// halt
		else if (!mystrcmpi(mnemonic, "halt" )) {
			macword = 0xf000;								// Assign macword
			fwrite(&macword, 2, 1, outfile);          		// write out instruction
		}
		// nl
		else if (!mystrcmpi(mnemonic, "nl" )) {
			macword = 0xf001;								// Assign macword
			fwrite(&macword, 2, 1, outfile);          		// write out instruction
		}
		// dout
		else if (!mystrcmpi(mnemonic, "dout" )) {
			macword = 0xf000;								// Assign macword
			sr = getreg(o1) << 9;							// get and format sr
			macword = macword | sr;							// join macword
			fwrite(&macword, 2, 1, outfile);          		// write out instruction
		}

		// .word
		else if (!mystrcmpi(mnemonic, ".word" )) {
			num = strtol(o1,&end,10);						// converts the string o1 to a decimal nubmer : &end is where the last character read as a number is stored, so it can be used to check if it was actualy a number that was read in. Im not doing this currently, but at some point this could be implemented
			if (num > 65535 || num < -65536)				// Checks if the pcoffset9 is within the valid range
				error("pcoffset9 out of range");
			macword = macword | num;						// assembly macword
			fwrite(&macword, 2, 1, outfile);          		// write out instruction
		}
		// .zero
		else if (!mystrcmpi(mnemonic, ".zero")) {
			sscanf(o1, "%d", &num);                   		// get size of block
			loc_ctr = loc_ctr + num;                  		// adjust loc_ctr 
			macword = 0;
			while (num--)                             		// write out a block of zeros
				fwrite(&macword, 2, 1, outfile);
			loc_ctr--;										// This is to account for loc_ctr++ at the end of the main while loop
		} else
			error("Invalid mnemonic or directive");

		// Increment the current line number : Instead of doing this individualy for most mnemonics, Ill do it once here, then subtract 1 anytime its not supposed to be incrimented
		loc_ctr++;
	}
   // Close files.
   fclose(infile);
   fclose(outfile);
}

