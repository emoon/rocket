/*
 * Audio spectrum generation for RocketEditor
 * Uses minimp3+kissfft for MP3 decoding and FFT
 */

#include <emgui/Emgui.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <tinycthread.h>
#include "Dialog.h"
#include "TrackData.h"

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_FLOAT_OUTPUT
#include "minimp3_ex.h"
#include "kiss_fft.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common constants and types

#define FFT_SIZE 2048
#define SPECTRUM_LENGTH 1024
#define IMAGE_HEIGHT 128
#define SAMPLING_FREQUENCY 100
#define COLOR_STEPS 255
#define PALETTE_SIZE (3 * COLOR_STEPS)

static mtx_t s_mutex;

typedef struct SpectrumContext {
    unsigned int colors[PALETTE_SIZE];
    int j_table[IMAGE_HEIGHT];
    int pj_table[IMAGE_HEIGHT];
    int nj_table[IMAGE_HEIGHT];
    float f2;
    float maxIntensity;
} SpectrumContext;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Shared spectrum generation code

static void init_spectrum_context(SpectrumContext* ctx) {
    ctx->maxIntensity = 500 * 2;
    ctx->f2 = (float)((PALETTE_SIZE - 1) / log(ctx->maxIntensity + 1));

    // Build color palette
    for (int i = 0; i < PALETTE_SIZE; ++i) {
        int a = 255;
        if (i < PALETTE_SIZE * 0.666f)
            a = (int)(i * 255 / (PALETTE_SIZE * 0.666f));

        int b = 0;
        if (i < PALETTE_SIZE * 0.333f)
            b = i;
        else if (i < PALETTE_SIZE * 0.666f)
            b = -i + 510;

        int r = 0;
        if (i > PALETTE_SIZE * 0.666f)
            r = 255;
        else if (i > PALETTE_SIZE * 0.333f)
            r = i - 255;

        int g = 0;
        if (i > PALETTE_SIZE * 0.666f)
            g = i - 510;

        ctx->colors[i] = Emgui_color32((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a);
    }

    // Build log lookup tables
    float f = (float)(SPECTRUM_LENGTH / log((float)(IMAGE_HEIGHT + 1)));
    for (int i = 0; i < IMAGE_HEIGHT; ++i) {
        int j_ = (int)(f * log(i + 1));
        int pj_ = (int)(i > 0 ? f * log(i - 1 + 1) : j_);
        int nj_ = (int)(i < IMAGE_HEIGHT - 1 ? f * log(i + 1 + 1) : j_);

        ctx->j_table[i] = j_;
        ctx->pj_table[i] = pj_;
        ctx->nj_table[i] = nj_;
    }
}

static void generate_spectrum_column(const SpectrumContext* ctx, const float* fftData, unsigned int* output) {
    for (int row = 0; row < IMAGE_HEIGHT; ++row) {
        int j_ = ctx->j_table[(IMAGE_HEIGHT - row) - 1];
        int pj_ = ctx->pj_table[(IMAGE_HEIGHT - row) - 1];
        int nj_ = ctx->nj_table[(IMAGE_HEIGHT - row) - 1];

        float intensity = 125.0f * 4.0f * fftData[SPECTRUM_LENGTH - pj_ - 1] +
                          750.0f * 4.0f * fftData[SPECTRUM_LENGTH - j_ - 1] +
                          125.0f * 4.0f * fftData[SPECTRUM_LENGTH - nj_ - 1];

        if (intensity > ctx->maxIntensity)
            intensity = ctx->maxIntensity;
        if (intensity < 0.0f)
            intensity = 0.0f;

        int palettePos = (int)(ctx->f2 * log(intensity + 1));
        output[row] = ctx->colors[palettePos];
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct ThreadFuncData {
    text_t* path;
    MusicData* data;
} ThreadFuncData;

static void compute_fft(const float* input, float* output, int n, kiss_fft_cfg cfg) {
    kiss_fft_cpx* fft_in = malloc(n * sizeof(kiss_fft_cpx));
    kiss_fft_cpx* fft_out = malloc(n * sizeof(kiss_fft_cpx));

    // Apply Hann window
    for (int i = 0; i < n; i++) {
        float window = 0.5f * (1.0f - cosf(2.0f * 3.14159265f * i / n));
        fft_in[i].r = input[i] * window;
        fft_in[i].i = 0.0f;
    }

    kiss_fft(cfg, fft_in, fft_out);

    // Normalize FFT output to 0-1 range
    float scale = 4.0f / n;
    for (int i = 0; i < n / 2; i++) {
        float mag = sqrtf(fft_out[i].r * fft_out[i].r + fft_out[i].i * fft_out[i].i);
        output[i] = mag * scale;
    }

    free(fft_in);
    free(fft_out);
}

int decodeFunc(void* inData) {
    ThreadFuncData* threadData = (ThreadFuncData*)inData;
    MusicData* data = threadData->data;

    mtx_lock(&s_mutex);

    // Open MP3 file
    mp3dec_ex_t dec;
#ifdef _WIN32
    char utf8_path[2048];
    WideCharToMultiByte(CP_UTF8, 0, threadData->path, -1, utf8_path, sizeof(utf8_path), NULL, NULL);
    if (mp3dec_ex_open(&dec, utf8_path, MP3D_SEEK_TO_SAMPLE)) {
#else
    if (mp3dec_ex_open(&dec, threadData->path, MP3D_SEEK_TO_SAMPLE)) {
#endif
        Dialog_showError(TEXT("Unable to decode MP3 file. No music data will be available."));
        free(threadData);
        mtx_unlock(&s_mutex);
        return 0;
    }

    int sample_rate = dec.info.hz;
    int channels = dec.info.channels;
    size_t total_samples = dec.samples;

    // Decode entire file to mono
    float* pcm = malloc((total_samples / channels) * sizeof(float));
    mp3d_sample_t* temp = malloc(total_samples * sizeof(mp3d_sample_t));

    size_t read = mp3dec_ex_read(&dec, temp, total_samples);
    size_t mono_count = read / channels;

    for (size_t s = 0; s < mono_count; s++) {
        float sum = 0.0f;
        for (int ch = 0; ch < channels; ch++) {
            sum += temp[s * channels + ch];
        }
        pcm[s] = sum / channels;
    }
    free(temp);
    mp3dec_ex_close(&dec);

    int samples_per_window = sample_rate / SAMPLING_FREQUENCY;
    double duration = (double)mono_count / sample_rate;
    int numSamples = (int)(duration * SAMPLING_FREQUENCY);

    data->sampleCount = numSamples;

    // Initialize spectrum generation
    SpectrumContext ctx;
    init_spectrum_context(&ctx);

    unsigned int* fftPtr = malloc(IMAGE_HEIGHT * 4 * (numSamples + 1));
    unsigned int* fftOutput = fftPtr;
    float fftData[SPECTRUM_LENGTH];
    float* fft_input = malloc(FFT_SIZE * sizeof(float));
    kiss_fft_cfg fft_cfg = kiss_fft_alloc(FFT_SIZE, 0, NULL, NULL);

    float step = 99.0f / numSamples;
    float percentDone = 1.0f;

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex) {
        size_t start = (size_t)sampleIndex * samples_per_window;

        for (int j = 0; j < FFT_SIZE; j++) {
            size_t idx = start + j;
            fft_input[j] = (idx < mono_count) ? pcm[idx] : 0.0f;
        }

        compute_fft(fft_input, fftData, FFT_SIZE, fft_cfg);
        generate_spectrum_column(&ctx, fftData, fftOutput);
        fftOutput += IMAGE_HEIGHT;

        data->percentDone = (int)(percentDone);
        percentDone += step;
    }

    kiss_fft_free(fft_cfg);
    free(fft_input);
    free(pcm);
    free(threadData);

    data->percentDone = 100;
    data->fftData = fftPtr;

    mtx_unlock(&s_mutex);
    return 1;
}

int Music_decode(text_t* path, MusicData* data) {
    thrd_t id;

    if (thrd_busy == mtx_trylock(&s_mutex)) {
        Dialog_showError(TEXT("Decoding of stream already in-progress. Re-open after it's been completed."));
        return 1;
    }

    free(data->fftData);
    data->fftData = 0;

    ThreadFuncData* threadData = malloc(sizeof(ThreadFuncData));
    threadData->path = path;
    threadData->data = data;

    data->percentDone = 1;
    data->fftData = 0;

    mtx_unlock(&s_mutex);

    if (thrd_create(&id, decodeFunc, threadData) != thrd_success) {
        printf("Unable to create decode thread. Stalling main thread and running...\n");
        decodeFunc(threadData);
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Music_init(void) {
    mtx_init(&s_mutex, mtx_plain);
}
