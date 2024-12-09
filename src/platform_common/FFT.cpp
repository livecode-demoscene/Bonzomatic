#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <kiss_fft.h>
#include <kiss_fftr.h>
#include <stdio.h>
#include <memory.h>
#include "FFT.h"
# define M_PI           3.14159265358979323846  /* pi */
namespace FFT
{
//////////////////////////////////////////////////////////////////////////

kiss_fftr_cfg fftcfg;
ma_context context;
ma_device captureDevice;
float sampleBuf[FFT_INPUT_LENGTH ];
float sampleBufWin[FFT_INPUT_LENGTH];
float fAmplification = 1.0f;

bool bPeakNormalization = true;
bool bPreProcessing = false;
float fPeakSmoothValue = 0.0f;
float fPeakMinValue = 0.01f;
float fPeakSmoothing = 0.995f;

bool bCreated = false;

void OnLog( ma_context * pContext, ma_device * pDevice, ma_uint32 logLevel, const char * message )
{
  printf( "[FFT] [mal:%p:%p]\n %s", pContext, pDevice, message );
}

void OnReceiveFrames( ma_device * pDevice, void * pOutput, const void * pInput, ma_uint32 frameCount )
{  
  frameCount = frameCount < FFT_INPUT_LENGTH ? frameCount : FFT_INPUT_LENGTH;

  // Just rotate the buffer; copy existing, append new
  const float * samples = (const float *) pInput;
  float * p = sampleBuf;
  for ( int i = 0; i < FFT_INPUT_LENGTH - frameCount; i++ )
  {
    *( p++ ) = sampleBuf[ i + frameCount ];
  }
  for ( int i = 0; i < frameCount; i++ )
  {
    *( p++ ) = ( samples[ i * 2 ] + samples[ i * 2 + 1 ] ) / 2.0f * fAmplification;
  }
}

void EnumerateDevices( FFT_ENUMERATE_FUNC pEnumerationFunction, void * pUserContext )
{
  if ( !bCreated )
  {
    return;
  }

  ma_device_info * pPlaybackDevices = NULL;
  ma_device_info * pCaptureDevices = NULL;
  ma_uint32 nPlaybackDeviceCount = 0;
  ma_uint32 nCaptureDeviceCount = 0;
  ma_result result = ma_context_get_devices( &context, &pPlaybackDevices, &nPlaybackDeviceCount, &pCaptureDevices, &nCaptureDeviceCount );
  if ( result != MA_SUCCESS ) 
  {
    printf( "[FFT] Failed to enumerate audio devices: %d\n", result );
    return;
  }

  pEnumerationFunction( true, "<default device>", NULL, pUserContext );
  for ( ma_uint32 i = 0; i < nCaptureDeviceCount; i++ )
  {
    pEnumerationFunction( true, pCaptureDevices[ i ].name, &pCaptureDevices[ i ].id, pUserContext );
  }
  if ( ma_is_loopback_supported( context.backend ) )
  {
    pEnumerationFunction( false, "<default device>", NULL, pUserContext );
    for ( ma_uint32 i = 0; i < nPlaybackDeviceCount; i++ )
    {
      pEnumerationFunction( false, pPlaybackDevices[ i ].name, &pPlaybackDevices[ i ].id, pUserContext );
    }
  }
}

bool Create()
{
  bCreated = false;
  ma_context_config context_config = ma_context_config_init();
  context_config.logCallback = OnLog;
  ma_result result = ma_context_init( NULL, 0, &context_config, &context );
  if ( result != MA_SUCCESS )
  {
    printf( "[FFT] Failed to initialize context: %d", result );
    return false;
  }

  printf( "[FFT] MAL context initialized, backend is '%s'\n", ma_get_backend_name( context.backend ) );
  bCreated = true;
  return true;
}

bool Destroy()
{
  if ( !bCreated )
  {
    return false;
  }

  ma_context_uninit( &context );

  bCreated = false;

  return true;
}

bool Open( FFT::Settings * pSettings )
{
  if ( !bCreated )
  {
    return false;
  }

  memset( sampleBuf, 0, sizeof( float ) * FFT_INPUT_LENGTH);

  fftcfg = kiss_fftr_alloc(FFT_INPUT_LENGTH, false, NULL, NULL );

  bool useLoopback = ma_is_loopback_supported( context.backend ) && !pSettings->bUseRecordingDevice;
  ma_device_config config = ma_device_config_init( useLoopback ? ma_device_type_loopback : ma_device_type_capture );
  config.capture.pDeviceID = (ma_device_id *) pSettings->pDeviceID;
  config.capture.format = ma_format_f32;
  config.capture.channels = 2;
  config.sampleRate = 44100;
  config.dataCallback = OnReceiveFrames;
  config.pUserData = NULL;

  ma_result result = ma_device_init( &context, &config, &captureDevice );
  if ( result != MA_SUCCESS )
  {
    printf( "[FFT] Failed to initialize capture device: %d\n", result );
    return false;
  }

  printf( "[FFT] Selected capture device: %s\n", captureDevice.capture.name );

  result = ma_device_start( &captureDevice );
  if ( result != MA_SUCCESS )
  {
    ma_device_uninit( &captureDevice );
    printf( "[FFT] Failed to start capture device: %d\n", result );
    return false;
  }

  return true;
}
float aweight(float hz) {
  if(hz<=10) return - 70.4;
  if (hz <= 12.5)return -63.4;
  if (hz <= 16)return -56.7;
  if (hz <= 20)return -50.5;
  if (hz <= 25)return -44.7;
  if (hz <= 31.5)return -39.4;
  if (hz <= 40)return -34.6;
  if (hz <= 50)return -30.2;
  if (hz <= 63)return -26.2;
  if (hz <= 80)return -22.5;
  if (hz <= 100)return -19.1;
  if (hz <= 125)return -16.1;
  if (hz <= 160)return -13.4;
  if (hz <= 200)return -10.9;
  if (hz <= 250)return -8.6;
  if (hz <= 315)return -6.6;
  if (hz <= 400) return-4.8;
  if (hz <= 500)return -3.2;
  if (hz <= 630)return -1.9;
  if (hz <= 800)return -0.8;
  if (hz <= 1000)return	0;
  if (hz <= 1250)return	0.6;
  if (hz <= 1600)return	1;
  if (hz <= 2000)return	1.2;
  if (hz <= 2500)return	1.3;
  if (hz <= 3150)return	1.2;
  if (hz <= 4000)return	1;
  if (hz <= 5000)	return 0.5;
  if (hz <= 6300)return	0.1;
  if (hz <= 8000)return -1.1;
  if (hz <= 10000)return -2.5;
  if (hz <= 12500)return -4.3;
  if (hz <= 16000)return -6.6;
  if (hz <= 20000)return -9.3;
    return -9.3;
}
bool GetFFT( float * _samples )
{
  memset(sampleBufWin, 0, sizeof(float) * FFT_INPUT_LENGTH);
  if ( !bCreated )
  {
    return false;
  }
  
  for (size_t i = 0; i < FFT_INPUT_LENGTH; i++) {
    float t = (float)i / (FFT_INPUT_LENGTH - 1);
    float hann = 0.5 - 0.5 * cosf(2 * M_PI * t);
    sampleBufWin[i] = sampleBuf[i] *20.f * hann;// powf(sinf(M_PI * i / FFT_SIZE), 2.f);

  }
  kiss_fft_cpx out[ FFT_BIN_SIZE + 1 ];
  kiss_fftr( fftcfg, sampleBufWin, out );
  if (!bPreProcessing && bPeakNormalization) { // Nusan's peakNormalization

    float peakValue = fPeakMinValue;
    for (int i = 0; i < FFT_BIN_SIZE; i++)
    {
      float val = 2.0f * sqrtf(out[i].r * out[i].r + out[i].i * out[i].i);
      if (val > peakValue) peakValue = val;
      _samples[i] = val * fAmplification;
    }
    if (peakValue > fPeakSmoothValue) {
      fPeakSmoothValue = peakValue;
    }
    if (peakValue < fPeakSmoothValue) {
      fPeakSmoothValue = fPeakSmoothValue * fPeakSmoothing + peakValue * (1 - fPeakSmoothing);
    }
    fAmplification = 1.0f / fPeakSmoothValue;
  }
  else if(bPreProcessing) // Totetmatt and Cacaooo Pre Processing
  {
    float fftResolution = 44100.0f / ((float)FFT_INPUT_LENGTH);
    float sumAmp = 0.f;
    for (int i = 0; i < FFT_BIN_SIZE; i++)
    {
      sumAmp += _samples[i];

    }
    float maxAmp = 0.1f;
    for (int i = 0; i < FFT_BIN_SIZE; i++)
    {
      static const float scaling = 1.0f / (float)FFT_BIN_SIZE;
      
      float amp = 2.0 * sqrtf(out[i].r * out[i].r + out[i].i * out[i].i);

      if (amp > 0.f) {
         amp = 20.0f*logf(amp);
        float currentFreq = fftResolution * (float)i;
        amp += aweight(currentFreq);
      
      }
      //printf("[%zu] - %f\n",i, amp);
      _samples[i] = (amp < 0.0f ? 0.0f:amp) * scaling;
      maxAmp = _samples[i] > maxAmp ? _samples[i] : maxAmp;

    }
    for (int i = 0; i < FFT_BIN_SIZE; i++)
    {
      _samples[i] /= maxAmp;

    }
  }
  else  // Original behaviour 
  { 
    for (int i = 0; i < FFT_BIN_SIZE; i++)
    {
      static const float scaling = 1.0f / (float)FFT_BIN_SIZE;
      _samples[i] = 2.0 * sqrtf(out[i].r * out[i].r + out[i].i * out[i].i) * scaling;
    }
  }

  return true;
}
void Close()
{
  if ( !bCreated )
  {
    return;
  }

  ma_device_stop( &captureDevice );

  ma_device_uninit( &captureDevice );

  kiss_fft_free( fftcfg );
}

//////////////////////////////////////////////////////////////////////////
}