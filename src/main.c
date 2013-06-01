#include <stdio.h>
#include <stdlib.h>
#include "../include/portaudio.h"
#include "../include/sndfile.h"

#define SAMPLE_RATE         (16000)
#define FRAMES_PER_BUFFER   (16)
#define NUM_SECONDS         (5)
#define WRITE_TO_FILE       (0)

#define PA_SAMPLE_TYPE      paInt16
#define SAMPLE_SILENCE      (0)
#define PRINTF_S_FORMAT     "%d"

typedef struct
{
    int         frameIndex;
    int         maxFrameIndex;
    short      *recordedSamples;
}
paData;

void herr(PaError);

static int recordCallback(
                    const void *inputBuffer,
                    void *outputBuffer,
                    unsigned long framesPerBuffer,
                    const PaStreamCallbackTimeInfo* timeInfo,
                    PaStreamCallbackFlags statusFlags,
                    void *userData)
{
    (void) outputBuffer;
    (void) timeInfo;
    (void) statusFlags;
    (void) userData;

    paData *data = (paData*)userData;

    const short *rptr = (const short*)inputBuffer;
    short *wptr = &data->recordedSamples[data->frameIndex];

    unsigned long framesLeft = data->maxFrameIndex - data->frameIndex;
    long framesToCalc = framesLeft < framesPerBuffer ? framesLeft : framesPerBuffer;

    if(inputBuffer == NULL)
    {
        for( long i = 0; i < framesToCalc; i++ )
        {
            *wptr++ = SAMPLE_SILENCE;
        }
    }
    else
    {
        for( long i = 0; i < framesToCalc; i++ )
        {
            *wptr++ = *rptr++;
        }
    }

    data->frameIndex += framesToCalc;

    return framesLeft < framesPerBuffer ? paComplete : paContinue;
}

int main(void)
{
    /*------ INITIALIZE INPUT ------*/

    PaStreamParameters  inP;
    PaStream*           str;
    paData              data;

    data = (paData) 
    {
        .maxFrameIndex      =   NUM_SECONDS * SAMPLE_RATE,
        .frameIndex         =   0,
        .recordedSamples    =   (short *)calloc(NUM_SECONDS * SAMPLE_RATE, sizeof(short)),
    };

    if(data.recordedSamples == NULL)
    {
        printf("Could not allocate record array.\n");
        exit(127);
    }

    atexit((void(*)())Pa_Terminate);
    herr(Pa_Initialize());

    PaDeviceIndex dev = Pa_GetDefaultInputDevice();

    if(dev == paNoDevice)
    {
        fprintf(stderr,"Error: No default input device.\n");
        exit(1);
    }

    inP = (PaStreamParameters) 
    {
        .channelCount                =   1,
        .sampleFormat                =   PA_SAMPLE_TYPE,
        .suggestedLatency            =   Pa_GetDeviceInfo(dev)->defaultLowInputLatency,
        .hostApiSpecificStreamInfo   =   NULL,
    };

    /*------ INITIALIZE OUTPUT ------*/

    SNDFILE *outfile;
    SF_INFO sfinfo;

    sfinfo.samplerate   =   SAMPLE_RATE;
    sfinfo.channels     =   1;
    sfinfo.format       =   SF_FORMAT_FLAC | SF_FORMAT_PCM_16;

    if(!(outfile = sf_open("output.flac", SFM_WRITE, &sfinfo)))
    {
        printf("Not able to open output file %s.\n", "output.flac");
        sf_perror(NULL);
        exit(127);
    }

    /*------ RECORD ------*/

    herr(Pa_OpenStream(
          &str,
          &inP,
          NULL,
          SAMPLE_RATE,
          FRAMES_PER_BUFFER,
          paClipOff,
          recordCallback,
          &data));

    herr(Pa_StartStream(str));

    printf("\n=== Now recording!! Please speak into the microphone. ===\n");
    fflush(stdout);

    PaError e;
    while((e = Pa_IsStreamActive(str)) == 1)
    {
        Pa_Sleep(1000);
        printf("Index = %d\n", data.frameIndex);
        fflush(stdout);
    }
    herr(e);

    sf_write_short(outfile, data.recordedSamples, data.maxFrameIndex);

    herr(Pa_CloseStream(str));

    sf_close(outfile);

    free(data.recordedSamples);

    return 0;
}


inline void herr(PaError e) 
{
    if(e != paNoError)
    {
        fprintf( stderr, "An error occured while using the portaudio stream\n" );
        fprintf( stderr, "Error number: %d\n", e );
        fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( e ) );
        exit(e);
    }
}
