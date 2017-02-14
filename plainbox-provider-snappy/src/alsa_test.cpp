#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include <alsa/asoundlib.h>
#include <complex>
#include <valarray>

typedef std::valarray<std::complex<float>> CArray;
void fft(CArray& x)
{
    // almost the same implementation as one on rosetta code
    const size_t N = x.size();
    if (N <= 1) return;
    CArray even = x[std::slice(0, N/2, 2)];
    CArray odd = x[std::slice(1, N/2, 2)];
    fft(even);
    fft(odd);
    for (size_t k = 0; k < N/2; ++k) {
        auto t = std::polar(1.0f, -2 * float(M_PI) * k / N) * odd[k];
        x[k] = even[k] + t;
        x[k+N/2] = even[k] - t;
    }
}

struct Logger {
    enum class Level {normal, info, debug};
    void set_level(Level new_lvl) {
        level = new_lvl;
    }
    std::ostream& info() {
        if (this->level >= Level::info) {
            return std::cout;
        } else {
            return this->nullStream;
        }
    }
    std::ostream& normal() {
            return std::cout;
    }
    Logger() : level(Level::normal) {}
    Logger(const Logger& l) {}

private:
    Level level;
    struct NullStream : std::ostream {
        template<typename T>
        NullStream& operator<<(T const&) {}
    };
    NullStream nullStream;
};

Logger logger = Logger();

namespace Alsa{

using std::string;

struct AlsaError: std::runtime_error {
    explicit AlsaError(const string& what_arg) : runtime_error(what_arg) {}
};
struct Pcm {
    enum class Mode {playback, capture};

    Pcm() : Pcm{"default", Mode::playback} {}
    Pcm(string device_name, Mode mode = Mode::playback) {
        snd_pcm_stream_t stream_mode;
        switch(mode) {
            case Mode::playback:
                stream_mode = SND_PCM_STREAM_PLAYBACK;
                break;
            case Mode::capture:
                stream_mode = SND_PCM_STREAM_CAPTURE;
                break;
        }

        int res = snd_pcm_open(&this->pcm_handle, device_name.c_str(),
                stream_mode, 0 /* blocking */);
        if (res < 0) {
            auto ec = std::error_code(-res, std::system_category());
            auto msg = string("Failed to open device: ") + string(device_name)
                + string(". ") + string(snd_strerror(res));
            throw std::system_error(ec, msg);
        }
    }
    ~Pcm() {
        snd_pcm_drain(this->pcm_handle);
        snd_pcm_close(this->pcm_handle);
    }
    void drain() {
        snd_pcm_drain(this->pcm_handle);
    }
    void set_params(const unsigned desired_rate) {
        snd_pcm_hw_params_t *params = nullptr;
        snd_pcm_hw_params_alloca(&params);
        snd_pcm_hw_params_any(this->pcm_handle, params);
        if (snd_pcm_hw_params_set_access(this->pcm_handle, params,
            SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
            throw AlsaError("Failed to set access mode");
        }
        if (snd_pcm_hw_params_set_channels(this->pcm_handle, params, 2) < 0) {
            throw AlsaError("Failed to set the number of channels");
        }
        if (auto res = snd_pcm_hw_params_set_format(this->pcm_handle, params,
            SND_PCM_FORMAT_FLOAT_LE) < 0) {
            throw AlsaError(string("Failed to set format") + string(
                snd_strerror(res)));
        }
        this->rate = desired_rate;
        // pick will determine how alsa picks value,
        // 0: exact value, -1: closest smaller value, +1: closest bigger value
        int pick = 0;
        if (snd_pcm_hw_params_set_rate_near(this->pcm_handle, params,
            &this->rate, &pick) < 0) {
            throw AlsaError("Failed to set rate");
        }
        if (snd_pcm_hw_params(this->pcm_handle, params) < 0) {
            throw AlsaError("Failed to write params to ALSA");
        }
        logger.info() << "got rate: " << rate << std::endl;
        snd_pcm_uframes_t frames;
        int dir;
        auto res = snd_pcm_hw_params_get_period_size(params, &frames, &dir);
        this->period = frames;
        unsigned period_time;
        snd_pcm_hw_params_get_period_time(params, &period_time, NULL);
        logger.info() << "period_time: " << period_time << std::endl;
        logger.info() << "state: " <<
            snd_pcm_state_name(snd_pcm_state(this->pcm_handle)) << std::endl;
        unsigned channs;
        snd_pcm_hw_params_get_channels_max(params, &channs);
        logger.info() << "no. of channels: " << channs << std::endl;
    }
    void sine(const float freq, const float duration, const float amplitude) const {
        auto *buff = new float[this->period * 2];
        void *ugly_ptr = static_cast<void*>(buff);
        unsigned t = 0;
        while (t < float(this->rate) * duration) {
            for (int i=0; i < this->period * 2; i+=2) {
                buff[i] = amplitude * sin(2 * M_PI *((t + i/2) / (this->rate / freq)));
                buff[i+1] = buff[i]; // the other channel
            }
            auto res = snd_pcm_writei(this->pcm_handle, ugly_ptr, this->period);
            if (res == -EPIPE) {
                logger.info() << "Buffer underrun" << std::endl;
                snd_pcm_prepare(this->pcm_handle);
            }
            t += this->period;
        }
        logger.info() << "state: " <<
            snd_pcm_state_name(snd_pcm_state(this->pcm_handle)) << std::endl;
        snd_pcm_start(this->pcm_handle);
        delete[] buff;
    }
    void record(float *buff, int buff_size /*in samples*/) {
        auto *local_buff = new float[this->period * 2];
        int res;
        snd_pcm_start(this->pcm_handle);
        logger.info() << "state: " <<
            snd_pcm_state_name(snd_pcm_state(this->pcm_handle)) << std::endl;

        while(buff_size > 0) {
            if (buff_size >= this->period * 2) {
                void *ugly_ptr = static_cast<void*>(buff);
                res = snd_pcm_readi(this->pcm_handle, ugly_ptr, this->period);
                buff_size -= this->period * 2;
                buff += this->period *2;
            } else {
                void *ugly_ptr = static_cast<void*>(local_buff);
                res = snd_pcm_readi(this->pcm_handle, ugly_ptr, this->period);
                std::memcpy(buff, local_buff, buff_size * sizeof(float));
                buff_size = 0;
            }
        }
        delete[] local_buff;
    }
    void play(float *buff, int buff_size) {
        snd_pcm_prepare(this->pcm_handle);
        while (buff_size > 0) {
            void *ugly_ptr = static_cast<void*>(buff);
            auto res = snd_pcm_writei(this->pcm_handle, ugly_ptr, this->period);
            buff_size -= this->period *2;
            buff += this->period * 2;
        }
        logger.info() << "state: " <<
            snd_pcm_state_name(snd_pcm_state(this->pcm_handle)) << std::endl;
    }


private:
    snd_pcm_t *pcm_handle;
    unsigned rate;
    snd_pcm_uframes_t period;
};
}; //namespace Alsa

int playback_test(float duration) {
    auto player = Alsa::Pcm();
    player.set_params(44100);
    player.sine(440, duration, 0.5f);
}


float dominant_freq(float *buff, int buffsize, int rate) {
    CArray data(buffsize);
    for (int i=0; i < buffsize; i++) {
        data[i] = std::complex<float>(buff[i], 0);
    }
    fft(data);
    auto freqs = std::vector<float>(buffsize/2); // drop mirrored freqs
    for (int i=0; i< buffsize / 2; i++){ 
        freqs[i] = std::abs(data[i]);
    }
    auto it = std::max_element(freqs.begin(), freqs.end());
    if (it != freqs.end()) {
        return float(std::distance(freqs.begin(), it)) / (float(buffsize) / rate);
    } else {
        return 0.0f;
    }
}

int loopback_test(float duration) {
    int buffsize = static_cast<int>(ceil(float(44100 * 2) * duration));


    auto *buff = new float[buffsize];
    for (int i=0; i<buffsize; i++) buff[i] = 0.0f;
    auto recorder = Alsa::Pcm("default", Alsa::Pcm::Mode::capture);
    recorder.set_params(44100);
    std::thread rec_thread([&recorder, &buff, &buffsize]() mutable{
        recorder.record(buff, buffsize);
    });
    const float test_freq = 440.0f; 
    auto player = Alsa::Pcm("default");
    player.set_params(44100);
    player.sine(test_freq, duration, 0.5f);
    player.drain();
    rec_thread.join();
    float dominant = dominant_freq(buff, buffsize, 88200); 
    if (dominant > 0.0f) {
        //buff contains stereo samples, so the sampling rate can be considered 88200
        logger.normal() << "Dominant frequency: " << dominant << std::endl;
        // inverse-proportional to duration - the longer it runs,
        // the more accurate the fft gets
        float epsilon = 2 / duration;
        return (abs(test_freq - dominant) < epsilon) ? 0 : 1;
    } else {
        return 1;
    }
}

int main(int argc, char *argv[]) {
    std::vector<std::string> args{};
    for (int i=0; i < argc; ++i) {
        args.push_back(std::string(argv[i]));
    }
    std::map<std::string, int(*)(float)> scenarios;
    scenarios["playback"] = playback_test;
    scenarios["loopback"] = loopback_test;
    if (args.size() < 2) {
        std::cerr << "Required 'scenario' argument missing" << std::endl;
        return 1;
    }
    if (scenarios.find(args[1]) == scenarios.end()) {
        std::cerr << args[1] << " scenario not found!" << std::endl;
        return 1;
    }
    if (std::find(args.begin(), args.end(), std::string("-v")) != args.end()) {
        logger.set_level(Logger::Level::info);
    }

    float duration = 1.0f;
    auto it = std::find(args.begin(), args.end(), std::string("-d"));
    if (it != args.end()) { // not doing && because of sequence points
        if (++it != args.end()) {
            duration = std::stof(*it);
        }
    }
    return scenarios[args[1]](duration);
}
