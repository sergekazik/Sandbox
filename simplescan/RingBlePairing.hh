#ifndef _RING_BLE_PAIRING_H_
#define _RING_BLE_PAIRING_H_

#include <stdio.h>
#include <stdlib.h>

#include "RingBleApi.hh"
#include "RingGattApi.hh"

namespace Ring { namespace Ble {

class BlePairing
{
public:
    static BlePairing* getInstance(); // singleton
    static const char* mPayloadReady;
    static const char* mWiFiConnected;
    static const char* mWiFiConnectFailed;
    static char mPublicPayload[ATT_MTU_MAX];
    static void getValByKeyfromJson(const char* json_str, const char* key, char* val, int len);

private:
    BlePairing();
    static BlePairing* instance;
    BleApi *mBleApi;

public:
    int Initialize();
    int StartAdvertising();
    int StopAdvertising();
    int Shutdown();

private:
    unsigned int   mPairingServiceIndex;
    ServiceInfo_t *mServiceTable;
    unsigned int   mServiceCount;

    char mRingDeviceName[DEV_NAME_LEN];
    unsigned int mLocalClassOfDevice;
    unsigned int mAdvIntervalMin_ms;
    unsigned int mAdvIntervalMax_ms;
    unsigned int mAdvertisingTimeout_sec;

};

} } /* namespace Ring::Ble */


#endif // _RING_BLE_PAIRING_H_
