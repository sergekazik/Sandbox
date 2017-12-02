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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

#include "version.h"
#include "gatt_test_srv.h"

#define COMM_BUF_LEN    1024
#define VALIDATE_AND_EXEC_ARGUMENT(_arg, _val, _cmd ) (!strcmp(_arg, _val)) { execute_hci_cmd(_cmd); }

#ifdef BLUEZ_TOOLS_SUPPORT

///
/// \brief raw_test_listen
/// \return
///
int raw_test_listen(void)
{
    struct sockaddr_rc loc_addr = { 0 }, rem_addr = { 0 };
    char buf[COMM_BUF_LEN] = { 0 };
    int s, client, bytes_read;
    socklen_t opt = sizeof(rem_addr);
    bdaddr_t bdaddr_any = {{0, 0, 0, 0, 0, 0}};

    // allocate socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // bind socket to port 1 of the first available
    // local bluetooth adapter
    loc_addr.rc_family = AF_BLUETOOTH;
    loc_addr.rc_bdaddr = bdaddr_any;
    loc_addr.rc_channel = (uint8_t) 1;

    bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));

    // put socket into listening mode
    listen(s, 1);

    // accept one connection
    client = accept(s, (struct sockaddr *)&rem_addr, &opt);

    ba2str( &rem_addr.rc_bdaddr, buf );
    fprintf(stderr, "accepted connection from %s\n", buf);
    memset(buf, 0, sizeof(buf));

    // read data from the client
    bytes_read = read(client, buf, sizeof(buf));
    if( bytes_read > 0 ) {
        printf("received [%s]\n", buf);

        // ACK
        sprintf(&buf[strlen(buf)], " - acknowledged");
        int len = strlen(buf);
        printf("sending ACK \"%s\"\n", buf);
        int status = write(client, buf, len);
        printf("sent %d byte, status %d\n", len, status);
    }

    // close connection
    close(client);
    close(s);
    return 0;
}

///
/// \brief raw_test_connect
/// \param dest
/// \param data
/// \param nLen
/// \param nRepeat
/// \return
///
int raw_test_connect(char *dest, const char *data, int nLen, int nRepeat)
{
    char buf[COMM_BUF_LEN] = { 0 };
    struct sockaddr_rc addr = { 0 };
    int s, status;
    // e.g. char dest[18] = "01:23:45:67:89:AB";
    const char *data_stored = data;
    int nLen_stored = nLen;

    printf("connecting to %s\n", dest);
    // allocate a socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t) 1;
    str2ba( dest, &addr.rc_bdaddr );

    // connect to server
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));

    while (nRepeat-- > 0)
    {
        // send a message
        if( status == 0 ) {
            int len = nLen ? nLen : (int) strlen(data);
            sprintf(buf, "%d.%s", nRepeat, data);
            data = buf;
            printf("sending to %s:\n%s\n", dest, data);
            status = write(s, data, len);
            printf("sent %d byte, status %d\n", len, status);
        }

        if ( status < 0 )
        {
            perror("uh oh");
            break;
        }
        else
        {
            int bytes_read = read(s, buf, sizeof(buf));
            printf("// read ACK data from the server = %d bytes\n", bytes_read);

            if( bytes_read > 0 ) {
                printf("received %d bytes from GATT(?):\n", bytes_read);
                printf("ASCII: [%s]\n", buf);
                if ((int) strlen(buf) == bytes_read)
                {
                    for (int i = 0; i < bytes_read; i++)
                    {
                        printf("%c", (buf[i] >= ' ') ? buf[i] : ' ');
                    }
                }
                printf("\n");

                for (int i = 0; i < bytes_read; i++)
                {
                    printf("%02X ", buf[i]);
                }
                printf("\n");

//                data = buf;
//                nLen = bytes_read;
                data = data_stored;
                nLen = nLen_stored;
            }
            else
            {
                data = data_stored;
                nLen = nLen_stored;
            }

        }
        status = 0;
    } // rnd of while repeat

    close(s);
    return status >= 0 ? 0 : status;
}

///
/// \brief raw_test_scan
/// \return
///
int raw_test_scan(void)
{
    inquiry_info *ii = NULL;
    int max_rsp, num_rsp;
    int dev_id, sock, len, flags;
    int i;
    char addr[19] = { 0 };
    char name[248] = { 0 };

    dev_id = hci_get_route(NULL);
    sock = hci_open_dev( dev_id );
    if (dev_id < 0 || sock < 0) {
        perror("opening socket");
        exit(1);
    }

    len  = 8;
    max_rsp = 255;
    flags = IREQ_CACHE_FLUSH;
    ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

    num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
    if( num_rsp < 0 ) perror("hci_inquiry");

    for (i = 0; i < num_rsp; i++) {
        ba2str(&(ii+i)->bdaddr, addr);
        memset(name, 0, sizeof(name));
        if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name),
            name, 0) < 0)
        strcpy(name, "[unknown]");
        printf("%s  %s\n", addr, name);
    }

    free( ii );
    close( sock );
    return 0;
}

#endif //  BLUEZ_TOOLS_SUPPORT


///
/// \brief print_help
///
void print_help(void)
{
    printf("********************************************************\n");
    printf("* BT/LE test tool (date %s)\n", version_date);
    printf("********************************************************\n");
#ifdef BLUEZ_TOOLS_SUPPORT
    printf("--scan                  scan for bluetooth device\n");
    printf("--listen                listen for incoming connection, read once\n");
    printf("--conn <dev_addr>       connect to device MAC address, write once\n");
    printf("--send <dev_addr> [\"txt] connect and send custom text once\n");
    printf("------------------------------------------------\n");
#endif
#if defined(BCM43) || defined(Linux_x86_64)
    printf("--up                    hciconfig hci0 up\n");
    printf("--down                  hciconfig hci0 down\n");
    printf("--piscan                hciconfig hci0 piscan\n");
    printf("--noscan                hciconfig hci0 noscan\n");
    printf("--leadv                 hciconfig hci0 leadv\n");
    printf("--noleadv               hciconfig hci0 noleadv\n");
    printf("--class                 hciconfig hci0 class 0x280430\n");
    printf("--hciinit               up, piscan, class 0x280430, leadv\n");
    printf("--hcishutdown           noleadv, noscan, down\n");
    printf("------------------------------------------------\n");
#endif  // not "else if"!
#if defined(WILINK18) || defined(Linux_x86_64)
    printf("--gattauto              run Bluetopia GATT Server sample\n");
    printf("--gatt                  load Bluetopia GATT Server sample\n");
    printf("------------------------------------------------\n");
#endif

}

///
/// \brief main
/// \param argc
/// \param argv
/// \return
///
int main(int argc, char **argv)
{
    int arg_idx, ret = 0;

    for (arg_idx = argc-1; arg_idx > 0; arg_idx--)
    {
        if (0) {;} // plaseholder for following "else if"

#ifdef BLUEZ_TOOLS_SUPPORT
        else if (!strcmp(argv[arg_idx], "--scan"))
        {
            ret = raw_test_scan();
            break;
        }
        else if (!strcmp(argv[arg_idx], "--listen"))
        {
            ret = raw_test_listen();
            break;
        }
        else if (!strcmp(argv[arg_idx], "--conn"))
        {
            if (arg_idx + 1 < argc)
            {
                char *dev_addr = argv[arg_idx+1];
                ret = raw_test_connect(dev_addr, "hello ring 123!", 0, 1);
            }
            else
            {
                printf("--conn missing <dev_addr>...\n");
                ret = -100;
            }
            break;
        }
        else if (!strcmp(argv[arg_idx], "--send"))
        {
            if (arg_idx + 1 < argc)
            {
                char dtdf[] = {"hello ring 123!"};
                char *dt = dtdf;

                char mac_addr[32];
                int repeat = 1;
                int rx = 0;
                int ll = 0;

                char *dev_addr = argv[arg_idx+1];

                // if address is not :-separated add ':'
                if (!strchr(dev_addr, ':'))
                {
                    memset(mac_addr, 0, sizeof(mac_addr));
                    for (int a = 0, b = 0; b < (int) strlen(dev_addr); b++)
                    {
                        mac_addr[a++] = dev_addr[b];
                        if ((b % 2) && (b+1 < (int) strlen(dev_addr)))
                            mac_addr[a++] = ':';
                    }
                    dev_addr = mac_addr;
                }

                if (arg_idx + 2 < argc)
                {
                    // send text payload if the argument is starting \"helloxxx
                    if (argv[arg_idx + 2][0] == '"')
                    {
                        dt = &argv[arg_idx + 2][1];
                    }
                    // repeat n times
                    else if (1 == sscanf(argv[arg_idx + 2], "%d", &rx))
                    {
                        repeat = rx;
                    }
                }
                ret = raw_test_connect(dev_addr, (const char*) dt, ll, repeat);
            }
            else
            {
                printf("--conn missing <dev_addr>...\n");
                ret = -100;
            }
            break;
        }
        // perform HCI commands:
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--up", eConfig_UP)
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--down", eConfig_DOWN)
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--piscan", eConfig_PISCAN)
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--noscan", eConfig_NOSCAN)
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--leadv", eConfig_LEADV)
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--noleadv", eConfig_NOLEADV)
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--class", eConfig_CLASS)
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--hciinit", eConfig_ALLUP)
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--hcishutdown", eConfig_ALLDOWN)

#endif // BLUEZ_TOOLS_SUPPORT

#if defined(WILINK18) || defined(Linux_x86_64)
        else if (!strcmp(argv[arg_idx], "--gattauto"))
        {
            ret = gatt_server_start("--autoinit");
        }
        else if (!strcmp(argv[arg_idx], "--gatt"))
        {
            ret = gatt_server_start(NULL);
        }
#endif // defined(WILINK18) || defined(Linux_x86_64)
    }

    if (!arg_idx)
    {
        print_help();
    }

    printf("%s ----- done; ret val=%d\n", argv[0], ret);
    return ret;
}
