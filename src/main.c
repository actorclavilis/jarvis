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
    
    PaStreamParameters  inputParameters;
    PaStream*           stream;
    paData              data;
    PaError             err = paNoError;

    data.maxFrameIndex      =   NUM_SECONDS * SAMPLE_RATE;
    data.frameIndex         =   0;
    data.recordedSamples    =   (short *) malloc( data.maxFrameIndex * sizeof(double) );

    if(data.recordedSamples == NULL)
    {
        printf("Could not allocate record array.\n");
        goto done;
    }

    for(int i = 0; i < data.maxFrameIndex; i++)
    {
        data.recordedSamples[i] = 0; //THIS LOOP IS STUPID, FIX LATER
    }

    err = Pa_Initialize();

    if(err != paNoError) 
    {
        goto done;
    }

    inputParameters.device = Pa_GetDefaultInputDevice();

    if(inputParameters.device == paNoDevice) 
    {
        fprintf(stderr,"Error: No default input device.\n");
        goto done;
    }
    
    inputParameters.channelCount                =   1;           
    inputParameters.sampleFormat                =   PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency            =   Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo   =   NULL;

    /*------ INITIALIZE OUTPUT ------*/

    SNDFILE *outfile;
    SF_INFO sfinfo;

    sfinfo.samplerate   =   SAMPLE_RATE;
    sfinfo.channels     =   1;
    sfinfo.format       =   SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    if(!(outfile = sf_open("output.wav", SFM_WRITE, &sfinfo)))
    {
        printf("Not able to open output file %s.\n", "output.wav");
        sf_perror(NULL);
        goto done;
    }
    
    /*------ RECORD ------*/

    err = Pa_OpenStream(
          &stream,
          &inputParameters,
          NULL,                 
          SAMPLE_RATE,
          FRAMES_PER_BUFFER,
          paClipOff,    
          recordCallback,
          &data);

    if(err != paNoError) 
    {
        goto done;
    }

    err = Pa_StartStream(stream);
    
    if(err != paNoError)
    {
        goto done;
    }

    printf("\n=== Now recording!! Please speak into the microphone. ===\n"); 
    fflush(stdout);
    
    while((err = Pa_IsStreamActive(stream)) == 1)
    {
        Pa_Sleep(1000);
        printf("Index = %d\n", data.frameIndex);
        fflush(stdout);
    }

    sf_write_short(outfile, data.recordedSamples, data.maxFrameIndex); 

    if(err < 0) 
    {
        goto done;
    }

    err = Pa_CloseStream(stream);
    
    if(err != paNoError) 
    {
        goto done;
    }

    sf_close(outfile);

done:    
    Pa_Terminate();
    
    if(data.recordedSamples)
    {
        free(data.recordedSamples);
    }
    
    if(err != paNoError)
    {
        fprintf( stderr, "An error occured while using the portaudio stream\n" );
        fprintf( stderr, "Error number: %d\n", err );
        fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
        err = 1;      
    }
    
    return err;
}

