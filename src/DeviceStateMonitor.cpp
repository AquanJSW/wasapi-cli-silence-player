#include <cstdio>
#include <mmdeviceapi.h>
#include <windows.h>
#include <minwindef.h>

constexpr DWORD MS_MONITOR_INTERVAL = 1000; // 1 second

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType) {
  if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
    printf("Exiting device state monitor...\n");
    ExitProcess(0);
  }
  return TRUE;
}

HRESULT DeviceStateMonitor(IMMDevice *pDevice) {
  HRESULT hr;
  // IPropertyStore *pProps = NULL;
  // PROPVARIANT varName;
  LPWSTR pstrId;
  DWORD dwState;
  DWORD lastState = 0;

  // printf("Opening property store...\n");
  // hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
  // if (FAILED(hr))
  //   return hr;

  // PropVariantInit(&varName);

  // printf("Getting device friendly name...\n");
  // // Get the device friendly name
  // hr = pProps->GetValue(PKEY_Devices_FriendlyName, &varName);
  // if (FAILED(hr))
  //   return hr;

  // if (varName.vt != VT_EMPTY)
  //   printf("Device Friendly Name: %ls\n", varName.pwszVal);

  printf("Getting device ID...\n");
  hr = pDevice->GetId(&pstrId);
  if (FAILED(hr))
    return hr;
  printf("Device ID: %ls\n", pstrId);

  printf("Monitoring device state...\n");
  SetConsoleCtrlHandler(HandlerRoutine, TRUE);
  while (true) {
    hr = pDevice->GetState(&dwState);
    if (FAILED(hr))
      return hr;
    if (dwState != lastState) {
      switch (dwState) {
      case DEVICE_STATE_ACTIVE:
        printf("Device is ACTIVE\n");
        break;
      case DEVICE_STATE_DISABLED:
        printf("Device is DISABLED\n");
        break;
      case DEVICE_STATE_NOTPRESENT:
        printf("Device is NOT PRESENT\n");
        break;
      case DEVICE_STATE_UNPLUGGED:
        printf("Device is UNPLUGGED\n");
        break;
      default:
        printf("Device state changed: %lu\n", dwState);
      }
      lastState = dwState;
    }
    Sleep(MS_MONITOR_INTERVAL);
  }
	// PropVariantClear(&varName);
	// SAVE_RELEASE(pProps);
  return S_OK;
}