#include <emgui/Emgui.h>
#include <Bass.h>
#include "Dialog.h"
#include <math.h>
#include <stdio.h>
#include "TrackData.h"

/*
 public static string GenerateSoundSprectrumAndVolume(String soundFilePath)
        {
            if ( String.IsNullOrEmpty(soundFilePath) || !File.Exists(soundFilePath))
                return null;

            var imageFilePath = soundFilePath+".waveform.png";
            if (File.Exists(imageFilePath))
            {
                Logger.Info("Reusing sound image file: {0}", imageFilePath);
                return imageFilePath;
            }

            Logger.Info("Generating {0}...", imageFilePath);

            Bass.BASS_Init(-1, 44100, BASSInit.BASS_DEVICE_LATENCY, IntPtr.Zero);
            var stream = Bass.BASS_StreamCreateFile(soundFilePath, 0, 0,
                                                    BASSFlag.BASS_STREAM_DECODE | BASSFlag.BASS_STREAM_PRESCAN);

            const int SAMPLING_FREQUENCY = 100; //warning: in TimelineImage this samplingfrequency is assumed to be 100
            const double SAMPLING_RESOLUTION = 1.0 / SAMPLING_FREQUENCY;

            var sampleLength = Bass.BASS_ChannelSeconds2Bytes(stream, SAMPLING_RESOLUTION);
            var numSamples = Bass.BASS_ChannelGetLength(stream, BASSMode.BASS_POS_BYTES) / sampleLength;

            Bass.BASS_ChannelPlay(stream, false);

            const int spectrumLength = 1024;
            const int imageHeight = 256;
            var spectrumImage = new Bitmap((int)numSamples, imageHeight);
            var volumeImage = new Bitmap((int)numSamples, imageHeight);

            var tempBuffer = new float[spectrumLength];

            const float maxIntensity = 500;
            const int COLOR_STEPS = 255;
            const int PALETTE_SIZE = 3 * COLOR_STEPS;

            int a, b, r, g;
            var palette = new Color[PALETTE_SIZE];
            int palettePos;

            for (palettePos = 0; palettePos < PALETTE_SIZE; ++palettePos)
            {
                a = 255;
                if (palettePos < PALETTE_SIZE * 0.666f)
                    a = (int)(palettePos * 255 / (PALETTE_SIZE * 0.666f));

                b = 0;
                if (palettePos < PALETTE_SIZE * 0.333f)
                    b = palettePos;
                else if (palettePos < PALETTE_SIZE * 0.666f)
                    b = -palettePos + 510;

                r = 0;
                if (palettePos > PALETTE_SIZE * 0.666f)
                    r = 255;
                else if (palettePos > PALETTE_SIZE * 0.333f)
                    r = palettePos - 255;

                g = 0;
                if (palettePos > PALETTE_SIZE * 0.666f)
                    g = palettePos - 510;

                palette[palettePos] = Color.FromArgb(a, r, g, b);
            }

            var f = (float)(spectrumLength / Math.Log((float)(imageHeight + 1)));
            var f2 = (float)((PALETTE_SIZE - 1) / Math.Log(maxIntensity + 1));
            var f3 = (float)((imageHeight - 1) / Math.Log(32768.0f + 1));

            for (var sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
            {
                Bass.BASS_ChannelSetPosition(stream, sampleIndex * sampleLength, BASSMode.BASS_POS_BYTES);
                Bass.BASS_ChannelGetData(stream, tempBuffer, (int)Un4seen.Bass.BASSData.BASS_DATA_FFT2048);

                for (var rowIndex = 0; rowIndex < imageHeight; ++rowIndex)
                {
                    var j_ = (int)(f * Math.Log(rowIndex + 1));
                    var pj_ = (int)(rowIndex > 0 ? f * Math.Log(rowIndex - 1 + 1) : j_);
                    var nj_ = (int)(rowIndex < imageHeight - 1 ? f * Math.Log(rowIndex + 1 + 1) : j_);
                    float intensity = 125.0f * tempBuffer[spectrumLength - pj_ - 1] +
                                      750.0f * tempBuffer[spectrumLength - j_ - 1] +
                                      125.0f * tempBuffer[spectrumLength - nj_ - 1];
                    intensity = Math.Min(maxIntensity, intensity);
                    intensity = Math.Max(0.0f, intensity);

                    palettePos = (int)(f2 * Math.Log(intensity + 1));
                    spectrumImage.SetPixel(sampleIndex, rowIndex, palette[palettePos]);
                }

                if (sampleIndex%1000 == 0)
                {
                    Logger.Info("   computing sound image {0}% complete", (100*sampleIndex/numSamples));
                }
            }

            spectrumImage.Save(imageFilePath);
            Bass.BASS_ChannelStop(stream);
            Bass.BASS_StreamFree(stream);

            return imageFilePath;
        }
    }
}
*/

// This code has in many parts been ported to C from this code under the MIT licence.
// https://github.com/framefield/tooll/blob/master/Tooll/Components/Helper/GenerateSoundImage.cs
// Copyright (c) 2016 Framefield. All rights reserved.

void Music_decode(text_t* path, MusicData* data)
{
    int a, b, r, g, i;
    //short waveData[8 * 1024];
    float fftData[1024];
    QWORD len;
    //double timeInSec = 0.0;
    const float maxIntensity = 500 * 2;
    const int COLOR_STEPS = 255;
    const int PALETTE_SIZE = 3 * COLOR_STEPS;
    const int spectrumLength = 1024;
    unsigned int colors[255 * 3];
    int imageHeight = 128;

	HSTREAM chan = BASS_StreamCreateFile(0, path, 0, 0, BASS_STREAM_DECODE | BASS_SAMPLE_MONO | BASS_POS_SCAN);

	if (!chan)
	{
	    Dialog_showError("Unable to open %s for decode. No music data will be availible.");
	    return;
	}

    len = BASS_ChannelGetLength(chan, BASS_POS_BYTE);

    if (len == -1)
    {
	    Dialog_showError("Stream %s has no length. No music data will be availible.");
        BASS_StreamFree(chan);
        return;
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
    data->fftData = (unsigned int*)malloc(128 * 4 * (numSamples + 1));

    unsigned int* fftOutput = data->fftData;

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

        colors[i] = Emgui_color32(r, g, b, a);
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

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        BASS_ChannelSetPosition(chan, sampleIndex * sampleLength, BASS_POS_BYTE);
        BASS_ChannelGetData(chan, fftData, BASS_DATA_FFT2048);

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

        /*
        if (sampleIndex%1000 == 0)
        {
            Logger.Info("   computing sound image {0}% complete", (100*sampleIndex/numSamples));
        }
        */
    }

    /*

    while (BASS_ChannelIsActive(chan))
    {
		int pos = BASS_ChannelGetPosition(chan, BASS_POS_BYTE);
		int c = BASS_ChannelGetData(chan, fftData, BASS_DATA_FFT1024);

        //printf("pos %d (len %d)\n", pos, c);

        //BASS_ChannelSetPosition(chan, pos, BASS_POS_BYTE);

	    //c = BASS_ChannelGetData(chan, waveData, c / 4); // 4 as size is from floats

        printf("pos %d (len %d)\n", pos, c);
    }
    */

    BASS_StreamFree(chan);

    //Editor_updateTrackScroll();
}

