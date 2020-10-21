/* parse_args.c

   Copyright Signalogic 2006-2010
   Created:  Dec 2006 NR
   Modified: Jun 2010 JHB, fixed some warnings, added atoi() include file stdlib.h
   Copyright Signalogic 2006
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef FALSE
  #define FALSE 0
#endif

#ifndef TRUE
  #define TRUE 1
#endif

#ifndef BOOL
  #define BOOL unsigned short int
#endif

#ifndef WORD
  #define WORD unsigned short int
#endif

static void usage(char *prog_name, FILE *out) {

  fprintf(out, "%s%s%s%s",
               "Usage: ", prog_name, " [options as below]\n",
               "   [ -h] print this list\n"
               "   [ -ips ] source IP address (for example, card 1 in 2-card test)\n"
               "   [ -ipd ] destination IP address (for example, card 2 in 2-card test)\n"
               "   [ init ] display all previous or hold-over WinPath output\n"
               "   [ tdm2ip ] TDM-to-IP mode operation (default)\n"
               "   [ ip2ip ] IP-to-IP mode operation\n"
               "   [ dnld ] perform DSP code download\n"
               "   [ lpdnld ] perform DSP code download, loop through each DSP one at a time and show results\n"
               "   [ tdm2ip ] TDM-to-IP mode operation (default if no entry)\n"
               "   [ g711 ] specify G711 codecs only (G729AB is default if no entry)\n"
               "   [ -v ] Verbose mode, print as much information as possible\n"
               "   [ -mLIST ] Mask for DSP list, for example -m1 = DSP0 active, -m3 = DSP0 and DSP1 active, -m143 = DSP 7 and DSPs 3-0 active (no entry indicates all DSPs on the card are active)\n"
               "   [ -nNUMCALL ] Number of calls to open, for example -n16 = 16 calls, -n384 = 384 calls (default is 1 call if no entry).\nNote that if ip2ip is specified, then each call consists of two IP channels (4 chan total).\n"
         );
}

int parse_args(int argc, char** argv, char* prog_name, char* IPSrc, char* IPDst, BOOL* Init, BOOL* Verbose, WORD* NumChan, int* Dnld, WORD* Mode, WORD* ProcList) {

int error_flag = 0;
int i;
char *tmp, tmpstr1[10];

  
   for (i=1; i<argc; i++) {

      if (!strcmp(argv[i], "-h")) {
  
         error_flag = 1;
      }
      else if ((strstr(argv[i],"-ips")) != NULL) {
   
         tmp = strstr(argv[i],"-ips");
         sprintf(IPSrc, "%s", (char*)&tmp[4]);
      }
      else if ((strstr(argv[i],"-ipd")) != NULL) {

         tmp = strstr(argv[i],"-ipd");
         sprintf(IPDst, "%s", (char*)&tmp[4]);
      }
      else if (!strcmp(argv[i],"init")) {

         *Init = TRUE;
      }
      else if (!strcmp(argv[i],"dnld")) {

         if (*Dnld != -1) *Dnld = 1;
      }
      else if (!strcmp(argv[i],"lpdnld")) {

         if (*Dnld != -1) *Dnld = 2;
      }
      else if (!strcmp(argv[i],"x86")) {

         *Dnld = -1;
      }
      else if (!strcmp(argv[i],"tdm2ip")) {

         *Mode = 0;
      }
      else if (!strcmp(argv[i],"ip2ip")) {

         *Mode = 1;
      }
      else if (!strcmp(argv[i],"ippassthru")) {

         *Mode = 2;
      }
      else if (!strcmp(argv[i],"g711")) {

         *Mode |= 0x100;
      }
      else if (!strcmp(argv[i],"-v")) {

         *Verbose = TRUE;
      }
      else if((strstr(argv[i],"-m")) != NULL) {

         tmp = strstr(argv[i],"-m");
         sprintf(tmpstr1, "%s", tmp);
         *ProcList = atoi((char*)&tmpstr1[2]);
      }
      else if((strstr(argv[i],"-n")) != NULL) {

         tmp = strstr(argv[i],"-n");
         sprintf(tmpstr1, "%s", tmp);
         *NumChan = atoi((char*)&tmpstr1[2]);
      }
      else error_flag = 2;
   }
  
   if (error_flag == 1) usage(prog_name, stdout);
   else if (error_flag == 2) usage(prog_name, stderr);

   return error_flag;
}
