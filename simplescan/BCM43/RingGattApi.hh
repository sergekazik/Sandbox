#ifndef _RING_GATT_API_H_
#define _RING_GATT_API_H_

#ifndef BCM43
#error WRONG RingGattSrv platform-related file included into build
#endif
#if !defined(BLUEZ_TOOLS_SUPPORT)
#error REQUIRED CONFIG_AMBARELLA_BLUEZ_TOOLS_SUPPORT and CONFIG_AMBARELLA_BLUETOOTH5_TOOL_SUPPORT
#endif

#include <stdio.h>
#include <stdlib.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "RingBleApi.hh"
#include "RingBlePairing.hh"

struct server_ref {
    struct bt_att *att;
    struct gatt_db *db;
    struct bt_gatt_server *gatt;
};

namespace Ring { namespace Ble {

typedef struct _GattServerInfo {
    int fd;
    int hci_socket;
    uint16_t ring_svc_handle;
    pthread_t hci_thread_id;
    server_ref *sref;
    uint16_t ring_attr_handle[RING_CHARACTERISTICS_MAX];
} GattServerInfo_t;

class GattSrv : BleApi
{
public:
    static BleApi* getInstance();
    static const int IOCAPABILITIESSTRINGS_SIZE;
    static const char *IOCapabilitiesStrings[];
    static GattServerInfo_t mServer;

private:
    static GattSrv* instance;
    GattSrv();

public:
    int Initialize();

    int QueryDevicePower() { return Error::NOT_IMPLEMENTED; }
    int SetDevicePower(Ble::ConfigArgument::Arg aOnOff);

    int ShutdownService() { return Error::NOT_IMPLEMENTED; }
    int Shutdown();

    int Initialize(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int SetDevicePower(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int QueryDevicePower(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int SetLocalDeviceName(ParameterList_t *aParams __attribute__ ((unused)));
    int SetLocalClassOfDevice(ParameterList_t *aParams __attribute__ ((unused)));
    int SetDiscoverable(ParameterList_t *aParams __attribute__ ((unused)));
    int SetConnectable(ParameterList_t *aParams __attribute__ ((unused)));
    int SetPairable(ParameterList_t *aParams __attribute__ ((unused)));
    int ShutdownService(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int Shutdown(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }

    int RegisterEventCallback(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int UnRegisterEventCallback(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }

    int SetLocalRemoteDebugZoneMask(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int QueryLocalRemoteDebugZoneMask(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int SetDebugZoneMaskPID(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int QueryLocalDeviceProperties(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int SetLocalDeviceAppearance(ParameterList_t *aParams __attribute__ ((unused)));

    // discovery
    int StartDeviceDiscovery(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int StopDeviceDiscovery(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }

    // remote device operations
    int QueryRemoteDeviceList(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int QueryRemoteDeviceProperties(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int AddRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int DeleteRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int UpdateRemoteDeviceApplicationData(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int DeleteRemoteDevices(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int PairWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int CancelPairWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int UnPairRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int QueryRemoteDeviceServices(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int QueryRemoteDeviceServiceSupported(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int QueryRemoteDevicesForService(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int QueryRemoteDeviceServiceClasses(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }

    // connection and security
    int EnableSCOnly(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int RegenerateP256LocalKeys(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int OOBGenerateParameters(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int AuthenticateRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int ConnectWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int DisconnectRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int EncryptRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int SetRemoteDeviceLinkActive(ParameterList_t *aParams __attribute__ ((unused)));

    // Service Discovery Protocol (SDP)
    int CreateSDPRecord(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int DeleteSDPRecord(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int AddSDPAttribute(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int DeleteSDPAttribute(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }

    int RegisterAuthentication(ParameterList_t *aParams __attribute__ ((unused)));
    int UnRegisterAuthentication(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NONE; }

    int PINCodeResponse(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int PassKeyResponse(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int UserConfirmationResponse(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int ChangeSimplePairingParameters(ParameterList_t *aParams __attribute__ ((unused)));
    int ChangeLEPairingParameters(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }

    // GATT
    int RegisterGATMEventCallback(ParameterList_t *aParams __attribute__ ((unused)));
    int UnRegisterGATMEventCallback(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NONE; }
    int GATTQueryConnectedDevices(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int GATTRegisterService(ParameterList_t *aParams __attribute__ ((unused)));
    int GATTUnRegisterService(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NONE; }
    int GATTUpdateCharacteristic(unsigned int aServiceID, int aAttrOffset, Byte_t *aAttrData, int aAttrLen) { (void) aAttrData; (void) aAttrLen; (void) aAttrOffset; (void) aServiceID; return Error::NOT_IMPLEMENTED; }
    int GATTIndicateCharacteristic(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int GATTNotifyCharacteristic(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int NotifyCharacteristic(int aServiceIdx, int aAttributeIdx, const char* aStrPayload);
    int ListCharacteristics(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int ListDescriptors(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int GATTQueryPublishedServices(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }

    // Advertising
    int SetAdvertisingInterval(ParameterList_t *aParams __attribute__ ((unused)));
    int StartAdvertising(ParameterList_t *aParams __attribute__ ((unused)));
    int StopAdvertising(ParameterList_t *aParams __attribute__ ((unused)));

    int SetAuthenticatedPayloadTimeout(ParameterList_t *aParams __attribute__ ((unused)));
    int QueryAuthenticatedPayloadTimeout(ParameterList_t *aParams __attribute__ ((unused))) { return Error::NOT_IMPLEMENTED; }
    int SetAndUpdateConnectionAndScanBLEParameters(ParameterList_t *aParams __attribute__ ((unused)));

    // debug
    int EnableBluetoothDebug(ParameterList_t *aParams __attribute__ ((unused)));

    // debug / display functions and helper functions
    void DisplayGATTUUID(GATT_UUID_t *UUID, const char *Prefix, unsigned int Level)  { (void) UUID, (void) Prefix, (void) Level; return; }
    void DisplayAttributeValue(unsigned int aServiceIdx, unsigned int aAttributeIdx)  { (void) aServiceIdx, (void) aAttributeIdx; return; }

    void BD_ADDRToStr(BD_ADDR_t Board_Address, char *BoardStr)  {(void)  Board_Address, (void)  BoardStr; return; }
    void StrToBD_ADDR(char *BoardStr, BD_ADDR_t *Board_Address)  {(void)  BoardStr, (void)  Board_Address; return; }
    void StrToUUIDEntry(char *UUIDStr, SDP_UUID_Entry_t *UUIDEntry)  {(void)  UUIDStr, (void)  UUIDEntry; return; }
    void DumpData(Boolean_t String, unsigned int Length, Byte_t *Data)  {(void) String, (void)  Length, (void)  Data; return; }
    char *GetServiceNameById(unsigned int ServiceID)  { (void) ServiceID; return NULL; }
    int GetServiceIndexById(unsigned int ServiceID)  { (void) ServiceID; return Error::NOT_IMPLEMENTED; }
    AttributeInfo_t *SearchServiceListByOffset(unsigned int ServiceID, unsigned int AttributeOffset)  { (void) ServiceID; (void) AttributeOffset; return NULL; }
    int GetAttributeIdxByOffset(unsigned int ServiceID, unsigned int AttributeOffset)  { (void) ServiceID; (void) AttributeOffset; return Error::NOT_IMPLEMENTED; }
    int ProcessRegisteredCallback(GATM_Event_Type_t aEventType, int aServiceID, int aAttrOffset);
    void SaveRemoteDeviceAddress(BD_ADDR_t aConnectedMACAddress)  { (void) aConnectedMACAddress; return; }

private:
    struct hci_dev_info di;

    int open_socket(struct hci_dev_info &di);
    void print_dev_hdr(struct hci_dev_info *di);
    void print_pkt_type(struct hci_dev_info *di);
    void print_link_policy(struct hci_dev_info *di);
    void print_link_mode(struct hci_dev_info *di);
    void print_dev_features(struct hci_dev_info *di, int format);
    void print_le_states(uint64_t states);
    const char *get_minor_device_name(int major, int minor);

public:
    // HCI methods
    void HCIrstat(int ctl, int hdev);
    void HCIscan(int ctl, int hdev, char *opt);
    void HCIle_addr(int hdev, char *opt);
    void HCIle_adv(int hdev, char *opt);
    void HCIno_le_adv(int hdev);
    void HCIle_states(int hdev);
    void HCIiac(int hdev, char *opt);
    void HCIauth(int ctl, int hdev, char *opt);
    void HCIencrypt(int ctl, int hdev, char *opt);
    void HCIup(int ctl, int hdev);
    void HCIdown(int ctl, int hdev);
    void HCIreset(int ctl, int hdev);
    void HCIptype(int ctl, int hdev, char *opt);
    void HCIlp(int ctl, int hdev, char *opt);
    void HCIlm(int ctl, int hdev, char *opt);
    void HCIaclmtu(int ctl, int hdev, char *opt);
    void HCIscomtu(int ctl, int hdev, char *opt);
    void HCIfeatures(int hdev);
    void HCIname(int hdev, char *opt);
    void HCIclass(int hdev, char *opt);
    void HCIvoice(int hdev, char *opt);
    void HCIdelkey(int hdev, char *opt);
    void HCIoob_data(int hdev);
    void HCIcommands(int hdev);
    void HCIversion(int hdev);
    void HCIinq_tpl(int hdev, char *opt);
    void HCIinq_mode(int hdev, char *opt);
    void HCIinq_data(int hdev, char *opt);
    void HCIinq_type(int hdev, char *opt);
    void HCIinq_parms(int hdev, char *opt);
    void HCIpage_parms(int hdev, char *opt);
    void HCIpage_to(int hdev, char *opt);
    void HCIafh_mode(int hdev, char *opt);
    void HCIssp_mode(int hdev, char *opt);
    void HCIrevision(int hdev);
    void HCIblock(int hdev, char *opt);
    void HCIunblock(int hdev, char *opt);
};

} } /* namespace Ring::Ble */


#endif // _RING_GATT_API_H_
