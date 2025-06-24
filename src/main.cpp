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

#define EXIT_ON_ERROR_WITH_MSG(hr, msg)                                        \
  if (FAILED(hr)) {                                                            \
    printf("%s\n", msg);                                                       \
    goto cleanup;                                                              \
  }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

int main(int argc, char *argv[]) {
  HRESULT hr = 0;
  IMMDeviceEnumerator *pEnumerator = NULL;
  IMMDevice *pDevice = NULL;
  LPWSTR pstrId = NULL;
  IAudioClient *pAudioClient = NULL;
  WAVEFORMATEX *pFormat = NULL;
  REFERENCE_TIME hnsDefaultDevicePeriod = 0;
  IAudioRenderClient *pRenderClient = NULL;
  UINT32 numBufferFrames = 0;
  UINT32 numPaddingFrames = 0;
  UINT32 numAvailableFrames = 0;
  BYTE *pData = NULL;

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

  printf("Creating IAudioClient...\n");
  hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL,
                         (void **)&pAudioClient);
  EXIT_ON_ERROR(hr);

  printf("GetMixFormat...\n");
  hr = pAudioClient->GetMixFormat(&pFormat);
  EXIT_ON_ERROR(hr);

  printf("GetDevicePeriod...\n");
  hr = pAudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, NULL);
  EXIT_ON_ERROR(hr);

  printf("Initializing audio client...\n");
  hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0,
                                hnsDefaultDevicePeriod, 0, pFormat, NULL);
  EXIT_ON_ERROR(hr);

  printf("Getting buffer size...\n");
  hr = pAudioClient->GetBufferSize(&numBufferFrames);
  EXIT_ON_ERROR(hr);

  printf("Getting render client...\n");
  hr =
      pAudioClient->GetService(IID_IAudioRenderClient, (void **)&pRenderClient);
  EXIT_ON_ERROR(hr);

  printf("Starting audio client...\n");
  hr = pAudioClient->Start();
  EXIT_ON_ERROR(hr);

  while (true) {
    numPaddingFrames = 0;

    hr = pAudioClient->GetCurrentPadding(&numPaddingFrames);
    EXIT_ON_ERROR_WITH_MSG(hr, "Failed to get current padding");

    numAvailableFrames = numBufferFrames - numPaddingFrames;

    if (numAvailableFrames == 0) {
      Sleep(5);
      continue;
    }

    hr = pRenderClient->GetBuffer(numAvailableFrames, &pData);
    EXIT_ON_ERROR_WITH_MSG(hr, "Failed to get buffer");

    for (int c = 0; c < pFormat->nChannels; ++c) {
      // Fill the buffer with silence or audio data
      memset(pData + c * sizeof(float), 0, numAvailableFrames * sizeof(float));
    }

    hr = pRenderClient->ReleaseBuffer(numAvailableFrames, 0);
    EXIT_ON_ERROR_WITH_MSG(hr, "Failed to release buffer");
  }

cleanup:
  CoTaskMemFree(pFormat);
  SAVE_RELEASE(pEnumerator);
  SAVE_RELEASE(pDevice);
  SAVE_RELEASE(pAudioClient);
  SAVE_RELEASE(pRenderClient);
  CoUninitialize();

  return hr;
}
