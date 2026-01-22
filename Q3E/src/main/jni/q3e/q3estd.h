#ifndef _Q3E_STD_H
#define _Q3E_STD_H

#define EVENT_QUEUE_TYPE_NATIVE 0
#define EVENT_QUEUE_TYPE_JAVA 1

#define GAME_THREAD_TYPE_NATIVE 0
#define GAME_THREAD_TYPE_JAVA 1

#define LOGD(fmt, args...) do { printf("[" LOG_TAG "/D]" fmt "\n", ##args); __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args); } while(0);
#define LOGI(fmt, args...) do { printf("[" LOG_TAG "/I]" fmt "\n", ##args); __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args); } while(0);
#define LOGW(fmt, args...) do { printf("[" LOG_TAG "/W]" fmt "\n", ##args); __android_log_print(ANDROID_LOG_WARN, LOG_TAG, fmt, ##args); } while(0);
#define LOGE(fmt, args...) do { printf("[" LOG_TAG "/E]" fmt "\n", ##args); __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args); } while(0);

#define Q3E_TRUE 1
#define Q3E_FALSE 0

#if 1
#define ASSERT(x)
#else
#define ASSERT assert
#endif

#endif // _Q3E_STD_H
