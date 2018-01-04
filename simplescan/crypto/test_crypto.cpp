#include <iostream>
#include <iomanip>
#include <string>
#include <mutex>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#define TEST_SODIUM_DIRECT  0

#include "test_handshake.h"
#include "RingCrypto.hh"

static const uint8_t sign_priv[] = {
    0x2b,0xf8,0x4f,0x99,0x6a,0xfa,0xb6,0x53,0xd0,0x57,0x9f,0xb0,0x5a,0x7b,0xa2,0xd1,
    0xca,0xd2,0xa4,0x80,0x13,0x2d,0x98,0xf9,0x82,0xe0,0x1c,0x49,0x7c,0x84,0xa0,0x49,
    0xbb,0x30,0x38,0xee,0x8c,0x71,0x2b,0x47,0x86,0xfb,0x57,0x5c,0xa0,0x57,0xf6,0x81,
    0xa5,0xef,0x24,0xfa,0x49,0x8f,0x25,0xf4,0x8d,0xe1,0xe0,0xfa,0x49,0x70,0xc4,0x97
};
static const uint8_t sign_pub[] = {
    0xbb,0x30,0x38,0xee,0x8c,0x71,0x2b,0x47,0x86,0xfb,0x57,0x5c,0xa0,0x57,0xf6,0x81,
    0xa5,0xef,0x24,0xfa,0x49,0x8f,0x25,0xf4,0x8d,0xe1,0xe0,0xfa,0x49,0x70,0xc4,0x97
};

using namespace std;

void debug_print(const char* name, const Ring::ByteArr& arr)
{
    printf("%s (%zu bytes): ", name, arr.size());
    for (auto c: arr)
    {
        printf("%02x", c);
    }
    printf("\n");
}

int main()
{
#if TEST_SODIUM_DIRECT
    //  this code demonstrates how to use lib and prints all the internal data
    //  so the developer can compare and check bytes in output

    //  TODO move out debug print from library internals to users callback
    //  TODO in bright future this can be converted to unit tests

    //  use this to generate signing key pair that shoud be embedded into software
    //auto pp = Ring::GenSignPK();

    using Ring::ByteArr;

    //  we use *_rx and *_tx as dummy transport layer for handshake
    //  same applies to lambdas that are passed in constructors
    ByteArr client_rx;
    ByteArr client_tx;
    mutex rx_mutex;
    mutex tx_mutex;

    Ring::EncHandshake client(
        //  this is dummy client->server transport
        [&](const ByteArr& data)
        {
            lock_guard<mutex> lock(tx_mutex);
            client_tx = data;
        },
        //  this is dummy server->client transport
        [&]()
        {
            auto tmp = ByteArr{};
            while(tmp.size() == 0) {
                lock_guard<mutex> lock(rx_mutex);
                tmp = client_rx;
                this_thread::sleep_for(chrono::milliseconds(1));
            }
            return tmp;
        }
    );

    Ring::EncHandshake server(
        //  this is dummy server->client transport
        [&](const ByteArr& data)
        {
            lock_guard<mutex> lock(rx_mutex);
            client_rx = data;
        }
    );

    //  server operates in different thread (pretend this is different device)
    auto server_thread = thread(
        [&]()
        {
            //  waiting until client sends its public key
            auto tmp = ByteArr{};
            while(tmp.size() == 0) {
                lock_guard<mutex> lock(tx_mutex);
                tmp = client_tx;
                this_thread::sleep_for(chrono::milliseconds(1));
            }

            //  start handshake
            server.doHandshake(tmp);
        }
    );

    //  pretend we start handshake from smartphone
    client.doHandshake();

    //  wait until client and server do their shady job
    if (server_thread.joinable())
        server_thread.join();

    //  lets test secret shared key
    auto msg = ByteArr{0x01, 0x02, 0x03, 0x04, 0x5, 0x06, 0xa, 0xb, 0xc, 0xd, 0xf, 0x11, 0x22};
    debug_print("[main] msg", msg);

    auto encr = client.encrypt(msg);
    debug_print("[main] encrypted", encr);

    auto decr = server.decrypt(encr);
    debug_print("[main] decrypted", decr);

    //  this is to show how nonce is incremented internally
    for (int i = 0; i < 10; ++i)
        client.encrypt(msg);
#else

    char client_public[0xff];
    Ring::Ble::Crypto::Client client((char*) sign_pub, sizeof(sign_pub));
    int client_public_lenght = sizeof(client_public);
    client.GetPublicKey(client_public, client_public_lenght);

    // the prepared client_public key is supposed to be sent to server and used to init server
    Ring::Ble::Crypto::Server server(client_public, client_public_lenght, sign_priv);
    char server_public[0xff];
    int server_public_lenght = sizeof(server_public);
    server.GetPublicPayload(server_public, server_public_lenght);

    // the prepared server_public payload to be sent back to client and processed
    int ret = client.ProcessPublicPayload(server_public, server_public_lenght);
    if (ret != Ring::Ble::Crypto::Error::NO_ERROR)
    {
        // some error, abort
        printf("ret.ProcessPublicPayload != Crypto::Error::NO_ERROR) = %d\n", ret);
        return ret;
    }

    char msg[0xff] = "HelloServerMessage-fromClinet";
    char crypted[0xff];
    int crypted_len = sizeof(crypted);

    ret = client.Encrypt(msg, strlen(msg), crypted, crypted_len);
    if (ret != Ring::Ble::Crypto::Error::NO_ERROR)
    {
        // some error, abort
        printf("ret.Encrypt != Crypto::Error::NO_ERROR) = %d\n", ret);
        return ret;
    }

    char decrypted[0xff];
    int decrypted_len = sizeof(decrypted);

    // crypted is to be sent to server and decoded by it
    ret = server.Decrypt(crypted, crypted_len, decrypted, decrypted_len);
    if (ret != Ring::Ble::Crypto::Error::NO_ERROR)
    {
        // some error, abort
        printf("ret.Decrypt != Crypto::Error::NO_ERROR) = %d\n", ret);
        return ret;
    }
    printf("test ok: msg: %s\n", msg);
    Ring::Ble::Crypto::Debug::DumpArray("enc: ", Ring::Ble::Crypto::ByteArr(crypted, crypted + crypted_len));
    printf("test ok: dec: %s\n", decrypted);

    // here is test of backward functionality when server encrypt and client decrypt it


    sprintf(msg, "HelloClientMessage-fromServer");
    ret = server.Encrypt(msg, strlen(msg), crypted, crypted_len);
    if (ret != Ring::Ble::Crypto::Error::NO_ERROR)
    {
        // some error, abort
        printf("ret.Encrypt != Crypto::Error::NO_ERROR) = %d\n", ret);
        return ret;
    }

    // crypted is to be read by client and decoded
    ret = client.Decrypt(crypted, crypted_len, decrypted, decrypted_len);
    if (ret != Ring::Ble::Crypto::Error::NO_ERROR)
    {
        // some error, abort
        printf("ret.Decrypt != Crypto::Error::NO_ERROR) = %d\n", ret);
        return ret;
    }
    printf("test ok: msg: %s\n", msg);
    Ring::Ble::Crypto::Debug::DumpArray("enc: ", Ring::Ble::Crypto::ByteArr(crypted, crypted + crypted_len));
    printf("test ok: dec: %s\n", decrypted);

#endif

    return 0;
}
