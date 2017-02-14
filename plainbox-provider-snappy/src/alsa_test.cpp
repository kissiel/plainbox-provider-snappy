#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <alsa/asoundlib.h>

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
        if (snd_pcm_hw_params_set_format(this->pcm_handle, params,
            SND_PCM_FORMAT_FLOAT_LE) < 0) {
            throw AlsaError("Failed to set format");
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
    }
    void sine(const float freq, const float duration, const float amplitude) const {
        auto *buff = new float[this->period * 2];
        const void *ugly_ptr = static_cast<const void*>(buff);
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
        delete[] buff;
    }
private:
    snd_pcm_t *pcm_handle;
    unsigned rate;
    snd_pcm_uframes_t period;
};
}; //namespace Alsa

void playback_test(float duration) {
    auto player = Alsa::Pcm();
    player.set_params(44100);
    player.sine(440, duration, 0.5f);
}


int main(int argc, char *argv[]) {
    // uncomment for a bit more logging
    // logger.set_level(Logger::Level::debug);
    std::vector<std::string> args{};
    for (int i=0; i < argc; ++i) {
        args.push_back(std::string(argv[i]));
    }
    std::map<std::string, void(*)(float)> scenarios;
    scenarios["playback"] = playback_test;
    if (args.size() < 2) {
        std::cerr << "Required 'scenario' argument missing" << std::endl;
        return 1;
    }
    if (scenarios.find(args[1]) == scenarios.end()) {
        std::cerr << args[1] << " scenario not found!" << std::endl;
        return 1;
    }
    float duration = 1.0f;
    auto it = std::find(args.begin(), args.end(), std::string("-d"));
    if (it != args.end()) { // not doing && because of sequence points
        if (++it != args.end()) {
            duration = std::stof(*it);
        }
    }
    scenarios[args[1]](duration);
}
