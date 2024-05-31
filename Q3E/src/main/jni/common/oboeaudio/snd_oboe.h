#ifndef _Q3E_SND_OBOE_H
#define _Q3E_SND_OBOE_H

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

#ifdef __cplusplus
};
#endif

#endif //_Q3E_SND_OBOE_H
