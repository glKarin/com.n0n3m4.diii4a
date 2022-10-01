#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#define HARM_CHECK_RESULT(result, ret, msg, args...) \
	if (SL_RESULT_SUCCESS != result) { \
		common->Error(msg, ##args); \
		return ret; \
	}

#define HARM_CHECK_RESULT_NORETURN(result, msg, args...) \
	if (SL_RESULT_SUCCESS != result) { \
		common->Warning(msg, ##args); \
	}

struct harmBufferData
{
	harmBufferData *next;
	char *data;
	int length;
	explicit harmBufferData()
		: next(0),
		data(0),
		length(0)
	{
	}

	explicit harmBufferData(const char *d, int len)
		: next(0),
		data(0),
		length(0)
	{
		if(d && len > 0)
		{
			data = malloc(len);
			memcpy(data, d, len);
		}
	}

	~harmBufferData()
	{
		free(data);
	}

#if 0
	private:
	explicit harmBufferData(const harmBufferData &other);
	harmBufferData & operator==(const harmBufferData &other);
#endif
};

template< class type, int nextOffset >
class idDeque
{
	public:
		idDeque(void)
			: size(0)
		{
			first = last = NULL;
		}

		void					Add(type *element)
		{
			QUEUE_NEXT_PTR(element) = NULL;

			if (last) {
				QUEUE_NEXT_PTR(last) = element;
			} else {
				first = element;
			}

			last = element;
			size++;
		}

		type 					*Get(void)
		{
			type *element;

			element = first;

			if (element) {
				first = QUEUE_NEXT_PTR(first);

				if (last == element) {
					last = NULL;
				}

				QUEUE_NEXT_PTR(element) = NULL;
				size--;
			}

			return element;
		}

		void					AddToFront(type *element)
		{
			QUEUE_NEXT_PTR(element) = first;

			first = element;
		}

		void Clear()
		{
			type *f = first;
			first = last = NULL;
			while(f)
			{
				type *t = QUEUE_NEXT_PTR(f);
				delete f;
				f = t;
			}
			size = 0;
		}

	//private:
		type 					*first;
		type 					*last;
		int size;
};

static void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bq, void *context);

class idAudioHardwareOpenSLES : public idAudioHardware
{
	public:
		explicit idAudioHardwareOpenSLES();

		virtual ~idAudioHardwareOpenSLES();

		bool Initialize();

		bool Lock(void **pDSLockedBuffer, ulong *dwDSLockedBufferSize)
		{
			return false;
		}

		bool Unlock(void *pDSLockedBuffer, dword dwDSLockedBufferSize)
		{
			return false;
		}

		bool GetCurrentPosition(ulong *pdwCurrentWriteCursor)
		{
			return false;
		}

		// try to write as many sound samples to the device as possible without blocking and prepare for a possible new mixing call
		// returns wether there is *some* space for writing available
		bool Flush(void){
			//Write(true);
			return true;
		};
		void Write(bool flushing){
#if 0
			if(flushing)
			{
				//queue.Clear();
				SLresult result = (*simpleBufferQueueItf)->Clear(simpleBufferQueueItf);
				//common->Printf("SLAndroidSimpleBufferQueueItf::Clear() -> %d", result);
				if (SL_RESULT_SUCCESS != result)
				{
					common->Warning("(SLAndroidSimpleBufferQueueItf)simpleBufferQueueItf::Clear() -> %d", result);
				}
			}
#endif
			queue.Add(new harmBufferData((char *)m_buffer, this->m_buffer_size));
		};

		int GetNumberOfSpeakers(void)
		{
			return m_channels;
		}
		int GetMixBufferSize(void)
		{
			return this->m_buffer_size;
		}
		short *GetMixBuffer(void)
		{
			return (short *)m_buffer;
		}

	private:
		bool InitSLMix();
		bool InitSLEngine();
		bool CreateSLPlayer();

	private:
		//引擎对象
		SLObjectItf engineObject;
		//引擎接口
		SLEngineItf engineItf;

		//混音器对象(混音器作用是做声音处理)
		SLObjectItf outputmixObject;
		//混音器环境接口
		SLEnvironmentalReverbItf outputEnvironmentalReverbItf;

		//播放器对象
		SLObjectItf pcmPlayerObject;
		//播放接口
		SLPlayItf playItf;
		//播放队列
		SLAndroidSimpleBufferQueueItf simpleBufferQueueItf;

		unsigned int m_channels;
		unsigned int m_speed;

	public:
		void *m_buffer;
		void *m_ebuffer;
		int m_buffer_size;
		idDeque<harmBufferData, offsetof(harmBufferData, next)> queue;

};

idAudioHardwareOpenSLES::idAudioHardwareOpenSLES()
	: idAudioHardware(),
	engineObject(NULL),
	engineItf(NULL),
	outputmixObject(NULL),
	outputEnvironmentalReverbItf(NULL),
	pcmPlayerObject(NULL),
	simpleBufferQueueItf(NULL),
	playItf(NULL),
	m_channels(2),
	m_speed(PRIMARYFREQ),
	m_buffer(0),
	m_buffer_size(MIXBUFFER_SAMPLES * m_channels * 2)
{
}

idAudioHardwareOpenSLES::~idAudioHardwareOpenSLES()
{
}

bool idAudioHardwareOpenSLES::Initialize()
{
	common->Printf("------ Android Sound Initialization ------\n");

	m_channels = 2;
	idSoundSystemLocal::s_numberOfSpeakers.SetInteger(2);
	m_speed = PRIMARYFREQ;
	this->m_buffer_size = MIXBUFFER_SAMPLES * m_channels * 2;
	m_buffer = malloc(this->m_buffer_size);
	memset(m_buffer,0,this->m_buffer_size);
	m_ebuffer = malloc(this->m_buffer_size);
	memset(m_ebuffer,0,this->m_buffer_size);

	InitSLEngine();
	InitSLMix();
	CreateSLPlayer();
	return true;
}

// init OpenSLES
bool idAudioHardwareOpenSLES::InitSLEngine()
{
	SLresult result;

	//初始化引擎对象并由对象得到接口
	result = slCreateEngine(&engineObject, 0, 0, 0, 0, 0);
	HARM_CHECK_RESULT(result, false, "slCreateEngine() -> %d", result)

	result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
	HARM_CHECK_RESULT(result, false, "(SLObjectItf)engineObject::Realize() -> %d", result)

	result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineItf);
	HARM_CHECK_RESULT(result, false, "(SLObjectItf)engineObject::GetInterface(SL_IID_ENGINE) -> %d", result)

		return true;
}

bool idAudioHardwareOpenSLES::InitSLMix()
{
	//需要做的声音处理功能数组
	const SLInterfaceID mid[1] = {SL_IID_ENVIRONMENTALREVERB};
	const SLboolean mird[1] = {SL_BOOLEAN_FALSE};
	SLresult result;
	
	//混音器对象创建
	result = (*engineItf)->CreateOutputMix(engineItf, &outputmixObject,1,mid,mird);
	HARM_CHECK_RESULT(result, false, "(SLEngineItf)engineItf::CreateOutputMix(SL_IID_ENVIRONMENTALREVERB) -> %d", result)

	//得到上面声明的处理功能的接口
	result = (*outputmixObject)->Realize(outputmixObject,SL_BOOLEAN_FALSE);
	HARM_CHECK_RESULT(result, false, "(SLObjectItf)outputmixObject::Realize() -> %d", result)

		//混音器接口创建
	result = (*outputmixObject)->GetInterface(outputmixObject,SL_IID_ENVIRONMENTALREVERB,&outputEnvironmentalReverbItf);
	HARM_CHECK_RESULT(result, false, "(SLObjectItf)outputmixObject::GetInterface(SL_IID_ENVIRONMENTALREVERB) -> %d", result)

		return true;
	//混音器环境属性设置
		const SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
	result = (*outputEnvironmentalReverbItf)->SetEnvironmentalReverbProperties(outputEnvironmentalReverbItf,&reverbSettings);
	HARM_CHECK_RESULT_NORETURN(result, "(SLEnvironmentalReverbItf)outputEnvironmentalReverbItf::SetEnvironmentalReverbProperties(SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR) -> %d", result)

	return true;
}

bool idAudioHardwareOpenSLES::CreateSLPlayer()
{
	SLresult result;

	//播放队列
	SLDataLocator_AndroidBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,2};
	SLDataFormat_PCM format_pcm = {
		SL_DATAFORMAT_PCM, //格式
		m_channels,//声道数
		SL_SAMPLINGRATE_44_1,//采样率 // TODO: 44100
		SL_PCMSAMPLEFORMAT_FIXED_16,//采样位数 一定要与播放的pcm的一样 要不然可能快也可能会慢
		SL_PCMSAMPLEFORMAT_FIXED_16,
		SL_SPEAKER_FRONT_RIGHT|SL_SPEAKER_FRONT_LEFT,//声道布局 前右|前左
		SL_BYTEORDER_LITTLEENDIAN
	};
	//播放源
	SLDataSource dataSource = {&android_queue,&format_pcm};
	//混音器
	SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX,outputmixObject};
	//关联混音器
	SLDataSink audiosnk = {&outputMix,NULL};
	//要实现的功能
	const SLInterfaceID ids[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION};//队列播放
	const SLboolean irds[] = {SL_BOOLEAN_FALSE, SL_BOOLEAN_FALSE};
	result = (*engineItf)->CreateAudioPlayer(engineItf,&pcmPlayerObject,&dataSource,&audiosnk,2,ids,irds);
	HARM_CHECK_RESULT(result, false, "(SLEngineItf)engineItf::CreateAudioPlayer() -> %d", result)
	(*pcmPlayerObject)->Realize(pcmPlayerObject,SL_BOOLEAN_FALSE);
	HARM_CHECK_RESULT(result, false, "(SLObjectItf)pcmPlayerObject::Realize() -> %d", result)

		SLAndroidConfigurationItf inputConfig;
	result = (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_ANDROIDCONFIGURATION, &inputConfig);
	HARM_CHECK_RESULT(result, false, "(SLObjectItf)pcmPlayerObject::GetInterface() -> %d", result)

		SLuint32 presetValue =SL_ANDROID_PERFORMANCE_LATENCY;
	result = (*inputConfig)->SetConfiguration(inputConfig, SL_ANDROID_KEY_PERFORMANCE_MODE, &presetValue, sizeof(SLuint32));
	//HARM_CHECK_RESULT(result, false, "(SLAndroidConfigurationItf)inputConfig::SetConfiguration() -> %d", result)

	//创建播放接口
	result = (*pcmPlayerObject)->GetInterface(pcmPlayerObject,SL_IID_PLAY,&playItf);
	HARM_CHECK_RESULT(result, false, "(SLObjectItf)pcmPlayerObject::GetInterface(SL_IID_PLAY) -> %d", result)

	//得到Androidbufferqueue接口
	result = (*pcmPlayerObject)->GetInterface(pcmPlayerObject,SL_IID_ANDROIDSIMPLEBUFFERQUEUE,&simpleBufferQueueItf);
	HARM_CHECK_RESULT(result, false, "(SLObjectItf)pcmPlayerObject::GetInterface(SL_IID_BUFFERQUEUE) -> %d", result)
#if 1
	//注册回掉函数
	result = (*simpleBufferQueueItf)->RegisterCallback(simpleBufferQueueItf,pcmBufferCallBack, this);
	HARM_CHECK_RESULT(result, false, "(SLAndroidSimpleBufferQueueItf)simpleBufferQueueItf::RegisterCallback() -> %d", result)
#endif
	//第一次手动调用回掉函数 开始播放
	pcmBufferCallBack(simpleBufferQueueItf, this);

	//设置播放状态
	result = (*playItf)->SetPlayState(playItf,SL_PLAYSTATE_PLAYING);
	HARM_CHECK_RESULT(result, false, "(SLPlayItf)playItf::SetPlayState(SL_PLAYSTATE_PLAYING) -> %d", result)

	return true;
}

// this callback handler is called every time a buffer finishes playing
void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	idAudioHardwareOpenSLES *object = (idAudioHardwareOpenSLES *)context;
	harmBufferData *data = object->queue.Get();
	int c = 0;
	if(data)
	{
		SLresult result;
		result = (*bq)->Enqueue(bq, data->data, data->length);
		//common->Printf("SLAndroidSimpleBufferQueueItf::Enqueue() -> %d  %p : %d\n", object->queue.size, data, result);
		if (SL_RESULT_SUCCESS != result) {
			//pthread_mutex_unlock(&audioEngineLock);
			//common->Warning("(SLAndroidSimpleBufferQueueItf)bq::Enqueue() -> %d", result);
		}
		delete data;
		c++;
	}
	if(c == 0)
		(*bq)->Enqueue(bq, object->m_ebuffer, object->m_buffer_size);
}

