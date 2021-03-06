/*
 *
 * Copyright 2018 Amazon.com, Inc. and its affiliates. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "icommon.h"

#define WAIT_FOREVER      0
#define SOCK_PORT_DEFAULT 2407
#define LOOPBACK_ADDR   ((char*) "127.0.0.1")
#define fdServerTx fdClientRx
#define fdClientTx fdServerRx

static int giPort = SOCK_PORT_DEFAULT;
static int fdServerRx = -1;
static int fdClientRx = -1;
static struct sockaddr_in gClient_addr;
static char *gsServerAdd = LOOPBACK_ADDR;
static bool gbInitialized = false;

#ifdef DEBUG_ENABLED
static const char *debug_msg[] =
{
    "MSG_SESSION",

    // controller / addapter
    "MSG_POWER",
    "MSG_CONFIG",

    // advertisement
    "MSG_ADVERTISEMENT",

    // GATT attribute operations
    "MSG_ADD_SERVICE",
    "MSG_ADD_ATTRIBUTE",
    "MSG_UPDATE_ATTRIBUTE",

    // Server notifications to Client
    "MSG_NOTIFY_CONNECT_STATUS",
    "MSG_NOTIFY_DATA_READ",
    "MSG_NOTIFY_DATA_WRITE",
};
static const char *err_msg[] =
{
    "NONE",
    "OPERATION_FAILED",
    "INVALID_COMMAND",
    "IGNORED",
    "FUNCTION",
    "TIMEOUT",
    "INVALID_PARAMETER",
    "NOT_INITIALIZED",
    "UNDEFINED",
    "NOT_IMPLEMENTED",
    "NOT_FOUND",
    "INVALID_STATE",
    "NOT_REGISTERED",
    "FAILED_INITIALIZE",
    "PTHREAD_ERROR",
    "RESOURCE_UNAVAILABLE",
    "CB_REGISTER_FAILED",
    "REGISTER_SVC_ERROR",
    "MEMORY_ALLOOCATION",
};
#endif

typedef struct comm_msgbuf
{
    __syscall_slong_t mtype;	/* type of received/sent message */
    Comm_Msg_t msg;
} Comm_Msgbuf_t;

/**
 * @brief die
 * @param s
 * @param err
 */
void die(const char *s, int err)
{
    DEBUG_PRINTF(("%s... err %d, %s", s, err, get_err_name(err)));
    perror(s);
    exit(1);
}

/**
 * @brief get_msg_name
 * @param cm
 * @return
 */
const char* get_msg_name(Comm_Msg_t *cm)
{
#ifdef DEBUG_ENABLED
    return cm->hdr.type < MSG_COUNT_MAX ? debug_msg[cm->hdr.type] : "unknown";
#endif
    return "";
}

/**
 * @brief get_err_name
 * @param ret
 * @return
 */
const char* get_err_name(int ret)
{
#ifdef DEBUG_ENABLED
    return ((Ble::Error::ERROR_COUNT_MAX < ret) && (ret <= 0)) ? err_msg[-ret] : "unknown";
#endif
    return "";
}

/**
 * @brief parse_command_line
 * @param argc
 * @param argv
 * @return
 */
int parse_command_line(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-p")) // port for UDP socket
        {
            int port = i+1<argc?atoi(argv[++i]):0;
            if (port > 0)
            {
                giPort = port;
                DEBUG_PRINTF(("set port %d\n", giPort));
            }
        }
        else if (!strcmp(argv[i], "-ip")) // Server IP for Client to connect
        {
            gsServerAdd = i+1<argc?argv[++i]:LOOPBACK_ADDR;
            if (gsServerAdd && strlen(gsServerAdd))
            {
                DEBUG_PRINTF(("set server ip %s\n", gsServerAdd));
            }
        }
        else
        {
            DEBUG_PRINTF(("invalid argument %s \nusage [-k key][-p port]\n", argv[i]));
            return Ble::Error::INVALID_PARAMETER;
        }
    }
    DEBUG_PRINTF(("configured to use %s %d [0x%08x]\n", gsServerAdd, giPort, giPort));
    return 0;
}

/*******************************************************************
 * Client-Server communication functions
 * *****************************************************************/
/**
 * @brief init_comm
 * @param bServer
 * @return
 */
int init_comm(bool bServer)
{
    if (!gbInitialized)
    {
        struct sockaddr_in rx_addr;

        // Creating listening socket file descriptor
        int *sockfd = bServer?&fdServerRx:&fdClientRx;
        if ( (*sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
        {
            DEBUG_RETURN(Ble::Error::FAILED_INITIALIZE);
        }

        memset(&rx_addr, 0, sizeof(rx_addr));

        // Binding to dedicated port
        int binding_port = giPort + (bServer?0:1);
        rx_addr.sin_family    = AF_INET; // IPv4
        rx_addr.sin_addr.s_addr = INADDR_ANY;
        rx_addr.sin_port = htons(binding_port);

        if ( bind(*sockfd, (const struct sockaddr *)&rx_addr, sizeof(rx_addr)) < 0 )
        {
            close(*sockfd);
            DEBUG_RETURN(Ble::Error::FAILED_INITIALIZE);
        }

        // create sending socket file descriptor
        sockfd = bServer?&fdServerTx:&fdClientTx;
        if ( (*sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
        {
            DEBUG_RETURN(Ble::Error::FAILED_INITIALIZE);
        }
        gbInitialized = true;
    }
    return Ble::Error::NONE;
}

/**
 * @brief send_comm
 * @param bServer
 * @param msg
 * @param size
 * @param bNotification
 * @return
 */
int send_comm(bool bServer, Comm_Msg_t *msg, int size, bool bNotification)
{
    if (!gbInitialized)
        return Ble::Error::NOT_INITIALIZED;

    struct sockaddr_in rx_addr;
    int dest_port = giPort + (bServer?(bNotification?1:0):0);
    int sockfd = bServer?fdServerTx:fdClientTx;

    rx_addr.sin_family = AF_INET;
    rx_addr.sin_port = !bServer?htons(dest_port): (bNotification ? htons(dest_port) : gClient_addr.sin_port);
    rx_addr.sin_addr.s_addr = !bServer ? inet_addr(gsServerAdd) : gClient_addr.sin_addr.s_addr;

    TRACE_DEBUG_PRINTF(("Responding client addr:port %d:%d", gClient_addr.sin_addr.s_addr, gClient_addr.sin_port));
    int bsent = sendto(sockfd, (const char *)msg, size, MSG_CONFIRM, (const struct sockaddr *) &rx_addr, sizeof(rx_addr));
    if (bsent != size)
    {
        DEBUG_RETURN(Ble::Error::OPERATION_FAILED);
    }
    return Ble::Error::NONE;
}

static int receive_from(bool bServer, Comm_Msg_t *buffer, int size, int timeout_ms, bool bNotification)
{
    if (!gbInitialized)
        return Ble::Error::NOT_INITIALIZED;

    struct sockaddr_in addr;
    socklen_t len = sizeof(struct sockaddr_in);

    memset(&addr, 0, sizeof(addr));

    int sockfd = bServer?fdServerRx:(bNotification?fdClientRx:fdClientTx);

    struct timeval tv;
    tv.tv_sec = timeout_ms > 0 ? ((int)timeout_ms/1000):0xFFFF;
    tv.tv_usec = timeout_ms > 0 ? (timeout_ms%1000)*1000:0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt error");
    }

    int brecv = recvfrom(sockfd, (char *)buffer, size, MSG_WAITALL, (struct sockaddr *) &addr, &len);
    if (brecv <= 0)
    {
        if ((timeout_ms > 0) && (brecv == -1))
            return Ble::Error::TIMEOUT;
        else
            return Ble::Error::OPERATION_FAILED;
    }

    // save client address for Server to respond
    if (bServer)
    {
        gClient_addr = addr;
        TRACE_DEBUG_PRINTF(("saving client addr:port %d:%d", gClient_addr.sin_addr.s_addr, gClient_addr.sin_port));
    }
    return Ble::Error::NONE;
}

/**
 * @brief send_to_server
 * @param buffer
 * @param size
 * @return
 */
int send_to_server(Comm_Msg_t *buffer, int size)
{
    return send_comm(TO_SERVER, buffer, size, false);
}

/**
 * @brief recv_from_client
 * @param buffer
 * @param size
 * @return
 */
int recv_from_client(Comm_Msg_t * buffer, int size)
{
    return receive_from(FROM_CLIENT, buffer, size, WAIT_FOREVER, false);
}

/**
 * @brief resp_to_client
 * @param msg
 * @param size
 * @return
 */
int resp_to_client(Comm_Msg_t *msg, int size)
{
    return send_comm(TO_CLIENT, msg, size, false);
}

/**
 * @brief resp_from_server
 * @param msg
 * @param size
 * @param timeout_ms
 * @return
 */
int resp_from_server(Comm_Msg_t *msg, int size, int timeout_ms)
{
    return receive_from(FROM_SERVER, msg, size, timeout_ms, false);
}

/**
 * @brief notify_client
 * @param msg
 * @param size
 * @return
 */
int notify_client(Comm_Msg_t *msg, int size)
{
    return send_comm(TO_CLIENT, msg, size, true);
}

/**
  * @brief wait_notification
  * @param buffer
  * @param size
  * @param timeout_ms
  * @return
  */
 int wait_notification(Comm_Msg_t *buffer, int size, int timeout_ms)
 {
    return receive_from(FROM_SERVER, buffer, size, timeout_ms, true);
 }


 /**
 * @brief shut_comm
 * @return
 */
int shut_comm()
{
    if (gbInitialized)
    {
        close(fdClientRx);
        close(fdServerRx);
        fdClientRx = fdServerRx = -1;
        gbInitialized = false;
    }
    return Ble::Error::NONE;
}

/*******************************************************************
 * Helper & formatting functions
 * *****************************************************************/

/**
 * @brief format_attr_add_msg
 * @param stash
 * @param attr_new
 * @return
 */
Comm_Msg_t *format_attr_add_msg(Comm_Msg_t *stash, Define_Attribute_t *attr_new)
{
    if (stash && attr_new)
    {
        Comm_Msg_t *msg = stash;
        msg->hdr.type = MSG_ADD_ATTRIBUTE;
        msg->hdr.size = sizeof(Common_Header_t) + sizeof(Add_Attribute_t);
        msg->data.add_attribute = *((Add_Attribute_t*)attr_new);
        msg->data.add_attribute.data[0]='\0';

        if (attr_new->size > 0)
        {   // re-alloc message to include value
            msg->hdr.size += attr_new->size-1;
            msg = (Comm_Msg_t *) malloc(msg->hdr.size);
            if (msg)
            {
                memcpy(msg, stash, stash->hdr.size); // copy header and values
                memcpy(msg->data.add_attribute.data, attr_new->data, attr_new->size);
                return msg;
            }
        }
    }
    return NULL;
}

/**
 * @brief format_attr_updated_msg
 * @param type
 * @param stash
 * @param attr_new
 * @return
 */
Comm_Msg_t *format_attr_updated_msg(Msg_Type_t type, Comm_Msg_t *stash, Define_Update_t *attr_new)
{
    if (stash && attr_new)
    {
        Comm_Msg_t *msg = stash;
        msg->hdr.type = type;
        msg->hdr.size = sizeof(Common_Header_t) + sizeof(Update_Attribute_t);
        msg->data.update_attribute.attr_idx = attr_new->attr_idx;
        msg->data.update_attribute.size = attr_new->size;

        if (attr_new->size > 1)
        {   // re-alloc message to include value
            int new_size = msg->hdr.size + attr_new->size - 1;
            msg = (Comm_Msg_t*) malloc(new_size);
            if (msg)
            {
                memcpy(msg, stash, stash->hdr.size); // copy header and values
                memcpy(msg->data.update_attribute.data, attr_new->data, attr_new->size);
                msg->hdr.size = stash->hdr.size = new_size;
            }
            else
            {   // Notify Client about Error
                DEBUG_PRINTF(("ERROR: Notify Client Ble::Property::Access::Write failed to allocate memory"));
                msg = stash; // restore pointer to static stash
                msg->hdr.error = Ble::Error::MEMORY_ALLOOCATION;
            }
        }
        else
        {
            msg->data.notify_data_write.data[0] = (attr_new->data != NULL)?*((char*)attr_new->data):0;
        }

        // Note: the calling function is responsible to
        // free memory if returned msg != stash
        return msg;
    }
    return NULL;
}

/**
 * @brief format_message_payload
 * @param session_id
 * @param type
 * @param msg
 * @param data
 * @return
 */
void* format_message_payload(uint16_t session_id, Msg_Type_t type, Comm_Msg_t &msg, void* data)
{
    Comm_Msg_t *msg_new = NULL;
    msg.hdr.size = sizeof(Common_Header_t);
    msg.hdr.error = Ble::Error::NONE;
    msg.hdr.session_id = session_id;
    msg.hdr.type = type;

    switch (type)
    {
    case MSG_SESSION:
        msg.hdr.size += sizeof(Session_t);
        msg.data.session.on_off = (uint8_t)(unsigned long)data;
        break;

    case MSG_POWER:
        msg.hdr.size += sizeof(Power_t);
        msg.data.power.on_off = (uint8_t)(unsigned long)data;
        break;

    case MSG_CONFIG:
        msg.hdr.size += sizeof(Config_t);
        msg.data.config = *((Config_t*)data);
        break;

    case MSG_ADVERTISEMENT:
        msg.hdr.size += sizeof(Advertisement_t);
        msg.data.advertisement.on_off = (uint8_t)(unsigned long)data;
        break;

    case MSG_ADD_SERVICE:
        msg.hdr.size += sizeof(Add_Service_t);
        msg.data.add_service = *((Add_Service_t*) data);
        break;

    case MSG_ADD_ATTRIBUTE:
        msg_new = format_attr_add_msg(&msg, (Define_Attribute_t*) data);
        break;

    case MSG_UPDATE_ATTRIBUTE:
        msg_new = format_attr_updated_msg(MSG_UPDATE_ATTRIBUTE, &msg, (Define_Update_t*) data);
        break;

    case CMD_SERVER_EXIT: // for debugging
        break;

    case MSG_NOTIFY_CONNECT_STATUS:
    case MSG_NOTIFY_DATA_READ:
    case MSG_NOTIFY_DATA_WRITE:
    default:
        DEBUG_PRINTF(("WARNING! Wrong handler - Notification and commands are not processed here\n"));
        break;
    }
    return msg_new;
}
