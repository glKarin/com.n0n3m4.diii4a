#ifndef _SND_OBOE_H
#define _SND_OBOE_H

typedef void (* Q3E_write_audio_data_f)(unsigned char *stream, int len);

#ifdef __cplusplus
extern "C" {
#endif

void Q3E_Oboe_Init(Q3E_write_audio_data_f f);
void Q3E_Oboe_Start();
void Q3E_Oboe_Stop();
void Q3E_Oboe_Shutdown();
void Q3E_Oboe_Lock();
void Q3E_Oboe_Unlock();

#ifdef __cplusplus
};
#endif

#endif //D_SND_OBOE_H
