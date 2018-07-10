#include "generator.h"

generator::generator()
{
  for (int s = 0; s <= 6; ++s) {
    tone t;
    t.old_f = -1;
    t.new_f = -1;
    t.freq = 0;
    t.env = 0;
    t.phase = 0;
    t.iphase = 0;
    t.amp = 0;
    tone_vector.push_back(t);
  }
  build_sine_table();
  // set envelope increment size based on samplerate.
  envelope_increment_base = 1 / (double)(sample_rate/2);
}

generator::~generator()
{ }

void generator::runLoop()
{
  setup_sdl_audio();
  while (!quit) {

  }
  SDL_CloseAudioDevice(audio_device);
  SDL_Quit();
  std::cout << "Generator closing.\n";
}

void generator::setQuit()
{
    quit = true;
}

void generator::setTone(int s, int f)
{
    s -= 1;
    tone_vector[s].new_f = f;
    if(tone_vector[s].new_f != tone_vector[s].old_f)
    {
      tone_mutex.lock();
      tone_vector[s].new_f = f;
      if(tone_vector[s].new_f > -1)
      {
       std::cout << "Tone changed String: " << s << " Fret: " << f << "\n\n";
        tone_vector[s].freq = ftable[s][f];
        tone_vector[s].env = 0;
        tone_vector[s].old_f = tone_vector[s].new_f;
      }
      tone_mutex.unlock();
    }
}

void generator::build_sine_table() {

    /*
        Build sine table to use as oscillator:
        Generate a 16bit signed integer sinewave table with 1024 samples.
        This table will be used to produce the notes.
        Different notes will be created by stepping through
        the table at different intervals (phase).
    */

    double phase_increment = (2.0f * pi) / (double)TLENGTH;
    double current_phase = 0;
    for(int i = 0; i < TLENGTH; i++) {
        int sample = (int)(sin(current_phase) * 32767);
        sine_wave_table[i] = (int16_t)sample;
        current_phase += phase_increment;
        //printf("iphase<%f>, cphase<%f>, sample<%d>, i<%i>\n", phase_increment, current_phase, sample, i);
    }
}

void generator::audio_callback(Uint8 *byte_stream, int byte_stream_length) {

    /*
        This function is called whenever the audio buffer needs to be filled to allow
        for a continuous stream of audio.
        Write samples to byteStream according to byteStreamLength.
        The audio buffer is interleaved, meaning that both left and right channels exist in the same
        buffer.
    */

    // zero the buffer
    memset(byte_stream, 0, byte_stream_length);

    if(quit) {
        return;
    }

    // cast buffer as 16bit signed int.
    Sint16 *s_byte_stream = (Sint16*)byte_stream;

    // buffer is interleaved, so get the length of 1 channel.
    int remain = byte_stream_length / 2;

    // split the rendering up in chunks to make it buffersize agnostic.
    long chunk_size = 64;
    int iterations = remain/chunk_size;
    for(long i = 0; i < iterations; i++) {
        long begin = i*chunk_size;
        long end = (i*chunk_size) + chunk_size;
        this->write_samples(s_byte_stream, begin, end, chunk_size);
    }
}

void generator::write_samples(int16_t *s_byteStream, long begin, long end, long length) {
        // loop through the buffer and write samples.
        for (int i = 0; i < length; i+=2) {
          for (int s = 0; s < tone_vector.size(); s++) {
            if(tone_vector[s].new_f == -1) {
              continue; }

            double d_sample_rate = sample_rate;
            double d_table_length = table_length;
            // get correct phase increment for note depending on sample rate and table length.
            double phase_increment = ((tone_vector[s].freq) / d_sample_rate) * d_table_length;

            tone_vector[s].phase += phase_increment;
            tone_vector[s].iphase = (int)tone_vector[s].phase;

            if(tone_vector[s].phase >= table_length) {
                double diff = tone_vector[s].phase - table_length;
                tone_vector[s].phase = diff;
                tone_vector[s].iphase = (int)diff;
            }

            if(tone_vector[s].iphase < table_length && tone_vector[s].iphase > -1) {
                if(s_byteStream != NULL) {
                    tone_vector[s].amp = update_envelope(s);
                    int sample = sine_wave_table[tone_vector[s].iphase] * tone_vector[s].amp;

                    s_byteStream[i+begin] += sample; // left channel
                    s_byteStream[i+begin+1] += sample; // right channel
                    //printf("HERE: phase <%d>, amp <%f>\n, wt <%d>, sample <%d>, bs <%d>", tone_vector[s].iphase, tone_vector[s].amp,sine_wave_table[tone_vector[s].iphase], sample, s_byteStream[i+begin]);
                }
            }
          }
        }
}

void callback(void *obj, Uint8 *stream, int len)
{
  generator *gen = (generator*) obj;
  gen->audio_callback(stream, len);
}

int generator::setup_sdl_audio(void) {
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER);
    SDL_AudioSpec want;
    SDL_zero(want);
    SDL_zero(audio_spec);

    want.freq = sample_rate;
    // request 16bit signed little-endian sample format.
    want.format = AUDIO_S16LSB;
    // request 2 channels (stereo)
    want.channels = 2;
    want.samples = buffer_size;

    /*
        Tell SDL to call this function (audio_callback) that we have defined whenever there is an audiobuffer ready to be filled.
    */
    want.userdata = (void*)this;
    want.callback = callback;

    if(debuglog) {
        printf("\naudioSpec want\n");
        printf("----------------\n");
        printf("sample rate:%d\n", want.freq);
        printf("channels:%d\n", want.channels);
        printf("samples:%d\n", want.samples);
        printf("----------------\n\n");
    }

    audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &audio_spec, 0);

    if(debuglog) {
        printf("\naudioSpec get\n");
        printf("----------------\n");
        printf("sample rate:%d\n", audio_spec.freq);
        printf("channels:%d\n", audio_spec.channels);
        printf("samples:%d\n", audio_spec.samples);
        printf("size:%d\n", audio_spec.size);
        printf("----------------\n");
    }

    if (audio_device == 0) {
        if(debuglog) { printf("\nFailed to open audio: %s\n", SDL_GetError()); }
        return 1;
    }

    if (audio_spec.format != want.format) {
        if(debuglog) { printf("\nCouldn't get requested audio format.\n"); }
        return 2;
    }

    buffer_size = audio_spec.samples;
    SDL_PauseAudioDevice(audio_device, 0); // unpause audio.
    return 0;
}

double generator::get_envelope_amp_by_node(int base_node, double cursor) {

    // interpolate amp value for the current cursor position.

    double n1 = base_node;
    double n2 = base_node + 1;
    double relative_cursor_pos = (cursor - n1) / (n2 - n1);
    double amp_diff = (envelope_data[base_node+1] - envelope_data[base_node]);
    double amp = envelope_data[base_node]+(relative_cursor_pos*amp_diff);
    return amp;
}

double generator::update_envelope(int s) {
    double amp = 0;
    if(tone_vector[s].new_f>-1 && tone_vector[s].env < 3 && tone_vector[s].env > 2) {
        // if a note key is longpressed and cursor is in range, stay for sustain.
        amp = get_envelope_amp_by_node(2, tone_vector[s].env);
    } else {
        double speed_multiplier = pow(2, envelope_speed_scale);
        double cursor_inc = envelope_increment_base * speed_multiplier;
        tone_vector[s].env += cursor_inc;
        if(tone_vector[s].env < 1) {
            amp = get_envelope_amp_by_node(0, tone_vector[s].env);
        } else if(tone_vector[s].env < 2) {
            amp = get_envelope_amp_by_node(1, tone_vector[s].env);
        } else if(tone_vector[s].env < 3) {
            amp = get_envelope_amp_by_node(2, tone_vector[s].env);
        } else {
            amp = envelope_data[3];
        }
    }
    return amp;
}
