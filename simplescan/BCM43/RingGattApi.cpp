/*---------------------------------------------------------------------
 *               ------------------      -----------------            *
 *               | Ring App Setup |      |   test_ble    |            *
 *               ------------------      -----------------            *
 *                          |                |                        *
 *                          |   --------------------                  *
 *                          |   |gatt_src_test.cpp |                  *
 *                          |   --------------------                  *
 *                          |          |                              *
 *                  ---------------------------------                 *
 *                  |    RingBleApi.cpp abstract    |                 *
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
#ifndef BCM43
#error WRONG RingGattSrv platform-related file included into build
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Bot_Notifier.h"
#include "RingGattApi.hh"

using namespace Ring;
using namespace Ring::Ble;

GattSrv* GattSrv::instance = NULL;

BleApi* GattSrv::getInstance() {
    if (instance == NULL)
        instance = new GattSrv();
    return (BleApi*) instance;
}

GattSrv::GattSrv() :
   mDeviceClass("0x280430")
  ,mDeviceName("Ring Doorbell Ampak BCM43")
{
}

int GattSrv::open_socket(struct hci_dev_info &di) {
    int ctl = -1;
    bdaddr_t  _BDADDR_ANY = {{0, 0, 0, 0, 0, 0}};

    /* Open HCI socket  */
    if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
        perror("Can't open HCI socket.");
        return errno;
    }

    if (ioctl(ctl, HCIGETDEVINFO, (void *) &di))
    {
        perror("Can't get device info");
        return errno;
    }

    di.dev_id = 0;

    if (hci_test_bit(HCI_RAW, &di.flags) && !bacmp(&di.bdaddr, &_BDADDR_ANY)) {
        int dd = hci_open_dev(di.dev_id);
        hci_read_bd_addr(dd, &di.bdaddr, 1000);
        hci_close_dev(dd);
    }
    return ctl;
}

int GattSrv::Initialize() {
    struct hci_dev_info di;
    int ctl;

    if (0 <= (ctl = open_socket(di)))
    {
        HCIup(ctl, di.dev_id);
        HCIscan(ctl, di.dev_id, (char*) "piscan");
        HCIclass(di.dev_id, mDeviceClass);
        HCIle_adv(di.dev_id, NULL);

        close(ctl);
        ctl = Error::NONE;
    }
    return ctl;
}

int GattSrv::Shutdown() {
    struct hci_dev_info di;
    int ctl;

    if (0 <= (ctl = open_socket(di)))
    {
        HCIno_le_adv(di.dev_id);
        HCIscan(ctl, di.dev_id, (char*) "noscan");
        HCIdown(ctl, di.dev_id);

        close(ctl);
        ctl = Error::NONE;
    }
    return ctl;
}

int GattSrv::Configure(DeviceConfig_t* aConfig) {
    int ret_val = Error::UNDEFINED;

    struct hci_dev_info di;
    int ctl;

    ctl = open_socket(di);

    while (0 <= ctl && aConfig != NULL && aConfig->tag != Ble::Config::EOL)
    {
        switch (aConfig->tag)
        {
        case Ble::Config::ServiceTable:
            ret_val = Error::NOT_IMPLEMENTED;
            break;

        case Ble::Config::LocalDeviceName:
            HCIname(di.dev_id, mDeviceName);
            ret_val = Error::NONE;
            break;

        case Ble::Config::LocalClassOfDevice:
            HCIclass(di.dev_id, mDeviceClass);
            ret_val = Error::NONE;
            break;

        case Ble::Config::Discoverable:
            HCIle_adv(di.dev_id, NULL);
            ret_val = Error::NONE;
            break;

        case Ble::Config::Connectable:
            ret_val = Error::NOT_IMPLEMENTED;
            break;

        case Ble::Config::Pairable:
            ret_val = Error::NOT_IMPLEMENTED;
            break;

        case Ble::Config::RemoteDeviceLinkActive:
            ret_val = Error::NOT_IMPLEMENTED;
            break;

        case Ble::Config::LocalDeviceAppearance:
            ret_val = Error::NOT_IMPLEMENTED;
            break;

        case Ble::Config::AdvertisingInterval:
            ret_val = Error::NOT_IMPLEMENTED;
            break;

        case Ble::Config::AndUpdateConnectionAndScanBLEParameters:
            ret_val = Error::NOT_IMPLEMENTED;
            break;

        case Ble::Config::AuthenticatedPayloadTimeout:
            ret_val = Error::NOT_IMPLEMENTED;
            break;

        case Ble::Config::RegisterGATTCallback:
            ret_val = Error::NOT_IMPLEMENTED;
            break;

        case Ble::Config::RegisterService:
            ret_val = Error::NOT_IMPLEMENTED;
            break;

        case Ble::Config::RegisterAuthentication:
            ret_val = Error::NOT_IMPLEMENTED;
            break;

        case Ble::Config::SetSimplePairing:
            ret_val = Error::NOT_IMPLEMENTED;
            break;

        case Ble::Config::EnableBluetoothDebug:
            ret_val = Error::NOT_IMPLEMENTED;
            break;

        default:
            BOT_NOTIFY_ERROR("Device Config: unknown tag = %d", aConfig->tag);
            ret_val = Error::INVALID_PARAMETERS;
            break;
        }

        if (ret_val != Error::NONE)
            break;

        aConfig++;
    }

    close(ctl);
    return ret_val;
}

/**************************** HCI ******************************/
/**************************** HCI ******************************/
/**************************** HCI ******************************/
void GattSrv::print_pkt_type(struct hci_dev_info *di) {
    char *str;
    str = hci_ptypetostr(di->pkt_type);
    BOT_NOTIFY_DEBUG("\tPacket type: %s", str);
    bt_free(str);
}

void GattSrv::print_link_policy(struct hci_dev_info *di) {
    BOT_NOTIFY_DEBUG("\tLink policy: %s", hci_lptostr(di->link_policy));
}

void GattSrv::print_link_mode(struct hci_dev_info *di) {
    char *str;
    str =  hci_lmtostr(di->link_mode);
    BOT_NOTIFY_DEBUG("\tLink mode: %s", str);
    bt_free(str);
}

void GattSrv::print_dev_features(struct hci_dev_info *di, int format) {
    BOT_NOTIFY_DEBUG("\tFeatures: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x "
       "0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x",
       di->features[0], di->features[1], di->features[2],
       di->features[3], di->features[4], di->features[5],
       di->features[6], di->features[7]);

    if (format) {
    char *tmp = lmp_featurestostr(di->features, "\t\t", 63);
    BOT_NOTIFY_DEBUG("%s", tmp);
    bt_free(tmp);
    }
}

void GattSrv::print_le_states(uint64_t states) {
    int i;
    const char *le_states[] = {
    "Non-connectable Advertising State" ,
    "Scannable Advertising State",
    "Connectable Advertising State",
    "Directed Advertising State",
    "Passive Scanning State",
    "Active Scanning State",
    "Initiating State/Connection State in Master Role",
    "Connection State in the Slave Role",
    "Non-connectable Advertising State and Passive Scanning State combination",
    "Scannable Advertising State and Passive Scanning State combination",
    "Connectable Advertising State and Passive Scanning State combination",
    "Directed Advertising State and Passive Scanning State combination",
    "Non-connectable Advertising State and Active Scanning State combination",
    "Scannable Advertising State and Active Scanning State combination",
    "Connectable Advertising State and Active Scanning State combination",
    "Directed Advertising State and Active Scanning State combination",
    "Non-connectable Advertising State and Initiating State combination",
    "Scannable Advertising State and Initiating State combination",
    "Non-connectable Advertising State and Master Role combination",
    "Scannable Advertising State and Master Role combination",
    "Non-connectable Advertising State and Slave Role combination",
    "Scannable Advertising State and Slave Role combination",
    "Passive Scanning State and Initiating State combination",
    "Active Scanning State and Initiating State combination",
    "Passive Scanning State and Master Role combination",
    "Active Scanning State and Master Role combination",
    "Passive Scanning State and Slave Role combination",
    "Active Scanning State and Slave Role combination",
    "Initiating State and Master Role combination/Master Role and Master Role combination",
    NULL
    };

    BOT_NOTIFY_DEBUG("Supported link layer states:");
    for (i = 0; le_states[i]; i++) {
    const char *status;
    status = states & (1 << i) ? "YES" : "NO ";
    BOT_NOTIFY_DEBUG("\t%s %s", status, le_states[i]);
    }
}

void GattSrv::HCIrstat(int ctl, int hdev) {
    /* Reset HCI device stat counters */
    if (ioctl(ctl, HCIDEVRESTAT, hdev) < 0) {
    BOT_NOTIFY_ERROR("Can't reset stats counters hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
}

void GattSrv::HCIscan(int ctl, int hdev, char *opt) {
    struct hci_dev_req dr;
    dr.dev_id  = hdev;
    dr.dev_opt = SCAN_DISABLED;
    if (!strcmp(opt, "iscan"))
    dr.dev_opt = SCAN_INQUIRY;
    else if (!strcmp(opt, "pscan"))
    dr.dev_opt = SCAN_PAGE;
    else if (!strcmp(opt, "piscan"))
    dr.dev_opt = SCAN_PAGE | SCAN_INQUIRY;
    if (ioctl(ctl, HCISETSCAN, (unsigned long) &dr) < 0) {
    BOT_NOTIFY_ERROR("Can't set scan mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
}

void GattSrv::HCIle_addr(int hdev, char *opt) {
    struct hci_request rq;
    le_set_random_address_cp cp;
    uint8_t status;
    int dd, err, ret;
    if (!opt)
    return;
    if (hdev < 0)
    hdev = hci_get_route(NULL);
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    err = -errno;
    BOT_NOTIFY_ERROR("Could not open device: %s(%d)", strerror(-err), -err);
    exit(1);
    }
    memset(&cp, 0, sizeof(cp));
    str2ba(opt, &cp.bdaddr);
    memset(&rq, 0, sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = OCF_LE_SET_RANDOM_ADDRESS;
    rq.cparam = &cp;
    rq.clen = LE_SET_RANDOM_ADDRESS_CP_SIZE;
    rq.rparam = &status;
    rq.rlen = 1;
    ret = hci_send_req(dd, &rq, 1000);
    if (status || ret < 0) {
    err = -errno;
    BOT_NOTIFY_ERROR("Can't set random address for hci%d: " "%s (%d)", hdev, strerror(-err), -err);
    }
    hci_close_dev(dd);
}

void GattSrv::HCIle_adv(int hdev, char *opt) {
    struct hci_request rq;
    le_set_advertise_enable_cp advertise_cp;
    le_set_advertising_parameters_cp adv_params_cp;
    uint8_t status;
    int dd, ret;
    if (hdev < 0)
    hdev = hci_get_route(NULL);
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    perror("Could not open device");
    exit(1);
    }
    memset(&adv_params_cp, 0, sizeof(adv_params_cp));
    adv_params_cp.min_interval = htobs(0x0800);
    adv_params_cp.max_interval = htobs(0x0800);
    if (opt)
    adv_params_cp.advtype = atoi(opt);
    adv_params_cp.chan_map = 7;
    memset(&rq, 0, sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = OCF_LE_SET_ADVERTISING_PARAMETERS;
    rq.cparam = &adv_params_cp;
    rq.clen = LE_SET_ADVERTISING_PARAMETERS_CP_SIZE;
    rq.rparam = &status;
    rq.rlen = 1;
    ret = hci_send_req(dd, &rq, 1000);
    if (ret < 0)
    goto done;
    memset(&advertise_cp, 0, sizeof(advertise_cp));
    advertise_cp.enable = 0x01;
    memset(&rq, 0, sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
    rq.cparam = &advertise_cp;
    rq.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
    rq.rparam = &status;
    rq.rlen = 1;
    ret = hci_send_req(dd, &rq, 1000);
done:
    hci_close_dev(dd);
    if (ret < 0) {
    BOT_NOTIFY_ERROR("Can't set advertise mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (status) {
    BOT_NOTIFY_ERROR("LE set advertise enable on hci%d returned status %d", hdev, status);
    exit(1);
    }
}

void GattSrv::HCIno_le_adv(int hdev) {
    struct hci_request rq;
    le_set_advertise_enable_cp advertise_cp;
    uint8_t status;
    int dd, ret;
    if (hdev < 0)
    hdev = hci_get_route(NULL);
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    perror("Could not open device");
    exit(1);
    }
    memset(&advertise_cp, 0, sizeof(advertise_cp));
    memset(&rq, 0, sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
    rq.cparam = &advertise_cp;
    rq.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
    rq.rparam = &status;
    rq.rlen = 1;
    ret = hci_send_req(dd, &rq, 1000);
    hci_close_dev(dd);
    if (ret < 0) {
    BOT_NOTIFY_ERROR("Can't set advertise mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (status) {
    BOT_NOTIFY_ERROR("LE set advertise enable on hci%d returned status %d", hdev, status);
    exit(1);
    }
}

void GattSrv::HCIle_states(int hdev) {
    le_read_supported_states_rp rp;
    struct hci_request rq;
    int err, dd;
    if (hdev < 0)
    hdev = hci_get_route(NULL);
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    memset(&rp, 0, sizeof(rp));
    memset(&rq, 0, sizeof(rq));
    rq.ogf    = OGF_LE_CTL;
    rq.ocf    = OCF_LE_READ_SUPPORTED_STATES;
    rq.rparam = &rp;
    rq.rlen   = LE_READ_SUPPORTED_STATES_RP_SIZE;
    err = hci_send_req(dd, &rq, 1000);
    hci_close_dev(dd);
    if (err < 0) {
    BOT_NOTIFY_ERROR("Can't read LE supported states on hci%d:" " %s(%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (rp.status) {
    BOT_NOTIFY_ERROR("Read LE supported states on hci%d" " returned status %d", hdev, rp.status);
    exit(1);
    }
    print_le_states(rp.states);
}

void GattSrv::HCIiac(int hdev, char *opt) {
    int s = hci_open_dev(hdev);
    if (s < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (opt) {
    int l = strtoul(opt, 0, 16);
    uint8_t lap[3];
    if (!strcasecmp(opt, "giac")) {
        l = 0x9e8b33;
    } else if (!strcasecmp(opt, "liac")) {
        l = 0x9e8b00;
    } else if (l < 0x9e8b00 || l > 0x9e8b3f) {
        BOT_NOTIFY_DEBUG("Invalid access code 0x%x", l);
        exit(1);
    }
    lap[0] = (l & 0xff);
    lap[1] = (l >> 8) & 0xff;
    lap[2] = (l >> 16) & 0xff;
    if (hci_write_current_iac_lap(s, 1, lap, 1000) < 0) {
        BOT_NOTIFY_DEBUG("Failed to set IAC on hci%d: %s", hdev, strerror(errno));
        exit(1);
    }
    } else {
    uint8_t lap[3 * MAX_IAC_LAP];
    int i, j;
    uint8_t n;
    if (hci_read_current_iac_lap(s, &n, lap, 1000) < 0) {
        BOT_NOTIFY_DEBUG("Failed to read IAC from hci%d: %s", hdev, strerror(errno));
        exit(1);
    }
    print_dev_hdr(&di);
    BOT_NOTIFY_DEBUG("\tIAC: ");
    for (i = 0; i < n; i++) {
        BOT_NOTIFY_DEBUG("0x");
        for (j = 3; j--; )
        BOT_NOTIFY_DEBUG("%02x", lap[j + 3 * i]);
        if (i < n - 1)
        BOT_NOTIFY_DEBUG(", ");
    }
    BOT_NOTIFY_DEBUG("");
    }
    close(s);
}

void GattSrv::HCIauth(int ctl, int hdev, char *opt) {
    struct hci_dev_req dr;
    dr.dev_id = hdev;
    if (!strcmp(opt, "auth"))
    dr.dev_opt = AUTH_ENABLED;
    else
    dr.dev_opt = AUTH_DISABLED;
    if (ioctl(ctl, HCISETAUTH, (unsigned long) &dr) < 0) {
    BOT_NOTIFY_ERROR("Can't set auth on hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
}

void GattSrv::HCIencrypt(int ctl, int hdev, char *opt) {
    struct hci_dev_req dr;
    dr.dev_id = hdev;
    if (!strcmp(opt, "encrypt"))
    dr.dev_opt = ENCRYPT_P2P;
    else
    dr.dev_opt = ENCRYPT_DISABLED;
    if (ioctl(ctl, HCISETENCRYPT, (unsigned long) &dr) < 0) {
    BOT_NOTIFY_ERROR("Can't set encrypt on hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
}

void GattSrv::HCIup(int ctl, int hdev) {
    /* Start HCI device */
    if (ioctl(ctl, HCIDEVUP, hdev) < 0) {
    if (errno == EALREADY)
        return;
    BOT_NOTIFY_ERROR("Can't init device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
}

void GattSrv::HCIdown(int ctl, int hdev) {
    /* Stop HCI device */
    if (ioctl(ctl, HCIDEVDOWN, hdev) < 0) {
    BOT_NOTIFY_ERROR("Can't down device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
}

void GattSrv::HCIreset(int ctl, int hdev) {
    /* Reset HCI device */
#if 0
    if (ioctl(ctl, HCIDEVRESET, hdev) < 0 ) {
    BOT_NOTIFY_ERROR("Reset failed for device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
#endif
    GattSrv::HCIdown(ctl, hdev);
    GattSrv::HCIup(ctl, hdev);
}

void GattSrv::HCIptype(int ctl, int hdev, char *opt) {
    struct hci_dev_req dr;
    dr.dev_id = hdev;
    if (hci_strtoptype(opt, &dr.dev_opt)) {
    if (ioctl(ctl, HCISETPTYPE, (unsigned long) &dr) < 0) {
        BOT_NOTIFY_ERROR("Can't set pkttype on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    } else {
    print_dev_hdr(&di);
    print_pkt_type(&di);
    }
}

void GattSrv::HCIlp(int ctl, int hdev, char *opt) {
    struct hci_dev_req dr;
    dr.dev_id = hdev;
    if (hci_strtolp(opt, &dr.dev_opt)) {
    if (ioctl(ctl, HCISETLINKPOL, (unsigned long) &dr) < 0) {
        BOT_NOTIFY_ERROR("Can't set link policy on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    } else {
    print_dev_hdr(&di);
    print_link_policy(&di);
    }
}

void GattSrv::HCIlm(int ctl, int hdev, char *opt) {
    struct hci_dev_req dr;
    dr.dev_id = hdev;
    if (hci_strtolm(opt, &dr.dev_opt)) {
    if (ioctl(ctl, HCISETLINKMODE, (unsigned long) &dr) < 0) {
        BOT_NOTIFY_ERROR("Can't set default link mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    } else {
    print_dev_hdr(&di);
    print_link_mode(&di);
    }
}

void GattSrv::HCIaclmtu(int ctl, int hdev, char *opt) {
    struct hci_dev_req dr = { .dev_id = (uint16_t) hdev, .dev_opt = 0 };
    uint16_t mtu, mpkt;
    if (!opt)
    return;
    if (sscanf(opt, "%4hu:%4hu", &mtu, &mpkt) != 2)
    return;
    dr.dev_opt = htobl(htobs(mpkt) | (htobs(mtu) << 16));
    if (ioctl(ctl, HCISETACLMTU, (unsigned long) &dr) < 0) {
    BOT_NOTIFY_ERROR("Can't set ACL mtu on hci%d: %s(%d)", hdev, strerror(errno), errno);
    exit(1);
    }
}

void GattSrv::HCIscomtu(int ctl, int hdev, char *opt) {
    struct hci_dev_req dr = { .dev_id = (uint16_t) hdev, .dev_opt = 0 };
    uint16_t mtu, mpkt;
    if (!opt)
    return;
    if (sscanf(opt, "%4hu:%4hu", &mtu, &mpkt) != 2)
    return;
    dr.dev_opt = htobl(htobs(mpkt) | (htobs(mtu) << 16));
    if (ioctl(ctl, HCISETSCOMTU, (unsigned long) &dr) < 0) {
    BOT_NOTIFY_ERROR("Can't set SCO mtu on hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
}

void GattSrv::HCIfeatures(int hdev) {
    uint8_t features[8], max_page = 0;
    char *tmp;
    int i, dd;
    if (!(di.features[7] & LMP_EXT_FEAT)) {
    print_dev_hdr(&di);
    print_dev_features(&di, 1);
    return;
    }
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (hci_read_local_ext_features(dd, 0, &max_page, features, 1000) < 0) {
    BOT_NOTIFY_ERROR("Can't read extended features hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    print_dev_hdr(&di);
    BOT_NOTIFY_DEBUG("\tFeatures%s: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x "
       "0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x",
       (max_page > 0) ? " page 0" : "",
       features[0], features[1], features[2], features[3],
       features[4], features[5], features[6], features[7]);
    tmp = lmp_featurestostr(di.features, "\t\t", 63);
    BOT_NOTIFY_DEBUG("%s", tmp);
    bt_free(tmp);
    for (i = 1; i <= max_page; i++) {
    if (hci_read_local_ext_features(dd, i, NULL,
                    features, 1000) < 0)
        continue;
    BOT_NOTIFY_DEBUG("\tFeatures page %d: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x "
           "0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x", i,
           features[0], features[1], features[2], features[3],
           features[4], features[5], features[6], features[7]);
    }
    hci_close_dev(dd);
}

void GattSrv::HCIname(int hdev, char *opt) {
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (opt) {
    if (hci_write_local_name(dd, opt, 2000) < 0) {
        BOT_NOTIFY_ERROR("Can't change local name on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    } else {
    char name[249];
    int i;
    if (hci_read_local_name(dd, sizeof(name), name, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read local name on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    for (i = 0; i < 248 && name[i]; i++) {
        if ((unsigned char) name[i] < 32 || name[i] == 127)
        name[i] = '.';
    }
    name[248] = '\0';
    print_dev_hdr(&di);
    BOT_NOTIFY_DEBUG("\tName: '%s'", name);
    }
    hci_close_dev(dd);
}

char *GattSrv::get_minor_device_name(int major, int minor) {
    switch (major) {
    case 0:	/* misc */
    return "";
    case 1:	/* computer */
    switch (minor) {
    case 0:
        return "Uncategorized";
    case 1:
        return "Desktop workstation";
    case 2:
        return "Server";
    case 3:
        return "Laptop";
    case 4:
        return "Handheld";
    case 5:
        return "Palm";
    case 6:
        return "Wearable";
    }
    break;
    case 2:	/* phone */
    switch (minor) {
    case 0:
        return "Uncategorized";
    case 1:
        return "Cellular";
    case 2:
        return "Cordless";
    case 3:
        return "Smart phone";
    case 4:
        return "Wired modem or voice gateway";
    case 5:
        return "Common ISDN Access";
    case 6:
        return "Sim Card Reader";
    }
    break;
    case 3:	/* lan access */
    if (minor == 0)
        return "Uncategorized";
    switch (minor / 8) {
    case 0:
        return "Fully available";
    case 1:
        return "1-17% utilized";
    case 2:
        return "17-33% utilized";
    case 3:
        return "33-50% utilized";
    case 4:
        return "50-67% utilized";
    case 5:
        return "67-83% utilized";
    case 6:
        return "83-99% utilized";
    case 7:
        return "No service available";
    }
    break;
    case 4:	/* audio/video */
    switch (minor) {
    case 0:
        return "Uncategorized";
    case 1:
        return "Device conforms to the Headset profile";
    case 2:
        return "Hands-free";
    /* 3 is reserved */
    case 4:
        return "Microphone";
    case 5:
        return "Loudspeaker";
    case 6:
        return "Headphones";
    case 7:
        return "Portable Audio";
    case 8:
        return "Car Audio";
    case 9:
        return "Set-top box";
    case 10:
        return "HiFi Audio Device";
    case 11:
        return "VCR";
    case 12:
        return "Video Camera";
    case 13:
        return "Camcorder";
    case 14:
        return "Video Monitor";
    case 15:
        return "Video Display and Loudspeaker";
    case 16:
        return "Video Conferencing";
    /* 17 is reserved */
    case 18:
        return "Gaming/Toy";
    }
    break;
    case 5: {	/* peripheral */
    static char cls_str[48];
    cls_str[0] = '\0';
    switch (minor & 48) {
    case 16:
        strncpy(cls_str, "Keyboard", sizeof(cls_str));
        break;
    case 32:
        strncpy(cls_str, "Pointing device", sizeof(cls_str));
        break;
    case 48:
        strncpy(cls_str, "Combo keyboard/pointing device", sizeof(cls_str));
        break;
    }
    if ((minor & 15) && (strlen(cls_str) > 0))
        strcat(cls_str, "/");
    switch (minor & 15) {
    case 0:
        break;
    case 1:
        strncat(cls_str, "Joystick", sizeof(cls_str) - strlen(cls_str));
        break;
    case 2:
        strncat(cls_str, "Gamepad", sizeof(cls_str) - strlen(cls_str));
        break;
    case 3:
        strncat(cls_str, "Remote control", sizeof(cls_str) - strlen(cls_str));
        break;
    case 4:
        strncat(cls_str, "Sensing device", sizeof(cls_str) - strlen(cls_str));
        break;
    case 5:
        strncat(cls_str, "Digitizer tablet", sizeof(cls_str) - strlen(cls_str));
        break;
    case 6:
        strncat(cls_str, "Card reader", sizeof(cls_str) - strlen(cls_str));
        break;
    default:
        strncat(cls_str, "(reserved)", sizeof(cls_str) - strlen(cls_str));
        break;
    }
    if (strlen(cls_str) > 0)
        return cls_str;
    }
    case 6:	/* imaging */
    if (minor & 4)
        return "Display";
    if (minor & 8)
        return "Camera";
    if (minor & 16)
        return "Scanner";
    if (minor & 32)
        return "Printer";
    break;
    case 7: /* wearable */
    switch (minor) {
    case 1:
        return "Wrist Watch";
    case 2:
        return "Pager";
    case 3:
        return "Jacket";
    case 4:
        return "Helmet";
    case 5:
        return "Glasses";
    }
    break;
    case 8: /* toy */
    switch (minor) {
    case 1:
        return "Robot";
    case 2:
        return "Vehicle";
    case 3:
        return "Doll / Action Figure";
    case 4:
        return "Controller";
    case 5:
        return "Game";
    }
    break;
    case 63:	/* uncategorised */
    return "";
    }
    return "Unknown (reserved) minor device class";
}

void GattSrv::HCIclass(int hdev, char *opt) {
    static const char *services[] = { "Positioning",
                      "Networking",
                      "Rendering",
                      "Capturing",
                      "Object Transfer",
                      "Audio",
                      "Telephony",
                      "Information"
                    };
    static const char *major_devices[] = { "Miscellaneous",
                       "Computer",
                       "Phone",
                       "LAN Access",
                       "Audio/Video",
                       "Peripheral",
                       "Imaging",
                       "Uncategorized"
                     };
    int s = hci_open_dev(hdev);
    if (s < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (opt) {
    uint32_t cod = strtoul(opt, NULL, 16);
    if (hci_write_class_of_dev(s, cod, 2000) < 0) {
        BOT_NOTIFY_ERROR("Can't write local class of device on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    } else {
    uint8_t cls[3];
    if (hci_read_class_of_dev(s, cls, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read class of device on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    print_dev_hdr(&di);
    BOT_NOTIFY_DEBUG("\tClass: 0x%02x%02x%02x", cls[2], cls[1], cls[0]);
    BOT_NOTIFY_DEBUG("\tService Classes: ");
    if (cls[2]) {
        unsigned int i;
        int first = 1;
        for (i = 0; i < (sizeof(services) / sizeof(*services)); i++)
        if (cls[2] & (1 << i)) {
            if (!first)
            BOT_NOTIFY_DEBUG(", ");
            BOT_NOTIFY_DEBUG("%s", services[i]);
            first = 0;
        }
    } else
        BOT_NOTIFY_DEBUG("Unspecified");
    BOT_NOTIFY_DEBUG("\n\tDevice Class: ");
    if ((cls[1] & 0x1f) >= sizeof(major_devices) / sizeof(*major_devices))
        BOT_NOTIFY_DEBUG("Invalid Device Class!");
    else
        BOT_NOTIFY_DEBUG("%s, %s", major_devices[cls[1] & 0x1f],
           get_minor_device_name(cls[1] & 0x1f, cls[0] >> 2));
    }
}

void GattSrv::HCIvoice(int hdev, char *opt) {
    static char *icf[] = {	"Linear",
                "u-Law",
                "A-Law",
                "Reserved"
             };
    static char *idf[] = {	"1's complement",
                "2's complement",
                "Sign-Magnitude",
                "Reserved"
             };
    static char *iss[] = {	"8 bit",
                "16 bit"
             };
    static char *acf[] = {	"CVSD",
                "u-Law",
                "A-Law",
                "Reserved"
             };
    int s = hci_open_dev(hdev);
    if (s < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (opt) {
    uint16_t vs = htobs(strtoul(opt, NULL, 16));
    if (hci_write_voice_setting(s, vs, 2000) < 0) {
        BOT_NOTIFY_ERROR("Can't write voice setting on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    } else {
    uint16_t vs;
    uint8_t ic;
    if (hci_read_voice_setting(s, &vs, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read voice setting on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    vs = htobs(vs);
    ic = (vs & 0x0300) >> 8;
    print_dev_hdr(&di);
    BOT_NOTIFY_DEBUG("\tVoice setting: 0x%04x%s", vs,
           ((vs & 0x03fc) == 0x0060) ? " (Default Condition)" : "");
    BOT_NOTIFY_DEBUG("\tInput Coding: %s", icf[ic]);
    BOT_NOTIFY_DEBUG("\tInput Data Format: %s", idf[(vs & 0xc0) >> 6]);
    if (!ic) {
        BOT_NOTIFY_DEBUG("\tInput Sample Size: %s",
           iss[(vs & 0x20) >> 5]);
        BOT_NOTIFY_DEBUG("\t# of bits padding at MSB: %d",
           (vs & 0x1c) >> 2);
    }
    BOT_NOTIFY_DEBUG("\tAir Coding Format: %s", acf[vs & 0x03]);
    }
}

void GattSrv::HCIdelkey(int hdev, char *opt) {
    bdaddr_t bdaddr;
    bdaddr_t  _BDADDR_ANY = {{0, 0, 0, 0, 0, 0}};
    uint8_t all;
    int dd;
    if (!opt)
    return;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (!strcasecmp(opt, "all")) {
    bacpy(&bdaddr, &_BDADDR_ANY);
    all = 1;
    } else {
    str2ba(opt, &bdaddr);
    all = 0;
    }
    if (hci_delete_stored_link_key(dd, &bdaddr, all, 1000) < 0) {
    BOT_NOTIFY_ERROR("Can't delete stored link key on hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    hci_close_dev(dd);
}

void GattSrv::HCIoob_data(int hdev) {
    uint8_t hash[16], randomizer[16];
    int i, dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (hci_read_local_oob_data(dd, hash, randomizer, 1000) < 0) {
    BOT_NOTIFY_ERROR("Can't read local OOB data on hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    print_dev_hdr(&di);
    BOT_NOTIFY_DEBUG("\tOOB Hash:  ");
    for (i = 0; i < 16; i++)
    BOT_NOTIFY_DEBUG(" %02x", hash[i]);
    BOT_NOTIFY_DEBUG("\n\tRandomizer:");
    for (i = 0; i < 16; i++)
    BOT_NOTIFY_DEBUG(" %02x", randomizer[i]);
    BOT_NOTIFY_DEBUG("");
    hci_close_dev(dd);
}

void GattSrv::HCIcommands(int hdev) {
    uint8_t cmds[64];
    char *str;
    int i, n, dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (hci_read_local_commands(dd, cmds, 1000) < 0) {
    BOT_NOTIFY_ERROR("Can't read support commands on hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    print_dev_hdr(&di);
    for (i = 0; i < 64; i++) {
    if (!cmds[i])
        continue;
    BOT_NOTIFY_DEBUG("%s Octet %-2d = 0x%02x (Bit",
           i ? "\t\t ": "\tCommands:", i, cmds[i]);
    for (n = 0; n < 8; n++)
        if (cmds[i] & (1 << n))
        BOT_NOTIFY_DEBUG(" %d", n);
    BOT_NOTIFY_DEBUG(")");
    }
    str = hci_commandstostr(cmds, "\t", 71);
    BOT_NOTIFY_DEBUG("%s", str);
    bt_free(str);
    hci_close_dev(dd);
}

void GattSrv::HCIversion(int hdev) {
    struct hci_version ver;
    char *hciver, *lmpver;
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (hci_read_local_version(dd, &ver, 1000) < 0) {
    BOT_NOTIFY_ERROR("Can't read version info hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    hciver = hci_vertostr(ver.hci_ver);
    if (((di.type & 0x30) >> 4) == HCI_BREDR)
    lmpver = lmp_vertostr(ver.lmp_ver);
    else
    lmpver = pal_vertostr(ver.lmp_ver);
    print_dev_hdr(&di);
    BOT_NOTIFY_DEBUG("\tHCI Version: %s (0x%x)  Revision: 0x%x"
       "\t%s Version: %s (0x%x)  Subversion: 0x%x"
       "\tManufacturer: %s (%d)",
       hciver ? hciver : "n/a", ver.hci_ver, ver.hci_rev,
       (((di.type & 0x30) >> 4) == HCI_BREDR) ? "LMP" : "PAL",
       lmpver ? lmpver : "n/a", ver.lmp_ver, ver.lmp_subver,
       bt_compidtostr(ver.manufacturer), ver.manufacturer);
    if (hciver)
    bt_free(hciver);
    if (lmpver)
    bt_free(lmpver);
    hci_close_dev(dd);
}

void GattSrv::HCIinq_tpl(int hdev, char *opt) {
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (opt) {
    int8_t level = atoi(opt);
    if (hci_write_inquiry_transmit_power_level(dd, level, 2000) < 0) {
        BOT_NOTIFY_ERROR("Can't set inquiry transmit power level on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    } else {
    int8_t level;
    if (hci_read_inq_response_tx_power_level(dd, &level, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read inquiry transmit power level on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    print_dev_hdr(&di);
    BOT_NOTIFY_DEBUG("\tInquiry transmit power level: %d", level);
    }
    hci_close_dev(dd);
}

void GattSrv::HCIinq_mode(int hdev, char *opt) {
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (opt) {
    uint8_t mode = atoi(opt);
    if (hci_write_inquiry_mode(dd, mode, 2000) < 0) {
        BOT_NOTIFY_ERROR("Can't set inquiry mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    } else {
    uint8_t mode;
    if (hci_read_inquiry_mode(dd, &mode, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read inquiry mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    print_dev_hdr(&di);
    BOT_NOTIFY_DEBUG("\tInquiry mode: ");
    switch (mode) {
    case 0:
        BOT_NOTIFY_DEBUG("Standard Inquiry");
        break;
    case 1:
        BOT_NOTIFY_DEBUG("Inquiry with RSSI");
        break;
    case 2:
        BOT_NOTIFY_DEBUG("Inquiry with RSSI or Extended Inquiry");
        break;
    default:
        BOT_NOTIFY_DEBUG("Unknown (0x%02x)", mode);
        break;
    }
    }
    hci_close_dev(dd);
}

void GattSrv::HCIinq_data(int hdev, char *opt) {
    int i, dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (opt) {
    uint8_t fec = 0, data[HCI_MAX_EIR_LENGTH];
    char tmp[3];
    int i, size;
    memset(data, 0, sizeof(data));
    memset(tmp, 0, sizeof(tmp));
    size = (strlen(opt) + 1) / 2;
    if (size > HCI_MAX_EIR_LENGTH)
        size = HCI_MAX_EIR_LENGTH;
    for (i = 0; i < size; i++) {
        memcpy(tmp, opt + (i * 2), 2);
        data[i] = strtol(tmp, NULL, 16);
    }
    if (hci_write_ext_inquiry_response(dd, fec, data, 2000) < 0) {
        BOT_NOTIFY_ERROR("Can't set extended inquiry response on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    } else {
    uint8_t fec, data[HCI_MAX_EIR_LENGTH], len, type, *ptr;
    char *str;
    if (hci_read_ext_inquiry_response(dd, &fec, data, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read extended inquiry response on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    print_dev_hdr(&di);
    BOT_NOTIFY_DEBUG("\tFEC %s\n\t\t", fec ? "enabled" : "disabled");
    for (i = 0; i < HCI_MAX_EIR_LENGTH; i++)
        BOT_NOTIFY_DEBUG("%02x%s%s", data[i], (i + 1) % 8 ? "" : " ",
           (i + 1) % 16 ? " " : (i < 239 ? "\n\t\t" : ""));
    ptr = data;
    while (*ptr) {
        len = *ptr++;
        type = *ptr++;
        switch (type) {
        case 0x01:
        BOT_NOTIFY_DEBUG("\tFlags:");
        for (i = 0; i < len - 1; i++)
            BOT_NOTIFY_DEBUG(" 0x%2.2x", *((uint8_t *) (ptr + i)));
        BOT_NOTIFY_DEBUG("");
        break;
        case 0x02:
        case 0x03:
        BOT_NOTIFY_DEBUG("\t%s service classes:",
               type == 0x02 ? "Shortened" : "Complete");
        for (i = 0; i < (len - 1) / 2; i++) {
            uint16_t val = bt_get_le16((ptr + (i * 2)));
            BOT_NOTIFY_DEBUG(" 0x%4.4x", val);
        }
        BOT_NOTIFY_DEBUG("");
        break;
        case 0x08:
        case 0x09:
        str = (char*) malloc(len);
        if (str) {
            snprintf(str, len, "%s", ptr);
            for (i = 0; i < len - 1; i++) {
            if ((unsigned char) str[i] < 32 || str[i] == 127)
                str[i] = '.';
            }
            BOT_NOTIFY_DEBUG("\t%s local name: \'%s\'",
               type == 0x08 ? "Shortened" : "Complete", str);
            free(str);
        }
        break;
        case 0x0a:
        BOT_NOTIFY_DEBUG("\tTX power level: %d", *((int8_t *) ptr));
        break;
        case 0x10:
        BOT_NOTIFY_DEBUG("\tDevice ID with %d bytes data",
               len - 1);
        break;
        default:
        BOT_NOTIFY_DEBUG("\tUnknown type 0x%02x with %d bytes data",
               type, len - 1);
        break;
        }
        ptr += (len - 1);
    }
    BOT_NOTIFY_DEBUG("");
    }
    hci_close_dev(dd);
}

void GattSrv::HCIinq_type(int hdev, char *opt) {
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (opt) {
    uint8_t type = atoi(opt);
    if (hci_write_inquiry_scan_type(dd, type, 2000) < 0) {
        BOT_NOTIFY_ERROR("Can't set inquiry scan type on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    } else {
    uint8_t type;
    if (hci_read_inquiry_scan_type(dd, &type, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read inquiry scan type on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    print_dev_hdr(&di);
    BOT_NOTIFY_DEBUG("\tInquiry scan type: %s",
           type == 1 ? "Interlaced Inquiry Scan" : "Standard Inquiry Scan");
    }
    hci_close_dev(dd);
}

void GattSrv::HCIinq_parms(int hdev, char *opt) {
    struct hci_request rq;
    int s;
    if ((s = hci_open_dev(hdev)) < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    memset(&rq, 0, sizeof(rq));
    if (opt) {
    unsigned int window, interval;
    write_inq_activity_cp cp;
    if (sscanf(opt,"%4u:%4u", &window, &interval) != 2) {
        BOT_NOTIFY_DEBUG("Invalid argument format");
        exit(1);
    }
    rq.ogf = OGF_HOST_CTL;
    rq.ocf = OCF_WRITE_INQ_ACTIVITY;
    rq.cparam = &cp;
    rq.clen = WRITE_INQ_ACTIVITY_CP_SIZE;
    cp.window = htobs((uint16_t) window);
    cp.interval = htobs((uint16_t) interval);
    if (window < 0x12 || window > 0x1000)
        BOT_NOTIFY_DEBUG("Warning: inquiry window out of range!");
    if (interval < 0x12 || interval > 0x1000)
        BOT_NOTIFY_DEBUG("Warning: inquiry interval out of range!");
    if (hci_send_req(s, &rq, 2000) < 0) {
        BOT_NOTIFY_ERROR("Can't set inquiry parameters name on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    } else {
    uint16_t window, interval;
    read_inq_activity_rp rp;
    rq.ogf = OGF_HOST_CTL;
    rq.ocf = OCF_READ_INQ_ACTIVITY;
    rq.rparam = &rp;
    rq.rlen = READ_INQ_ACTIVITY_RP_SIZE;
    if (hci_send_req(s, &rq, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read inquiry parameters on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    if (rp.status) {
        BOT_NOTIFY_DEBUG("Read inquiry parameters on hci%d returned status %d",
           hdev, rp.status);
        exit(1);
    }
    print_dev_hdr(&di);
    window   = btohs(rp.window);
    interval = btohs(rp.interval);
    BOT_NOTIFY_DEBUG("\tInquiry interval: %u slots (%.2f ms), window: %u slots (%.2f ms)",
           interval, (float)interval * 0.625, window, (float)window * 0.625);
    }
}

void GattSrv::HCIpage_parms(int hdev, char *opt) {
    struct hci_request rq;
    int s;
    if ((s = hci_open_dev(hdev)) < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    memset(&rq, 0, sizeof(rq));
    if (opt) {
    unsigned int window, interval;
    write_page_activity_cp cp;
    if (sscanf(opt,"%4u:%4u", &window, &interval) != 2) {
        BOT_NOTIFY_DEBUG("Invalid argument format");
        exit(1);
    }
    rq.ogf = OGF_HOST_CTL;
    rq.ocf = OCF_WRITE_PAGE_ACTIVITY;
    rq.cparam = &cp;
    rq.clen = WRITE_PAGE_ACTIVITY_CP_SIZE;
    cp.window = htobs((uint16_t) window);
    cp.interval = htobs((uint16_t) interval);
    if (window < 0x12 || window > 0x1000)
        BOT_NOTIFY_DEBUG("Warning: page window out of range!");
    if (interval < 0x12 || interval > 0x1000)
        BOT_NOTIFY_DEBUG("Warning: page interval out of range!");
    if (hci_send_req(s, &rq, 2000) < 0) {
        BOT_NOTIFY_ERROR("Can't set page parameters name on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    } else {
    uint16_t window, interval;
    read_page_activity_rp rp;
    rq.ogf = OGF_HOST_CTL;
    rq.ocf = OCF_READ_PAGE_ACTIVITY;
    rq.rparam = &rp;
    rq.rlen = READ_PAGE_ACTIVITY_RP_SIZE;
    if (hci_send_req(s, &rq, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read page parameters on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    if (rp.status) {
        BOT_NOTIFY_DEBUG("Read page parameters on hci%d returned status %d",
           hdev, rp.status);
        exit(1);
    }
    print_dev_hdr(&di);
    window   = btohs(rp.window);
    interval = btohs(rp.interval);
    BOT_NOTIFY_DEBUG("\tPage interval: %u slots (%.2f ms), "
           "window: %u slots (%.2f ms)",
           interval, (float)interval * 0.625,
           window, (float)window * 0.625);
    }
}

void GattSrv::HCIpage_to(int hdev, char *opt) {
    struct hci_request rq;
    int s;
    if ((s = hci_open_dev(hdev)) < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    memset(&rq, 0, sizeof(rq));
    if (opt) {
    unsigned int timeout;
    write_page_timeout_cp cp;
    if (sscanf(opt,"%5u", &timeout) != 1) {
        BOT_NOTIFY_DEBUG("Invalid argument format");
        exit(1);
    }
    rq.ogf = OGF_HOST_CTL;
    rq.ocf = OCF_WRITE_PAGE_TIMEOUT;
    rq.cparam = &cp;
    rq.clen = WRITE_PAGE_TIMEOUT_CP_SIZE;
    cp.timeout = htobs((uint16_t) timeout);
    if (timeout < 0x01 || timeout > 0xFFFF)
        BOT_NOTIFY_DEBUG("Warning: page timeout out of range!");
    if (hci_send_req(s, &rq, 2000) < 0) {
        BOT_NOTIFY_ERROR("Can't set page timeout on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    } else {
    uint16_t timeout;
    read_page_timeout_rp rp;
    rq.ogf = OGF_HOST_CTL;
    rq.ocf = OCF_READ_PAGE_TIMEOUT;
    rq.rparam = &rp;
    rq.rlen = READ_PAGE_TIMEOUT_RP_SIZE;
    if (hci_send_req(s, &rq, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read page timeout on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    if (rp.status) {
        BOT_NOTIFY_DEBUG("Read page timeout on hci%d returned status %d",
           hdev, rp.status);
        exit(1);
    }
    print_dev_hdr(&di);
    timeout = btohs(rp.timeout);
    BOT_NOTIFY_DEBUG("\tPage timeout: %u slots (%.2f ms)",
           timeout, (float)timeout * 0.625);
    }
}

void GattSrv::HCIafh_mode(int hdev, char *opt) {
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (opt) {
    uint8_t mode = atoi(opt);
    if (hci_write_afh_mode(dd, mode, 2000) < 0) {
        BOT_NOTIFY_ERROR("Can't set AFH mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    } else {
    uint8_t mode;
    if (hci_read_afh_mode(dd, &mode, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read AFH mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    print_dev_hdr(&di);
    BOT_NOTIFY_DEBUG("\tAFH mode: %s", mode == 1 ? "Enabled" : "Disabled");
    }
}

void GattSrv::HCIssp_mode(int hdev, char *opt) {
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (opt) {
    uint8_t mode = atoi(opt);
    if (hci_write_simple_pairing_mode(dd, mode, 2000) < 0) {
        BOT_NOTIFY_ERROR("Can't set Simple Pairing mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    } else {
    uint8_t mode;
    if (hci_read_simple_pairing_mode(dd, &mode, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read Simple Pairing mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
        exit(1);
    }
    print_dev_hdr(&di);
    BOT_NOTIFY_DEBUG("\tSimple Pairing mode: %s",
           mode == 1 ? "Enabled" : "Disabled");
    }
}

void GattSrv::HCIrevision(int hdev) {
    struct hci_version ver;
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    return;
    }
    if (hci_read_local_version(dd, &ver, 1000) < 0) {
    BOT_NOTIFY_ERROR("Can't read version info for hci%d: %s (%d)", hdev, strerror(errno), errno);
    return;
    }
    return;
}

void GattSrv::HCIblock(int hdev, char *opt) {
    bdaddr_t bdaddr;
    int dd;
    if (!opt)
    return;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    str2ba(opt, &bdaddr);
    if (ioctl(dd, HCIBLOCKADDR, &bdaddr) < 0) {
    perror("ioctl(HCIBLOCKADDR)");
    exit(1);
    }
    hci_close_dev(dd);
}

void GattSrv::HCIunblock(int hdev, char *opt) {
    bdaddr_t bdaddr = {{0, 0, 0, 0, 0, 0}};
    bdaddr_t  _BDADDR_ANY = {{0, 0, 0, 0, 0, 0}};

    int dd;
    if (!opt)
    return;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
    BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
    exit(1);
    }
    if (!strcasecmp(opt, "all"))
    bacpy(&bdaddr, &_BDADDR_ANY);
    else
    str2ba(opt, &bdaddr);
    if (ioctl(dd, HCIUNBLOCKADDR, &bdaddr) < 0) {
    perror("ioctl(HCIUNBLOCKADDR)");
    exit(1);
    }
    hci_close_dev(dd);
}

void GattSrv::print_dev_hdr(struct hci_dev_info *di) {
    static int hdr = -1;
    char addr[18];
    if (hdr == di->dev_id)
    return;
    hdr = di->dev_id;
    ba2str(&di->bdaddr, addr);
    BOT_NOTIFY_DEBUG("%s:\tType: %s  Bus: %s", di->name,
       hci_typetostr((di->type & 0x30) >> 4),
       hci_bustostr(di->type & 0x0f));
    BOT_NOTIFY_DEBUG("\tBD Address: %s  ACL MTU: %d:%d  SCO MTU: %d:%d",
       addr, di->acl_mtu, di->acl_pkts,
       di->sco_mtu, di->sco_pkts);
}