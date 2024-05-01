//
// Created by Administrator on 2024/4/30.
//
#include "snd_oboe.h"

#include "../../deplibs/oboe/include/oboe/Oboe.h"

#include <mutex>

#define LOCK_AUDIO() std::lock_guard<std::mutex> guard(mutex);

using namespace oboe;

static Q3E_write_audio_data_f func = nullptr;

class Q3EAudio : public AudioStreamDataCallback {
public:
    Q3EAudio() = default;
    virtual ~Q3EAudio();

    void Init();
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

    DataCallbackResult onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) override;

private:
    AudioStream *mStream;
    std::mutex mutex;

    int kChannelCount = 2; // oboe::ChannelCount::Stereo
    int kSampleRate = 44100;
};

static Q3EAudio audio;

Q3EAudio::~Q3EAudio()
{

}

void Q3EAudio::Init() {
    LOCK_AUDIO();

    AudioStreamBuilder builder;
    // The builder set methods can be chained for convenience.
    Result result = builder.setSharingMode(SharingMode::Exclusive)
            ->setPerformanceMode(PerformanceMode::LowLatency)
            ->setChannelCount(kChannelCount)
            ->setSampleRate(kSampleRate)
            ->setSampleRateConversionQuality(SampleRateConversionQuality::Medium)
            ->setFormat(AudioFormat::I16)
            ->setDataCallback(this)
            ->openStream(&mStream);
    if (result != Result::OK) {
        return;
    }

    // Typically, start the stream after querying some stream information, as well as some input from the user
    // result = mStream->requestStart();
}

void Q3EAudio::Shutdown() {
    LOCK_AUDIO();

    // Stop, close and delete in case not already closed.
    if (mStream) {
        mStream->stop();
        mStream->close();
        delete mStream;
        mStream = nullptr;
    }
}

void Q3EAudio::Start() {
    LOCK_AUDIO();

    mStream->requestStart();
}

void Q3EAudio::Stop() {
    LOCK_AUDIO();

    mStream->requestStop();
}

DataCallbackResult Q3EAudio::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames)
{
    if(func)
    {
        unsigned char *stream = (unsigned char *) audioData;
        func(stream, numFrames * 4); // numFrames * numChannels * numWidth(Stereo)
        return DataCallbackResult::Continue;
    }
    return DataCallbackResult::Stop; // no register callback
}

void Q3E_Oboe_Init(Q3E_write_audio_data_f f)
{
    func = f;
    audio.Init();
}

void Q3E_Oboe_Start()
{
    audio.Start();
}

void Q3E_Oboe_Stop()
{
    audio.Stop();
}

void Q3E_Oboe_Shutdown()
{
    audio.Shutdown();
}

void Q3E_Oboe_Lock()
{
    audio.Lock();
}

void Q3E_Oboe_Unlock()
{
    audio.Unlock();
}