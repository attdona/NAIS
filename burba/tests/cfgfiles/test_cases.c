#include "../../../sys/include/xtimer.h"
#include "sbapi.h"
#include "unity.h"

#include "protofile.h"

#define SSID_NAME "zakke"
#define PASSWORD  "geranio1"


#if 0
void setUp(void)
{
}

void tearDown(void)
{
}
#endif



void test_cfg_file_not_exists(void) {
    int16_t sts;

    SlFsFileInfo_t finfo;

    sbapi_init(SBAPI_DEFAULT_CFG);

    sts = sl_FsGetInfo((const unsigned char*) BOCIA_CFG_FILE, 0, &finfo);
    TEST_ASSERT(sts == SL_FS_ERR_FILE_NOT_EXISTS);
}

void test_create_config(void) {
    Config cfg = CONFIG__INIT;
    //size_t blen;
    //unsigned char buff[64];

    cfg.network = "mynet";
    cfg.board = "myhome";
    cfg.host = "1.2.3.4";

    bocia_write_cfg(&cfg);

#if 0
    blen = protobuf_c_message_get_packed_size((const ProtobufCMessage*) &cfg);
    printf("bin len: %d\n", blen);
    protobuf_c_message_pack((const ProtobufCMessage*) &cfg, buff);

    write_proto(BOCIA_CFG_FILE, buff, blen);

    memcpy(outbuff, buff, 64);

    obj = protobuf_c_message_unpack(&config__descriptor, NULL, blen, outbuff);
    target = (Config*) obj;
    printf("network: %s\n", target->network);
    printf("board: %s\n", target->board);
#endif
}

/**
 * @brief read bocia.cfg file
 */
void test_read_cfg_file(void) {
    Config *target;

    target = bocia_read_cfg();
    printf("network: %s\n", target->network);
    printf("board: %s\n", target->board);

}
