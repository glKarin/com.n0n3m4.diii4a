#ifndef _Q3E_SND_OBOE_H
#define _Q3E_SND_OBOE_H

// channel
#define Q3E_OBOE_CHANNEL_MONO 1
#define Q3E_OBOE_CHANNEL_STEREO 2

// format
#define Q3E_OBOE_FORMAT_SINT16 1
#define Q3E_OBOE_FORMAT_FLOAT 2
#define Q3E_OBOE_FORMAT_SINT24 3 // API 31
#define Q3E_OBOE_FORMAT_SINT32 4 // API 31

#ifdef __cplusplus
extern "C" {
#endif

typedef void (* Q3E_write_audio_data_f)(unsigned char *stream, int len);

void Q3E_Oboe_Init(int sampleRate, int channel, int format, Q3E_write_audio_data_f func);
void Q3E_Oboe_Start(void);
void Q3E_Oboe_Stop(void);
void Q3E_Oboe_Shutdown(void);
void Q3E_Oboe_Lock(void);
void Q3E_Oboe_Unlock(void);

typedef void * Q3E_AudioDevice;
Q3E_AudioDevice Q3E_Audio_Create(int sampleRate, int channel, int format, Q3E_write_audio_data_f func);
void Q3E_Audio_Destroy(Q3E_AudioDevice device);
void Q3E_Audio_Start(Q3E_AudioDevice device);
void Q3E_Audio_Stop(Q3E_AudioDevice device);
void Q3E_Audio_Shutdown(Q3E_AudioDevice device);
void Q3E_Audio_Lock(Q3E_AudioDevice device);
void Q3E_Audio_Unlock(Q3E_AudioDevice device);

#ifdef __cplusplus
};
#endif

#endif //_Q3E_SND_OBOE_H
