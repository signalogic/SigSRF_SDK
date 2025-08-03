/*
 $Header: /root/Signalogic/DirectCore/apps/common/cmdLineOpt.cpp
 
 Purpose: parse commanmd line options for SigSRF and DirectCore programs
  
 Copyright (C) Signalogic Inc. 2005-2025

 Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Revision History

   Modified Nov 2014 JHB, fixed some naming to support multiple targets, unify/consolidate command line params for all test programs
   Modified Dec 2014 JHB, added support for IP addr, UDP port, and MAC addr entry (e.g. -Daa.bb.cc.dd:port:aa-bb-cc-dd-ee-ff-gg)
   Modified Feb 2015 JHB, added support for multiple instances of some cmd line params.  Also fix problem with default values being overwritten
   Modified Jul 2015 JHB, added support for multiple integer values, in format -option NN:NN:NN
   Modified Jan 2017 CKJ, added support for x86 which doesn't require the same mandatories
   Modified Aug 2017 JHB, added support for program sub mode as a suffix character at end of INTEGER entries
   Modified Sep 2017 JHB, added a couple of exceptions for -L entry (log file), see comments
   Modified Jul 2018 JHB, changed mandatory requirements to handle x86 and coCPU separately
   Modified Dec 2019 JHB, fix bug in int value parsing, where suffix char code would strip off last a-f digit of hex values
   Modified Jan 2021 JHB, allow overloaded options, for example '-sN' integer for app type A, and '-sfilename' string for app type B. See comments below and in getUserInterface.cpp
   Modified Dec 2022 JHB, start work on allowing input specs to include IP addr:port type of input, e.g. -iaa.bb.cc.dd:port:mm-mm-mm-mm-mm-mm. Inputs are strings, so we first look for xx.xx... and xx:xx patterns, if found convert those to IP addr:port, if not then assume it's a path/file input. Code for IPADR input type can be re-used
   Modified May 2023 JHB, support FLOAT option type, add FLOAT case to switch statements, add getFloat(), change getUdpPort() from unsigned int to uint16_t
   Modified Jul 2023 JHB, start using getopt_long(); initially we support --version cmd line option
   Modified Jul 2023 JHB, several improvements in error handling and options printout when wrong things are entered
   Modified Dec 2023 JHB, add --group_pcap cmd line option
   Modified Feb 2024 JHB, add --md5sum and --show_aud_clas cmd line options
   Modified Feb 2024 JHB, handle "no_argument" case for long options (look for fNoArgument)
   Modified Feb 2024 JHB, add static to long_options[]. Reference apps (mediaTest, mediaMin) and both cimlib.so pull in cmdLineOpt.cpp and depending on gcc and ld version, without static long_options[] may appear twice and cause warning message such as "/usr/bin/ld: warning: size of symbol `long_options' changed from 160 in cmdLineOpt.o (symbol from plugin) to 192 in /tmp/ccqwricj.ltrans0.ltrans.o"
   Modified Jul 2024 JHB, add --group_pcap_nocopy and --random_bit_error cmd line options
   Modified Aug 2024 JHB, add --sha1sum and --sha512sum cmd line options
   Modified Mar 2025 JHB, handle ALLOW_XX attributes defined in cmdLineOpt.h for overloaded options, for example -rN can accept N either int or float and fInvalidFormat is not set if the option can't be converted to a valid integer
   Modified Jul 2025 JHB, add --profile_stdout_ready and --exclude_payload_type_from_key cmd line options
*/

#include <stdint.h>

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <getopt.h>

#include "cmdLineOpt.h"
#include "alias.h"
#include "userInfo.h"

#define MAX_OPTIONS MAX_INPUT_LEN

using namespace std;

/* used when calling getopt_long(), JHB Jul 2023 */

#define requires_argument required_argument  /* convenient definition to fix grammar in getopt_long() definitions */

static const struct option long_options[] = { { "version", no_argument, NULL, (char)128 }, { "cut", requires_argument, NULL, (char)129 }, { "group_pcap", requires_argument, NULL, (char)130 }, { "group_pcap_nocopy", requires_argument, NULL, (char)131 }, { "md5sum", no_argument, NULL, (char)132 }, { "sha1sum", no_argument, NULL, (char)133 }, { "sha512sum", no_argument, NULL, (char)134 }, { "show_aud_clas", no_argument, NULL, (char)135 }, { "random_bit_error", requires_argument, NULL, (char)136 },  { "profile_stdout_ready", no_argument, NULL, (char)137 }, { "exclude_payload_type_from_key", no_argument, NULL, (char)138 }, /* insert additional options here */ {NULL, 0, NULL, 0 } };

//
// CmdLineOpt - Default constructor.
//
CmdLineOpt::CmdLineOpt(Record* options, int numOptions) {

int optCounter;

   this->options = options;
   this->numOptions = numOptions;

   for (optCounter=0; optCounter<this->numOptions; optCounter++) {
   
//      this->options[optCounter].value[0] = NULL;  /* NULL here overwrites default values set in CmdLineOpt::Record options[] in getUserInterface.cpp, JHB, Feb 2015 */
      this->options[optCounter].nInstances = 0;
  }
}

//
// ~CmdLineOpt - Default (Null) destructor.
//
CmdLineOpt::~CmdLineOpt(void) {
}

/* helper function to validate decimal, float, and hex number entry, JHB Nov 2023 */

bool valid_number(const char* num, bool fHexVal, bool fAllowFloat) {

   #if 0
   printf("inside valid_number num = %s, fHexVal = %d \n", num, fHexVal);
   #endif

   if (fHexVal) {
      for (int i=0; i<(int)strlen(num); i++) if ((num[i] < '0' || num[i] > '9') && num[i] != 'a' && num[i] != 'A' && num[i] != 'b' && num[i] != 'B' && num[i] != 'c' && num[i] != 'C' && num[i] != 'd' && num[i] != 'D' && num[i] != 'e' && num[i] != 'E' && num[i] != 'f' && num[i] != 'F') return false;
   }
   else for (int i=0; i<(int)strlen(num); i++) if ((num[i] < '0' || num[i] > '9') && num[i] != '-' && num[i] != '+' && (!fAllowFloat || (num[i] != '.' && num[i] != 'e' && num[i] != 'E'))) return false;

   return true;
}

//
// scanOptions - Scans a command line for any options.
//
bool CmdLineOpt::scanOptions(int argc, char* argv[], unsigned int uFlags) {

bool       fError = false;
int        optCounter;
int        optionFound;
char*      optionChar;
char       optionString[MAX_OPTIONS*2];
intptr_t   x;
float      f;
long long  llx;
char*      p, *p2, *p3;
int        d[10] = { 0 };
int        i, ret;
uint64_t   m[10] = { 0 };
uint64_t   ulx;
int        instance_index, nMultiple, valueSuffix;
char       suffix;
bool       fx86, fOptionFound, fInvalidFormat;
char       tmpstr[CMDOPT_MAX_INPUT_LEN];

int long_option_index = -1;  /* index of long option, if found */

   if (MAX_OPTIONS <= this->numOptions) {

      cout << "CmdLineOpt::scanOptions: Cannot have more than " << MAX_OPTIONS - 1 << "options." << endl;
      return false;
   }

/* build a string of all possible options */

   optionChar = optionString;

   for (optCounter=0; optCounter<this->numOptions; optCounter++) {

      *optionChar++ = this->options[optCounter].option;

      if (this->options[optCounter].type != BOOLEAN) *optionChar++ = ':';

      if (this->options[optCounter].option == 'L') *optionChar++ = ':';  /* for -L we use GNU extension char (:) that allows an optional argument, so either just -L can be entered, or -Lsomething, JHB Sep 2017. Note for long options this is done by defining no_argument in option long_options[], JHB Feb 2024 */
   }

   *optionChar = '\0';

/* call GNU API getopt() in a while loop to parse cmd line string option-by-option and compare with optionString looking for valid options. Note we are now using getopt_long(), which allows input form --option (where '--' indicates a "long" option), JHB Jul 2023 */

   bool fFirstOption = true;
   opterr = 0;  /* we turn off opterr so we handle all error types (missing mandatory option, unrecognized option, option requires an argument that wasn't given, etc. See error handling below, after the while loop, JHB Nov 2023 */

   while ((optionFound = getopt_long(argc, argv, optionString, long_options, &long_option_index)) != -1 && optionFound != ':') {

      #ifdef GETOPT_DEBUG
      printf(" *** %s returned = %d, optionString = %s, optarg = %s, long option index = %d \n", optionFound != '?' ? "option" : "error char ?", optionFound, optionString, optarg, long_option_index);
      #endif

      if (fFirstOption) {

         if (optarg) {

            string cpp_optarg(optarg);

            if (cpp_optarg == "?") {  /* handle single "?" on cmd line, a legacy way of asking for cmd line help, JHB Jul 2023 */

               this->printOptions();
               return false;
            }
         }

         fFirstOption = false;
      }

      fOptionFound = false;
      fInvalidFormat = false;

      for (optCounter=0; optCounter<this->numOptions; optCounter++) {

         if (this->options[optCounter].option == optionFound) {

            instance_index = this->options[optCounter].nInstances;  /* options[].nInstance starts at zero and increments each time an instance is found */

            #ifdef GETOPT_DEBUG
            printf(" *** in loop option = %d \n", optionFound);
            #endif

            switch (this->options[optCounter].type & OPTION_TYPE_MASK) {

               case INTEGER:  /* usually accept entry in format -option NN, but also -option 0xNN and -option NN:NN:NN (up to 3 values) */

                  nMultiple = 0;
                  if (!optarg) {
                     #ifdef GETOPT_DEBUG
                     printf("optarg is NULL for option = %d \n", this->options[optCounter].option);
                     #endif
                     break;
                  }
                  strcpy(tmpstr, optarg);  /* temporary working buffer */
                  p = tmpstr;
                  valueSuffix = -1;

                  do {

                     p2 = p;

                     p = strstr(p2, ":");
                     if (p != NULL) *p++ = 0;

                     bool fHexVal = p2[0] == '0' && (p2[1] == 'x' || p2[1] == 'X');  /* hex values must have 0x prefix */

                     if (strlen(p2) > 1) {  /* look for option suffix char */

                        suffix = p2[strlen(p2)-1];

                        if ((!fHexVal && suffix >= 'a' && suffix <= 'w') || (suffix >= 'y' && suffix <= 'z') || (suffix == 'x' && strlen(p2) > 3)) {  /* add hex value check, otherwise this suffix char code was stripping off the last a-f hex digit :-( JHB Dec 2019 */

                           valueSuffix = (int)(p2[strlen(p2)-1] - 'a' + 1);
                           p2[strlen(p2)-1] = 0;  /* remove suffix from option string before converting to int */
                        }
                     }

                     if (fHexVal) ret = sscanf(&p2[2], "%x", (unsigned int*)&x);
                     #if 0
                     else x = atoi(p2);
                     #else
                     else ret = sscanf(p2, "%d", (int*)&x);
                     #endif

                     if ((ret != 1 || !valid_number(fHexVal ? &p2[2] : p2, fHexVal, this->options[optCounter].type & ALLOW_FLOAT)) && !(this->options[optCounter].type & ALLOW_STRING)) fInvalidFormat = true;

                     if (valueSuffix >= 0) {

                        x |= valueSuffix << 24;  /* option suffix value stored in bits 31-24.  Probably that could be 63-56 on 64-bit systems, but for now the typical option where a suffix is used doesn't have much range in values */
                     }

                     this->options[optCounter].value[instance_index][nMultiple] = (void*)x;

                  } while (p != NULL && ++nMultiple < MAX_MULTIPLES);

                  fOptionFound = true;
                  break;

               case INT64:  /* 64-bit support, JHB Aug 2015 */

                  nMultiple = 0;
                  strcpy(tmpstr, optarg);  /* temporary working buffer */
                  p = tmpstr;

                  do {

                     bool fHexVal = false;
                     p2 = p;

                     p = strstr(p2, ":");
                     if (p != NULL) *p++ = 0;

                     if (p2[0] == '0' && (p2[1] == 'x' || p2[1] == 'X')) {
                        ret = sscanf(&p2[2], "%llx", (unsigned long long*)&llx);
                        fHexVal = true;
                     }
                     else
                       #if 0
                       llx = atol(p2);
                       #else
                       ret = sscanf(p2, "%lld", (unsigned long long*)&llx);
                       #endif

                     if ((ret != 1 || !valid_number(fHexVal ? &p2[2] : p2, fHexVal, false)) && !(this->options[optCounter].type & ALLOW_STRING)) fInvalidFormat = true;

                     this->options[optCounter].value3[instance_index] = llx;

                  } while (p != NULL && ++nMultiple < MAX_MULTIPLES);

                  fOptionFound = true;
                  break;

               case IPADDR:  /* accept entry in format -Daa.bb.cc.dd:port:mm-mm-mm-mm-mm-mm, where a, b, c, d, and port are decimal numbers, and mm are hex digits. Also allow -iaa.bb.cc.dd:port:mm-mm-mm-mm-mm-mm for input UDP ports, JHB Dec 2022 */

                  strcpy(tmpstr, optarg);
                  p = strstr(tmpstr, ":");

                  if (p != NULL) {  /* get integer after ':' char */
   
                     *p++ = 0;

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

                           this->options[optCounter].value3[instance_index] = ulx;
                        }
                     }

                     x = atoi(p);
                     this->options[optCounter].value2[instance_index] = x;
                  }

                  p = strstr(tmpstr, ".");  /* IP addr format */

                  if (p != NULL) {

                     p2 = tmpstr;
                     i = 0;

                     while (p != NULL) {

                        *p++ = 0;
                        d[i++] = atoi(p2);
                        p2 = p;
                        p = strstr(p2, ".");
                     }

                     if (p2 != NULL) d[i++] = atoi(p2);

                     x = (d[0] << 24) + (d[1] << 16) + (d[2] << 8) + d[3];

                     this->options[optCounter].value[instance_index][0] = (void*)x;
                  }

                  fOptionFound = true;
                  break;

               case FLOAT:  /* add FLOAT type, May 2023 JHB */
                  #if 0
                  f = atof(optarg);
                  #else
                  ret = sscanf(optarg, "%f", (float*)&f);
                  if ((ret != 1 || !valid_number(optarg, false, true)) && !(this->options[optCounter].type & ALLOW_STRING)) fInvalidFormat = true;  /* not a hex value, allow float chars */
                  #endif

                  #if 0
                  printf(" inside FLOAT argument = %s, ret = %d, fInvalidFormat = %d \n", optarg, ret, fInvalidFormat);
                  #endif
                  memcpy(&x, &f, sizeof(float));  /* hack to store float in a void* */
                  this->options[optCounter].value[instance_index][0] = (void*)x;
                  fOptionFound = true;
                  break;

               case CHAR:
                  x = (intptr_t)optarg[0];
                  this->options[optCounter].value[instance_index][0] = (void*)x;  /* store first optarg char in void* */
                  fOptionFound = true;
                  break;

               case STRING:
                  if ((char)optionFound == 'L' && optarg == NULL) {}  /* if only -L is entered (with no string value), don't overwrite the default value in getUserInterface.cpp, JHB Sep2017 */
                  else this->options[optCounter].value[instance_index][0] = (void*)optarg;
                  fOptionFound = true;
                  break;

               case BOOLEAN:
                  this->options[optCounter].value[instance_index][0] = (void*)true;  /* if the option exist on command line its true */
                  fOptionFound = true;
                  break;

               default:
                  cout << "Error in option -" << this->options[optCounter].description << ":" << endl;
                  cout << "  Unknown option type" << endl;
                  fError = true;
                  break;
            }

            if (!fError) {
               this->options[optCounter].nInstances++;
// debug   printf("option = %s, optCounter = %d, count = %d\n", optarg, optCounter, this->options[optCounter].nInstances);
            }

            #if 0  /* remove this for-loop break so all defined options will be checked vs. cmd line option found. This allows options to be overloaded (e.g. two 's' definitions, one integer for app type A, one string for app type B), JHB Jan 2021 */ 
            break;
            #endif
         }

      }  /* end of for loop comparing optionFound vs allowed options */

   /* error handling - note we have opterr turned off so we handle all error types (missing mandatory option, unrecognized option, invalid format, option requires an argument that wasn't given, etc, JHB Nov 2023 */

      if (!fOptionFound || fInvalidFormat) {

         char cmdoptstr[500];
         strcpy(cmdoptstr, argv[max(0, optind-1)]);
         bool fNeedsArgument = false, fNoArgument = false;
         int long_index;

         for (i=0; i<this->numOptions; i++) {

            char optstr[3] = "";
            optstr[0] = '-';
            optstr[1] = this->options[i].option;
            optstr[2] = 0;
            if (!strcmp(cmdoptstr, optstr)) {  /* check for short options that need an argument */
               fNeedsArgument = true;
               break;
            }

            if ((long_index = (unsigned char)this->options[i].option - 128) >= 0 && strlen(cmdoptstr) > 2 && !strcmp(&cmdoptstr[2], long_options[long_index].name)) {  /* check for long arguments that need an argument. The strcmp() is a sanity check because we already matched options[i].option */

               if (long_options[long_index].has_arg == no_argument) fNoArgument = true;
               else if (long_options[long_index].has_arg == requires_argument) fNeedsArgument = true;
               break;
            }
         }

         if (!fNoArgument) {

            cout << "Option " << cmdoptstr << " " << (fInvalidFormat ? "invalid format" : (fNeedsArgument ? "requires an argument" : "is unrecognized")) << endl;  /* option has invalid format, not found, or it requires an argument. Continue processing all cmd line args, JHB Jan2021. Add informative display for the type of error (note that optionFound is useless in error case, getopt_long() will return it as "?"), JHB Jul 2023 */
            fError = true;
         }
      }

   }  /* end of while (optionFound = getopt_long() ...) loop */

   if (fError) {
      this->printOptions();
      return false;
   }

/* Disable mandatories for x86 - CJ Jan2017 */
//   if (this->nInstances('c') && (!strcmp(this->getStr('c', 0), "x86") || !strcmp(this->getStr('c', 0), "X86"))) {
   if (this->nInstances('c') && !strcasecmp(this->getStr('c', 0), "x86")) {

//     uFlags |= CLI_DISABLE_MANDATORIES;  /* no longer needed, JHB Aug 2018 */
      fx86 = true;
   }
   else fx86 = false;

   if ((uFlags & CLI_DISABLE_MANDATORIES) == 0) {  /* CLI_DISABLE_MANDATORIES added 11May15 JHB.  Using this should no longer be needed due to new isMandatory options (see cmdLineOpt.h), JHB Aug 2018 */

   // Find out if any mandatory options were omitted
      for (optCounter=0; optCounter<this->numOptions; optCounter++) {

         if ((((this->options[optCounter].isMandatory == 1) || (this->options[optCounter].isMandatory == 2 && !fx86)) && this->options[optCounter].nInstances == 0) && !this->getPosition('-', STRING) == 0) {

            cout << "Error in options:" << endl;
            cout << "  Option -" << this->options[optCounter].option << " is mandatory" << endl;
    
            this->printOptions( );
            fError = true;
            break;
         }
      }
   }

   return !fError;
}

//
// isSpecified - Identify if a command line option was provided or not
//
int CmdLineOpt::nInstances(char option) {

Record *record = this->getOption(option, -1);

   if (record) return record->nInstances;

   return 0;
}

//
// getInt - Returns the value of an integer command line option.
//
int CmdLineOpt::getInt(char option, int nInstance, int nMultiple) {

Record *record = this->getOption(option, INTEGER);
int value = 0;

   #if 0
   if (record && record->type == INTEGER) {
      value = (intptr_t)record->value[nInstance][nMultiple];
   }
   #else
   if (record) value = (intptr_t)record->value[nInstance][nMultiple];
   #endif

   return value;
}

float CmdLineOpt::getFloat(char option, int nInstance, int nMultiple) {  /* add getFloat() JHB May 2023 */

Record *record = this->getOption(option, FLOAT);
float value = 0;

   #if 0
   if (record && record->type == FLOAT) {
      value = (float*)record->value[nInstance][nMultiple];
   }
   #else
   if (record) memcpy(&value, &record->value[nInstance][nMultiple], sizeof(float));  /* reverse the hack in FLOAT case in scanOptions(), JHB May 2023 */
   #endif

   return value;
}

long long CmdLineOpt::getInt64(char option, int nInstance) {

Record *record = this->getOption(option, INT64);
long long value = 0;

   #if 0
   if (record && record->type == INT64) {
      value = (long long)record->value3[nInstance];
   }
   #else
   if (record) value = (int64_t)record->value3[nInstance];
   #endif

   return value;
}

unsigned int CmdLineOpt::getIpAddr(char option, int nInstance) {

Record *record = this->getOption(option, IPADDR);
unsigned int value = 0;

   #if 0
   if (record && record->type == IPADDR) {
      value = (intptr_t)record->value[nInstance][0];
   }
   #else
   if (record) value = (intptr_t)record->value[nInstance][0];
   #endif

   return value;
}

uint16_t CmdLineOpt::getUdpPort(char option, int nInstance) {

Record *record = this->getOption(option, IPADDR);
uint16_t value = 0;

   #if 0
   if (record && record->type == IPADDR) {
      value = (intptr_t)record->value2[nInstance];
   }
   #else
   if (record) value = (intptr_t)record->value2[nInstance];
   #endif

   return value;
}

uint64_t CmdLineOpt::getMacAddr(char option, int nInstance) {

Record *record = this->getOption(option, IPADDR);
uint64_t value = 0;

   #if 0
   if (record && record->type == IPADDR) {
      value = (uint64_t)record->value3[nInstance];
   }
   #else
   if (record) value = (uint64_t)record->value3[nInstance];
   #endif

   return value;
}


//
// getChar - Returns the value of a CHAR command line option
//
char CmdLineOpt::getChar(char option, int nInstance) {

Record *record = this->getOption(option, CHAR);
char value = '\0';

   #if 0
   if (record && CHAR == record->type) {
      value = (char)(intptr_t)record->value[nInstance][0];
   }
   #else
   if (record) value = (char)(intptr_t)record->value[nInstance][0];
   #endif

   return value;
}

//
// getStr - Returns the value of a STRING command line option
//
char* CmdLineOpt::getStr(char option, int nInstance) {

Record *record = this->getOption(option, STRING);
char *value = NULL;

   #if 0
   if (record && record->type == STRING) {
      value = (char*)record->value[nInstance][0];
   }
   #else
   if (record) value = (char*)record->value[nInstance][0];
   #endif

   return value;
}

//
// getBool - Returns the value of a BOOLEAN command line option
//
bool CmdLineOpt::getBool(char option, int nInstance) {

Record *record = this->getOption(option, BOOLEAN);
bool value = false;

   #if 0
   if (record && record->type == BOOLEAN) {
      value = record->value[nInstance][0] ? true:false;
   }
   #else
   if (record) value = record->value[nInstance][0] ? true:false;
   #endif

   return value;
}

#if 0  /* currently not used, JHB Nov 2023 */

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

         case FLOAT:  /* add FLOAT case, JHB May 2023 */
            cout << "  -" << this->options[optCounter].option << ": <"
                 << (float)(intptr_t)this->options[optCounter].value[0][0] << ">\t"
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
#endif

//
// printOptions - Print a list of all the valid options and their description
//
void CmdLineOpt::printOptions(void) {

int optCounter;
char type[32], option[32];
int long_index;

   cout << "Command line option syntax:" << endl;
   cout << "! is mandatory for all platforms" << endl;
   cout << "+ is mandatory for coCPU" << endl;

   for (optCounter=0; optCounter<this->numOptions; optCounter++) {

   /* build presentable string for either short or long options, JHB Nov 2023 */

      if ((long_index = (unsigned char)this->options[optCounter].option - 128) >= 0) { strcpy(option, "--"); strcat(option, long_options[long_index].name); }
      else { option[0] = '-'; option[1] = this->options[optCounter].option; option[2] = 0; }

      switch (this->options[optCounter].type) {

         case INTEGER:
            strcpy( type, "(integer)" );
            break;

         case FLOAT:  /* add FLOAT case, JHB May 2023 */
            strcpy( type, "(float)" );
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

      cout << "    " << option << " " << setw( 11 );
      cout.setf( ios::right );    
      cout << type << ":" << (this->options[optCounter].isMandatory == 1 ? '!' : this->options[optCounter].isMandatory == 2 ? '+' : ' ') << this->options[optCounter].description << endl;
   }
}

//
// getOption (private) - Retrieves an option if specified on the command line, including type match
//
CmdLineOpt::Record* CmdLineOpt::getOption(char option, int type) {

Record *record = NULL;
int optCounter;

   for (optCounter=0; optCounter<this->numOptions; optCounter++) {

      if (option == this->options[optCounter].option) {

         if (type == -1 || type == (this->options + optCounter)->type) {  /* checking for type here allows options to be overloaded; e.g. two options for '-s' of different types can be defined and checked in getUserInfo(), but instead of first one found here being returned, it also has to match the type check before being returned, JHB Jan2021 */

            record = this->options + optCounter;
            break;
         }
      }
   }

   return record;
}

//
// getPosition (public) - Retrieves position of option if specified on the command line (similar to getOption above), JHB Jul 2023
//
int CmdLineOpt::getPosition(char option, Type type) {

int optCounter, pos = -1;

   for (optCounter=0; optCounter<this->numOptions; optCounter++) {

      if (option == this->options[optCounter].option) {

         if (type == -1 || type == (this->options + optCounter)->type) {

            pos = optCounter;
            break;
         }
      }
   }

   return pos;
}
