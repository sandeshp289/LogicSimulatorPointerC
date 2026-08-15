/////////////////////////////////////////////////////////////////////////////////////
// File       : logic_simulator_pointer.c
// Language   : C/C++
// Description: Event-Driven Combinational/Sequential Logic Simulator (Pointer-based)
// Author     : Sandesh Prabhakar
// Created    : December 8, 2007
// Modified   : August 15, 2026
// Version    : 1.0
//
// Copyright (c) 2026 Sandesh Prabhakar
// All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
   G_JUNK,         /* 0 */
   G_INPUT,        /* 1 */
   G_OUTPUT,       /* 2 */
   G_XOR,          /* 3 */
   G_XNOR,         /* 4 */
   G_DFF,          /* 5 */
   G_AND,          /* 6 */
   G_NAND,         /* 7 */
   G_OR,           /* 8 */
   G_NOR,          /* 9 */
   G_NOT,          /* 10 */
   G_BUF,          /* 11 */
};

struct node
{

//for the gates
        char value; 
        bool flag;
//for the event list
        int data;
        node *link;
};

////////////////////////////////////////////////////////////////////////
// circuit class
////////////////////////////////////////////////////////////////////////

class circuit
{
    // circuit information
    int numgates;       // total number of gates (faulty included)
    int maxlevels;      // number of levels in gate level ckt
    int numInputs;	// number of inputs
    int numOutputs;	// number of outputs
    unsigned char *gtype;	// gate type
    short *fanin;		// number of fanins of gate
    short *fanout;		// number of fanouts of gate
    int *levelNum;		// level number of gate
    int **faninlist;		// fanin list of gate
    int **fanoutlist;		// fanout list of gate

    // you may wish to add more private variables here
    
    node *gate;
    node **eventlist;
    char *pattern_buffer;
    int *pout, *dff;
    int numDFFs;

    public:
    circuit(char *);            // constructor

    // you may wish to add more function declarations (or public variables) here
    void add_event (int gate_number, int level_number ); 
    void logic_simulate(int level_number, node *gate, char *pattern_buffer, int* levelNum, short* fanin, int** faninlist, short* fanout, int** fanoutlist, unsigned char* gtype);
    void evaluate(int j, unsigned char* gtype, node* gate, char* pattern_buffer, short* fanin, int** faninlist );
};

////////////////////////////////////////////////////////////////////////
// constructor: reads in the *.lev file and builds basic data structure
// 		for the gate-level ckt
////////////////////////////////////////////////////////////////////////
circuit::circuit(char *cktName)
{
    FILE *inFile;
    char fName[40];
    int i, j, count;
    char c;
    int gatenum, junk;
    int f1, f2, f3;

    int lines, bit_count, vec_junk;

    strcpy(fName, cktName);
    strcat(fName, ".lev");
    inFile = fopen(fName, "r");
    if (inFile == NULL)
    {
	fprintf(stderr, "Can't open .lev file\n");
	exit(-1);
    }

    numgates = maxlevels = 0;
    numInputs = numOutputs = 0;

    fscanf(inFile, "%d", &count);	// number of gates
    fscanf(inFile, "%d", &junk);	// skip the second line

    // allocate space for gates data structure
    gtype = new unsigned char[count];
    fanin = new short[count];
    fanout = new short[count];
    levelNum = new int[count];
    faninlist = new int * [count];
    fanoutlist = new int * [count];

    gate = new node[count];
    pout = new int[count];
    dff = new int[count];

    // now read in the circuit
    for (i=1; i<count; i++)
    {

	fscanf(inFile, "%d", &gatenum);
	fscanf(inFile, "%d", &f1);
	fscanf(inFile, "%d", &f2);
	fscanf(inFile, "%d", &f3);

	numgates++;
	gtype[gatenum] = (unsigned char) f1;
	if (gtype[gatenum] > 13)
	    printf("gate %d is an unimplemented gate type\n", gatenum);
	else if (gtype[gatenum] == G_INPUT)
	    numInputs++;
	else if (gtype[gatenum] == G_OUTPUT)
            {
	    numOutputs++;
            pout[numOutputs]=gatenum; 
	    }
        else if (gtype[gatenum] == G_DFF)
            {
            numDFFs++;
            dff[numDFFs]=gatenum;  
            }


	f2 = (int) f2;
        //levelNum[gatenum] = f2;
        levelNum[gatenum] = (f2+5)/5;	


	if (f2 >= (maxlevels))
	    maxlevels = f2 + 5;

        //initializing the output of all gates to X and flags to 0
        gate[gatenum].value = 'X';
        gate[gatenum].flag = 0; 

	fanin[gatenum] = (int) f3;
	// now read in the faninlist
	faninlist[gatenum] = new int[fanin[gatenum]];
	for (j=0; j<fanin[gatenum]; j++)
	{
	    fscanf(inFile, "%d", &f1);
	    faninlist[gatenum][j] = (int) f1;
	}

	for (j=0; j<fanin[gatenum]; j++) // followed by samethings
	    fscanf(inFile, "%d", &junk);

	// read in the fanout list
	fscanf(inFile, "%d", &f1);
	fanout[gatenum] = (int) f1;

	fanoutlist[gatenum] = new int[fanout[gatenum]];
	for (j=0; j<fanout[gatenum]; j++)
	{
	    fscanf(inFile, "%d", &f1);
	    fanoutlist[gatenum][j] = (int) f1;
	}

	// skip till end of line
	while ((c = getc(inFile)) != '\n' && c != EOF)
	    ;
    }	// for (i...)
    fclose(inFile);

    int total_levels = maxlevels/5; 

    printf("Successfully read in circuit:\n");
    printf("\t%d PIs.\n", numInputs);
    printf("\t%d POs.\n", numOutputs);
    printf("\t%d Dffs.\n", numDFFs);
    printf("\t%d total number of gates\n", numgates);
    printf("\t%d levels in the circuit.\n", total_levels);

    // (LOGIC SIMULATOR CODE STARTS HERE)
    
    // allocate space for event list and pattern buffer
    eventlist = new node*[total_levels+1];

    for(i=0;i<total_levels+1;i++)
    eventlist[i]=NULL;

    pattern_buffer = new char[numInputs+2];

    strcpy(fName, cktName);
	strcat(fName, ".vec");
	inFile = fopen(fName, "r");
	if (inFile == NULL)
	  {
		fprintf(stderr, "Can't open .vec file\n");
		exit(-1);
	  }

        lines = 0;
        vec_junk = 's';

	fscanf(inFile, "%d", &bit_count);	// number of bits in each vector

        while (vec_junk!='\n')
	fscanf(inFile, "%c", &vec_junk);	// reading space and newline char

        //For each input vector
    	while ((c = getc(inFile)) != 'E')
	  {
               lines = lines + 1;
            
	       pattern_buffer[1]=c;
               printf("\n \n");

	       //Reading the input vector
	       printf("Input Vector  %d:     %c", lines-1, pattern_buffer[1]);
               if(gate[1].value != pattern_buffer[1])
               {
               add_event(1,1); 
               gate[1].flag = 1;
               }

	       for (i=2; i<=bit_count; i++)
	       {
	         fscanf(inFile, "%c", &pattern_buffer[i]);
		 printf("%c", pattern_buffer[i]);
                   if((gate[i].value != pattern_buffer[i])&&(i!=bit_count+1))
                     { 
                      add_event(i,1); 
                      gate[i].flag = 1;
                     }
               } 

               vec_junk = 's';
               while (vec_junk!='\n') 
               fscanf(inFile, "%c", &vec_junk);

               //For each level
               for(i=1;i<=total_levels;i++)            
               {
                 //Calling the Logic Simulate function
                 logic_simulate(i, gate, pattern_buffer, levelNum, fanin, faninlist, fanout, fanoutlist, gtype);
               }

               //Output Vector
               printf("\nOutput Vector %d:     ", lines-1);
               for(i=1;i<=numOutputs;i++)
               printf("%c",gate[pout[i]].value);

               //Next state for sequential circuits 
	       printf("\n"); 
               if(numDFFs>0)
               {
               for(i=1;i<=numDFFs;i++)
               printf("%c",gate[dff[i]].value);
               }

           }//While loop ends
               fclose(inFile);
               //Final next state for sequential circuits
               if(numDFFs>0)
               {
               printf("\n \nFinal Next State = ");
               for(i=1;i<=numDFFs;i++)
               printf("%c",gate[faninlist[dff[i]][0]].value);
               }
               printf("\n \nTotal number of patterns is %d\n", lines);
}//Constructor ends

////////////////////////////////////////////////////////////////////////
// other member functions here
////////////////////////////////////////////////////////////////////////

//Function to schedule gate onto the eventwheel
void circuit::add_event (int gate_number, int level_number )
 	{
	node *current = new node;
	current->data =gate_number; 
	current->link = eventlist[level_number];
        eventlist[level_number] = current;
        }

//Logic simulator function
void circuit::logic_simulate(int level_number, node *gate, char *pattern_buffer, int* levelNum, short* fanin, int** faninlist, short* fanout, int** fanoutlist, unsigned char* gtype)
        {
           node *delNode = new node;
           char temp;
           int i, num;
           while(eventlist[level_number] != NULL)
	   {
           num = eventlist[level_number]->data;
           temp = gate[num].value;
           evaluate(num, gtype, gate, pattern_buffer, fanin, faninlist );
           gate[num].flag = 0;

           if((temp != gate[num].value)&&gtype[num]!=2)
                    for(i=0; i<fanout[num]; i++)
                    { 
                      if(gate[fanoutlist[num][i]].flag == 0)
		      { 
                      add_event(fanoutlist[num][i], levelNum[fanoutlist[num][i]]); 
                      gate[fanoutlist[num][i]].flag = 1;
                      }
                    }

	   delNode = eventlist[level_number];
	   eventlist[level_number] = eventlist[level_number]->link;
	   delete delNode;
	   }
        }

//Gate evaluations for logic simulator
void circuit::evaluate (int j, unsigned char* gtype, node* gate, char* pattern_buffer, short* fanin, int** faninlist )
   {
    int nn;
    char out;
    bool out_int;

         switch (gtype[j])
              { //switch begins
			 //INPUT
		          case 1:
		          gate[j].value=pattern_buffer[j];
		          break;

                          //NAND gate
			  case 7:
                          out='0';
			  for (nn=0; nn<=(fanin[j]-1);nn++)
				     {
                                        if (gate[faninlist[j][nn]].value == '0')
					 {
					 gate[j].value='1';
					 return;
                                         }
                                         else if (gate[faninlist[j][nn]].value == 'X')
                                         out='X';
				     }
                                         gate[j].value=out; 
					 break;

                              //AND gate
                              case 6:

                                     out='1';
			             for (nn=0; nn<=(fanin[j]-1);nn++)
				     {
                                        if (gate[faninlist[j][nn]].value == '0')
					 {
					 gate[j].value='0';
					 return;
                                         }
                                         else if (gate[faninlist[j][nn]].value == 'X')
                                         out='X';
				     }
                                         gate[j].value=out; 
					 break;

                              //OR gate
                              case 8:

                                     out='0';
			             for (nn=0; nn<=(fanin[j]-1);nn++)
				     {
                                        if (gate[faninlist[j][nn]].value == '1')
					 {
					 gate[j].value='1';
					 return;
                                         }
                                         else if (gate[faninlist[j][nn]].value == 'X')
                                         out='X';
				     }
                                         gate[j].value=out; 
					 break;

                              //NOR gate
                              case 9:
                                     out='1';
			             for (nn=0; nn<=(fanin[j]-1);nn++)
				     {
                                        if (gate[faninlist[j][nn]].value == '1')
					 {
					 gate[j].value='0';
					 return;
                                         }
                                         else if (gate[faninlist[j][nn]].value == 'X')
                                         out='X';
				     }
                                         gate[j].value=out; 
					 break; 

                              //XOR gate
			            case 3:
                                      out_int=0;
                                      for (nn=0; nn<=(fanin[j]-1);nn++)
				         {
                                        if (gate[faninlist[j][nn]].value == 'X')
					 {
					 gate[j].value='X';
					 return;
                                         }
                                         else if (gate[faninlist[j][nn]].value == '1')
                                         out_int=!out_int;
				        }

                                        switch (out_int)
                                            {
                                         case 0: out = '0';
                                                 break;
                                         case 1: out = '1';
                                                 break;
                                            }
                                         gate[j].value=out; 
					 break; 

                             //XNOR gate
				    case 4:
				      out_int=0;
                                      for (nn=0; nn<=(fanin[j]-1);nn++)
				         {
                                        if (gate[faninlist[j][nn]].value == 'X')
					 {
					 gate[j].value='X';
					 return;
                                         }
                                         else if (gate[faninlist[j][nn]].value == '1')
                                         out_int=!out_int;
				        }

                                        switch (out_int)
                                            {
                                         case 0: out = '1';
                                                 break;
                                         case 1: out = '0';
                                                 break;
                                            }
                                         gate[j].value=out; 
					 break; 

			      //NOT gate
                              case 10:
				        if ( gate[faninlist[j][0]].value == '0')
				  	       {
					        gate[j].value='1';
					       }
					else if (gate[faninlist[j][0]].value == '1')
					       gate[j].value='0';
					else if (gate[faninlist[j][0]].value == 'X')
						{
						gate[j].value='X';
					        }
					break;

                             //BUF gate
			      case 11:

				  	gate[j].value=gate[faninlist[j][0]].value;
					break;

                              //TIE_1
                              case 12:
					gate[j].value='1';
					break;

                              //TIE_0
			      case 13:
				        gate[j].value = '0';
				        break;

                              //OUTPUT
			      case 2:
				        gate[j].value=gate[faninlist[j][0]].value;
				        break;

			      //D_FF
			      case 5:
                                        gate[j].value=gate[faninlist[j][0]].value;
				        break;

			   } //switch ends


		} //function ends

////////////////////////////////////////////////////////////////////////
// main starts here
////////////////////////////////////////////////////////////////////////
main(int argc, char *argv[])
{
    char cktName[20];
    circuit *ckt;
    int vecWidth, poNum;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <ckt>\n", argv[0]);
        exit(-1);
    }

    strcpy(cktName, argv[1]);

    // read in the circuit and build data structure
    ckt = new circuit(cktName);
    // NOW ADD YOUR FUNCTION HERE
}

