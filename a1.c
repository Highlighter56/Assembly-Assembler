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

void error(char *p)
{
   // Displays error message p points to, line number in linenum, and line in linesave.
   // Error message format is the same as the LCC
   printf("Error on line %d of %s:\n%s\n%s\n",linenum,fileName,linesave,p);
   exit(0); // On error, exit program
}
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
unsigned short getadd(char *p)
{
   // Code missing here:
   // Returns address of symbol p points to accessed from the symbol table. Calls error() if symbol not in symbol table.
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
   printf("Starting Pass 1\n");              // OMG OMG THIS IS WHAT THE LCC MEANS WHEN IT PRINTS Starting Pass 1
   while (fgets(buf, sizeof(buf), infile))
   {
      linenum++;  // update line number
      cp = buf;                              // cp is a char pointer, buf is a char array, your now making cp a string essentialy
      while (isspace(*cp))
         cp++;
      if (*cp == '\0' || *cp ==';')  // if line all blank, or at line end, go to next line
         continue;   // go to next loop of while loop
      strcpy(linesave, buf);        // save line for error messages - take buf, and copy the string into linesave - buff and line save are both char[100]
      if (!isspace(buf[0]))         // line starts with label - if the first character in a line isnt a space, meanign theres a letter, meaning its a label
      { // basicly if its a label break up the line into three parts, the label the opcode and the rest of the string. If its not a label, break it up into two parts, the opcode and the rest of the line
         label = strdup(strtok(buf, " \r\n\t:"));  // strtok is getting breaking up a string, based on the entered string - so this is breaking up buf, by searching for the first occurence of  \r\n\t:
         // Code Missing Here Add code here that checks for a duplicate label, use strcmp().
         symbol[stsize] = label;          // stsize is a bad name, but its an int that keeps in index inside of symbol and symadd, to make sure were alwasu at the same place
         symadd[stsize++] = loc_ctr;      // adding the label and its adress to symbol and symadd
         mnemonic = strtok(NULL," \r\n\t:"); // get ptr to mnemonic/directive - if on subsequent calls you done enter a string and instead enter null as the string to tokenize, strtok will remember the last string you called on, and contiue to tokenize from where it last left off
         o1 = strtok(NULL, " \r\n\t,");      // get ptr to first operand
      }
      else   // if the line does not start with a label - tokenize line with no label
      {
         mnemonic = strtok(buf, " \r\n\t");  // get ptr to mnemonic
         o1 = strtok(NULL, " \r\n\t,");      // get ptr to first operand
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
      }
      else 
         loc_ctr++; // if the line is anything other than .zero, than incriment normaly
      if (loc_ctr > 65536)
         error("Program too big");
   }

   rewind(infile);

   // Pass 2
   printf("Starting Pass 2\n");           // so this is pass two, now its makign sense
   loc_ctr = linenum = 0;      // reinitialize - after going through all of the code, the first time collecting all of the labels, go back to the top and start pass 2
   while (fgets(buf, sizeof(buf), infile))
   {
      linenum++;
      // Code missing here:
      // Discard blank/comment lines, and save buf in linesave as in pass 1
      // Tokenize entire current line.
      // Do not make any new entries into the symbol table

      if (mnemonic == NULL)
         continue;
      if (!mystrncmpi(mnemonic, "br", 2))    // case sensitive compares
      {
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

         pcoffset9 = (getadd(o1) - loc_ctr - 1);    // compute pcoffset9
         if (pcoffset9 > 255 || pcoffset9 < -256)
            error("pcoffset9 out of range");
         macword = macword | (pcoffset9 & 0x01ff);  // assemble inst
         fwrite(&macword, 2, 1, outfile);           // write out instruction
         loc_ctr++;
      }
      else
      if (!mystrcmpi(mnemonic, "add" ))
      {
         if (!o3)
            error("Missing operand");
         dr = getreg(o1) << 9;   // get and position dest reg number
         sr1 = getreg(o2) << 6;  // get and position srce reg number
         if (isreg(o3)) // is 3rd operand a reg?
         {
            sr2 = getreg(o3);      // get third reg number
            macword = 0x1000 | dr | sr1 | sr2; // assemble inst
         }
         else
         {
            if (sscanf(o3,"%d", &num) != 1)    // convert imm5 field
               error("Bad imm5");
            if (num > 15 || num < -16)
               error("imm5 out of range");
            macword = 0x1000 | dr | sr1 | 0x0020 | (num & 0x1f);
         }
         fwrite(&macword, 2, 1, outfile);      // write out instruction
         loc_ctr++;
      }
      else
      if (!mystrcmpi(mnemonic, "ld" ))
      {
         dr = getreg(o1) << 9;// get and position destination reg number
         pcoffset9 = (getadd(o2) - loc_ctr - 1);
         if (pcoffset9 > 255 || pcoffset9 < -256)
            error("pcoffset9 out of range");
         macword = 0x2000 | dr | (pcoffset9 & 0x1ff);   // assemble inst
         fwrite(&macword, 2, 1, outfile); // write out instruction
         loc_ctr++;
      }

      // code missing here for st, bl, blr, and, ldr, str, not

      else
      if (!mystrcmpi(mnemonic, "jmp" ))     // also ret instruction
      {
         baser = getreg(o1) << 6;        // get reg number and position it
         if (o2)                         // offset6 specified?
         {
            if (sscanf(o2,"%d", &num) != 1)    // convert offset6 field
               error("Bad offset6");
            if (num > 31 || num < -32)
               error("offset6 out of range");
         }
         else
            num = 0;                           // offset6 defaults to 0
         // combine opcode, reg number, and offset6
         macword = 0xc000 | baser | num;       
         fwrite(&macword, 2, 1, outfile);  // write out instruction
         loc_ctr++;
      }
      else
      if (!mystrcmpi(mnemonic, "ret" ))     // also ret instruction
      {
         // code here is similar to code for jmp except baser
         // is always 7 and optional offset6 is pointed to by
         // o1, not by o2 as in jmp
      }

      // code missing here for lea, trap (halt, nl, dout), .word

      else
      if (!mystrcmpi(mnemonic, ".zero"))
      {
         sscanf(o1, "%d", &num);             // get size of block
         loc_ctr = loc_ctr + num;            // adjust loc_ctr
         macword = 0;
         while (num--)                       // write out a block of zeros
            fwrite(&macword, 2, 1, outfile);
      }
      else
         error("Invalid mnemonic or directive");
   }
   // Close files.
}

