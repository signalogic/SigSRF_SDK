/*
  $Header: /root/Signalogic_YYYYvN/DirectCore/apps/common/timer.cpp
 
  Purpose:
 
    Provide precise timing for test/demo apps, for example frame rate timing for video streaming apps

  Description
  
    timer callback related functions

  Copyright (C) Signalogic Inc. 2005-2016
 
  Revision History:
 
    Created, 2005, JHB
    Revised, May15, JHB.  Added API interface in addition to global var
    Revised, Apr16, JHB.  Added IsTimerEnabled API and also notes about disabling timer
    
*/


#include <sys/time.h>
#include <signal.h>
#include "test_programs.h"

volatile bool fTimerCallbackOccurred = false;  /* global flag that can be used by apps to know that timer event occurred (or they can use APIs) */

static bool fTimerEnabled = false;

void setTimerInterval(time_t _sec, time_t _usec) {

struct sigaction sa;
struct itimerval timer;

	/* Install timer_handler as the signal handler for SIGVTALRM */
	memset(&sa, 0, sizeof (sa));
	//sa.sa_handler = &timer_handler;
	sa.sa_handler = &TargetTimerTestHdl;
	sigaction(SIGALRM, &sa, NULL);

	/* Configure the timer to expire after ... */
	timer.it_value.tv_sec = _sec;
	timer.it_value.tv_usec = _usec;
	/* ... and every xxx msec after that */
	timer.it_interval.tv_sec = _sec;
	timer.it_interval.tv_usec = _usec;

   fTimerCallbackOccurred = false;
 
/* Start a real-time timer (note that if _sec and _usec are both zero, the timer is actually disabled */

	setitimer(ITIMER_REAL, &timer, NULL);
   
   fTimerEnabled = _sec != 0 || _usec != 0;
}


bool IsTimerEventReady() {

bool temp = fTimerCallbackOccurred;

   fTimerCallbackOccurred = false;
   
   return temp;
}


void ResetTimerEvent() {

   fTimerCallbackOccurred = false;
}


void TargetTimerTestHdl(int signum) {

//	if (dspHdl.WaitForBuffer())

	if (!fTimerCallbackOccurred) {
   
		fTimerCallbackOccurred = true;

	//	printf("bufferReady = %s \n",  bufferReady ? "true" : "false");
	//	printf("&bufferReady = 0x%x \n", &bufferReady);
	}
}		

bool IsTimerEnabled() {

   return fTimerEnabled;
}
