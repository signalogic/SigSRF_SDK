/*
  $Header: /root/Signalogic/DirectCore/include/aviolib.h
 
  Description: API and definition header file aviolib, which provides audio and video I/O, including USB audio I/O (mics, line input, mixers, etc), USB and GbE cameras, etc

  Projects: SigSRF, DirectCore
 
  Copyright Signalogic Inc. 2018

  Revision History:
  
   Created Mar 2018 Anish Mathew
   Modified Mar 2018 JHB, add DSCreateAvioSession() and DSDeleteAvioSession(), modified API params to be consistent with SigSRF libs
   Modified Jun 2018 JHB, add uFlags arg to DSOpenAvioDevice() (which can be DS_SND_PCM_STREAM_CAPTURE or DS_SND_PCM_STREAM_PLAYBACK), add DSWriteAvioBuffer()
   Modified Jun 2018 JHB, add shift arg to DSReadAvioBuffer() and DSWriteAvioBuffer()
*/
 
#ifndef _AVIOLIB_H_
#define _AVIOLIB_H_

#include <alsa/asoundlib.h>

/* audio USB definitions */

#define AUDIO_INPUT_USB0        1
#define AUDIO_INPUT_USB1        2

#define AUDIO_OUTPUT_USB0       0x100
#define AUDIO_OUTPUT_USB1       0x200

/* DSOpenAvioDevice() uFlags definitions */

#define DS_SND_PCM_STREAM_CAPTURE       SND_PCM_STREAM_CAPTURE    /* these values are 0 and 1, they should not be combined.  DSOpenAvioDevice() must be called twice for full-duplex operation */
#define DS_SND_PCM_STREAM_PLAYBACK      SND_PCM_STREAM_PLAYBACK

#define DS_SND_PCM_STREAM_MASK          0xff                      /* other flags start at 0x100 and can be combined */

/* DSReadAvioBuffer() and DSWriteAvioBuffer() uFlags definitions */

#define DS_AVIO_BUFFER_USE_UPPER_16BITS   1
#define DS_AVIO_BUFFER_USE_LOWER_16BITS   2
#define DS_AVIO_BUFFER_LEFT_CHANNEL       4
#define DS_AVIO_BUFFER_RIGHT_CHANNEL      8
#define DS_AVIO_BUFFER_RIGHT_STEREO       0  /* left and right channel should not both be set */

#ifdef __cplusplus
extern "C" {
#endif

/* aviolib version string global var */

   extern const char AVIOLIB_VERSION[256];

   snd_pcm_t* DSOpenAvioDevice(snd_pcm_hw_params_t*   hw_params,
                               unsigned int           uFlags,
                               snd_pcm_uframes_t      buffer_size,
                               snd_pcm_uframes_t      period_size,
                               snd_async_handler_t**  pcm_callback,
                               snd_async_callback_t   callBack,
                               const char*            hwDevice,
                               unsigned int           sampleRate);

/* DSReadAvioBuffer() and DSWriteAvioBuffer() notes:

    -these APIs should be called from inside an ALSA callback function.  See USBAudioCallback() in x86_mediaTest.c for an example

    -if buf32 or buf16 is NULL, no sample format conversion is performed.  If both are non-NULL, then uFlags controls the format conversion (see constants above)
*/
   
   int DSReadAvioBuffer(snd_pcm_t*            alsa_handle,
                        snd_async_handler_t*  pcm_callback,
                        snd_pcm_uframes_t     period_size,
                        int*                  buf32,
                        short int*            buf16,
                        short int             shift,
                        unsigned int          uFlags);
   
   int DSWriteAvioBuffer(snd_pcm_t*            alsa_handle,
                         snd_async_handler_t*  pcm_callback,
                         snd_pcm_uframes_t     period_size,
                         int*                  buf32,
                         short int*            buf16,
                         short int             shift,
                         unsigned int          uFlags);

   int DSCloseAvioDevice(snd_pcm_t*            alsa_handle,
                         snd_async_handler_t*  pcm_callback);

#ifdef __cplusplus
}
#endif

#endif   /* _AVIOLIB_H_ */
