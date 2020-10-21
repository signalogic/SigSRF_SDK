/*
   $Header: /root/Signalogic/DirectCore/apps/common/cmdLineOpt.cpp
 
   Purpose:
    Parses commanmd line options for SigSRF and DirectCore programs
  
   Copyright (C) Signalogic Inc. 2005-2019
  
   Revision History:

     Modified Nov 2014 JHB, fixed some naming to support multiple targets, unify/consolidate command line params for all test programs
     Modified Dec 2014 JHB, added support for IP addr, UDP port, and MAC addr entry (e.g. -Daa.bb.cc.dd:port:aa-bb-cc-dd-ee-ff-gg)
     Modified Feb 2015 JHB, added support for multiple instances of some cmd line params.  Also fix problem with default values being overwritten
     Modified Jul 2015 JHB, added support for multiple integer values, in format -option NN:NN:NN
     Modified Jan 2017 CKJ, added support for x86 which doesn't require the same mandatories
     Modified Aug 2017 JHB, added support for program sub mode as a suffix character at end of INTEGER entries
     Modified Sep 2017 JHB, added a couple of exceptions for -L entry (log file), see comments
     Modified Jul 2018 JHB, changed mandatory requirements to handle x86 and coCPU separately
     Modified Dec 2019 JHB, fix bug in int value parsing, where suffix char code would strip off last a-f digit of hex values
*/

#include <stdint.h>

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <cstring>

#include "cmdLineOpt.h"
#include "alias.h"

#define MAX_OPTIONS MAX_INPUT_LEN

using namespace std;

//
// CmdLineOpt - Default constructor.
//
CmdLineOpt::CmdLineOpt(Record* options, int numOptions) {

int optCounter;

   this->options = options;
   this->numOptions = numOptions;

   for (optCounter=0; optCounter<this->numOptions; optCounter++) {
   
//      this->options[optCounter].value[0] = NULL;  /* NULL here overwrites default values set in getUserInterface.cpp.  JHB, Feb 2015 */
      this->options[optCounter].nInstances = 0;
  }
}

//
// ~CmdLineOpt - Default (Null) destructor.
//
CmdLineOpt::~CmdLineOpt(void) {
}

//
// scanOptions - Scans a command line for any options.
//
bool CmdLineOpt::scanOptions(int argc, char* argv[], unsigned int uFlags) {

bool       rc = true;
int        optCounter;
int        optionFound;
char*      optionChar;
char       optionString[MAX_OPTIONS*2];
intptr_t   x;
long long  llx;
char*      p;
char*      p2;
char*      p3;
int        d[10] = {0,0,0,0,0,0,0,0,0,0};
int        i;
uint64_t   m[10] = {0,0,0,0,0,0,0,0,0,0};
uint64_t   ulx;
int        nInstances, nMultiple, valueSuffix;
char       suffix;
bool       x86;


   if (MAX_OPTIONS <= this->numOptions) {

      cout << "CmdLineOpt::scanOptions: Cannot have more than "
           << MAX_OPTIONS - 1 << "options." << endl;

      rc = false;
   }

   if (rc) {
   
      optionChar = optionString;

      for (optCounter=0; optCounter<this->numOptions; optCounter++) {
         
         *optionChar++ = this->options[optCounter].option;

         if (BOOLEAN != this->options[optCounter].type) *optionChar++ = ':';

         if (this->options[optCounter].option == 'L') *optionChar++ = ':';  /* for -L we use the GNU extension that allows an optional argument, so either just -L can be entered, or -Lsomething, JHB Sep2017 */
      }

      *optionChar = '\0';

   // Parse the command line string looking for the commands.

      optionFound = getopt(argc, argv, optionString);

      while (-1 != optionFound && ':' != optionFound) {

         for (optCounter=0; optCounter<this->numOptions; optCounter++) {

            if (this->options[optCounter].option == optionFound) {

               nInstances = this->options[optCounter].nInstances;

               switch (this->options[optCounter].type) {

                  case INTEGER:  /* usually accept entry in format -option NN, but also -option 0xNN and -option NN:NN:NN (up to 3 values) */

                     nMultiple = 0;
                     p = optarg;
                     valueSuffix = -1;

                     do {

                        p2 = p;

                        p = strstr(p2, ":");
                        if (p != NULL) *p++ = 0;

                        bool fHexVal = p2[0] == '0' && (p2[1] == 'x' || p2[1] == 'X');

                        if (strlen(p2) > 1) {  /* look for option suffix char */

                            suffix = p2[strlen(p2)-1];

                            if ((!fHexVal && suffix >= 'a' && suffix <= 'w') || (suffix >= 'y' && suffix <= 'z') || (suffix == 'x' && strlen(p2) > 3)) {  /* add hex value check, otherwise this suffix char code was stripping off the last a-f hex digit :-( JHB Dec 2019 */

                              valueSuffix = (int)(p2[strlen(p2)-1] - 'a' + 1);
                              p2[strlen(p2)-1] = 0;  /* remove suffix from option string before converting to int */
                           }
                        }

                        if (fHexVal) sscanf(&p2[2], "%x", (unsigned int*)&x);
                        else x = atoi(p2);

                        if (valueSuffix >= 0) {

                           x |= valueSuffix << 24;  /* option suffix value stored in bits 31-24.  Probably that could be 63-56 on 64-bit systems, but for now the typical option where a suffix is used doesn't have much range in values */
                        }

                        this->options[optCounter].value[nInstances][nMultiple] = (void*)x;

                     } while (p != NULL && ++nMultiple < MAX_MULTIPLES);

                     break;

                  case INT64:  /* 64-bit support, JHB Aug 2015 */

                     nMultiple = 0;
                     p = optarg;

                     do {

                        p2 = p;

                        p = strstr(p2, ":");
                        if (p != NULL) *p++ = 0;

                        if (p2[0] == '0' && (p2[1] == 'x' || p2[1] == 'X'))
                          sscanf(&p2[2], "%llx", (unsigned long long*)&llx);
                        else
                          llx = atol(p2);

                        this->options[optCounter].value3[nInstances] = llx;

                     } while (p != NULL && ++nMultiple < MAX_MULTIPLES);

                     break;

                  case IPADDR:  /* accept entry in format -Daa.bb.cc.dd:port:mm-mm-mm-mm-mm-mm, where a, b, c, d, and port are decimal numbers, and mm are hex digits */

                     p = strstr(optarg, ":");

                     if (p != NULL) {  /* get integer after ':' char */
   
                        *p++ = NULL;

                        if (p != NULL) {

                           p2 = strstr(p, ":");  /* entry after 2nd : is MAC addr */

                           if (p2 != NULL) {

                              *p2++ = 0;

                              p3 = strstr(p2, "-");
                              i = 0;

                              while ((p3 != NULL) || (p2 != NULL)) {

                                 if (p3 != NULL) *p3++ = 0;

                                 if (p2 != NULL) {

                                    sscanf(p2, "%x", (unsigned int*)&x);
                                    m[i++] = (uint64_t)x;
                                 }
                                 p2 = p3;
                                 if (p2 != NULL) p3 = strstr(p2, "-");
                                 else p3 = NULL;
                              }

                              ulx = (m[0] << 40) + (m[1] << 32) + (m[2] << 24) + (m[3] << 16) + (m[4] << 8) + m[5];

                              this->options[optCounter].value3[nInstances] = ulx;
                           }
                        }

                        x = atoi(p);
                        this->options[optCounter].value2[nInstances] = x;
                     }

                     p = strstr(optarg, ".");  /* IP addr format */

                     if (p != NULL) {

                        p2 = optarg;
                        i = 0;

                        while (p != NULL) {

                           *p++ = 0;
                           d[i++] = atoi(p2);
                           p2 = p;
                           p = strstr(p2, ".");
                        }

                        if (p2 != NULL) d[i++] = atoi(p2);

                        x = (d[0] << 24) + (d[1] << 16) + (d[2] << 8) + d[3];

                        this->options[optCounter].value[nInstances][0] = (void*)x;
                     }

                     break;

                  case CHAR:
                     x = (intptr_t)optarg[0];
                     this->options[optCounter].value[nInstances][0] = (void*)x;
                     break;

                  case STRING:
                     if ((char)optionFound == 'L' && optarg == NULL) {}  /* if only -L is entered (with no string value), don't overwrite the default value in getUserInterface.cpp, JHB Sep2017 */
                     else this->options[optCounter].value[nInstances][0] = (void*)optarg;
                     break;

                  case BOOLEAN:
                     this->options[optCounter].value[nInstances][0] = (void*)true;
                     break;

                  default:
                     cout << "Error in option -"
                          << this->options[optCounter].description
                          << ":" << endl;
                     cout << "  Unknown option type." << endl;
                     //this->printOptions( );
                     rc = false;
                     break;
               }

               if (rc) {
                  this->options[optCounter].nInstances++;
// debug   printf("option = %s, optCounter = %d, count = %d\n", optarg, optCounter, this->options[optCounter].nInstances);
               }
               break;
            }
         }

         if (optCounter == this->numOptions) {
            rc = false;
         }

         if (!rc && '?' == optionFound) {

            this->printOptions( );
            return false;
         }

         optionFound = getopt(argc, argv, optionString);
      }

      /* Disable mandatories for x86 - CJ Jan2017 */
      if (this->nInstances('c') && (!strcmp(this->getStr('c', 0), "x86") || !strcmp(this->getStr('c', 0), "X86"))) {

//         uFlags |= CLI_DISABLE_MANDATORIES;  /* no longer needed, JHB Aug 2018 */
         x86 = true;
      }
      else x86 = false;

      if ((uFlags & CLI_DISABLE_MANDATORIES) == 0) {  /* CLI_DISABLE_MANDATORIES added 11May15 JHB.  Using this should no longer be needed due to new isMandatory options (see cmdLineOpt.h), JHB Aug 2018 */

      // Find out if any mandatory options were omitted
         for (optCounter=0; optCounter<this->numOptions; optCounter++) {

            if (((this->options[optCounter].isMandatory == 1) || (this->options[optCounter].isMandatory == 2 && !x86)) &&
                  this->options[optCounter].nInstances == 0) {

               cout << "Error in options:" << endl;
               cout << "  Option -" << this->options[optCounter].option
                    << " is mandatory" << endl;
    
               this->printOptions( );
               rc = false;
               break;
            }
         }
      }
   }

   return rc;
}

//
// isSpecified - Identify if a command line option was provided or not.
//

int CmdLineOpt::nInstances(char option) {

Record *record = this->getOption(option);

   if (record) return record->nInstances;

   return 0;
}

//
// getInt - Returns the value of an integer command line option.
//
int CmdLineOpt::getInt(char option, int nInstance, int nMultiple) {

Record *record = this->getOption(option);
int value = 0;

   if (record && record->type == INTEGER) {
      value = (intptr_t)record->value[nInstance][nMultiple];
   }

   return value;
}

long long CmdLineOpt::getInt64(char option, int nInstance) {

Record *record = this->getOption(option);
long long value = 0;

   if (record && record->type == INT64) {
      value = (long long)record->value3[nInstance];
   }

   return value;
}

unsigned int CmdLineOpt::getIpAddr(char option, int nInstance) {

Record *record = this->getOption(option);
unsigned int value = 0;

   if (record && record->type == IPADDR) {
      value = (intptr_t)record->value[nInstance][0];
   }

   return value;
}

unsigned int CmdLineOpt::getUdpPort(char option, int nInstance) {

Record *record = this->getOption(option);
unsigned int value = 0;

   if (record && record->type == IPADDR) {
      value = (intptr_t)record->value2[nInstance];
   }

   return value;
}

uint64_t CmdLineOpt::getMacAddr(char option, int nInstance) {

Record *record = this->getOption(option);
uint64_t value = 0;

   if (record && record->type == IPADDR) {
      value = (uint64_t)record->value3[nInstance];
   }

   return value;
}


//
// getChar - Returns the value of a CHAR command line option
//
char CmdLineOpt::getChar(char option, int nInstance) {

Record *record = this->getOption(option);
char value = '\0';

   if (record && CHAR == record->type) {
      value = (char)(intptr_t)record->value[nInstance][0];
   }

   return value;
}

//
// getStr - Returns the value of a STRING command line option
//
char* CmdLineOpt::getStr(char option, int nInstance) {

Record *record = this->getOption(option);
char *value = NULL;

   if (record && record->type == STRING) {
      value = (char*)record->value[nInstance][0];
   }

   return value;
}

//
// getBool - Returns the value of a BOOLEAN command line option
//
bool CmdLineOpt::getBool(char option, int nInstance) {

Record *record = this->getOption(option);
bool value = false;

   if (record && record->type == BOOLEAN) {
      value = record->value[nInstance][0] ? true:false;
   }

   return value;
}

//
// display - Print the current value of all of the options.
//
void CmdLineOpt::display(void) {

int optCounter;

   cout << "Options from the command line:" << endl;

   for (optCounter=0;optCounter<this->numOptions;optCounter++) {

      switch (this->options[optCounter].type) {

         case INTEGER:
            cout << "  -" << this->options[optCounter].option << ": <"
                 << (intptr_t)this->options[optCounter].value[0][0] << ">\t"
                 << this->options[optCounter].description << endl;
            break;

         case CHAR:
            cout << "  -" << this->options[optCounter].option << ": <"
                 << (char)(intptr_t)this->options[optCounter].value[0][0] << ">\t"
                 << this->options[optCounter].description << endl;
            break;

         case STRING:
            cout << "  -" << this->options[optCounter].option << ": <"
                 << (char*)this->options[optCounter].value[0][0] << ">\t"
                 << this->options[optCounter].description << endl;
            break;

         case BOOLEAN:
            cout << "  -" << this->options[optCounter].option << ": <"
                 << (this->options[optCounter].value[0][0] ? "true" : "false")
                 << ">\t" << this->options[optCounter].description << endl;
            break;

         case IPADDR:
         case INT64:
            cout << "  -" << this->options[optCounter].option << ": <"
                 << (int64_t)this->options[optCounter].value[0][0] << ">\t"
                 << this->options[optCounter].description << endl;
            break;
      }
   }
}

//
// printOptions - Print a list of all the valid options and their description.
//
void CmdLineOpt::printOptions(void) {

int optCounter;
char type[32];

   cout << "Command line option syntax:" << endl;
   cout << "! is mandatory for all platforms" << endl;
   cout << "+ is mandatory for coCPU" << endl;

   for (optCounter=0; optCounter<this->numOptions; optCounter++) {

      switch (this->options[optCounter].type) {

         case INTEGER:
            strcpy( type, "(integer)" );
            break;

         case CHAR:
            strcpy( type, "(char)" );
            break;

         case STRING:
            strcpy( type, "(string)" );
            break;

         case BOOLEAN:
            strcpy( type, "(boolean)" );
            break;

         case INT64:
            strcpy( type, "(int64)" );
            break;

         case IPADDR:
            strcpy( type, "(IP Addr)" );
            break;

         default:
            strcpy( type, "(UNKNOWN)" );
            break;
      }

      cout << "    -" << this->options[optCounter].option << " "
           << setw( 11 );
      cout.setf( ios::right );    
      cout << type << ":"
           << (this->options[optCounter].isMandatory == 1 ? '!' : this->options[optCounter].isMandatory == 2 ? '+' : ' ')
           << this->options[optCounter].description << endl;
   }
}

//
// getOption - Retrieves options that were specified on the command line.
//
CmdLineOpt::Record* CmdLineOpt::getOption(char option) {

Record *record = NULL;
int optCounter;

   for (optCounter=0; optCounter<this->numOptions; optCounter++) {

       if (option == this->options[optCounter].option) {
         record = this->options + optCounter;
         break;
      }
   }

   return record;
}
