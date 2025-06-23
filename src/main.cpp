#include "AudioFile.h"
#include "stdio.h"
#include <AudioSessionTypes.h>
#include <audioclient.h>
#include <combaseapi.h>
#include <consoleapi.h>
#include <mmdeviceapi.h>
#include <objbase.h>
#include <processthreadsapi.h>
#include <propkey.h>
#include <synchapi.h>
#include <wingdi.h>
#include <winnt.h>
#include <winscard.h>
#include <wtypes.h>

#ifdef ENABLE_DEVICE_STATE_MONITOR
#include "DeviceStateMonitor.hpp"
#endif // ENABLE_DEVICE_STATE_MONITOR

#define EXIT_ON_ERROR(hr)                                                      \
  if (FAILED(hr)) {                                                            \
    printf("Error: 0x%lx\n", hr);                                              \
    goto cleanup;                                                              \
  }

#define SAVE_RELEASE(ptr)                                                      \
  if (ptr) {                                                                   \
    ptr->Release();                                                            \
    ptr = NULL;                                                                \
  }

#define RETURN_ON_ERROR(hr)                                                    \
  if (FAILED(hr))                                                              \
    return hr;

#define RETURN_ON_ERROR_WITH_MSG(hr, msg)                                      \
  if (FAILED(hr)) {                                                            \
    printf("%s\n", msg);                                                       \
    return hr;                                                                 \
  }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

HRESULT Playing(IAudioClient *pAudioClient, AudioFile<float> &audioFile) {
  HRESULT hr = 0;
  IAudioRenderClient *pRenderClient = NULL;
  const int numChannels = audioFile.getNumChannels();
  const int numFrames = audioFile.getNumSamplesPerChannel();
  int frameIndex = 0;
  UINT32 padding = 0;
	UINT32 numFramesAvailable = 0;
  UINT32 bufferFrameCount = 0;

  printf("Getting buffer size...\n");
  hr = pAudioClient->GetBufferSize(&bufferFrameCount);
  RETURN_ON_ERROR(hr);

  printf("Getting render client...\n");
  hr =
      pAudioClient->GetService(IID_IAudioRenderClient, (void **)&pRenderClient);
  RETURN_ON_ERROR(hr);

  while (frameIndex < numFrames) {
    padding = 0;
    hr = pAudioClient->GetCurrentPadding(&padding);
		RETURN_ON_ERROR_WITH_MSG(hr, "Failed to get current padding");

		if (numFramesAvailable == 0) {
			Sleep(5); // Wait for some frames to be available
			continue;
		}

  }

  return hr;
}

int main(int argc, char *argv[]) {
  HRESULT hr = 0;
  IMMDeviceEnumerator *pEnumerator = NULL;
  IMMDevice *pDevice = NULL;
  LPWSTR pstrId = NULL;
  IAudioClient *pAudioClient = NULL;
  WAVEFORMATEX *pFormat = NULL;
  REFERENCE_TIME hnsDefaultDevicePeriod = 0;
  char *filePath = NULL;
  AudioFile<float> audioFile;

  printf("CoInitialize...\n");
  hr = CoInitialize(NULL);
  EXIT_ON_ERROR(hr);

  printf("CoCreateInstance for IMMDeviceEnumerator...\n");
  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                        IID_IMMDeviceEnumerator, (void **)(&pEnumerator));
  EXIT_ON_ERROR(hr);

  printf("GetDefaultAudioEndpoint...\n");
  hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
  EXIT_ON_ERROR(hr);

#ifdef ENABLE_DEVICE_STATE_MONITOR
  printf("Activating device state monitor...\n");
  hr = DeviceStateMonitor(pDevice);
  EXIT_ON_ERROR(hr);
#endif // ENABLE_DEVICE_STATE_MONITOR

  if (argc < 2) {
    printf("Usage: %s <audio file path>\n", argv[0]);
    goto cleanup;
  }

  printf("Creating IAudioClient...\n");
  hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL,
                         (void **)&pAudioClient);
  EXIT_ON_ERROR(hr);

  printf("GetMixFormat...\n");
  hr = pAudioClient->GetMixFormat(&pFormat);
  EXIT_ON_ERROR(hr);

  printf("|======================================|\n");
  printf("Num Channels: %d\n", pFormat->nChannels);
  printf("Sample Rate: %ld\n", pFormat->nSamplesPerSec);
  printf("|======================================|\n");

  printf("GetDevicePeriod...\n");
  hr = pAudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, NULL);
  EXIT_ON_ERROR(hr);

  printf("Initializing audio client...\n");
  hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0,
                                hnsDefaultDevicePeriod, 0, pFormat, NULL);
  EXIT_ON_ERROR(hr);

  filePath = argv[1];
  if (!audioFile.load(filePath)) {
    printf("Failed to load audio file: %s\n", filePath);
    goto cleanup;
  }
  audioFile.printSummary();

  if (pFormat->nSamplesPerSec != audioFile.getSampleRate())
    audioFile.setSampleRate(pFormat->nSamplesPerSec);

  printf("Starting audio client...\n");
  hr = pAudioClient->Start();
  EXIT_ON_ERROR(hr);

  printf("Playing...\n");
  hr = Playing(pAudioClient, audioFile);
  EXIT_ON_ERROR(hr);

cleanup:
  pAudioClient->Stop();
  if (pFormat)
    CoTaskMemFree(pFormat);
  SAVE_RELEASE(pAudioClient);
  SAVE_RELEASE(pDevice);
  SAVE_RELEASE(pEnumerator);

  CoUninitialize();

  return hr;
}