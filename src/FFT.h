#define FFT_BIN_SIZE 1024
#define FFT_INPUT_LENGTH (FFT_BIN_SIZE*2)
namespace FFT
{
//////////////////////////////////////////////////////////////////////////

struct Settings
{
  bool bUseRecordingDevice;
  void * pDeviceID;
};

typedef void ( *FFT_ENUMERATE_FUNC )( const bool bIsCaptureDevice, const char * szDeviceName, void * pDeviceID, void * pUserContext );

extern float fAmplification;
extern bool bPeakNormalization;
extern float fPeakMinValue;
extern float fPeakSmoothing;
extern bool bPreProcessing;
void EnumerateDevices( FFT_ENUMERATE_FUNC pEnumerationFunction, void * pUserContext );

bool Create();
bool Destroy();
bool Open( Settings * pSettings );
bool GetFFT( float * _samples );
void Close();

//////////////////////////////////////////////////////////////////////////
}