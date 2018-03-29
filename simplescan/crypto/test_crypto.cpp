#include <iostream>
#include <iomanip>
#include <string>
#include <mutex>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sodium.h>

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define TEST_SODIUM_DIRECT  1
#define KEY_SIZE            32

#include "version.h"
#include "test_handshake.h"
#include "RingCrypto.hh"

#define IO_MSG_BUFFER_SIZE 2000
static const uint8_t sign_priv[] = {
    0x2b,0x3a,0x3d,0x35,0x75,0xe5,0x94,0xec,0x77,0xf5,0xeb,0x96,0xf3,0xb9,0xda,0xc6,
    0x8a,0x00,0x21,0xdd,0x5a,0x9c,0x15,0xf0,0x0e,0xa7,0x46,0xc0,0xf8,0x21,0x22,0x38,
    0x9f,0x9c,0x2c,0x9a,0xc1,0xbd,0x07,0x7d,0xd9,0x2f,0xeb,0xa3,0x89,0x34,0x5e,0x0a,
    0x11,0xa3,0x85,0x72,0x84,0x66,0x7a,0xc4,0x85,0x56,0xf9,0x2d,0x36,0x0f,0xdf,0x97,
};
static const uint8_t sign_pub[] = {
    0x9f,0x9c,0x2c,0x9a,0xc1,0xbd,0x07,0x7d,0xd9,0x2f,0xeb,0xa3,0x89,0x34,0x5e,0x0a,
    0x11,0xa3,0x85,0x72,0x84,0x66,0x7a,0xc4,0x85,0x56,0xf9,0x2d,0x36,0x0f,0xdf,0x97,
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

static bool print_help(char* arg)
{
    if (!strcmp(arg, "--help") || !strcmp(arg, "-h") || !strcmp(arg, "?"))
    {
        printf("test_crypto v. %s\nusage:\n--------------------------------------\n",version_date);
        printf("-gen            generate pub/sec keys pair\n");
        printf("-gpk            get public key\n");
        printf("-ppp <payload>  process public payload\n");
        printf("-enc <input>    encrypt input\n");
        printf("-dec <input>    decrypt input\n");
        printf("--------------------------------------\nExamples:\n");
        printf("$ test_crypto -gpk\n");
        printf("gpk:220 195 210 183 120 127 217 131 175 141 4 23 235 56 157 134 114 146 242 122 65 220 213 246 208 58 221 169 142 230 91 97:gpk\n");
        printf("$ test_crypto -ppp 0c59aa12a31f3eff0ac5948dab17cd39efb07dd8abcfe8d2409f62edfa634661f2e519e02086b04f0e615b3aa628d7a1c651e51ff8796528c54b36b8c304e62a256537e6b3f0d8ea8d1ae54cc462fa620c0bec8d657a36b7202368b884ee6007c8063e3b8eb5ac85c75030d86229656700675b12\n");
        printf("ppp:0c59aa12a31f3eff0ac5948dab17cd39efb07dd8abcfe8d2409f62edfa634661f2e519e02086b04f0e615b3aa628d7a1c651e51ff8796528c54b36b8c304e62a256537e6b3f0d8ea8d1ae54cc462fa620c0bec8d657a36b7202368b884ee6007c8063e3b8eb5ac85c75030d86229656700675b12\n");
        printf("$ test_crypto -enc HelloWorld\n");
        printf("msg:HelloWorld\n");
        printf("enc:0100000074da7d314bef2fb44aacea563a75c8eab3ee1bd13911b7141d0d\n");
        printf("$ test_crypto -dec 0100000074da7d314bef2fb44aacea563a75c8eab3ee1bd13911b7141d0d\n");
        printf("enc:0100000074da7d314bef2fb44aacea563a75c8eab3ee1bd13911b7141d0d\n");
        printf("dec:HelloWorld\n");

        return true;
    }
    return false;
}

static bool print_gen(char *arg)
{
    if (!strcmp(arg, "--generate") || !strcmp(arg, "-gen"))
    {
        (void) sodium_init();
        unsigned char pk[KEY_SIZE];
        unsigned char sk[KEY_SIZE];

        crypto_box_seed_keypair(pk, sk, (const unsigned char*) "3245;lkjh fd[0g946 /ljf"); // just string to seed
        crypto_sign_keypair(pk, sk);

        Ring::ByteArr tpk(pk, pk + KEY_SIZE);
        Ring::ByteArr tsk(sk, sk + KEY_SIZE);
        debug_print("pub", tpk);
        debug_print("prv", tsk);
        return true;
    }
    return false;
}

#if !TEST_SODIUM_DIRECT
int main(int argc, char* argv[])
{
    bool bServerCmdline = false;
    char outstr[IO_MSG_BUFFER_SIZE];
    if (argc > 1)
    {
        if (print_help(argv[1]))
            return Ring::Ble::Crypto::Error::NO_ERROR;
        if (print_gen(argv[1]))
            return Ring::Ble::Crypto::Error::NO_ERROR;

        // suppress all debug if in the command mode
        Ring::Ble::Crypto::Debug::Suppress(true);
    }

    char client_public[IO_MSG_BUFFER_SIZE];
    char server_public[IO_MSG_BUFFER_SIZE];
    int server_public_lenght = sizeof(server_public);
    char crypted[IO_MSG_BUFFER_SIZE];
    int crypted_len = sizeof(crypted);
    char decrypted[IO_MSG_BUFFER_SIZE];
    int decrypted_len = sizeof(decrypted);
    char msg[IO_MSG_BUFFER_SIZE] = "HelloServerMessage-fromClient";

    memset(client_public, 0, IO_MSG_BUFFER_SIZE);
    memset(server_public, 0, IO_MSG_BUFFER_SIZE);

    Ring::Ble::Crypto::Client client((char*) sign_pub, sizeof(sign_pub));
    int client_public_lenght = sizeof(client_public);

    if (argc < 2 || (argc > 1 && (!strcmp(argv[1], "-gpk")||!strcmp(argv[1], "-gpw"))))
        client.GetPublicKey(client_public, client_public_lenght);

    // single command mode client get public key
    if (argc > 1 && (!strcmp(argv[1], "-gpk")||!strcmp(argv[1], "-gpw")))
    {
        memset(outstr, 0, IO_MSG_BUFFER_SIZE);

        int offset = sprintf(outstr, "HEX|");
        for (int i = 0; i < client_public_lenght; i++)
            offset += sprintf(&outstr[offset], "%s%02X", i?" ":"", (unsigned char) client_public[i]);
        sprintf(&outstr[offset], "|HEX");
        printf("%s\n", outstr);

        offset = sprintf(outstr, "SER|");
        for (int i = 0; i < client_public_lenght; i++)
            offset += sprintf(&outstr[offset], "%02X", (unsigned char) client_public[i]);
        sprintf(&outstr[offset], "|SER");
        printf("%s\n", outstr);

        offset = sprintf(outstr, "gpk:");
        for (int i = 0; i < client_public_lenght; i++)
            offset += sprintf(&outstr[offset], "%s%d", i?" ":"", (unsigned char) client_public[i]);
        sprintf(&outstr[offset], ":gpk");
        printf("%s\n", outstr);

        if (!strcmp(argv[1], "-gpw"))
        {
            printf("DEC>");
            cin >> outstr;
            server_public_lenght = strlen(outstr) / 2;
            for (int idx = 0; idx < server_public_lenght; idx++)
            {
                int val;
                sscanf(&outstr[idx*2], "%02x", &val);
                server_public[idx] = (unsigned char) val;
            }
            client.ProcessPublicPayload(server_public, server_public_lenght);
            client.Encrypt("hello", 5, crypted, crypted_len);

            memset(outstr, 0, IO_MSG_BUFFER_SIZE);

            offset = sprintf(outstr, "HEX|");
            for (int i = 0; i < crypted_len; i++)
                offset += sprintf(&outstr[offset], "%s%02X", i?" ":"", (unsigned char) crypted[i]);
            sprintf(&outstr[offset], "|HEX");
            printf("%s\n", outstr);
            offset = sprintf(outstr, "DEC|");
            for (int i = 0; i < crypted_len; i++)
                offset += sprintf(&outstr[offset], "%02X", (unsigned char) crypted[i]);
            sprintf(&outstr[offset], "|DEC");
            printf("%s\n", outstr);

            offset = sprintf(outstr, "enc:");
            for (int i = 0; i < crypted_len; i++)
                offset += sprintf(&outstr[offset], "%s%d", i?" ":"", (unsigned char) crypted[i]);
            sprintf(&outstr[offset], ":enc");
            printf("%s\n", outstr);
        }
        client.SaveSecrets();
        return Ring::Ble::Crypto::Error::NO_ERROR;
    }

    if (argc > 2 && !strcmp(argv[1], "-server"))
    {
        // argv[1], "-server"
        // argv[2], "gpk" key from client no spaces
        // argv[3], "payload to decode" no spaces
        bServerCmdline = true;
        strcpy(client_public, argv[2]);
        client_public_lenght = strlen(client_public);
    }

    // the prepared client_public key is supposed to be sent to server and used to init server
    Ring::Ble::Crypto::Server server(client_public, client_public_lenght, sign_priv);
    server.GetPublicPayload(server_public, server_public_lenght);
    // the prepared server_public payload to be sent back to client and processed

    if (bServerCmdline)
    {
        memset(outstr, 0, IO_MSG_BUFFER_SIZE);

        if (argc == 3) // asking for PublicPayload only
        {
            int offset = sprintf(outstr, "PPP|");
            for (int i = 0; i < server_public_lenght; i++)
                offset += sprintf(&outstr[offset], "%02X", (unsigned char) server_public[i]);
            sprintf(&outstr[offset], "|PPP");
            printf("%s\n", outstr);

            offset = sprintf(outstr, "gpp:");
            for (int i = 0; i < server_public_lenght; i++)
                offset += sprintf(&outstr[offset], "%s%d", i?" ":"", (unsigned char) server_public[i]);
            sprintf(&outstr[offset], ":gpp");
            printf("%s\n", outstr);

            printf("DEC>");
            cin >> outstr;

//            while (outstr[strlen(outstr)-1] == '\n' || outstr[strlen(outstr)-1] == '\r')
//                outstr[strlen(outstr)-1] = '\0';

            crypted_len = strlen(outstr) / 2;
            for (int idx = 0; idx < crypted_len; idx++)
            {
                int val;
                sscanf(&outstr[idx*2], "%02x", &val);
                crypted[idx] = (unsigned char) val;
                printf("%02X", val);
            }
            printf("\n");

            server.Decrypt(crypted, crypted_len, decrypted, decrypted_len);
            printf("TEST \"%s\" = %s (%d)\n", outstr, decrypted, decrypted_len);

            decrypted_len = sizeof(decrypted);
            server.Encrypt("server", 6, decrypted, decrypted_len);
            offset = sprintf(outstr, "server-DEC|");
            for (int i = 0; i < decrypted_len; i++)
                offset += sprintf(&outstr[offset], "%02X", (unsigned char) decrypted[i]);
            sprintf(&outstr[offset], "|DEC");
            printf("%s\n", outstr);

            return Ring::Ble::Crypto::Error::NO_ERROR;
        }
        else if (argc == 4) // asking to decode
        {
            char *in = argv[3];
            crypted_len = strlen(in) / 2;
            for (int idx = 0; idx < crypted_len; idx++)
            {
                int val;
                sscanf(&in[idx*2], "%02x", &val);
                crypted[idx] = (unsigned char) val;
            }
            server.Decrypt(crypted, crypted_len, decrypted, decrypted_len);
            printf("dec:%s:dec\n", decrypted);
            return Ring::Ble::Crypto::Error::NO_ERROR;
        }
    }

    //-------------------------- step 2 -------------------------------------------------

    // single command mode client process public payload
    char out[1024];
    memset(out, 0, sizeof(out));
    if (argc > 2 && (!strcmp(argv[1], "-ppf") || !strcmp(argv[1], "-def")))
    {
        char *filename = argv[2]; // public_payload.log
        FILE *fin = fopen(filename, "rt");
        if (!fin)
        {
            printf("can't open payload file \"%s\". Abort.\n", filename);
            return -666;
        }

        char *ch, strline[255];
        int out_offset = 0;

        while (fgets(strline, 255, fin))
        {
            if (strstr(strline, "Flags") || strstr(strline, "Notifying"))
            {
                break;
            }

            printf("%s", strline);
            if (NULL != (ch = strchr(strline, ' ')) )
            {
                char *esc = NULL;
                esc = strrchr(strline, 27);
                ch = (esc?esc:ch)+1;
                if (*ch == ' ') ch++;

                int b[16];
                if (16 == sscanf(ch, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ",
                                 &b[0],&b[1],&b[2],&b[3],&b[4],&b[5],&b[6],&b[7],&b[8],&b[9],
                                 &b[10],&b[11],&b[12],&b[13],&b[14],&b[15]))
                {
                    out_offset += sprintf(&out[out_offset],"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                                        b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],b[8],b[9],
                                        b[10],b[11],b[12],b[13],b[14],b[15]);
                }
                else
                {
                    char * ending = strstr(ch, "   ");
                    if (ending) *ending = '\0';

                    int num = sscanf(ch, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ",
                                     &b[0],&b[1],&b[2],&b[3],&b[4],&b[5],&b[6],&b[7],&b[8],&b[9],
                                     &b[10],&b[11],&b[12],&b[13],&b[14],&b[15]);

                    for (int i = 0; i < num; i++)
                    {
                        out_offset += sprintf(&out[out_offset], "%02x", b[i]);
                    }
                }
            }
        }
        fclose(fin);
    }

    if (argc > 2 && (!strcmp(argv[1], "-ppp") || !strcmp(argv[1], "-ppf")))
    {
        int val;
        char *in = strlen(out) ? out : argv[2];
        server_public_lenght = strlen(in) / 2;
        if (1 != sscanf(&in[0], "%02x", &val))
        {
            printf("\"%s\" - doesn't look like HEX payload\n", in);
            return -777;
        }
        else for (int idx = 0; idx < server_public_lenght; idx++)
        {
            sscanf(&in[idx*2], "%02x", &val);
            server_public[idx] = (unsigned char) val;
        }
    }

    int ret = client.ProcessPublicPayload(server_public, server_public_lenght);
    if (ret != Ring::Ble::Crypto::Error::NO_ERROR)
    {
        // some error, abort
        printf("ret.ProcessPublicPayload != Crypto::Error::NO_ERROR) = %d\n", ret);
        return ret;
    }

    if (argc > 2 && (!strcmp(argv[1], "-ppp") || !strcmp(argv[1], "-ppf")))
    {
        memset(outstr, 0, IO_MSG_BUFFER_SIZE);
        int offset = sprintf(outstr, "ppp:");
        for (int i = 0; i < server_public_lenght; i++)
            offset += sprintf(&outstr[offset], "%02x", (unsigned char) server_public[i]);
        sprintf(&outstr[offset], ":ppp");
        printf("%s\n", outstr);

        client.SaveSecrets();
        return Ring::Ble::Crypto::Error::NO_ERROR;
    }

    //-------------------------- step 3 -------------------------------------------------
    // single command to encrypt the given message
    if (argc > 2 && !strcmp(argv[1], "-enc"))
    {
        sprintf(msg, "%s", argv[2]);
        client.RestoreSecrets();
    }

    ret = client.Encrypt(msg, strlen(msg), crypted, crypted_len);
    if (ret != Ring::Ble::Crypto::Error::NO_ERROR)
    {
        // some error, abort
        printf("ret.Encrypt != Crypto::Error::NO_ERROR) = %d\n", ret);
        return ret;
    }

    if (argc > 2 && !strcmp(argv[1], "-enc"))
    {
        printf("msg:%s\n", msg);
        memset(outstr, 0, IO_MSG_BUFFER_SIZE);

        int offset = sprintf(outstr, "HEX|");
        for (int i = 0; i < crypted_len; i++)
            offset += sprintf(&outstr[offset], "%s%02X", i?" ":"", (unsigned char) crypted[i]);
        sprintf(&outstr[offset], "|HEX");
        printf("%s\n", outstr);
        offset = sprintf(outstr, "DEC|");
        for (int i = 0; i < crypted_len; i++)
            offset += sprintf(&outstr[offset], "%02X", (unsigned char) crypted[i]);
        sprintf(&outstr[offset], "|DEC");
        printf("%s\n", outstr);

        offset = sprintf(outstr, "enc:");
        for (int i = 0; i < crypted_len; i++)
            offset += sprintf(&outstr[offset], "%s%d", i?" ":"", (unsigned char) crypted[i]);
        sprintf(&outstr[offset], ":enc");
        printf("%s\n", outstr);
        return Ring::Ble::Crypto::Error::NO_ERROR;
    }

    //-------------------------- step 4 -------------------------------------------------
    // single command to decrypt the given message
    if (argc > 2 && (!strcmp(argv[1], "-dec") || !strcmp(argv[1], "-def")))
    {
        char *in = strlen(out) ? out : argv[2];
        crypted_len = strlen(in) / 2;
        for (int idx = 0; idx < crypted_len; idx++)
        {
            int val;
            sscanf(&in[idx*2], "%02x", &val);
            crypted[idx] = (unsigned char) val;
        }
        client.RestoreSecrets();
        ret = client.Decrypt(crypted, crypted_len, decrypted, decrypted_len);
    }
    else
    {
        // crypted is to be sent to server and decoded by it
        ret = server.Decrypt(crypted, crypted_len, decrypted, decrypted_len);
        printf("\n\n----------------------\nServer decpypted %d bytes:\n%s\n-------------------------------\n\n", decrypted_len, decrypted);
    }

    if (ret != Ring::Ble::Crypto::Error::NO_ERROR)
    {
        // some error, abort
        printf("ret.Decrypt != Crypto::Error::NO_ERROR) = %d\n", ret);
        return ret;
    }

    decrypted[decrypted_len]='\0';

    if (argc > 2 && (!strcmp(argv[1], "-dec") || !strcmp(argv[1], "-def")))
    {
        printf("enc:%s:enc\n", argv[2]);
        printf("dec:%s:dec\n", decrypted);
        return Ring::Ble::Crypto::Error::NO_ERROR;
    }

    printf("test ok: origin: %s\n", msg);
    Ring::Ble::Crypto::Debug::DumpArray("encrypted:", Ring::Ble::Crypto::ByteArr(crypted, crypted + crypted_len));
    printf("test ok: decrypted: %s\n", decrypted);

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
    printf("test ok: origin: %s\n", msg);
    Ring::Ble::Crypto::Debug::DumpArray("encrypted:", Ring::Ble::Crypto::ByteArr(crypted, crypted + crypted_len));
    decrypted[decrypted_len]='\0';
    printf("test ok: decrypted: %s\n", decrypted);

    return 0;
}

#else
int main(int argc, char* argv[])
{

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
    for (int i = 0; i < 10; ++i) {
        auto encr = client.encrypt(msg);
        debug_print("[main] encrypted", encr);
        auto decr = server.decrypt(encr);
        debug_print("[main] decrypted", decr);
    }
}
#endif
    //    if (argc > 2 && !strcmp(argv[1], "-def")) // decode from file
    //    {
    //        char *filename = argv[2];
    //        FILE *fin = fopen(filename, "rt");
    //        if (!fin)
    //        {
    //            printf("can't open payload file \"%s\". Abort.\n", filename);
    //            return -666;
    //        }

    //        char *ch, strline[255];
    //        int out_offset = 0;

    //        while (fgets(strline, 255, fin))
    //        {
    //            if (strstr(strline, "Flags") || strstr(strline, "Notifying"))
    //            {
    //                break;
    //            }

    //            printf("%s", strline);
    //            if (NULL != (ch = strchr(strline, ' ')) )
    //            {
    //                char *esc = NULL;
    //                esc = strrchr(strline, 27);
    //                ch = (esc?esc:ch)+1;
    //                if (*ch == ' ') ch++;

    //                int b[16];
    //                if (16 == sscanf(ch, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ",
    //                                 &b[0],&b[1],&b[2],&b[3],&b[4],&b[5],&b[6],&b[7],&b[8],&b[9],
    //                                 &b[10],&b[11],&b[12],&b[13],&b[14],&b[15]))
    //                {
    //                    out_offset += sprintf(&out[out_offset],"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
    //                                        b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],b[8],b[9],
    //                                        b[10],b[11],b[12],b[13],b[14],b[15]);
    //                }
    //                else
    //                {
    //                    char * ending = strstr(ch, "   ");
    //                    if (ending) *ending = '\0';

    //                    int num = sscanf(ch, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ",
    //                                     &b[0],&b[1],&b[2],&b[3],&b[4],&b[5],&b[6],&b[7],&b[8],&b[9],
    //                                     &b[10],&b[11],&b[12],&b[13],&b[14],&b[15]);

    //                    for (int i = 0; i < num; i++)
    //                    {
    //                        out_offset += sprintf(&out[out_offset], "%02x", b[i]);
    //                    }
    //                }
    //            }
    //        }
    //        fclose(fin);
    //    }
