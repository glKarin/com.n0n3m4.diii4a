//
// Created by Administrator on 2024/4/30.
//
#include "snd_oboe.h"

#include "oboe/Oboe.h"

#include <mutex>
#include <memory>

#define Q3E_OBOE_DEFAULT_SAMPLE_RATE 44100
#define Q3E_OBOE_DEFAULT_CHANNEL Q3E_OBOE_CHANNEL_STEREO
#define Q3E_OBOE_DEFAULT_FORMAT AudioFormat::I16
#define Q3E_OBOE_DEFAULT_WIDTH 2 * 2 // numChannels * numWidth(Stereo)

#define Q3E_OBOE_FORMAT_CAST(x) static_cast<AudioFormat>(x)

#define LOCK_AUDIO() std::lock_guard<std::mutex> guard(mutex);

using namespace oboe;

class Q3EOboeAudio : public AudioStreamDataCallback {
public:
    Q3EOboeAudio() = default;
    virtual ~Q3EOboeAudio();

    void Init();
    void Init(int sampleRate, int channel, int format);
    void Shutdown();
    // Call this from Activity onResume()
    void Start();
    // Call this from Activity onPause()
    void Stop();
    void Lock() {
        //printf("oboe::lock %zu\n", pthread_self());
        mutex.lock();
    }
    void Unlock() {
        //printf("oboe::unlock %zu\n", pthread_self());
        mutex.unlock();
    }
    bool IsInited() const {
        return (bool)mStream;
    }
    void SetCallback(Q3E_write_audio_data_f f) {
        LOCK_AUDIO();
        func = f;
    }

    DataCallbackResult onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) override;

    Q3EOboeAudio(const Q3EOboeAudio &) = delete;
    Q3EOboeAudio(Q3EOboeAudio &&) = delete;
    Q3EOboeAudio & operator=(const Q3EOboeAudio &) = delete;
    Q3EOboeAudio & operator=(Q3EOboeAudio &&) = delete;

private:
    void InitStream();

private:
    std::shared_ptr<AudioStream> mStream;
    std::mutex mutex;

    int sampleRate = Q3E_OBOE_DEFAULT_SAMPLE_RATE;
    int channelCount = Q3E_OBOE_DEFAULT_CHANNEL; // oboe::ChannelCount::Stereo
    AudioFormat format = Q3E_OBOE_DEFAULT_FORMAT;
    Q3E_write_audio_data_f func = nullptr;
    unsigned int width = Q3E_OBOE_DEFAULT_WIDTH;
};

static Q3EOboeAudio audio;

Q3EOboeAudio::~Q3EOboeAudio()
{
    Shutdown();
}

void Q3EOboeAudio::Init(int sampleRate, int channel, int format)
{
    LOCK_AUDIO();

    if(IsInited())
    {
        fprintf(stderr, "Q3EOboeAudio has inited\n");
        return;
    }

    if(sampleRate > 0)
        this->sampleRate = sampleRate;
    if(channelCount > 0 && channelCount <= Q3E_OBOE_CHANNEL_STEREO)
        this->channelCount = channel;
    if(format >= 0 && format <= Q3E_OBOE_FORMAT_FLOAT)
        this->format = Q3E_OBOE_FORMAT_CAST(format);

    unsigned int bytes_per_frame;
    switch(this->format)
    {
        case oboe::AudioFormat::I16:
            bytes_per_frame = 2;
            break;
        case oboe::AudioFormat::Float:
            bytes_per_frame = 4;
            break;
        case oboe::AudioFormat::I24:
            bytes_per_frame = 3;
            break;
        case oboe::AudioFormat::I32:
            bytes_per_frame = 4;
            break;
        default:
            bytes_per_frame = 0;
            break;
    }
    this->width = bytes_per_frame * this->channelCount;

    InitStream();
}

void Q3EOboeAudio::Init()
{
    LOCK_AUDIO();

    if(IsInited())
    {
        fprintf(stderr, "Q3EOboeAudio has initialized\n");
        return;
    }

    InitStream();
}

void Q3EOboeAudio::InitStream()
{
    AudioStreamBuilder builder;
    // The builder set methods can be chained for convenience.
    Result result = builder.setSharingMode(SharingMode::Exclusive)
            ->setDirection(oboe::Direction::Output)
            ->setPerformanceMode(PerformanceMode::LowLatency)
            ->setUsage(oboe::Usage::Game)
            ->setChannelCount(channelCount)
            ->setSampleRate(sampleRate)
            ->setSampleRateConversionQuality(SampleRateConversionQuality::Medium)
            ->setFormat(format)
            ->setDataCallback(this)
            ->openStream(mStream);
    if (result != Result::OK) {
        fprintf(stderr, "Q3EOboeAudio build stream error: %d\n", result);
        return;
    }

    printf("Q3EOboeAudio init: sampleRate %d, channelCount %d, format %d\n", sampleRate, channelCount, format);
    // Typically, start the stream after querying some stream information, as well as some input from the user
    // result = mStream->requestStart();
}

void Q3EOboeAudio::Shutdown() {
    LOCK_AUDIO();
    printf("Q3EOboeAudio shutdown\n");

    // Stop, close and delete in case not already closed.
    if (mStream) {
        mStream->stop();
        mStream->close();
        mStream.reset();
    }
}

void Q3EOboeAudio::Start() {
    LOCK_AUDIO();

    if(!IsInited())
    {
        fprintf(stderr, "Q3EOboeAudio not initialized\n");
        return;
    }

    if(!func)
    {
        fprintf(stderr, "Q3EOboeAudio callback function not setup\n");
        return;
    }

    mStream->requestStart();
}

void Q3EOboeAudio::Stop() {
    LOCK_AUDIO();

    if(!IsInited())
    {
        fprintf(stderr, "Q3EOboeAudio not initialized\n");
        return;
    }

    mStream->requestStop();
}

DataCallbackResult Q3EOboeAudio::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames)
{
#if 0
    if(func)
#endif
    {
        unsigned char *stream = (unsigned char *) audioData;
        func(stream, numFrames * width); // numFrames * numChannels * numWidth(Stereo)
        return DataCallbackResult::Continue;
    }
    return DataCallbackResult::Stop; // no register callback
}

Q3E_AudioDevice Q3E_Audio_Create(int sampleRate, int channel, int format, Q3E_write_audio_data_f func)
{
    Q3EOboeAudio *device = new Q3EOboeAudio;
    device->Init(sampleRate, channel, format);
    device->SetCallback(func);
    return device;
}

#if 0
#define Q3E_AUDIO_CHECK_DEVICE(x) if(!x) return;
#else
#define Q3E_AUDIO_CHECK_DEVICE(x)
#endif
#define Q3E_AUDIO_DEVICE_CAST(x) ((Q3EOboeAudio *)x)
void Q3E_Audio_Destroy(Q3E_AudioDevice device)
{
    Q3E_AUDIO_CHECK_DEVICE(device);
    auto ptr = Q3E_AUDIO_DEVICE_CAST(device);
    delete ptr;
}

void Q3E_Audio_Start(Q3E_AudioDevice device)
{
    Q3E_AUDIO_CHECK_DEVICE(device);
    Q3E_AUDIO_DEVICE_CAST(device)->Start();
}

void Q3E_Audio_Stop(Q3E_AudioDevice device)
{
    Q3E_AUDIO_CHECK_DEVICE(device);
    Q3E_AUDIO_DEVICE_CAST(device)->Stop();
}

void Q3E_Audio_Shutdown(Q3E_AudioDevice device)
{
    Q3E_AUDIO_CHECK_DEVICE(device);
    Q3E_AUDIO_DEVICE_CAST(device)->Shutdown();
}

void Q3E_Audio_Lock(Q3E_AudioDevice device)
{
    Q3E_AUDIO_CHECK_DEVICE(device);
    Q3E_AUDIO_DEVICE_CAST(device)->Lock();
}

void Q3E_Audio_Unlock(Q3E_AudioDevice device)
{
    Q3E_AUDIO_CHECK_DEVICE(device);
    Q3E_AUDIO_DEVICE_CAST(device)->Unlock();
}



// C-style interface
void Q3E_Oboe_Init(int sampleRate, int channel, int format, Q3E_write_audio_data_f func)
{
    if(audio.IsInited())
    {
        fprintf(stderr, "Q3EOboeAudio has initialized\n");
        return;
    }
    audio.Init(sampleRate, channel, format);
    audio.SetCallback(func);
}

void Q3E_Oboe_Start(void)
{
    audio.Start();
}

void Q3E_Oboe_Stop(void)
{
    audio.Stop();
}

void Q3E_Oboe_Shutdown(void)
{
    audio.Shutdown();
}

void Q3E_Oboe_Lock(void)
{
    audio.Lock();
}

void Q3E_Oboe_Unlock(void)
{
    audio.Unlock();
}