#include <iostream>
#include <stdint.h>
#include <string.h>
#include <cmath>

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

// check your file's sample rate before using, you may experience unwanted pitch-shift! default to 48kHz
#define SAMPLE_RATE 48000   
#define CHANNELS 2
#define MAX_SEC 400
#define MAX_SAMPLES (SAMPLE_RATE*MAX_SEC*CHANNELS)

#define NUM_COMBS 8
#define NUM_ALLPASS 4
#define MAX_DELAY 20000

float hpf_x_prev_l=0.f, hpf_y_prev_l=0.f;
float hpf_x_prev_r=0.f, hpf_y_prev_r=0.f;
constexpr float PI=3.14159265358979323846f;
const float HPF_FREQ=20.0f;
const float HPF_R=expf(-2.0f*PI*HPF_FREQ/SAMPLE_RATE);

// parameters (knobs)
float wet=.6f;                 // 0.0 to 1.0
float room_size=0.85f;         // affects comb feedback gains
float damping=.5f;             // high-freq damping
float pre_delay_ms=100.0f;     // pre-delay before reverb starts
float stereo_width=1.f;        // 0.0 mono, 1.0 wide

// delay buffers
float comb_left[NUM_COMBS][MAX_DELAY];
float comb_right[NUM_COMBS][MAX_DELAY];
int comb_indices_left[NUM_COMBS]={0};
int comb_indices_right[NUM_COMBS]={0};

float allpass_left[NUM_ALLPASS][MAX_DELAY];
float allpass_right[NUM_ALLPASS][MAX_DELAY];
int allpass_indices_left[NUM_ALLPASS]={0};
int allpass_indices_right[NUM_ALLPASS]={0};

// mutually prime delay lengths (44,100Hz gives a sample each ~22.7us)
int comb_delays[NUM_COMBS]={1909, 2767, 3217, 3559, 4133, 4639, 4999, 5511};
int allpass_delays[NUM_ALLPASS]={439, 599, 739, 881};
int pre_delay_samples=(int)(SAMPLE_RATE*pre_delay_ms/1000.0f);

int16_t song[MAX_SAMPLES];
int16_t output[MAX_SAMPLES];

inline float process_comb(int idx, float x, float delay[][MAX_DELAY], int *indices) {
    const int d=comb_delays[idx];
    int& i=indices[idx];
    float y=delay[idx][i];           // delayed sample

    // one pole low-pass in the feedback path for damping
    float fb=y+x;                    // feedforward
    float damped=(delay[idx][i]-fb*room_size);

    // HF damping with I/O relation y=y_prev*(1-damping)+damped*damping
    damped=damped*(1.f-damping)+delay[idx][i]*damping;
    delay[idx][i]=damped;

    i=(i+1)%d;
    return y;
}

inline float process_allpass(int idx, float x, float delay[][MAX_DELAY], int *indices) {
    const int d=allpass_delays[idx];
    int& i=indices[idx];
    float buf=delay[idx][i];
    float y=buf-0.5f*x;              // I/O relation -gx[n]+y[n-d] with global gain g=0.5
    delay[idx][i]=x+0.5f*buf;
    i=(i+1)%d;
    return y;
}

int clamp16(int x) {
    if (x>32767) return 32767;
    if (x<-32768) return -32768;
    return x;
}

inline float highpass_filt(float x, float& x_prev, float& y_prev) {
    float y=x-x_prev+HPF_R*y_prev;
    x_prev=x; y_prev=y;
    return y;
}

int main() {
    drmp3 mp3;
    if (!drmp3_init_file(&mp3,"input.mp3", NULL)) {
        std::cerr << "failed to open the mp3 music file\n";
        return 1;
    }

    drmp3_uint64 frames=drmp3_get_pcm_frame_count(&mp3);
    if (frames>SAMPLE_RATE*MAX_SEC) {
        std::cerr << "file's too long, clipping to " << MAX_SEC << " seconds\n";
        frames=SAMPLE_RATE*MAX_SEC-1;
    }

    drmp3_read_pcm_frames_s16(&mp3, frames, song);
    drmp3_uninit(&mp3);

    uint64_t total_samples=frames*CHANNELS;
    memset(output, 0, sizeof(output));

    const float scale_i16=1.0f/32768.0f;    // int16_t to float conversion
    const float scale_o16=32767.0f;

    for (uint64_t i=0; i<frames; ++i)
    {
        float dry_l=song[2*i]*scale_i16;
        float dry_r=song[2*i+1]*scale_i16;

        if (i<(uint64_t)pre_delay_samples) {
            output[2*i]=song[2*i];
            output[2*i+1]=song[2*i+1];
            continue;
        }

        float wet_l=0.f, wet_r=0.f;

        for (int c=0; c<NUM_COMBS; ++c) {
            wet_l+=process_comb(c, dry_l, comb_left, comb_indices_left);
            wet_r+=process_comb(c, dry_r, comb_right, comb_indices_right);
        }

        // average
        wet_l/=NUM_COMBS;
        wet_r/=NUM_COMBS;

        for (int a=0; a<NUM_ALLPASS; ++a) {
            wet_l=process_allpass(a, wet_l, allpass_left, allpass_indices_left);
            wet_r=process_allpass(a, wet_r, allpass_right, allpass_indices_right);
        }

        float mid=0.5f*(wet_l+wet_r);
        float side=0.5f*(wet_l-wet_r);
        wet_l=mid+side*stereo_width;
        wet_r=mid-side*stereo_width;

        const float dry_mix=1.0f-wet;      // keep constant power if you wish
        float out_l=dry_l*dry_mix+wet_l*wet;
        float out_r=dry_r*dry_mix+wet_r*wet;

        out_l=highpass_filt(out_l, hpf_x_prev_l, hpf_y_prev_l);
        out_r=highpass_filt(out_r, hpf_x_prev_r, hpf_y_prev_r);
        out_l=fmaxf(-1.0f, fminf(1.0f, out_l));
        out_r=fmaxf(-1.0f, fminf(1.0f, out_r));

        auto softclip=[](float x) {
            return (fabsf(x)>0.95f) ? (0.95f+(x>0?1:-1) *
                   (fabsf(x)-0.95f)/(1.0f+powf((fabsf(x)-0.95f),2))):x;
        };

        output[2*i]=(int16_t)clamp16(softclip(out_l)*scale_o16);
        output[2*i+1]=(int16_t)clamp16(softclip(out_r)*scale_o16);
    }

    // now that it's over...
    drwav_data_format format={};
    format.container=drwav_container_riff;
    format.format=DR_WAVE_FORMAT_PCM;
    format.channels=CHANNELS;
    format.sampleRate=SAMPLE_RATE;
    format.bitsPerSample=16;

    drwav wav;
    if (!drwav_init_file_write(&wav, "REVERBEDoutput.wav", &format, NULL)) {
        std::cerr << "couldn't write, failed to open wav file\n";
        return 1;
    }

    drwav_write_pcm_frames(&wav, frames, output);
    drwav_uninit(&wav);

    std::cout << "all done\n";  // FUCK YEAH
    return 0;
}
