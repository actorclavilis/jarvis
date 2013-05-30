#include <stdio.h>
#include <stdlib.h>
#include "portaudio.h"

#define SAMPLE_RATE         (16000)
#define FRAMES_PER_BUFFER   (16)
#define NUM_SECONDS         (5)
#define WRITE_TO_FILE       (0)

#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"

typedef struct
{
    int         frameIndex;
    int         maxFrameIndex;
    SAMPLE      *recordedSamples;
}
paTestData;

static int recordCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    (void) outputBuffer;
    (void) timeInfo;
    (void) statusFlags;
    (void) userData;
    
    paTestData *data = (paTestData*)userData;

    const SAMPLE *rptr = (const SAMPLE*)inputBuffer;
    SAMPLE *wptr = &data->recordedSamples[data->frameIndex];

    unsigned long framesLeft = data->maxFrameIndex - data->frameIndex;
    long framesToCalc = framesLeft < framesPerBuffer ? framesLeft : framesPerBuffer; 

    if( inputBuffer == NULL )
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

static int playCallback( const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData )
{
    (void) inputBuffer;
    (void) timeInfo;
    (void) statusFlags;
    (void) userData;

    paTestData *data = (paTestData*)userData;

    SAMPLE *rptr = &data->recordedSamples[data->frameIndex];
    SAMPLE *wptr = (SAMPLE*)outputBuffer;
    
    unsigned int i;
    unsigned int framesLeft = data->maxFrameIndex - data->frameIndex;

    if( framesLeft < framesPerBuffer )
    {
        for( i = 0; i < framesLeft; i++ )
        {
            *wptr++ = *rptr++; 
        }
        for( ; i < framesPerBuffer; i++ )
        {
            *wptr++ = 0;
        }
        data->frameIndex += framesLeft;
    }
    else
    {
        for( i = 0; i < framesPerBuffer; i++ )
        {
            *wptr++ = *rptr++;
        }
        data->frameIndex += framesPerBuffer;
    }

    return framesLeft < framesPerBuffer ? paComplete : paContinue;
}

int main(void)
{
    PaStreamParameters  inputParameters,
                        outputParameters;
    PaStream*           stream;
    PaError             err = paNoError;
    paTestData          data;
    int                 numSamples;
    int                 numBytes;
    SAMPLE              max, val;
    double              average;

    data.maxFrameIndex = NUM_SECONDS * SAMPLE_RATE;
    data.frameIndex = 0;
    numSamples = data.maxFrameIndex;
    numBytes = numSamples * sizeof(SAMPLE);
    data.recordedSamples = (SAMPLE *) malloc( numBytes );

    if( data.recordedSamples == NULL )
    {
        printf("Could not allocate record array.\n");
        goto done;
    }

    for( int i = 0; i < numSamples; i++ )
    {
        data.recordedSamples[i] = 0;
    }

    err = Pa_Initialize();

    if( err != paNoError ) 
    {
        goto done;
    }

    inputParameters.device = Pa_GetDefaultInputDevice();

    if (inputParameters.device == paNoDevice) 
    {
        fprintf(stderr,"Error: No default input device.\n");
        goto done;
    }

    inputParameters.channelCount = 1;           
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    /* Record some audio. -------------------------------------------- */
    err = Pa_OpenStream(
              &stream,
              &inputParameters,
              NULL,                 
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,    
              recordCallback,
              &data );

    if( err != paNoError ) 
    {
        goto done;
    }

    err = Pa_StartStream( stream );
    
    if( err != paNoError )
    {
        goto done;
    }

    printf("\n=== Now recording!! Please speak into the microphone. ===\n"); 
    fflush(stdout);

    while( ( err = Pa_IsStreamActive( stream ) ) == 1 )
    {
        Pa_Sleep(1000);
        printf("index = %d\n", data.frameIndex ); 
        fflush(stdout);
    }

    if( err < 0 ) 
    {
        goto done;
    }

    err = Pa_CloseStream( stream );
    
    if( err != paNoError ) 
    {
        goto done;
    }

    /* Write recorded data to a file. */
#if WRITE_TO_FILE
    {
        FILE  *fid;
        fid = fopen("recorded.raw", "wb");
        if( fid == NULL )
        {
            printf("Could not open file.");
        }
        else
        {
            fwrite( data.recordedSamples, sizeof(SAMPLE), data.maxFrameIndex, fid );
            fclose( fid );
            printf("Wrote data to 'recorded.raw'\n");
        }
    }
#endif

    /* Playback recorded data.  -------------------------------------------- */
    data.frameIndex = 0;

    outputParameters.device = Pa_GetDefaultOutputDevice();
    
    if (outputParameters.device == paNoDevice) 
    {
        fprintf(stderr,"Error: No default output device.\n");
        goto done;
    }

    outputParameters.channelCount = 1;                   
    outputParameters.sampleFormat =  PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    printf("\n=== Now playing back. ===\n"); 
    fflush(stdout);
    
    err = Pa_OpenStream(
              &stream,
              NULL,
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,
              playCallback,
              &data );

    if( err != paNoError ) 
    {
        goto done;
    }

    if( stream )
    {
        err = Pa_StartStream( stream );
        if( err != paNoError ) 
        {
            goto done;
        }
        
        printf("Waiting for playback to finish.\n"); 
        fflush(stdout);

        while( ( err = Pa_IsStreamActive( stream ) ) == 1 )
        {
            Pa_Sleep(100);
        }

        if( err < 0 )
        {
            goto done;
        }
        
        err = Pa_CloseStream( stream );
        
        if( err != paNoError )
        {
            goto done;
        }

        printf("Done.\n"); 
        fflush(stdout);
    }

done:
    Pa_Terminate();
    
    if( data.recordedSamples )
    {
        free( data.recordedSamples );
    }
    
    if( err != paNoError )
    {
        fprintf( stderr, "An error occured while using the portaudio stream\n" );
        fprintf( stderr, "Error number: %d\n", err );
        fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
        err = 1;      
    }
    
    return err;
}

