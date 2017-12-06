/*---------------------------------------------------------------------
 *               ------------------      -----------------            *
 *               | Ring App Setup |      |   test_ble    |            *
 *               ------------------      -----------------            *
 *                          |                |                        *
 *                          |   --------------------                  *
 *                          |   |gatt_src_test.cpp |                  *
 *                          |   --------------------                  *
 *                          |     |          |                        *
 *              ----------------------       |                        *
 *              | RingBlePairing.cpp |       |                        *
 *              ----------------------       |                        *
 *                          |                |                        *
 *                  ---------------------------------                 *
 *                  |    RinmBleApi.cpp abstract    |                 *
 *                  ---------------------------------                 *
 *                  |  RingGattSrv  |  RingGattSrv  |                 *
 *                  |   WILINK18    |    BCM43      |                 *
 *                  --------------------------------                  *
 *                        |                  |                        *
 *       ------------------------       ---------------               *
 *       |      TIBT lib        |       |   BlueZ     |               *
 *       ------------------------       ---------------               *
 *                  |                        |                        *
 *       ------------------------       ----------------              *
 *       | TI WiLink18xx BlueTP |       | libbluetooth |              *
 *       ------------------------       ----------------              *
 *--------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "Bot_Notifier.h"
#include "RingBleApi.hh"
#include "RingGattApi.hh"
#include "RingBlePairing.hh"

#include "gatt_svc_defs.h"

using namespace Ring;
using namespace Ring::Ble;

/* -------------------------------------------------------------*
 * section of auto-declaration
 * the define file may be included multiple times to generate
 * different stuctures correlated by order of definition
 * -------------------------------------------------------------*/
#define RING_PAIRING_TABLE_SERVICE_DECL
#include "gatt_svc_defs.h"

#define RING_PAIRING_TABLE_SERVICE_DEFINE_TABLE
static AttributeInfo_t SrvTable0[] = {
    #include "gatt_svc_defs.h"
};

#define RING_PAIRING_TABLE_ATTR_ENUM
enum GattAttributeIndexByName {
    #include "gatt_svc_defs.h"
};

#define RING_PAIRING_SERVICE_INFO_DEFINE
#include "gatt_svc_defs.h"

/* -------------------------------------------------------------*
 * static callback function declaration
 * -------------------------------------------------------------*/
static void OnAttributeAccessCallback(int aServiceIdx, int aAttributeIdx, Ble::Characteristic::Access aAccessType);

/* -------------------------------------------------------------*
 * BlePairing class implementation
 * -------------------------------------------------------------*/
BlePairing* BlePairing::instance = NULL;
const char* BlePairing::mPayloadReady = "PAYLOAD_READY";
const char* BlePairing::mWiFiConnected = "WIFI_CONNECTED";
const char* BlePairing::mWiFiConnectFailed = "WIFI_CONNECT_FAILED";
const char* BlePairing::mNetworkConfigFile = "/ring/network";
const char* BlePairing::mWiFiConfigFile = "/ring/wifi.ble.conf";  // TODO: right name is wifi.conf, for tests only - wifi.ble.conf

char BlePairing::mMacAddress[DEV_MAC_ADDR_LEN] = "XX:XX:XX:XX:XX:XX";
char BlePairing::mPublicPayload[ATT_MTU_MAX] = "placeholder for public payload generated by GATT server";

// singleton instance
BlePairing* BlePairing::getInstance() {
    if (instance == NULL)
        instance = new BlePairing();
    return instance;
}

// private constructor
BlePairing::BlePairing() :
    mPairingServiceIndex(0)
{
    if (NULL == (mBleApi = GattSrv::getInstance()))
    {
        BOT_NOTIFY_INFO("BlePairing failed to obtain BleApi instance");
    }

    // set other config parameters
    mLocalClassOfDevice = 0x280430;
    mAdvertisingTimeout_sec = 200;
    mAdvIntervalMin_ms = 2000;
    mAdvIntervalMax_ms = 3000;

    // set definitions from RingGattServices.hh
    mServiceTable = sServiceTable;
    mServiceCount = RING_GATT_SERVICES_COUNT;

    // read config from the system and populate characteristics
    strncpy(mRingDeviceName, "Ring-XXXX", DEV_NAME_LEN);
    FILE *fin = fopen(mNetworkConfigFile, "rt");
    if (fin)
    {
        char read_line[DEV_NAME_LEN];
        while(fgets(read_line, DEV_NAME_LEN, fin))
        {
            if (strstr(read_line, "<mac_addr>"))
            {
                int a,b,c,d,e,f;
                if (6 == sscanf(read_line, "<mac_addr>%02x:%02x:%02x:%02x:%02x:%02x</mac_addr>", &a,&b,&c,&d,&e,&f))
                {
                    snprintf(mRingDeviceName, DEV_NAME_LEN, "Ring-%02X%02X", e, f);

                    AttributeInfo_t *attribute_list = mServiceTable[RING_PAIRING_SVC_IDX].AttributeList;
                    sprintf(mMacAddress, "%02X:%02X:%02X:%02X:%02X:%02X", a,b,c,d,e,f);
                    ((CharacteristicInfo_t*) (attribute_list[GET_MAC_ADDRESS].Attribute))->Value = (Byte_t*) mMacAddress;
                    ((CharacteristicInfo_t*) (attribute_list[GET_MAC_ADDRESS].Attribute))->ValueLength = strlen(mMacAddress);
                }
                break;
            }
        }
        fclose(fin);
    }

}

///
/// \brief BlePairing::Initialize
/// \return Ble::Error
///
int BlePairing::Initialize(char *aDeviceName)
{
    int ret_val = Error::UNDEFINED;

    // BLE Device Configuration
    if (aDeviceName != NULL)
    {
        // override device name
        strncpy(mRingDeviceName, aDeviceName, DEV_NAME_LEN);
    }

    DeviceConfig_t config[] =
    { // config tag                         count   params
        {Ble::Config::ServiceTable,           {1,   {{(char*) mServiceTable, mServiceCount}}}},
        {Ble::Config::LocalDeviceName,        {1,   {{(char*) mRingDeviceName, Ble::ConfigArgument::None}}}},
        {Ble::Config::LocalClassOfDevice,     {1,   {{NULL, mLocalClassOfDevice}}}},
        {Ble::Config::Discoverable,           {1,   {{NULL, Ble::ConfigArgument::Enable}}}},
        {Ble::Config::Connectable,            {1,   {{NULL, Ble::ConfigArgument::Enable}}}},
        {Ble::Config::Pairable,               {1,   {{NULL, Ble::ConfigArgument::Enable}}}},
        {Ble::Config::LocalDeviceAppearance,  {1,   {{NULL, BleApi::BLE_APPEARANCE_GENERIC_COMPUTER}}}},
        {Ble::Config::AdvertisingInterval,    {2,   {{NULL, mAdvIntervalMin_ms}, {NULL, mAdvIntervalMax_ms}}}},
        {Ble::Config::RegisterGATTCallback,   {0,   {{NULL, Ble::ConfigArgument::None}}}},
        {Ble::Config::RegisterService,        {1,   {{NULL, mPairingServiceIndex}}}},
        {Ble::Config::RegisterAuthentication, {0,   {{NULL, Ble::ConfigArgument::None}}}},
        {Ble::Config::SetSimplePairing,       {2,   {{NULL, Ble::ConfigArgument::None}, {NULL, Ble::ConfigArgument::None}}}},
        {Ble::Config::EnableBluetoothDebug,   {2,   {{NULL, Ble::ConfigArgument::Enable}, {NULL, Ble::ConfigArgument::Terminal}}}},
        {Ble::Config::EOL,                    {0,   {{NULL, Ble::ConfigArgument::None}}}},
    };

    // try one more time
    if (mBleApi == NULL)
    {
        mBleApi = GattSrv::getInstance();
    }

    // if still no luck - return error
    if (mBleApi == NULL)
    {
        BOT_NOTIFY_ERROR("BlePairing failed to obtain BleApi instance");
        ret_val = Error::FAILED_INITIALIZE;
    }
    else
    {
        if (Ble::Error::NONE != (ret_val = mBleApi->Initialize()))
        {
            BOT_NOTIFY_ERROR("mBleApi->Initialize() failed.");
        }
        else if (Ble::Error::NONE != (ret_val = mBleApi->SetDevicePower(Ble::ConfigArgument::PowerOn)))
        {
            BOT_NOTIFY_ERROR("mBleApi->SetDevicePower(ON) failed.");
        }
        else if (Ble::Error::NONE != (ret_val = mBleApi->Configure(config)))
        {
            BOT_NOTIFY_ERROR("mBleApi->Configure failed, ret = %d, Abort.", ret_val);
        }
        else if (Ble::Error::NONE != (ret_val = mBleApi->RegisterCharacteristicAccessCallback(OnAttributeAccessCallback)))
        {
            BOT_NOTIFY_ERROR("mBleApi->RegisterCharacteristicAccessCallback failed, ret = %d, Abort.", ret_val);
        }
    }

    return ret_val;
}

///
/// \brief BlePairing::StartAdvertising
/// \return  Ble::Error
///
int BlePairing::StartAdvertising(int aTimeout)
{
    int ret_val = Error::UNDEFINED;

    if (mBleApi == NULL)
    {
        BOT_NOTIFY_WARNING("BlePairing failed to obtain BleApi instance");
        ret_val = Error::NOT_INITIALIZED;
    }
    else
    {
        if (aTimeout > 0)
        {
            // override default mAdvertisingTimeout_sec
            mAdvertisingTimeout_sec = aTimeout;
        }

        // start advertising
        unsigned int adv_flags = Advertising::Discoverable  | Advertising::Connectable      |
                                 Advertising::AdvertiseName | Advertising::AdvertiseTxPower | Advertising::AdvertiseAppearance;
        ParameterList_t params = {2, {{NULL, adv_flags}, {NULL, mAdvertisingTimeout_sec}}};

        if (Ble::Error::NONE != (ret_val = mBleApi->StartAdvertising(&params)))
        {
            BOT_NOTIFY_ERROR("mBleApi->StartAdvertising failed, ret = %d, Abort.", ret_val);
        }
    }
    return ret_val;
}

///
/// \brief BlePairing::StopAdvertising
/// \return
///
int BlePairing::StopAdvertising()
{
    int ret_val = Error::UNDEFINED;

    if (mBleApi == NULL)
    {
        BOT_NOTIFY_WARNING("BlePairing failed to obtain BleApi instance");
        ret_val = Error::NOT_INITIALIZED;
    }
    else
    {
        ParameterList_t params = {1, {{NULL, 0}}};
        if (Ble::Error::NONE != (ret_val = mBleApi->StopAdvertising(&params)))
        {
            // ret_val can be != Error::NONE also if adv was not started or already expired
            BOT_NOTIFY_WARNING("mBleApi->StopAdvertising ret = %d", ret_val);
        }
    }
    return ret_val;
}

///
/// \brief BlePairing::Shutdown
/// \return
///
int BlePairing::Shutdown()
{
    int ret_val = Error::UNDEFINED;

    if (mBleApi == NULL)
    {
        BOT_NOTIFY_WARNING("BlePairing failed to obtain BleApi instance");
        ret_val = Error::NOT_INITIALIZED;
    }
    else
    {   // deactivate GATT Server
        this->StopAdvertising();

        // TODO: do we want to power off the BLE device completely and shutdown?
        if (Ble::Error::NONE != (ret_val = mBleApi->SetDevicePower(Ble::ConfigArgument::PowerOff)))
        {
            BOT_NOTIFY_ERROR("mBleApi->SetDevicePower(OFF) failed.");
        }
        else if (Ble::Error::NONE != (ret_val = mBleApi->Shutdown()))
        {
            BOT_NOTIFY_ERROR("mBleApi->Shutdown failed.");
        }
    }
    return ret_val;
}

///
/// \brief BlePairing::getValByKeyfromJson
/// \param json
/// \param key
/// \param val
/// \note dummy json parser to be replaced with something actual
///
void BlePairing::getValByKeyfromJson(const char* json_str, const char* key, char* val, int len)
{
    if (json_str && key && val) {
        memset(val, 0, len);
        char *pStart = (char*) strstr(json_str, key);
        if (pStart) {
            pStart += strlen(key)+3; //3 for ":"
            char *pEnd = (char*) strchr(pStart, '"');
            if (pEnd && (pEnd > pStart)) {
                int ln = pEnd-pStart+1;
                if (ln > len) ln = len;
                strncpy(val, pStart, ln-1);
            }
        }
    }
}

///
/// \brief OnAttributeAccessCallback
/// \param aServiceIdx
/// \param aAttributeIdx
/// \param aAccessType
///
static void OnAttributeAccessCallback(int aServiceIdx, int aAttributeIdx, Ble::Characteristic::Access aAccessType)
{
    #define ATTRIBUTE_OFFSET(_idx) attribute_list[_idx].AttributeOffset
    #define SET_ATTRIBUTE_STR_VAL(_value) (Byte_t *) _value, strlen(_value)

    static const unsigned int service_id = sServiceTable[RING_PAIRING_SVC_IDX].ServiceID;
    static const AttributeInfo_t *attribute_list = sServiceTable[RING_PAIRING_SVC_IDX].AttributeList;

    printf("\npairing-sample_callback on Ble::Characteristic::%s for %s %s\n",
           aAccessType == Ble::Characteristic::Read ? "Read": aAccessType == Ble::Characteristic::Write ? "Write":"Confirmed",
           sServiceTable[aServiceIdx].ServiceName, sServiceTable[aServiceIdx].AttributeList[aAttributeIdx].AttributeName);

#if !defined(Linux_x86_64) || !defined(WILINK18)
    BleApi* bleApi = GattSrv::getInstance();
    if (bleApi == NULL)
    {
        BOT_NOTIFY_ERROR("OnAttributeAccessCallback failed to obtain BleApi instance");
        return;
    }
#endif

    Ble::GATT_UUID_t uuid;
    uuid.UUID_Type     = Ble::guUUID_128;
    uuid.UUID.UUID_128 = ((CharacteristicInfo_t*) (attribute_list[aAttributeIdx].Attribute))->CharacteristicUUID;
    bleApi->DisplayGATTUUID(&uuid, "Characteristic: ", 0);

    switch (aAccessType)
    {
    case Ble::Characteristic::Read:
    case Ble::Characteristic::Confirmed:
        bleApi->DisplayAttributeValue(aServiceIdx, aAttributeIdx);
        // other things todo...
        break;

    case Ble::Characteristic::Write:
            printf("Value:\n");
            bleApi->DisplayAttributeValue(aServiceIdx, aAttributeIdx);

            switch (aAttributeIdx)
            {
            case SET_PUBLIC_KEY:
                printf("\n\tOn geting PUBLIC_KEY the device generates Public/Private keys. A randomly generated nonce start\n" \
                       "\tset of 20 bytes is generated. The the device then takes the public key, and signs it using ed25519\n" \
                       "\tand a private key that is embedded in the application and creates a 64 byte signature of the ephemeral\n" \
                       "\tpublic key. The public key, signature, and nonce start are saved as the Public Payload - GET_PUBLIC_PAYLOAD.\n" \
                       "\tThe device notifies the payload ready with GET_PAIRING_STATE value PAYLOAD_READY\n\n");

                bleApi->GATTUpdateCharacteristic(service_id, ATTRIBUTE_OFFSET(GET_PUBLIC_PAYLOAD), SET_ATTRIBUTE_STR_VAL(BlePairing::mPublicPayload));
                bleApi->NotifyCharacteristic(RING_PAIRING_SVC_IDX, GET_PAIRING_STATE, BlePairing::mPayloadReady);

                break;

            case SET_WIFI_NETWORK:
            {
                printf("\n\tOn geting SET_PAIRING_START the device shoud login into WiFi network using SSID and PASSWORD\n" \
                       "\tfrom its properties supposed to be already set by client app\n"   \
                       "\tThe device updates the status of WIFI connectivity in the GET_WIFI_STATUS with CONNECTED or DISCONNECTED\n" \
                       "\tThe device notifies the setup result with the GET_PAIRING_STATE value WIFI_CONNECTED or WIFI_CONNECT_FAILED\n\n");

                char *net_config = (char*) ((CharacteristicInfo_t*) (attribute_list[SET_WIFI_NETWORK].Attribute))->Value;
                char ssid[((CharacteristicInfo_t*) (attribute_list[GET_SSID_WIFI].Attribute))->MaximumValueLength];
                char pass[((CharacteristicInfo_t*) (attribute_list[GET_SSID_WIFI].Attribute))->MaximumValueLength];

                BlePairing::getValByKeyfromJson((const char*) net_config, "ssid", ssid, sizeof(ssid));
                BlePairing::getValByKeyfromJson((const char*) net_config, "pass", pass, sizeof(pass));
                printf("Using parsed settings:\nssid:\t%s\npass:\t%s\n", ssid, pass);

                // use parsed SSID and PASS to create wifi.conf file same format as light-http script
                // "wifi:O${#object}:$object;S${#true_ssid}:$true_ssid;P${#true_password}:$true_password;K${#secret_key}:$secret_key;T${#security}:$security;E${#ethernet}:${ethernet};;"
                // TODO: object? security? - FIXME
                const char* object = "223192";
                const char* secret_key = (const char*) ((CharacteristicInfo_t*) (attribute_list[SET_SECRET_KEY].Attribute))->Value;
                const char* security = "wpa-personal";
                const char* ethernet = "0";

                FILE *fout = fopen(BlePairing::mWiFiConfigFile, "wt");
                if (fout)
                {
                    char str_tmp[0xff];
                    sprintf(str_tmp, "wifi:O%d:%s;S%d:\"%s\";P%d:\"%s\";K%d:%s;T%d:%s;E%d:%s\n",
                            (int) strlen(object), object,
                            (int) strlen(ssid)+2, ssid,
                            (int) strlen(pass)+2, pass,
                            (int) strlen(secret_key), secret_key,
                            (int) strlen(security), security,
                            (int) strlen(ethernet), ethernet);
                    fputs(str_tmp, fout);
                    fclose(fout);
                }

                // THIS IS --EXAMPLE-- of what should be done on network connection OK
                // currently is not used because it requires notification from newtrok manager
                // update WIFI status, save WIFI ssid, update PAIRING_STATE
                // TODO: FIXME
                sleep(2); // TBD: replace with waiting for notification

                if (BlePairing::mWiFiConnected) // connected OK
                {
                    bleApi->GATTUpdateCharacteristic(service_id, ATTRIBUTE_OFFSET(GET_WIFI_STATUS), SET_ATTRIBUTE_STR_VAL(BlePairing::mWiFiConnected));
                    bleApi->GATTUpdateCharacteristic(service_id, ATTRIBUTE_OFFSET(GET_SSID_WIFI), SET_ATTRIBUTE_STR_VAL(ssid));
                    bleApi->GATTUpdateCharacteristic(service_id, ATTRIBUTE_OFFSET(GET_PAIRING_STATE), SET_ATTRIBUTE_STR_VAL(BlePairing::mWiFiConnected));
                    bleApi->NotifyCharacteristic(RING_PAIRING_SVC_IDX, GET_PAIRING_STATE, BlePairing::mWiFiConnected);
                }
                else // connection failed
                {
                    bleApi->GATTUpdateCharacteristic(service_id, ATTRIBUTE_OFFSET(GET_PAIRING_STATE), SET_ATTRIBUTE_STR_VAL(BlePairing::mWiFiConnectFailed));
                    bleApi->NotifyCharacteristic(RING_PAIRING_SVC_IDX, GET_PAIRING_STATE, BlePairing::mWiFiConnectFailed);
                }
            }
                break;
            default:
                break;
        }
        break;

    default:
        break;
    }
}
