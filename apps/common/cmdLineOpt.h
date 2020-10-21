/*
 $Header: /root/Signalogic/DirectCore/apps/common/cmdLineOpt.h

 Purpose:  keep track of any options specified on the command line
  
 (C) Signalogic Inc. 2005-2019

 Revision History:

   Created 10/06/05 3:31p Nithin

   Modified Nov 2014 JHB, fix some naming to support multiple targets, unify/consolidate command line params for all test programs
   Modified Dec 2014 JHB, add support for IP addr, UDP port, and MAC addr entry (e.g. -Daa.bb.cc.dd:port:aa-bb-cc-dd-ee-ff-gg)
   Modified Jul 2015 JHB, add -l and -t cmd lne options (lib configuration and core task assignments) to support multifunctional .out file, e.g. image analytics
   Modified Jul 2015 JHB, add INT64 type to support 64-bit core list entries
   Modified Jul 2018 JHB, changed mandatory support to handle x86 and coCPU separately.  Changed isMandatory element in Record struct from bool to unsigned char
   Modified Sep 2019 JHB, add CLI_MEDIA_APPS flag
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

#define MANDATORY                   1
#define MANDATORY_COCPU             2
#define NOTMANDATORY                0

class CmdLineOpt {

public:

/* option types supported */

   enum Type {
      INTEGER,
      INT64,
      CHAR,
      STRING,
      BOOLEAN,
      IPADDR
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
   long long getInt64(char option, int nInstance);
   unsigned int getIpAddr(char option, int nInstance);
   unsigned int getUdpPort(char option, int nInstance);
   uint64_t getMacAddr(char option, int nInstance);
   char getChar(char option, int nInstance);
   char *getStr(char option, int nInstance);
   bool getBool(char option, int nInstance);

   void display(void);
   void printOptions(void);


private:

   Record* getOption(char option);

   Record* options;
   int numOptions;
};


#endif /* _CMDLINEOPT_H_ */
