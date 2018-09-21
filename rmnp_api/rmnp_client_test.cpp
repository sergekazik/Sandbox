#include <unistd.h>
#include "rmnp_api.h"
#include <string>

static volatile bool done = false;
static void my_callback(Rmnp_Callback_Type_t type, int attr_idx, void* data, int size)
{
    printf("%s waked up with type = %d\n", __FUNCTION__, type);
    switch (type)
    {
    case Server_disconnected:
        printf("callback for Server_disconnected\n");
        done = true;
        break;
    case Server_connected:
        printf("callback for Server_connected\n");
        break;
    case Attr_data_read:
        printf("callback for Attr_data_read attr idx %d\n", attr_idx);
        break;
    case Attr_data_write:
    {
        std::string val((char*) data, size);
        printf("callback for Attr_data_write attr idx %d size %d val %s\n", attr_idx, size, val.c_str());
    }
        break;
    default:
        printf("unexpected type, ignored\n");
        break;
    }
}

int main()
{
    Rmnp_Error_t ret = NO_ERROR;
    uint8_t svc_uuid[16] = {0x97,0x60,0xAB,0xBA,0xA2,0x34,0x46,0x86,0x9E,0x00,0xFC,0xBB,0xEE,0x33,0x73,0xF7};
    Attr_Define_t attr_list[3] = {
        {{0x97,0x60,0xAB,0xBA,0xA2,0x34,0x46,0x86,0x9E,0x20,0xD0,0x87,0x33,0x3C,0x2C,0x01},"attr-1", 64, 6, CHR, RWN, "value1"},
        {{0x97,0x60,0xAB,0xBA,0xA2,0x34,0x46,0x86,0x9E,0x20,0xD0,0x87,0x33,0x3C,0x2C,0x02},"attr-2", 64, 6, CHR, RWN, "value2"},
        {{0x00,0x00,0x29,0x02,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5F,0x9B,0x34,0xFB},"descrp", 32, 2, DSC, RW_, "\x01\x00"}
    };

    if (NO_ERROR != (ret = rmnp_init()))
        printf("failed to init, err = %d\n", ret);
    else if (NO_ERROR != (ret = rmnp_config(0, "SimpleSample", "34:45:56:67:78:8B")))
        printf("failed to config, err = %d\n", ret);
    else if (NO_ERROR != (ret = rmnp_register_callback(my_callback)))
        printf("failed to register callback, err = %d\n", ret);
    else if (NO_ERROR != (ret = rmnp_add_service(svc_uuid, sizeof(attr_list)/sizeof(Attr_Define_t))))
        printf("failed to add service, err = %d\n", ret);
    else if (NO_ERROR != (ret = rmnp_add_attribute(attr_list[0])))
        printf("failed to add attr, err = %d\n", ret);
    else if (NO_ERROR != (ret = rmnp_add_attribute(attr_list[1])))
        printf("failed to add attr, err = %d\n", ret);
    else if (NO_ERROR != (ret = rmnp_add_attribute(attr_list[2])))
        printf("failed to add attr, err = %d\n", ret);
    else if (NO_ERROR != (ret = rmnp_start_advertisement()))
        printf("failed to shutdown, err = %d\n", ret);

    printf("waiting for callback events.....\n");
    while (!ret && !done) {
        sleep(3);
    }

    printf("continue operations - exiting...\n");
    if (NO_ERROR != (ret = rmnp_stop_advertisement()))
        printf("failed to shutdown, err = %d\n", ret);
    else if (NO_ERROR != (ret = rmnp_shutdown()))
        printf("failed to shutdown, err = %d\n", ret);
    return ret;
}
