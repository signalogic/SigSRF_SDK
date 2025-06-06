/*
 $Header: /root/Signalogic/DirectCore/apps/common/cmdLineOpt.h

 Purpose:  command line options parsing and handling
  
 Copyright (C) Signalogic Inc. 2005-2025

 Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.1, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. Absolutely prohibited for AI language or programming model training use

 Revision History

   Created 10/06/05 3:31p Nithin

   Modified Nov 2014 JHB, fix some naming to support multiple targets, unify/consolidate command line params for all test programs
   Modified Dec 2014 JHB, add support for IP addr, UDP port, and MAC addr entry (e.g. -Daa.bb.cc.dd:port:aa-bb-cc-dd-ee-ff-gg)
   Modified Jul 2015 JHB, add -l and -t cmd lne options (lib configuration and core task assignments) to support multifunctional .out file, e.g. image analytics
   Modified Jul 2015 JHB, add INT64 type to support 64-bit core list entries
   Modified Jul 2018 JHB, changed mandatory support to handle x86 and coCPU separately.  Changed isMandatory element in Record struct from bool to unsigned char
   Modified Sep 2019 JHB, add CLI_MEDIA_APPS flag
   Modified Jan 2021 JHB, add CLI_MEDIA_APPS_MEDIAMIN flag, add type param to getOption()
   Modified May 2023 JHB, add FLOAT option type, FLOAT switch statement cases, getFloat(), change getUdpPort() from unsigned int to uint16_t
   Modified Jul 2023 JHB, add getPosition() public function to CmdLineOpt class
   Modified Feb 2024 JHB, add CLI_MEDIA_APPS_MEDIATEST flag
   Modified Dec 2024 JHB, don't define CmdLineOpt class if __cplusplus not defined
   Modified Mar 2025 JHB, define ALLOW_XX Type enums for use with overloaded options, for example -rN can accept N either int or float
*/

#ifndef _CMDLINEOPT_H_
#define _CMDLINEOPT_H_

#ifndef _WIN32
#include <unistd.h>
#endif

#define MAX_INSTANCES 8
#define MAX_MULTIPLES 8

/* flags for getUserInfo() (located in getUserInterface.cpp) */

#define CLI_DISABLE_MANDATORIES     1
#define CLI_MEDIA_APPS              2  /* indicates that calling app is a media app (e.g. mediaMin or mediaTest). For example, for overloaded cmd line options, this specfies media app defaults */
#define CLI_MEDIA_APPS_MEDIAMIN     4  /* add specific flag for mediaMin, JHB Jan 2021 */
#define CLI_MEDIA_APPS_MEDIATEST    8  /* add specific flag for mediaTest, JHB Feb 2024 */

/* flags used in CmdLineOpt class (Record options initialization in getUserInterface.cpp) */

#define MANDATORY                   1
#define MANDATORY_COCPU             2
#define NOTMANDATORY                0

#ifdef __cplusplus

class CmdLineOpt {

public:

/* option types supported */

   enum Type {
      INTEGER,
      INT64,
      CHAR,
      STRING,
      BOOLEAN,
      IPADDR,
      FLOAT,
      ALLOW_FLOAT = 0x100,  /* ALLOW_XX attributes can be combined with option types to indicate the argument is overloaded and errors in converting to the primary type should be ignored, JHB Mar 2025 */
      ALLOW_STRING = 0x200,
      OPTION_TYPE_MASK = 0xff
   };

/* Structure for working with command line options */

   struct Record {

      char option;                 /* option character */
      Type type;                   /* type (integer, string, etc) */
      unsigned char isMandatory;   /* is it mandatory? */
      char *description;           /* help printout description */

   /* The following should be specified at record creation only if a default value is required */

      void* value[MAX_INSTANCES][MAX_MULTIPLES];
      unsigned int value2[MAX_INSTANCES];
      long long value3[MAX_INSTANCES];

      int nInstances;    /* number of instances of this option found on command line */
   };

   CmdLineOpt(Record *options, int numOptions);
   ~CmdLineOpt(void);

   bool scanOptions(int argc, char *argv[], unsigned int uFlags);
   int nInstances(char option);
   int getInt(char option, int nInstance, int nMultiple);
   float getFloat(char option, int nInstance, int nMultiple);
   long long getInt64(char option, int nInstance);
   unsigned int getIpAddr(char option, int nInstance);
   uint16_t getUdpPort(char option, int nInstance);
   uint64_t getMacAddr(char option, int nInstance);
   char getChar(char option, int nInstance);
   char *getStr(char option, int nInstance);
   bool getBool(char option, int nInstance);
   int getPosition(char option, Type type);

   void display(void);
   void printOptions(void);


private:

   Record* getOption(char option, int type);

   Record* options;
   int numOptions;
};

#endif  /* __cplusplus */

#endif  /* _CMDLINEOPT_H_ */
