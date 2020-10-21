
/* $Header: /root/Signalogic/DirectCore/include/minmax.h 2     10/10/05 12:32p
 *
 * Description: local versions of min and max
 *
 * Project: DirectCore
 *
 * Copyright Signalogic Inc. 1994-2013
 * 
 * 2     10/10/05 12:32p
 * 8 DSP support.
 * 
 * 1     8/24/05 7:13p
 * Clean up code and rebuild.
*/
 
#ifndef _MINMAX_H_
#define _MINMAX_H_

#ifdef _LINUX_ 
   #ifndef min
      #define min(a,b)  (((a) < (b)) ? (a) : (b))
   #endif
   #ifndef max
      #define max(a,b) (((a) > (b)) ? (a) : (b))
   #endif
#endif

#if !defined(min) && defined(__cplusplus) && !defined(__NOMINMAX) && !defined(__MINMAX_DEFINED)

  #if defined(__WATCOMC__) || defined (_LINUX_)
    #ifndef max
    #define max(a,b)            (((a) > (b)) ? (a) : (b))
    #endif
    #ifndef min
    #define min(a,b)            (((a) < (b)) ? (a) : (b))
    #endif
  #else 
    template <class T> inline const T _FAR &min( const T _FAR &t1, const T _FAR &t2 ) {

        if  (t1 < t2) return t1;
        else return t2;
    }

    template <class T> inline const T _FAR &max( const T _FAR &t1, const T _FAR &t2 ) {

        if  (t1 > t2) return t1;
        else return t2;
    }
  #endif
#endif

#endif
