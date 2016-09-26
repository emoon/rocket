#include <emgui/Emgui.h>
#include <Bass.h>
#include "Dialog.h"
#include <math.h>
#include <stdio.h>
#include "TrackData.h"
#include <tinycthread.h>

typedef struct ThreadFuncData {
    text_t* filename;
    MusicData* data;
} ThreadFuncData;

// This code has in many parts been ported to C from this code under the MIT licence.
// https://github.com/framefield/tooll/blob/master/Tooll/Components/Helper/GenerateSoundImage.cs
// Copyright (c) 2016 Framefield. All rights reserved.

int decodeFunc(void* inData)
{
    int a, b, r, g, i;
    //short waveData[8 * 1024];
    unsigned int* fftPtr;
    unsigned int* fftOutput;
    float fftData[1024];
    QWORD len;
    //double timeInSec = 0.0;
    const float maxIntensity = 500 * 2;
    const int COLOR_STEPS = 255;
    const int PALETTE_SIZE = 3 * COLOR_STEPS;
    const int spectrumLength = 1024;
    ThreadFuncData* threadData = (ThreadFuncData*)inData;
    unsigned int colors[255 * 3];
    int imageHeight = 128;
	int flags = BASS_STREAM_DECODE | BASS_SAMPLE_MONO | BASS_POS_SCAN;
	text_t* path = threadData->filename;
	MusicData* data = threadData->data;
    float percentDone = 1.0f;
    float step = 0.0f;

#ifdef _WIN32
	flags |= BASS_UNICODE;
#endif

	HSTREAM chan = BASS_StreamCreateFile(0, path, 0, 0, flags);

	if (!chan)
	{
	    //Dialog_showError(TEXT("Unable to open stream for decode. No music data will be availible."));
	    return 0;
	}

    len = BASS_ChannelGetLength(chan, BASS_POS_BYTE);

    if (len == -1)
    {
	    //Dialog_showError(TEXT("Stream has no length. No music data will be availible."));
        BASS_StreamFree(chan);
        return 0;
    }

    const int SAMPLING_FREQUENCY = 100; //warning: in TimelineImage this samplingfrequency is assumed to be 100
    const double SAMPLING_RESOLUTION = 1.0 / SAMPLING_FREQUENCY;

    QWORD sampleLength = BASS_ChannelSeconds2Bytes(chan, SAMPLING_RESOLUTION);
    int numSamples = (int)((double)BASS_ChannelGetLength(chan, BASS_POS_BYTE) / (double)sampleLength);

    printf("Num samples %d\n", (int)sampleLength);
    printf("Num samples %d\n", numSamples);

    BASS_ChannelPlay(chan, 0);

    int j_table[128];
    int pj_table[128];
    int nj_table[128];

    // TODO: Size optimize?

    data->sampleCount = numSamples;

    fftPtr = fftOutput = (unsigned int*)malloc(128 * 4 * (numSamples + 1));

    //const int spectrumLength = 1024;
    //const int imageHeight = 256;

    //var spectrumImage = new Bitmap((int)numSamples, imageHeight);
    //var volumeImage = new Bitmap((int)numSamples, imageHeight);

    //s_editorData.waveViewSize = 128;
	//s_editorData.trackViewInfo.windowSizeX -= s_editorData.waveViewSize;

    //timeInSec =(DWORD)BASS_ChannelBytes2Seconds(chan, len);
    //printf(" %u:%02u\n", (int)timeInSec / 60, (int)timeInSec % 60);

    for (i = 0; i < PALETTE_SIZE; ++i)
    {
        a = 255;
        if (i < PALETTE_SIZE * 0.666f)
            a = (int)(i * 255 / (PALETTE_SIZE * 0.666f));

        b = 0;
        if (i < PALETTE_SIZE * 0.333f)
            b = i;
        else if (i < PALETTE_SIZE * 0.666f)
            b = -i + 510;

        r = 0;
        if (i > PALETTE_SIZE * 0.666f)
            r = 255;
        else if (i > PALETTE_SIZE * 0.333f)
            r = i - 255;

        g = 0;
        if (i > PALETTE_SIZE * 0.666f)
            g = i - 510;

        colors[i] = Emgui_color32((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a);
    }

    float f = (float)(spectrumLength / log((float)(imageHeight + 1)));
    float f2 = (float)((PALETTE_SIZE - 1) / log(maxIntensity + 1));

    (void)f2;

    for (int rowIndex = 0; rowIndex < imageHeight; ++rowIndex)
    {
        int j_ = (int)(f * log(rowIndex + 1));
        int pj_ = (int)(rowIndex > 0 ? f * log(rowIndex - 1 + 1) : j_);
        int nj_ = (int)(rowIndex < imageHeight - 1 ? f * log(rowIndex + 1 + 1) : j_);

        j_table[rowIndex] = j_;
        pj_table[rowIndex] = pj_;
        nj_table[rowIndex] = nj_;
    }

    step = (float)(99.0f / numSamples);
    percentDone = 1.0f;

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        BASS_ChannelSetPosition(chan, sampleIndex * sampleLength, BASS_POS_BYTE);
        BASS_ChannelGetData(chan, fftData, BASS_DATA_FFT2048);

        data->percentDone = (int)(percentDone);

        //printf("%d/%d\n", sampleIndex, numSamples);

        for (int rowIndex = 0; rowIndex < imageHeight; ++rowIndex)
        {
            //int j_ = (int)(f * log(rowIndex + 1));
            //int pj_ = (int)(rowIndex > 0 ? f * log(rowIndex - 1 + 1) : j_);
            //int nj_ = (int)(rowIndex < imageHeight - 1 ? f * log(rowIndex + 1 + 1) : j_);

            int j_ = j_table[(imageHeight - rowIndex) - 1];
            int pj_ = pj_table[(imageHeight - rowIndex) - 1];
            int nj_ = nj_table[(imageHeight - rowIndex) - 1];

            //printf("index %d - %d %d %d\n", rowIndex, j_, pj_, nj_);

            float intensity = 125.0f * 4.0f * fftData[spectrumLength - pj_ - 1] +
                              750.0f * 4.0f * fftData[spectrumLength - j_ - 1] +
                              125.0f * 4.0f * fftData[spectrumLength - nj_ - 1];
            if (intensity > maxIntensity)
                intensity = maxIntensity;

            if (intensity < 0.0f)
                intensity = 0.0f;

            int palettePos = (int)(f2 * log(intensity + 1));

            *fftOutput++ = colors[palettePos];
        }

        percentDone += step;
    }

    BASS_StreamFree(chan);

    free(threadData->filename);
    free(threadData);

    data->percentDone = 100;
    data->fftData = fftPtr;

    printf("thread done\n");

    return 1;
}

int Music_decode(text_t* path, MusicData* data)
{
    thrd_t id;

    free(data->fftData);

    ThreadFuncData* threadData = malloc(sizeof(ThreadFuncData));
    threadData->filename = strdup(path);
    threadData->data = data;

    data->percentDone = 1;
    data->fftData = 0;

    if (thrd_create(&id, decodeFunc, threadData) != thrd_success)
    {
        printf("Unable to crate decode thread. Stalling main thread and running...\n");
    }

    return 1;
}


