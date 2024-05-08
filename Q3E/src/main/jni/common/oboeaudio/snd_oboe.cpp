//
// Created by Administrator on 2024/4/30.
//
#include "snd_oboe.h"

#include "oboe/Oboe.h"

#include <mutex>
#include <memory>

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

    int sampleRate = 44100;
    int channelCount = 2; // oboe::ChannelCount::Stereo
    AudioFormat format = AudioFormat::I16;
    Q3E_write_audio_data_f func = nullptr;
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
    if(channelCount > 0)
        this->channelCount = channel;
    if(format >= 0)
        this->format = static_cast<AudioFormat>(format);

    InitStream();
}

void Q3EOboeAudio::Init()
{
    LOCK_AUDIO();

    if(IsInited())
    {
        fprintf(stderr, "Q3EOboeAudio has inited\n");
        return;
    }

    InitStream();
}

void Q3EOboeAudio::InitStream()
{
    AudioStreamBuilder builder;
    // The builder set methods can be chained for convenience.
    Result result = builder.setSharingMode(SharingMode::Exclusive)
            ->setPerformanceMode(PerformanceMode::LowLatency)
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
        fprintf(stderr, "Q3EOboeAudio not init\n");
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
        fprintf(stderr, "Q3EOboeAudio not init\n");
        return;
    }

    mStream->requestStop();
}

DataCallbackResult Q3EOboeAudio::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames)
{
    if(func)
    {
        unsigned char *stream = (unsigned char *) audioData;
        func(stream, numFrames * channelCount * 2); // numFrames * numChannels * numWidth(Stereo)
        return DataCallbackResult::Continue;
    }
    return DataCallbackResult::Stop; // no register callback
}

void Q3E_Oboe_Init(int sampleRate, int channel, int format, Q3E_write_audio_data_f func)
{
    if(audio.IsInited())
    {
        fprintf(stderr, "Q3EOboeAudio has inited\n");
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