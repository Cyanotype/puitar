#ifndef GENERATOR_H
#define GENERATOR_H

#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <math.h>
#include <SDL2/SDL.h>

#define TLENGTH 1024

class generator
{
    public:
        generator();
        virtual ~generator();

        void runLoop();
        void setQuit();

        void setTone(int string, int fret);

        void audio_callback(Uint8 *byte_stream, int byte_stream_length);

    protected:

    private:
      bool quit = false;
      bool debuglog = false;

      // SDL
      Uint16 buffer_size = 512; // must be a power of two, decrease to allow for a lower latency, increase to reduce risk of underrun.
      SDL_AudioDeviceID audio_device;
      SDL_AudioSpec audio_spec;
      int sample_rate = 44100;
      int table_length = TLENGTH;
      int sine_wave_table[TLENGTH];
      // voice
      const double pi = 3.14159265358979323846;
      const double chromatic_ratio = 1.059463094359295264562;

      float ftable[6][22] = {
      {329.6300f,349.2310f,369.9970f,391.9980f,415.3080f,440.0030f,466.1670f,493.8870f,523.2550f,554.3700f,587.3340f,
      622.2590f,659.2600f,698.4620f,739.9950f,783.9970f,830.6160f,880.0070f,932.3350f,987.7750f,1046.5100f,1108.7400f},
      {246.9400f,261.6240f,277.1810f,293.6630f,311.1250f,329.6250f,349.2260f,369.9920f,391.9930f,415.3020f,439.9970f,
      466.1610f,493.8800f,523.2480f,554.3620f,587.3260f,622.2500f,659.2510f,698.4520f,739.9840f,783.9860f,830.6050f},
      {196.0000f,207.6550f,220.0030f,233.0850f,246.9450f,261.6290f,277.1860f,293.6680f,311.1310f,329.6320f,349.2320f,
      369.9990f,392.0000f,415.3100f,440.0050f,466.1690f,493.8890f,523.2580f,554.3720f,587.3370f,622.2620f,659.2630f},
      {146.8300f,155.5610f,164.8110f,174.6110f,184.9940f,195.9950f,207.6490f,219.9960f,233.0780f,246.9380f,261.6210f,
      277.1780f,293.6600f,311.1220f,329.6220f,349.2230f,369.9890f,391.9890f,415.2980f,439.9930f,466.1570f,493.8760f},
      {110.0000f,116.5410f,123.4710f,130.8130f,138.5910f,146.8320f,155.5640f,164.8140f,174.6140f,184.9970f,195.9980f,
      207.6520f,220.0000f,233.0820f,246.9420f,261.6260f,277.1830f,293.6650f,311.1270f,329.6280f,349.2290f,369.9950f},
      {82.4100f,87.3104f,92.5021f,98.0026f,103.8300f,110.0040f,116.5450f,123.4760f,130.8180f,138.5970f,146.8380f,
      155.5690f,164.8200f,174.6210f,185.0040f,196.0050f,207.6600f,220.0080f,233.0910f,246.9510f,261.6360f,277.1930f}};

      // functions
      void build_sine_table();
      void write_samples(int16_t *s_byteStream, long begin, long end, long length);
      int setup_sdl_audio(void);

      // amplitude envelope
      double update_envelope(int s);
      double get_envelope_amp_by_node(int base_node, double cursor);
      double envelope_speed_scale = 2.75; // set envelope speed 1-8
//      double envelope_data[4] = {0.99, 1.00, 0.75, 0.0}; // ADSR amp range 0.0-1.0
      double envelope_data[4] = {0.99, 1.15, 1.05, 0.0}; // ADSR amp range 0.0-1.0
      double envelope_increment_base = 0; // this will be set in init_data based on current samplingrate.

      typedef struct
      {
        int old_f;
        int new_f;
        double freq;
        float env;
        double phase;
        int iphase;
        double amp;
      } tone;

      std::mutex tone_mutex;
      std::vector<tone> tone_vector;
};

#endif // GENERATOR_H
