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

#define BUFFER_FRAME_COUNT 3

static idCVar harm_s_OpenSLESBufferCount("harm_s_OpenSLESBufferCount", "3", CVAR_SOUND | CVAR_ARCHIVE | CVAR_ROM, "Audio buffer count for OpenSLES, min is 3");

class idOpenSLESAudioBuffer
{
	public:
		idOpenSLESAudioBuffer(int size, int count);
		virtual ~idOpenSLESAudioBuffer();
		void * GetWriteBuffer(bool ifBlockReturn0 = false);
		void * GetReadBuffer(bool ifBlockReturn0 = false);
		void NextWriteFrame(void);
		void NextReadFrame(void);
		void DiscardUnreadBuffer(void);
		void SkipUnreadBuffer(void);
		void * BufferData() { return m_data; }
		void WaitReadFinished(void) {
			while(!m_readFinished)
			{
				Sys_WaitForEvent(TRIGGER_EVENT_SOUND_BACKEND_READ_FINISHED);
			}
		}
		void WaitWriteFinished(void) {
			while(!m_writeFinished)
			{
				Sys_WaitForEvent(TRIGGER_EVENT_SOUND_FRONTEND_WRITE_FINISHED);
			}
		}
		void Release(void);

	private:
		int NormalizeFrame(int frame) const {
			return (frame) % m_count;
		}

	private:
		void *m_data;
		int m_size;
		int m_count;
		volatile int m_writeFrame;
		volatile int m_readFrame;
		volatile bool m_writeFinished;
		volatile bool m_readFinished;
};

idOpenSLESAudioBuffer::idOpenSLESAudioBuffer(int size, int count)
	: m_data(0),
	m_size(size),
	m_count(count),
	m_writeFrame(2),
	m_readFrame(0),
	m_writeFinished(true),
	m_readFinished(true)
{
	m_data = malloc(size * count);
	memset(m_data, 0, size * count);
}

idOpenSLESAudioBuffer::~idOpenSLESAudioBuffer()
{
	free(m_data);
}

void idOpenSLESAudioBuffer::Release(void)
{
	m_writeFrame = 2;
	m_readFrame = 0;
	m_writeFinished = true;
	m_readFinished = true;
	Sys_TriggerEvent(TRIGGER_EVENT_SOUND_BACKEND_READ_FINISHED);
	Sys_TriggerEvent(TRIGGER_EVENT_SOUND_FRONTEND_WRITE_FINISHED);
}

ID_INLINE void * idOpenSLESAudioBuffer::GetWriteBuffer(bool ifBlockReturn0)
{
	while(NormalizeFrame(m_writeFrame + 1) == m_readFrame)
	{
		if(ifBlockReturn0)
			return 0;
		//LOGI("WWWFFF %d %d", m_writeFrame, m_readFrame);
		WaitReadFinished();
		DiscardUnreadBuffer();
		//SkipUnreadBuffer();
	}
	m_writeFinished = false;
	return (char *)m_data + m_size * m_writeFrame;
}

ID_INLINE void idOpenSLESAudioBuffer::NextWriteFrame(void)
{
	m_writeFrame = NormalizeFrame(m_writeFrame + 1);
	m_writeFinished = true;
	Sys_TriggerEvent(TRIGGER_EVENT_SOUND_FRONTEND_WRITE_FINISHED);
}

ID_INLINE void * idOpenSLESAudioBuffer::GetReadBuffer(bool ifBlockReturn0)
{
	while(NormalizeFrame(m_readFrame + 1) == m_writeFrame)
	{
		if(ifBlockReturn0)
			return 0;
		WaitWriteFinished();
		//LOGI("WWWBBB %d %d", m_writeFrame, m_readFrame);
	}
	m_readFinished = false;
	return (char *)m_data + m_size * m_readFrame;
}

ID_INLINE void idOpenSLESAudioBuffer::NextReadFrame(void)
{
	m_readFrame = NormalizeFrame(m_readFrame + 1);
	m_readFinished = true;
	Sys_TriggerEvent(TRIGGER_EVENT_SOUND_BACKEND_READ_FINISHED);
}

ID_INLINE void idOpenSLESAudioBuffer::DiscardUnreadBuffer(void)
{
	m_readFrame = NormalizeFrame(m_writeFrame - 1);
	memset((char *)m_data + NormalizeFrame(m_readFrame) * m_size, 0, m_size);
	//LOGI("RRR %d %d", m_writeFrame, m_readFrame);
}

ID_INLINE void idOpenSLESAudioBuffer::SkipUnreadBuffer(void)
{
	m_writeFrame = NormalizeFrame(m_readFrame + 1);
	//LOGI("RRR %d %d", m_writeFrame, m_readFrame);
}

static void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bq, void *context);

class idAudioHardwareOpenSLES : public idAudioHardware
{
	public:
		explicit idAudioHardwareOpenSLES();

		virtual ~idAudioHardwareOpenSLES();

		bool Initialize();
		void Release();

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
		bool Flush(void)
		{
			if(paused && m_playing)
			{
				m_buffer->WaitReadFinished();
				m_playing = false;
				(*playItf)->SetPlayState(playItf, SL_PLAYSTATE_PAUSED);
				common->Printf("[Harmattan]: OpenSLES paused.\n");
			}
			else if(!paused && !m_playing)
			{
				m_buffer->WaitReadFinished();
				m_playing = true;
				(*playItf)->SetPlayState(playItf, SL_PLAYSTATE_PLAYING);
				common->Printf("[Harmattan]: OpenSLES playing.\n");
			}
			return true;
		}
		void Write(bool flushing) // write finish in idSoundSystem::UpdateAsyncWrite
		{
			if(m_playing)
				m_buffer->NextWriteFrame();
		}

		int GetMixBufferSize(void)
		{
			return this->m_buffer_size;
		}
		short *GetMixBuffer(void) // start write in idSoundSystem::UpdateAsyncWrite
		{
			return (short *)m_buffer->GetWriteBuffer();
		}

		int GetNumberOfSpeakers(void)
		{
			return m_channels;
		}

		bool GetBackendBuffer(void **ebuffer, int *buffer_size) // call by backend
		{
			if(!m_playing)
				return false;

			*ebuffer = m_buffer->GetReadBuffer();
			if(!m_playing)
				return false;

			*buffer_size = m_buffer_size;
			return true;
		}

		void UpdateBackendFrame()
		{
			m_buffer->NextReadFrame();
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
		idOpenSLESAudioBuffer *m_buffer;
		int m_buffer_size;
		volatile bool m_playing;
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
	m_buffer_size(MIXBUFFER_SAMPLES * m_channels * 2),
	m_playing(false)
{
}

idAudioHardwareOpenSLES::~idAudioHardwareOpenSLES()
{
	common->Printf("----------- Android OpenSLES Shutdown ------------\n");
	Release();
	common->Printf("--------------------------------------\n");
}

void idAudioHardwareOpenSLES::Release()
{
	m_playing = false;
	if(m_buffer)
		m_buffer->Release();
	if(playItf)
	{
		(*playItf)->SetPlayState(playItf, SL_PLAYSTATE_STOPPED);
		playItf = NULL;
		simpleBufferQueueItf = NULL;
	}
	if(pcmPlayerObject)
	{
		(*pcmPlayerObject)->Destroy(pcmPlayerObject);
		pcmPlayerObject = NULL;
		simpleBufferQueueItf = NULL;
	}
	if(outputmixObject)
	{
		(*outputmixObject)->Destroy(outputmixObject);
		outputmixObject = NULL;
		outputEnvironmentalReverbItf = NULL;
	}
	if(engineObject)
	{
		(*engineObject)->Destroy(engineObject);
		engineObject = NULL;
		engineItf = NULL;
	}

	delete m_buffer;
	m_buffer = 0;
}

bool idAudioHardwareOpenSLES::Initialize()
{
	common->Printf("------ Android OpenSLES Sound Initialization ------\n");

	m_channels = 2;
	idSoundSystemLocal::s_numberOfSpeakers.SetInteger(2);
	m_speed = PRIMARYFREQ;
	this->m_buffer_size = MIXBUFFER_SAMPLES * m_channels * 2;
	int bufferCount = harm_s_OpenSLESBufferCount.GetInteger();
	if(bufferCount < BUFFER_FRAME_COUNT)
		bufferCount = BUFFER_FRAME_COUNT;
	m_buffer = new idOpenSLESAudioBuffer(m_buffer_size, bufferCount);

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
	HARM_CHECK_RESULT(result, false, "slCreateEngine() -> %d", result);

	result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
	HARM_CHECK_RESULT(result, false, "(SLObjectItf)engineObject::Realize() -> %d", result);

	result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineItf);
	HARM_CHECK_RESULT(result, false, "(SLObjectItf)engineObject::GetInterface(SL_IID_ENGINE) -> %d", result);

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
	HARM_CHECK_RESULT(result, false, "(SLEngineItf)engineItf::CreateOutputMix(SL_IID_ENVIRONMENTALREVERB) -> %d", result);

	//得到上面声明的处理功能的接口
	result = (*outputmixObject)->Realize(outputmixObject,SL_BOOLEAN_FALSE);
	HARM_CHECK_RESULT(result, false, "(SLObjectItf)outputmixObject::Realize() -> %d", result);

	//混音器接口创建
	result = (*outputmixObject)->GetInterface(outputmixObject,SL_IID_ENVIRONMENTALREVERB,&outputEnvironmentalReverbItf);
	HARM_CHECK_RESULT(result, false, "(SLObjectItf)outputmixObject::GetInterface(SL_IID_ENVIRONMENTALREVERB) -> %d", result);

	return true;
	//混音器环境属性设置
	const SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
	result = (*outputEnvironmentalReverbItf)->SetEnvironmentalReverbProperties(outputEnvironmentalReverbItf,&reverbSettings);
	HARM_CHECK_RESULT_NORETURN(result, "(SLEnvironmentalReverbItf)outputEnvironmentalReverbItf::SetEnvironmentalReverbProperties(SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR) -> %d", result);

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
	const SLboolean irds[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
	result = (*engineItf)->CreateAudioPlayer(engineItf,&pcmPlayerObject,&dataSource,&audiosnk,2,ids,irds);
	HARM_CHECK_RESULT(result, false, "(SLEngineItf)engineItf::CreateAudioPlayer() -> %d", result);
	(*pcmPlayerObject)->Realize(pcmPlayerObject,SL_BOOLEAN_FALSE);
	HARM_CHECK_RESULT(result, false, "(SLObjectItf)pcmPlayerObject::Realize() -> %d", result);

	SLAndroidConfigurationItf inputConfig;
	result = (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_ANDROIDCONFIGURATION, &inputConfig);
	HARM_CHECK_RESULT(result, false, "(SLObjectItf)pcmPlayerObject::GetInterface() -> %d", result);

//#if __ANDROID_API__ >= 25
	SLuint32 presetValue = SL_ANDROID_PERFORMANCE_LATENCY;
	result = (*inputConfig)->SetConfiguration(inputConfig, SL_ANDROID_KEY_PERFORMANCE_MODE, &presetValue, sizeof(SLuint32));
	//HARM_CHECK_RESULT(result, false, "(SLAndroidConfigurationItf)inputConfig::SetConfiguration() -> %d", result);
//#endif

	//创建播放接口
	result = (*pcmPlayerObject)->GetInterface(pcmPlayerObject,SL_IID_PLAY,&playItf);
	HARM_CHECK_RESULT(result, false, "(SLObjectItf)pcmPlayerObject::GetInterface(SL_IID_PLAY) -> %d", result);

	//得到Androidbufferqueue接口
	result = (*pcmPlayerObject)->GetInterface(pcmPlayerObject,SL_IID_ANDROIDSIMPLEBUFFERQUEUE,&simpleBufferQueueItf);
	HARM_CHECK_RESULT(result, false, "(SLObjectItf)pcmPlayerObject::GetInterface(SL_IID_BUFFERQUEUE) -> %d", result);
	//注册回掉函数
	result = (*simpleBufferQueueItf)->RegisterCallback(simpleBufferQueueItf,pcmBufferCallBack, this);
	HARM_CHECK_RESULT(result, false, "(SLAndroidSimpleBufferQueueItf)simpleBufferQueueItf::RegisterCallback() -> %d", result);
	//第一次手动调用回掉函数 开始播放
	m_playing = true;
	pcmBufferCallBack(simpleBufferQueueItf, this);
	//k result = (*simpleBufferQueueItf)->Enqueue(simpleBufferQueueItf, m_buffer->BufferData(), m_buffer_size);

	//设置播放状态
	result = (*playItf)->SetPlayState(playItf,SL_PLAYSTATE_PLAYING);
	HARM_CHECK_RESULT(result, false, "(SLPlayItf)playItf::SetPlayState(SL_PLAYSTATE_PLAYING) -> %d", result);

	return true;
}

// this callback handler is called every time a buffer finishes playing
void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	idAudioHardwareOpenSLES *object = (idAudioHardwareOpenSLES *)context;
	SLresult result;
	void *ebuffer;
	int buffer_size;

	if(!object->GetBackendBuffer(&ebuffer, &buffer_size))
		return;

	result = (*bq)->Enqueue(bq, ebuffer, buffer_size);
	//common->Printf("SLAndroidSimpleBufferQueueItf::Enqueue() -> %d\n", result);
#ifdef _DEBUG
	if (SL_RESULT_SUCCESS != result) {
		common->Warning("(SLAndroidSimpleBufferQueueItf)bq::Enqueue() -> %d", result);
	}
#endif
	object->UpdateBackendFrame();
}

