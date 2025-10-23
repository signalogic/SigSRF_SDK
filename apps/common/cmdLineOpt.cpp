/*
 $Header: /root/Signalogic/DirectCore/apps/common/cmdLineOpt.cpp
 
 Purpose: parse commanmd line options for SigSRF and DirectCore programs
  
 Copyright (C) Signalogic Inc. 2005-2025

 Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Revision History

   Modified Nov 2014 JHB, fixed some naming to support multiple targets, unify/consolidate command line options for all test programs
   Modified Dec 2014 JHB, added support for IP addr, UDP port, and MAC addr entry (e.g. -Daa.bb.cc.dd:port:aa-bb-cc-dd-ee-ff-gg)
   Modified Feb 2015 JHB, added support for multiple instances of some cmd line options.  Also fix problem with default values being overwritten
   Modified Jul 2015 JHB, added support for multiple integer values, in format -option NN:NN:NN
   Modified Jan 2017 CKJ, added support for x86 which doesn't require the same mandatories
   Modified Aug 2017 JHB, added support for program sub mode as a suffix character at end of ARG_TYPE_INT entries
   Modified Sep 2017 JHB, added a couple of exceptions for -L entry (log file), see comments
   Modified Jul 2018 JHB, changed mandatory requirements to handle x86 and coCPU separately
   Modified Dec 2019 JHB, fix bug in int value parsing, where suffix char code would strip off last a-f digit of hex values
   Modified Jan 2021 JHB, allow overloaded options, for example '-sN' integer for app type A, and '-sfilename' string for app type B. See comments below and in getUserInterface.cpp
   Modified Dec 2022 JHB, start work on allowing input specs to include IP addr:port type of input, e.g. -iaa.bb.cc.dd:port:mm-mm-mm-mm-mm-mm. Inputs are strings, so we first look for xx.xx... and xx:xx patterns, if found convert those to IP addr:port, if not then assume it's a path/file input. Code for IPADR input type can be re-used
   Modified May 2023 JHB, support FLOAT option type, add FLOAT case to switch statements, add getFloat(), change getUdpPort() from unsigned int to uint16_t
   Modified Jul 2023 JHB, start using getopt_long(); initially we support --version cmd line option
   Modified Jul 2023 JHB, several improvements in error handling and options printout when wrong things are entered
   Modified Dec 2023 JHB, add --group_pcap_path cmd line option
   Modified Feb 2024 JHB, add --md5sum and --show_aud_clas cmd line options
   Modified Feb 2024 JHB, handle "no_argument" case for long options (look for fNoArgument)
   Modified Feb 2024 JHB, add static to long_options[]. Reference apps (mediaTest, mediaMin) and both cimlib.so pull in cmdLineOpt.cpp and depending on gcc and ld version, without static long_options[] may appear twice and cause warning message such as "/usr/bin/ld: warning: size of symbol `long_options' changed from 160 in cmdLineOpt.o (symbol from plugin) to 192 in /tmp/ccqwricj.ltrans0.ltrans.o"
   Modified Jul 2024 JHB, add --group_pcap_path_nocopy and --random_bit_error cmd line options
   Modified Aug 2024 JHB, add --sha1sum and --sha512sum cmd line options
   Modified Mar 2025 JHB, handle ARG_ALLOW_XX enums defined in cmdLineOpt.h for overloaded options, for example -rN can accept N either int or float and fInvalidFormat is not set if the option can't be converted to a valid integer
   Modified Jul 2025 JHB, add --profile_stdout_ready and --exclude_payload_type_from_key command line options
   Modified Aug 2025 JHB, add --stdout_mode command line option
   Modified Sep 2025 JHB, add --event_log_path and --suppress_packet_info_messages options
   Modified Sep 2025 JHB, implement ARG_TYPE_NONE and ARG_OPTIONAL enums defined in cmdLineOpt.h, rework option display content and format (i.e. when -h or ? is entered)
   Modified Sep 2025 JHB, improved cmd line syntax error reporting:
                            -non-options (i.e. text or wrong syntax without preceding - or --)
                            -unrecognized options (getopt_long() returns option error character '?')
                            -options missing required argument (getopt_long() returns ':')
                            -if cmd line has multiple errors report as many as possible
   Modified Oct 2025 JHB, support optional argument long options with a space instead of '=' before the argument (e.g. --suppress_packet_info_messages 1)
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

#define requires_argument required_argument  /* definition to fix GNU grammar error in getopt_long() definitions */

static const struct option long_options[] = { { "version", no_argument, NULL, (char)128 }, { "cut", requires_argument, NULL, (char)129 }, { "group_pcap_path", requires_argument, NULL, (char)130 }, { "group_pcap_path_nocopy", requires_argument, NULL, (char)131 }, { "md5sum", no_argument, NULL, (char)132 }, { "sha1sum", no_argument, NULL, (char)133 }, { "sha512sum", no_argument, NULL, (char)134 }, { "show_aud_clas", no_argument, NULL, (char)135 }, { "random_bit_error", requires_argument, NULL, (char)136 },  { "profile_stdout_ready", no_argument, NULL, (char)137 }, { "exclude_payload_type_from_key", no_argument, NULL, (char)138 }, { "disable_codec_flc", no_argument, NULL, (char)139 },  { "stdout_mode", requires_argument, NULL, (char)140 }, { "event_log_path", requires_argument, NULL, (char)141 }, { "suppress_packet_info_messages", optional_argument, NULL, (char)142 }, /* insert additional options here */ {NULL, 0, NULL, 0 } };

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

/* helper function to build a string describing argument error cases */

int CmdLineOpt::arg_error_reporting(char* arg_info_str, ArgType arg_types[], int num_arg_types) {

auto isVowel = [](char ch) { return ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u'; };

   if (!arg_info_str) return -1;

   for (int i=0; i<num_arg_types; i++) {

      if (arg_types[i] != ARG_TYPE_NONE) {

         std::string arg_type_str = "";

         switch (arg_types[i]) {

            case ARG_TYPE_INT:
               arg_type_str = "int";
               break;
            case ARG_TYPE_INT64:
               arg_type_str = "int64";
               break;
            case ARG_TYPE_CHAR:
               arg_type_str = "char";
               break;
            case ARG_TYPE_STR:
               arg_type_str = "str";
               break;
            case ARG_TYPE_PATH:
               arg_type_str = "path";
               break;
            case ARG_TYPE_BOOL:
               arg_type_str = "bool";
               break;
            case ARG_TYPE_IPADDR:
               arg_type_str = "IP addr";
               break;
            case ARG_TYPE_FLOAT:
               arg_type_str = "float";
               break;
            default:
               break;
         }

         if (i == 0) sprintf(&arg_info_str[strlen(arg_info_str)], ", requires ");
         else sprintf(&arg_info_str[strlen(arg_info_str)], " or ");
         sprintf(&arg_info_str[strlen(arg_info_str)], "a%s %s", isVowel(arg_type_str.c_str()[0]) ? "n" : "", arg_type_str.c_str());
         if (i == num_arg_types-1) sprintf(&arg_info_str[strlen(arg_info_str)], " arg ");
      }
   }

   return strlen(arg_info_str);
}

//
// scanOptions - Scans a command line for any options.
//
bool CmdLineOpt::scanOptions(int argc, char* argv[], unsigned int uFlags) {

bool       fError = false, fPrintOptions = false, fMissingRequiredArg = false;
int        optCounter;
int        optionFound;
char       optionString[MAX_OPTIONS*2];
char*      p, *p2, *p3;
int        ret;
int        instance_index, nMultiple, valueSuffix;
char       suffix;
bool       fx86, fOptionFound, fInvalidFormat;
char       tmpstr[CMDOPT_MAX_INPUT_LEN];

int long_option_index = -1;  /* index of long option, if found */

   #ifdef GETOPT_DEBUG
   FILE* fpCmdLine = fopen("/proc/self/cmdline", "rb");

   if (fpCmdLine) {

      char cmdlinestr[500];
      int ret = fread(cmdlinestr, sizeof(char), sizeof(cmdlinestr), fpCmdLine);  /* unknown as to why size must be 1 and count must be size, I got that from https://stackoverflow.com/questions/1406635/parsing-proc-pid-cmdline-to-get-function-parameters which mentions cmd line options are separated by NULLs. But still doesn't make sense since file is opened in binary mode. Maybe because OS forces the file to open in text mode */
      fclose(fpCmdLine);

      for (int i=0; i<ret; i++) if (cmdlinestr[i] == 0) cmdlinestr[i] = ' ';  /* replace NULLs with spaces */

      printf(" start of CmdLineOpt::scanOptions, cmd line: %s \n", cmdlinestr);  /* show whole command line as entered */
   }
   #endif

   if (this->numOptions > MAX_OPTIONS) {

      cout << " number of cmd line options exceeds " << MAX_OPTIONS << endl;
      fError = true;
      return false;
   }

/* build a string of all possible options */

   for (int i=0,optCounter=0; optCounter < this->numOptions; optCounter++) {

      if (i == 0) { optionString[i++] = '+'; optionString[i++] = ':'; }  /* indicate 1) POSIX compliance (disable option permutation) and 2) we want to know about options missing a required argument, JHB Oct 2025 */

      optionString[i++] = this->options[optCounter].option;

      if ((this->options[optCounter].arg_type & ARG_TYPE_MASK) != ARG_TYPE_NONE) optionString[i++] = ':';

      #if 0
      if (this->options[optCounter].option == 'L') optionString[i++]  = ':';  /* for -L we use a second : char to indicate an optional argument, so either just -L can be entered, or -Lsomething (note this is a GNU extension; see "two colons" in https://linux.die.net/man/3/optarg), JHB Sep 2017. Note for long options this is done by defining no_argument in option long_options[], JHB Feb 2024 */
      #else
      if (this->options[optCounter].arg_type & ARG_OPTIONAL) optionString[i++]  = ':';  /* we use a second ":" to indicate an optional argument. For example, just -L or -L log_file_path can be entered (note that "two colons" is a GNU extension; see https://linux.die.net/man/3/optarg), JHB Sep 2017. For long options this is done by specifying optional_argument in option long_options[] above, JHB Feb 2024. General optional arguments are now supported with ARG_OPTIONAL enum defined in cmdLineOpt.h. For long options *both* ARG_OPTIONAL should be set in the CmdLineOpt::Record options[] definition in getUserInterface.cpp and optional_argument set in the long_options[] struct definition above, JHB Sep 2025 */
      #endif

      if (optCounter == this->numOptions-1) optionString[i] = (char)0;  /* if last option terminate string */
   }

/* call GNU API getopt() in a while loop to parse cmd line string option-by-option and compare with optionString looking for valid options. Note we are now using getopt_long(), which allows input form --option (where '--' indicates a "long" option), JHB Jul 2023 */

   opterr = 0;  /* we turn off opterr so we handle all error types (unrecognized option, option requires an argument that wasn't given, etc. See error handling below, both after each option inside the while loop, and after the loop, JHB Nov 2023 */

   while ((optionFound = getopt_long(argc, argv, optionString, long_options, &long_option_index)) != -1) {

      #ifdef GETOPT_DEBUG
      static int arg_count = 1;
      printf(" *** opt %d/%d %s returned = %d, optionString = %s, optarg = %s, long option index = %d \n", arg_count++, argc, optionFound != '?' ? "option" : "error char ?", optionFound, optionString, optarg, long_option_index);  /* optarg is declared extern char* */
      #endif

   /* support optional argument long opts with space instead of '=' before the argument, Oct 2025 */

      if (optind < argc && argv[optind][0] != '-' && !optarg) {  /* option without preceding '-' or '--' could be legit if it follows an optional argument. If so handle it here, otherwise it gets caught after while (getopt_long) loop finishes */

         for (int i=0; i<this->numOptions; i++) if (this->options[i].option == optionFound && (this->options[i].arg_type & ARG_OPTIONAL)) { optarg = argv[optind++]; break; }  /* if option has optional argument flag set optarg to whatever was entered and advance getopt_long()'s index */
      }

      if (!(argc == 2 && !strcmp(argv[1], "-?")) && optionFound == '?') {  /* if getopt_long() returns an error character, show which option caused the error. Note we look for the case where -? is the only command line option (i.e. display cmd line help), Sep 2025 */

         #if 0  /* optopt is != 0 for short opts and 0 for long opts, use if needed when indexing on error */
         printf("optopt = %d, optind = %d, argc = %d, argv[optind] = %s, argv[optind+1] = %s \n", optopt, optind, argc, argv[optind], argv[optind+1]);
         int index_ofs = (optopt > 0) ? 0 : -1;
         #else
         int index_ofs = 0;
         #endif
         int max_optind = min(optind-1, argc-1);
         printf(" cmd line option %s is unrecognized \n", argv[max_optind+index_ofs][0] == '-' ? argv[max_optind+index_ofs] : argv[max_optind]);
         fError = true;
         fPrintOptions = true;
         break;
      }

   /* check for missing arguments when argument is required. Note this depends on ARG_OPTIONAL flag being set (or not) in CmdLineOpt::Record options[] in getUserInterface.cpp, JHB Oct 2025 */

      ArgType arg_types[ARG_NUM_TYPES] = { ARG_TYPE_NONE };
      int num_arg_types = 0;
      bool fOptionalArg = false;

      for (int i=0; i<this->numOptions; i++) if (optionFound == this->options[i].option || (optionFound == ':' && optopt == this->options[i].option)) {  /* normally getopt_long() returns ':' for an option missing a required argument, but not always, see next comments */

        if (this->options[i].arg_type != ARG_TYPE_NONE) {
            arg_types[num_arg_types++] = (ArgType)(this->options[i].arg_type & ARG_TYPE_MASK);
            fOptionalArg = this->options[i].arg_type & ARG_OPTIONAL;
         } 
      }

      if (optionFound == ':' ||  /* getopt_long() returns ':' for missing argument ... */
          (!fOptionalArg && optarg && optarg[0] == '-')) {  /* ... but evidently fails to return ':' for missing argument when the next item on cmd line is another option */

         char arg_info_str[500] = "";

         arg_error_reporting(arg_info_str, arg_types, num_arg_types);

         printf(" cmd line option %s missing required argument%s \n", argv[optionFound == ':' ? optind-1 : optind-2], arg_info_str);
         fMissingRequiredArg = true;
         fError = true;
         break;
      }

      fOptionFound = false;
      fInvalidFormat = false;

      for (optCounter=0; optCounter < this->numOptions; optCounter++) if (this->options[optCounter].option == optionFound) {

         instance_index = this->options[optCounter].nInstances;  /* options[].nInstance starts at zero and increments each time an instance is found */

         #ifdef GETOPT_DEBUG
         printf(" *** processing option = %d \n", optionFound);
         #endif

         switch (this->options[optCounter].arg_type & ARG_TYPE_MASK) {

            case ARG_TYPE_NONE:

               fOptionFound = true;
               break;

            case ARG_TYPE_BOOL:

               #if 0
               this->options[optCounter].value[instance_index][0] = (void*)true;  /* if the option exist on command line its true */
               fOptionFound = true;
               break;
               #else  /* options that take no argument(s) now have ARG_TYPE_NONE. If an option takes boolean entry (e.g. y/n, t/f, 1/0) then it should have the ARG_TYPE_BOOL type when defined in CmdLineOpt::Record options in getUserInterface.cpp, JHB Sep 2025 */

               if (!optarg) break;

               if (!strcasecmp(optarg, "true") || !strcasecmp(optarg, "t") || !strcasecmp(optarg, "y") || !strcmp(optarg, "1")) this->options[optCounter].value[instance_index][0] = (void*)true;
               else if (!strcasecmp(optarg, "false") || !strcasecmp(optarg, "f") || !strcasecmp(optarg, "n") || !strcmp(optarg, "0")) this->options[optCounter].value[instance_index][0] = (void*)false;
               else fInvalidFormat = true;

               fOptionFound = true;
               #endif

               break;

            case ARG_TYPE_INT:  /* usually accept entry in format -option NN, but also -option 0xNN and -option NN:NN:NN (up to 3 values) */

               if ((this->options[optCounter].arg_type & ARG_OPTIONAL) && !optarg) { fOptionFound = true; break; }  /* if the option has an optional argument then no parsing and don't overwrite the default value in CmdLineOpt::Record options[] in getUserInterface.cpp, JHB Sep 2025 */

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

                  intptr_t x;

                  if (fHexVal) ret = sscanf(&p2[2], "%x", (unsigned int*)&x);
                  #if 0
                  else x = atoi(p2);
                  #else
                  else ret = sscanf(p2, "%d", (int*)&x);
                  #endif

                  if ((ret != 1 || !valid_number(fHexVal ? &p2[2] : p2, fHexVal, this->options[optCounter].arg_type & ARG_ALLOW_FLOAT)) && !(this->options[optCounter].arg_type & ARG_ALLOW_STR)) fInvalidFormat = true;

                  if (valueSuffix >= 0) {

                     x |= valueSuffix << 24;  /* option suffix value stored in bits 31-24.  Probably that could be 63-56 on 64-bit systems, but for now the typical option where a suffix is used doesn't have much range in values */
                  }

                  this->options[optCounter].value[instance_index][nMultiple] = (void*)x;

               } while (p != NULL && ++nMultiple < MAX_MULTIPLES);

               fOptionFound = true;
               break;

            case ARG_TYPE_INT64:  /* 64-bit support, JHB Aug 2015 */

               if (!optarg) break;

               nMultiple = 0;
               strcpy(tmpstr, optarg);  /* temporary working buffer */
               p = tmpstr;

               do {

                  bool fHexVal = false;
                  long long llx;
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

                  if ((ret != 1 || !valid_number(fHexVal ? &p2[2] : p2, fHexVal, false)) && !(this->options[optCounter].arg_type & ARG_ALLOW_STR)) fInvalidFormat = true;

                  this->options[optCounter].value3[instance_index] = llx;

               } while (p != NULL && ++nMultiple < MAX_MULTIPLES);

               fOptionFound = true;
               break;

            case ARG_TYPE_IPADDR:  /* accept entry in format -Daa.bb.cc.dd:port:mm-mm-mm-mm-mm-mm, where a, b, c, d, and port are decimal numbers, and mm are hex digits. Also allow -iaa.bb.cc.dd:port:mm-mm-mm-mm-mm-mm for input UDP ports, JHB Dec 2022 */

               strcpy(tmpstr, optarg);
               p = strstr(tmpstr, ":");

               if (p != NULL) {  /* get integer after ':' char */

                  *p++ = 0;

                  if (p != NULL) {

                     p2 = strstr(p, ":");  /* entry after 2nd : is MAC addr */

                     if (p2 != NULL) {

                        *p2++ = 0;

                        p3 = strstr(p2, "-");
                        int i = 0;
                        #define MAC_ADDR_LEN 6
                        uint64_t ulx, m[MAC_ADDR_LEN] = { 0 };

                        while ((p3 != NULL) || (p2 != NULL)) {

                           if (p3 != NULL) *p3++ = 0;

                           if (p2 != NULL) {

                              intptr_t x;
                              sscanf(p2, "%x", (unsigned int*)&x);
                              if (i < MAC_ADDR_LEN) m[i++] = (uint64_t)x;
                              else { fInvalidFormat = true; break; }
                           }
                           p2 = p3;
                           if (p2 != NULL) p3 = strstr(p2, "-");
                           else p3 = NULL;
                        }

                        ulx = (m[0] << 40) + (m[1] << 32) + (m[2] << 24) + (m[3] << 16) + (m[4] << 8) + m[5];

                        this->options[optCounter].value3[instance_index] = ulx;
                     }
                  }

                  intptr_t x = atoi(p);
                  this->options[optCounter].value2[instance_index] = x;
               }

               p = strstr(tmpstr, ".");  /* IP addr format */

               if (p != NULL) {

                  p2 = tmpstr;
                  int i = 0;
                  #define IP_ADDR_LEN 4
                  int d[IP_ADDR_LEN] = { 0 };

                  while (p != NULL) {

                     *p++ = 0;
                     if (i < IP_ADDR_LEN) d[i++] = atoi(p2);
                     else { fInvalidFormat = true; break; }
                     p2 = p;
                     p = strstr(p2, ".");
                  }

                  if (p2 != NULL) d[i++] = atoi(p2);

                  intptr_t x = (d[0] << 24) + (d[1] << 16) + (d[2] << 8) + d[3];

                  this->options[optCounter].value[instance_index][0] = (void*)x;
               }

               fOptionFound = true;
               break;

            case ARG_TYPE_FLOAT:  /* add FLOAT type, May 2023 JHB */

               if (!optarg) break;

               float f;
               intptr_t x;

               #if 0
               f = atof(optarg);
               #else
               ret = sscanf(optarg, "%f", (float*)&f);
               if ((ret != 1 || !valid_number(optarg, false, true)) && !(this->options[optCounter].arg_type & ARG_ALLOW_STR)) fInvalidFormat = true;  /* not a hex value, allow float chars */
               #endif

               #if 0
               printf(" inside FLOAT argument = %s, ret = %d, fInvalidFormat = %d \n", optarg, ret, fInvalidFormat);
               #endif
               memcpy(&x, &f, sizeof(float));  /* hack to store float in a void* */
               this->options[optCounter].value[instance_index][0] = (void*)x;
               fOptionFound = true;
               break;

            case ARG_TYPE_CHAR:

               x = (intptr_t)optarg[0];
               this->options[optCounter].value[instance_index][0] = (void*)x;  /* store first optarg char in void* */
               fOptionFound = true;
               break;

            case ARG_TYPE_STR:
            case ARG_TYPE_PATH:  /* handled identical to ARG_TYPE_STR, JHB Sep 2025 */

               if ((this->options[optCounter].arg_type & ARG_OPTIONAL) && !optarg) {}  /* if the option has an optional argument (e.g. -L with no string value), then don't overwrite the default value in CmdLineOpt::Record options[] in getUserInterface.cpp, JHB Sep 2017 */
               else this->options[optCounter].value[instance_index][0] = (void*)optarg;
               fOptionFound = true;
               break;

            default:

               cout << " cmd line option " << optionFound << "has type " << (this->options[optCounter].arg_type & ARG_TYPE_MASK) << " with flags " << (this->options[optCounter].arg_type & ~ARG_TYPE_MASK) << " is unknown" << endl;
               fError = true;
               break;
         }

         if (!fError) {
            this->options[optCounter].nInstances++;
// debug   printf("option = %s, optCounter = %d, count = %d\n", optarg, optCounter, this->options[optCounter].nInstances);
         }

         #if 0  /* remove this for-loop break so all defined options will be checked vs. cmd line option found. This allows options to be overloaded (e.g. two 's' definitions, integer for app A and string for app B), JHB Jan 2021 */ 
         break;
         #endif
      }  /* end of for loop comparing optionFound vs defined options */

   /* per-option error handling - note we have opterr turned off so we handle all error types (unrecognized option, invalid format, option requires an argument that wasn't given, etc, JHB Nov 2023 */

      if (!fOptionFound || fInvalidFormat) {

         char cmdoptstr[500] = "";
         strncpy(cmdoptstr, argv[max(0, optind - (fInvalidFormat ? 2 : 1))], sizeof(cmdoptstr)-1);  /* for invalid format optind-1 currently points at the arg so we need optind-2 for the option */

         bool fNeedsArgument = false, fNoArgument = false, fOptionalArgument = false, fFoundShort = false, fFoundLong = false;
         ArgType arg_types[ARG_NUM_TYPES] = { ARG_TYPE_NONE };
         int num_arg_types = 0;

         for (int i=0; i < this->numOptions; i++) {

            char optstr[3] = "";
            optstr[0] = '-';
            optstr[1] = this->options[i].option;
            optstr[2] = 0;

            if (!strcmp(cmdoptstr, optstr)) {  /* check for short options that need an argument */

               ArgType arg_type = (ArgType)(this->options[i].arg_type & ARG_TYPE_MASK);
               if (arg_type == ARG_TYPE_NONE) { if (!num_arg_types) fNoArgument = true; else fNoArgument = false; }
               else if (!(this->options[i].arg_type & ARG_OPTIONAL))  { if (!num_arg_types || fNeedsArgument) fNeedsArgument = true; else fNeedsArgument = false; }
               else { if (!num_arg_types || fOptionalArgument) fOptionalArgument = true; else fOptionalArgument = false; }
               if (arg_type != ARG_TYPE_NONE) arg_types[num_arg_types++] = arg_type;

               fFoundShort = true;  /* continue to loop, there might be more than one option with same short option char (a few are overloaded, see CmdLineOpt::Record options[] in getUserInterface.cpp */
            }

            int long_index = (uint8_t)this->options[i].option - 128;  /* long options start with option char 128, see struct option long_options[] above and CmdLineOpt::Record options[] in getUserInterface.cpp */

            if (long_index >= 0 && optionFound == this->options[i].option) {  /* check for long options that need an argument */

               strcpy(cmdoptstr, long_options[long_index].name);

               if (long_options[long_index].has_arg == no_argument) fNoArgument = true;
               else if (long_options[long_index].has_arg == requires_argument) fNeedsArgument = true;
               else if (long_options[long_index].has_arg == optional_argument) fOptionalArgument = true;
               if (long_options[long_index].has_arg != no_argument) arg_types[num_arg_types++] = (ArgType)(this->options[i].arg_type & ARG_TYPE_MASK);

               fFoundLong = true;
               break;  /* break on found, no long options are overloaded */
            }
         }

// printf("\n *** fInvalidFormat = %d, cmdoptstr = %s, optstr = %s, long_index = %d, fFoundShort = %d, fFoundLong = %d \n", fInvalidFormat, cmdoptstr, optstr, long_index, fFoundShort, fFoundLong);
 
         (void)fNoArgument;  /* not used yet */
         (void)fOptionalArgument;

      /* display error messages for cases where getopt_long() finds nothing wrong, but we do */

         if (fFoundShort || fFoundLong) {

            char arg_info_str[500] = "";

            if (fInvalidFormat) {

               sprintf(arg_info_str, "has argument %s with invalid format", optarg ? optarg : "(null)");

               arg_error_reporting(arg_info_str, arg_types, num_arg_types);
            }

            cout << " cmd line option " << cmdoptstr << " " << (fInvalidFormat ? arg_info_str : (fNeedsArgument ? "requires an argument" : "is unrecognized")) << endl;  /* option has invalid format, not found, or requires an argument, JHB Jan 2021 */
         }
         else if (!fOptionFound) cout << " cmd line option " << cmdoptstr << " not found" << endl;
         else if (fInvalidFormat) cout << " cmd line option " << cmdoptstr << " invalid format" << endl;

         fError = true;
      }
   }  /* end of while (optionFound = getopt_long() ...) loop */

/* check for options not parsed by getopt_long(), which could be typos, misc cmd line junk, etc. If the text has no '-' or '--' prefix then we check if the preceding text was a valid option that either requires, or allows an optional, argument, JHB Oct 2025 */

   for (int nLastTextPossibleArg=0,i=optind-(fMissingRequiredArg ? 1 : 0); i<argc; i++) {  /* fMissingRequiredArg set above if getopt_long() returns ':' for an option */

      if (!strlen(argv[i])) { printf(" inside left-over argv[] processing, argv[%d] null \n", i); continue; }  /* warning message but should never happen */

      if (argv[i][0] != '-') {

         if (!nLastTextPossibleArg) { printf(" cmd line option \"%s\" without preceding '-' or '--' \n", argv[i]); fError = true; }  /* fError may already be set */

         nLastTextPossibleArg = 0;
      }
      else {

         if (strlen(argv[i]) >= 3 && argv[i][1] == '-') {  /* search long options */
            for (int j=0; j<(int)(sizeof(long_options)/sizeof(long_options[0])); j++) if (long_options[j].name && !strcmp(&argv[i][2], long_options[j].name) && long_options[j].has_arg != no_argument) { nLastTextPossibleArg = i; break; }
         }
         else if (strlen(argv[i]) >= 2) {  /* search short options */
            for (int j=0; j<this->numOptions; j++) if (argv[i][1] == this->options[j].option && !(this->options[j].arg_type & ARG_OPTIONAL)) { nLastTextPossibleArg = i; break; }
         }
      }
   }

   if (fError) {

      if (fPrintOptions) { this->printOptions(); cout << "Please use the above options" << endl; }
      else cout << " enter -h or -? to see command line options" << endl;
      return false;
   }

/* Disable mandatories for x86 - CJ Jan2017 */
//   if (this->nInstances('c') && (!strcmp(this->getStr('c', 0), "x86") || !strcmp(this->getStr('c', 0), "X86"))) {
   if (this->nInstances('c') && !strcasecmp(this->getStr('c', 0), "x86")) {

//     uFlags |= CLI_DISABLE_MANDATORIES;  /* no longer needed, JHB Aug 2018 */
      fx86 = true;
   }
   else fx86 = false;

   if (!(uFlags & CLI_DISABLE_MANDATORIES)) {  /* CLI_DISABLE_MANDATORIES added JHB 11May15. Applying this flag should only be done if you know what you're doing (see isMandatory in cmdLineOpt.h), JHB Aug 2018 */

   // Find out if any mandatory options were omitted
      for (optCounter=0; optCounter<this->numOptions; optCounter++) {

         if ((((this->options[optCounter].isMandatory == 1) || (this->options[optCounter].isMandatory == 2 && !fx86)) && this->options[optCounter].nInstances == 0) && !this->getPosition('-', ARG_TYPE_STR) == 0 && !this->getPosition('-', ARG_TYPE_PATH) == 0) {

            cout << "Error in options:" << endl;
            cout << "  Option -" << this->options[optCounter].option << " is mandatory" << endl;

            this->printOptions();
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

Record *record = this->getOption(option, ARG_TYPE_INT);
int value = 0;

   #if 0
   if (record && record->arg_type == ARG_TYPE_INT) {
      value = (intptr_t)record->value[nInstance][nMultiple];
   }
   #else
   if (record) value = (intptr_t)record->value[nInstance][nMultiple];
   #endif

   return value;
}

float CmdLineOpt::getFloat(char option, int nInstance, int nMultiple) {  /* add getFloat() JHB May 2023 */

Record *record = this->getOption(option, ARG_TYPE_FLOAT);
float value = 0;

   #if 0
   if (record && record->arg_type == ARG_TYPE_FLOAT) {
      value = (float*)record->value[nInstance][nMultiple];
   }
   #else
   if (record) memcpy(&value, &record->value[nInstance][nMultiple], sizeof(float));  /* reverse the hack in FLOAT case in scanOptions(), JHB May 2023 */
   #endif

   return value;
}

long long CmdLineOpt::getInt64(char option, int nInstance) {

Record *record = this->getOption(option, ARG_TYPE_INT64);
long long value = 0;

   #if 0
   if (record && record->arg_type == ARG_TYPE_INT64) {
      value = (long long)record->value3[nInstance];
   }
   #else
   if (record) value = (int64_t)record->value3[nInstance];
   #endif

   return value;
}

unsigned int CmdLineOpt::getIpAddr(char option, int nInstance) {

Record *record = this->getOption(option, ARG_TYPE_IPADDR);
unsigned int value = 0;

   #if 0
   if (record && record->arg_type == ARG_TYPE_IPADDR) {
      value = (intptr_t)record->value[nInstance][0];
   }
   #else
   if (record) value = (intptr_t)record->value[nInstance][0];
   #endif

   return value;
}

uint16_t CmdLineOpt::getUdpPort(char option, int nInstance) {

Record *record = this->getOption(option, ARG_TYPE_IPADDR);
uint16_t value = 0;

   #if 0
   if (record && record->arg_type == ARG_TYPE_IPADDR) {
      value = (intptr_t)record->value2[nInstance];
   }
   #else
   if (record) value = (intptr_t)record->value2[nInstance];
   #endif

   return value;
}

uint64_t CmdLineOpt::getMacAddr(char option, int nInstance) {

Record *record = this->getOption(option, ARG_TYPE_IPADDR);
uint64_t value = 0;

   #if 0
   if (record && record->arg_type == ARG_TYPE_IPADDR) {
      value = (uint64_t)record->value3[nInstance];
   }
   #else
   if (record) value = (uint64_t)record->value3[nInstance];
   #endif

   return value;
}


//
// getChar - Returns value of CHAR command line options
//
char CmdLineOpt::getChar(char option, int nInstance) {

Record *record = this->getOption(option, ARG_TYPE_CHAR);
char value = '\0';

   #if 0
   if (record && record->arg_type == ARG_TYPE_CHAR) {
      value = (char)(intptr_t)record->value[nInstance][0];
   }
   #else
   if (record) value = (char)(intptr_t)record->value[nInstance][0];
   #endif

   return value;
}

//
// getStr - Returns value of ARG_TYPE_STR and ARG_TYPE_PATH command line options
//
char* CmdLineOpt::getStr(char option, int nInstance) {

Record *record = this->getOption(option, ARG_TYPE_STR);
if (!record) record = this->getOption(option, ARG_TYPE_PATH);  /* getUserInfo() (in getUserInterface.cpp) should call getStr() for both STR and PATH args, JHB Sep 2025 */
char *value = NULL;

   #if 0
   if (record && record->arg_type == ARG_TYPE_STR) {
      value = (char*)record->value[nInstance][0];
   }
   #else
   if (record) value = (char*)record->value[nInstance][0];
   #endif

   return value;
}

//
// getBool - Returns value of ARG_TYPE_BOOL command line options
//
bool CmdLineOpt::getBool(char option, int nInstance) {

Record *record = this->getOption(option, ARG_TYPE_BOOL);
bool value = false;

   #if 0
   if (record && record->arg_type == ARG_TYPE_BOOL) {
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

      switch (this->options[optCounter].arg_type) {

         case ARG_TYPE_INT:
            cout << "  -" << this->options[optCounter].option << ": <"
                 << (intptr_t)this->options[optCounter].value[0][0] << ">\t"
                 << this->options[optCounter].description << endl;
            break;

         case ARG_TYPE_FLOAT:  /* add FLOAT case, JHB May 2023 */
            cout << "  -" << this->options[optCounter].option << ": <"
                 << (float)(intptr_t)this->options[optCounter].value[0][0] << ">\t"
                 << this->options[optCounter].description << endl;
            break;

         case ARG_TYPE_CHAR:
            cout << "  -" << this->options[optCounter].option << ": <"
                 << (char)(intptr_t)this->options[optCounter].value[0][0] << ">\t"
                 << this->options[optCounter].description << endl;
            break;

         case ARG_TYPE_STR:
         case ARG_TYPE_PATH:
            cout << "  -" << this->options[optCounter].option << ": <"
                 << (char*)this->options[optCounter].value[0][0] << ">\t"
                 << this->options[optCounter].description << endl;
            break;

         case ARG_TYPE_BOOL:
            cout << "  -" << this->options[optCounter].option << ": <"
                 << (this->options[optCounter].value[0][0] ? "true" : "false")
                 << ">\t" << this->options[optCounter].description << endl;
            break;

         case ARG_TYPE_IPADDR:
         case ARG_TYPE_INT64:
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
std::string arg_type_str;
char option[50];
int long_index;

   cout << "Command line option syntax:" << endl;
   cout << "! is mandatory for all platforms" << endl;
   cout << "+ is mandatory for coCPU" << endl;

   for (optCounter=0; optCounter<this->numOptions; optCounter++) {

   /* build cmd line spec string for either short or long options, JHB Nov 2023 */

      if ((long_index = (unsigned char)this->options[optCounter].option - 128) >= 0) { strcpy(option, "--"); strcat(option, long_options[long_index].name); }
      else { option[0] = '-'; option[1] = this->options[optCounter].option; option[2] = 0; }

      if (this->options[optCounter].arg_type & ARG_OPTIONAL) arg_type_str = "[]";
      else arg_type_str = "<>"; 

      switch (this->options[optCounter].arg_type & ARG_TYPE_MASK) {

         case ARG_TYPE_NONE:
            arg_type_str = "";
            break;

         case ARG_TYPE_INT:
            arg_type_str.insert(1, "int");
            break;

         case ARG_TYPE_FLOAT:  /* add FLOAT case, JHB May 2023 */
            arg_type_str.insert(1, "float");
            break;

         case ARG_TYPE_CHAR:
            arg_type_str.insert(1, "char");
            break;

         case ARG_TYPE_PATH:
            arg_type_str.insert(1, "path");
            break;

         case ARG_TYPE_STR:
            arg_type_str.insert(1, "string");
            break;

         case ARG_TYPE_BOOL:
            arg_type_str.insert(1, "bool");
            break;

         case ARG_TYPE_INT64:
            arg_type_str.insert(1, "int64");
            break;

         case ARG_TYPE_IPADDR:
            arg_type_str.insert(1, "IP addr");
            break;

         default:
            arg_type_str = "UNKNOWN";
            break;
      }

   /* use basic C formatting to handle justification to a specific column and get a presentable display of options, entry specs, and descriptions, JHB Sep 2025 */

      #define DESCRIPTION_COLUMN 40
      char outstr[200];
      sprintf(outstr, " %s %s %s", option, arg_type_str.c_str(), (this->options[optCounter].isMandatory == 1 ? "!" : this->options[optCounter].isMandatory == 2 ? "+" : ""));
      int padlen = max(DESCRIPTION_COLUMN-(int)strlen(outstr), 0);
      for (int i=0; i<padlen; i++) strcat(outstr, " ");
      strcat(outstr, this->options[optCounter].description);
      printf("%s \n", outstr);
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

         if (type == -1 || type == ((this->options + optCounter)->arg_type & ARG_TYPE_MASK)) {  /* checking for type here allows options to be overloaded; e.g. two options for '-s' of different types can be defined and checked in getUserInfo(), but instead of first one found here being returned, it also has to match the type check before being returned, JHB Jan 2021 */

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
int CmdLineOpt::getPosition(char option, ArgType arg_type) {

int optCounter, pos = -1;

   for (optCounter=0; optCounter<this->numOptions; optCounter++) {

      if (option == this->options[optCounter].option) {

         if (arg_type == -1 || arg_type == (this->options + optCounter)->arg_type) {

            pos = optCounter;
            break;
         }
      }
   }

   return pos;
}
