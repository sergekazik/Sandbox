#ifndef _SERVER_GATT_API_H_
#define _SERVER_GATT_API_H_

#include <stdio.h>
#include <stdlib.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

struct server_ref
{
    struct bt_att *att;
    struct gatt_db *db;
    struct bt_gatt_server *gatt;
};

namespace Ble {

#define GATT_CHARACTERISTICS_MAX            32
#define BLE_READ_PACKET_MAX                 22
#define DEV_CLASS_LEN                       16      /* device class bitmask as a string */
#define DEV_NAME_LEN                        64      /* text string = device name */
#define DEV_MAC_ADDR_LEN                    18      /* text string = device mac address */
#define ATT_MTU_MAX                         512     /* maximum allowed value size of 512 bytes.*/
#define ATTR_NAME_LEN                       20
#define ToInt(_x) (((_x) > 0x39)?(((_x) & ~0x20)-0x37):((_x)-0x30))

typedef struct uuid_16
{
   uint8_t UUID_Byte0;
   uint8_t UUID_Byte1;
} UUID_16_t;

typedef struct uuid_32
{
   uint8_t UUID_Byte0;
   uint8_t UUID_Byte1;
   uint8_t UUID_Byte2;
   uint8_t UUID_Byte3;
} UUID_32_t;

typedef struct uuid_128
{
   uint8_t UUID_Byte0;
   uint8_t UUID_Byte1;
   uint8_t UUID_Byte2;
   uint8_t UUID_Byte3;
   uint8_t UUID_Byte4;
   uint8_t UUID_Byte5;
   uint8_t UUID_Byte6;
   uint8_t UUID_Byte7;
   uint8_t UUID_Byte8;
   uint8_t UUID_Byte9;
   uint8_t UUID_Byte10;
   uint8_t UUID_Byte11;
   uint8_t UUID_Byte12;
   uint8_t UUID_Byte13;
   uint8_t UUID_Byte14;
   uint8_t UUID_Byte15;
} UUID_128_t;

typedef struct bd_addr
{
   uint8_t BD_ADDR0;
   uint8_t BD_ADDR1;
   uint8_t BD_ADDR2;
   uint8_t BD_ADDR3;
   uint8_t BD_ADDR4;
   uint8_t BD_ADDR5;
} BD_ADDR_t;

typedef struct parameter
{
    int             count;
    char           *strParam;
    unsigned int    intParam;
} Parameter_t;

namespace Config { enum Tag {
    EOL                  = 0, // End of list
    ServiceTable,
    LocalDeviceName,
    LocalClassOfDevice,
    MACAddress
};}

typedef struct deviceconfig
{
    Ble::Config::Tag   tag;
    Parameter_t params;
} DeviceConfig_t;

#define GATM_SECURITY_PROPERTIES_NO_SECURITY                         0x00000000
#define GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE    0x00000001
#define GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE      0x00000002
#define GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_READ     0x00000004
#define GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_READ       0x00000008
#define GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_SIGNED_WRITES       0x00000010
#define GATM_SECURITY_PROPERTIES_AUTHENTICATED_SIGNED_WRITES         0x00000020

#define GATM_CHARACTERISTIC_PROPERTIES_BROADCAST                     0x00000001
#define GATM_CHARACTERISTIC_PROPERTIES_READ                          0x00000002
#define GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP                 0x00000004
#define GATM_CHARACTERISTIC_PROPERTIES_WRITE                         0x00000008
#define GATM_CHARACTERISTIC_PROPERTIES_NOTIFY                        0x00000010
#define GATM_CHARACTERISTIC_PROPERTIES_INDICATE                      0x00000020
#define GATM_CHARACTERISTIC_PROPERTIES_AUTHENTICATED_SIGNED_WRITES   0x00000040
#define GATM_CHARACTERISTIC_PROPERTIES_EXT_PROPERTIES                0x00000080

#define SERVICE_TABLE_FLAGS_USE_PERSISTENT_UID                       0x00000001
#define SERVICE_TABLE_FLAGS_SECONDARY_SERVICE                        0x00000002

typedef enum
{
    atInclude,
    atCharacteristic,
    atDescriptor
} AttributeType_t;

typedef struct attributeinfo
{
    AttributeType_t AttributeType;
    unsigned int    AttributeOffset;
    char            AttributeName[ATTR_NAME_LEN];
    unsigned long   CharacteristicPropertiesMask;
    unsigned long   SecurityPropertiesMask;
    UUID_128_t      CharacteristicUUID;
    uint8_t         AllocatedValue;
    unsigned int    MaximumValueLength;
    unsigned int    ValueLength;
    char           *Value;
} AttributeInfo_t;

typedef struct serviceinfo
{
    unsigned int     ServiceID;
    UUID_128_t       ServiceUUID;
    unsigned int     NumberAttributes;
    AttributeInfo_t *AttributeList;
} ServiceInfo_t;

namespace Error { enum Error {
    NONE                =  0, // NO ERROR, SUCCESS
    OPERATION_FAILED    = -1, // no command was specified to the parser.
    INVALID_COMMAND     = -2, // the Command does not exist for processing.
    EXIT_CODE           = -3, // the Command specified was the Exit Command.
    FUNCTION            = -4, // an error occurred in execution of the Command Function.
    TOO_MANY_PARAMS     = -5, // there are more parameters then will fit in the UserCommand.
    INVALID_PARAMETERS  = -6, // an error occurred due to the fact that one or more of the required parameters were invalid.
    NOT_INITIALIZED     = -7, // an error occurred due to the fact that the Platform Manager has not been initialized.
    UNDEFINED           = -8, // Not initialized value; not all paths of the function modify return value
    NOT_IMPLEMENTED     = -9, // Not yet implemented or not supported for this target
    NOT_FOUND           = -10,// Search not found
    INVALID_STATE       = -11,// Already set or single use error
    NOT_REGISTERED      = -12,// Callback is not registered
    FAILED_INITIALIZE   = -13,//
    PTHREAD_ERROR       = -14,// create or cancel error
    CONNECTED_STATE     = -15,// already connected
    CB_REGISTER_FAILED  = -16,// registering callback failed
    REGISTER_SVC_ERROR  = -17,// registering gatt service failed
};}

namespace ConfigArgument { enum Arg {
    None    = 0,
    Enable  = 1,
    Disable = 0,
    PowerOn = 1,
    PowerOff = 0,
    ASCII_File = 1,
    Terminal = 2,
    FTS_File = 3,
};}

namespace Property
{
    enum Access {
        Read,
        Write,
        Confirmed,
        // GAP events
        Connected,
        Disconnected,
    };
    enum Permission
    {
        R__     = GATM_CHARACTERISTIC_PROPERTIES_READ,
        _W_     = GATM_CHARACTERISTIC_PROPERTIES_WRITE,
        RW_     = GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE,
        __N     = GATM_CHARACTERISTIC_PROPERTIES_NOTIFY,
        R_N     = GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_NOTIFY,
        _WN     = GATM_CHARACTERISTIC_PROPERTIES_WRITE | GATM_CHARACTERISTIC_PROPERTIES_NOTIFY,
        RWN     = GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE | GATM_CHARACTERISTIC_PROPERTIES_NOTIFY,
    };
}

namespace Advertising { enum Flags {
    // Flags is a bitmask in the following table
    LocalPublicAddress  = 0x00000001, // [Local Random Address] - if not set
    Discoverable        = 0x00000002, // [Non Discoverable] - if not set
    Connectable         = 0x00000004, // [Non Connectable] - if not set
    AdvertiseName       = 0x00000010, // [Advertise Name off] - if not set
    AdvertiseTxPower    = 0x00000020, // [Advertise Tx Power off] - if not set
    AdvertiseAppearance = 0x00000040, // [Advertise Appearance off] - if not set
    PeerPublicAddress   = 0x00000100, // [Peer Random Address] - if not set
    DirectConnectable   = 0x00000200, // [Undirect Connectable] // When Connectable bit (0x0004) is set: - if not set
    LowDutyCycle        = 0x00000400, // [High Duty Cycle] // When Direct Connectable bit (0x0200) is set: - if not set
};}

typedef struct _GattServerInfo {
    int fd;
    int hci_socket;
    pthread_t hci_thread_id;
    server_ref *sref;
    uint16_t client_svc_handle;
    uint16_t client_attr_handle[GATT_CHARACTERISTICS_MAX];
} GattServerInfo_t;

class GattSrv
{
public:
    static GattSrv* getInstance();
    static GattServerInfo_t mServer;
    bool            mInitialized;              // initialization state
    ServiceInfo_t  *mServiceTable;             // pointer to populated service tbl

    // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.gap.appearance.xml
    enum Appearance {
        BLE_APPEARANCE_UNKNOWN                  = 0,
        BLE_APPEARANCE_GENERIC_PHONE            = 64,
        BLE_APPEARANCE_GENERIC_COMPUTER         = 128,
        BLE_APPEARANCE_GENERIC_WATCH            = 192,
        BLE_APPEARANCE_GENERIC_SENSOR           = 1344,
        BLE_APPEARANCE_MOTION_SENSOR            = 1345,
    };

    typedef void (*onCharacteristicAccessCallback) (int aAttribueIdx, Ble::Property::Access aAccessType);

private:
    static GattSrv* instance;
    GattSrv();
    char mDeviceName[DEV_NAME_LEN];            // device name
    char mMacAddress[DEV_MAC_ADDR_LEN];
    onCharacteristicAccessCallback  mOnCharCb; // callback to client function on characteristic change - Note: single user only

public:
    int Initialize();
    int Configure(DeviceConfig_t* aConfig);

    int SetDevicePower(Ble::ConfigArgument::Arg aOnOff);

    int Shutdown();

    int SetLocalDeviceName(Parameter_t *aParams);
    int SetLocalClassOfDevice(Parameter_t *aParams);

    int RegisterCharacteristicAccessCallback(onCharacteristicAccessCallback aCb);
    int UnregisterCharacteristicAccessCallback(onCharacteristicAccessCallback aCb);

    int QueryLocalDeviceProperties(Parameter_t *aParams);
    int SetLocalDeviceAppearance(Parameter_t *aParams);

    // connection and security
    int SetRemoteDeviceLinkActive(Parameter_t *aParams);
    int RegisterAuthentication(Parameter_t *aParams);
    int ChangeSimplepairingParameters(Parameter_t *aParams);

    // GATT
    int RegisterGATMEventCallback(Parameter_t *aParams);
    int GATTRegisterService(Parameter_t *aParams);
    int GATTUpdateCharacteristic(unsigned int aServiceID, int aAttrOffset, uint8_t *aAttrData, int aAttrLen);
    int NotifyCharacteristic(int aAttributeIdx, const char* aPayload, int len=0);

    // Advertising
    int SetAdvertisingInterval(Parameter_t *aParams);
    int StartAdvertising(Parameter_t *aParams);
    int StopAdvertising(Parameter_t *aParams);

    int SetAuthenticatedPayloadTimeout(Parameter_t *aParams);
    int SetAndUpdateConnectionAndScanBLEParameters(Parameter_t *aParams);
    void CleanupServiceList(void);

    // debug
    int EnableBluetoothDebug(Parameter_t *aParams);

    // debug / display functions and helper functions
    int GetAttributeIdxByOffset(unsigned int AttributeOffset);
    int ProcessRegisteredCallback(Ble::Property::Access aEventType, int aAttrOffset);
    int UpdateServiceTable(int attr_idx, const char *str_data, int len);

private:
    struct hci_dev_info di;
    int OpenSocket(struct hci_dev_info &di);
    void PrintDeviceHeader(struct hci_dev_info *di);
    void WriteBdaddr(int dd, char *opt);

public: // HCI methods
    void HCIreset(int ctl, int hdev);
    void HCIscan(int ctl, int hdev, char *opt);
    void HCIssp_mode(int hdev, char *opt);
    void HCIup(int ctl, int hdev);
    void HCIdown(int ctl, int hdev);
    void HCIle_adv(int hdev, char *opt);
    void HCIno_le_adv(int hdev);
    void HCIname(int hdev, char *opt);
    void HCIclass(int hdev, char *opt);
};

} /* namespace Ble */


#endif // _SERVER_GATT_API_H_
