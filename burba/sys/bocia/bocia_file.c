/*
 * Copyright (C) 2016 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 *
 * @addtogroup     <group>
 * @{
 *
 * @file
 * @brief
 *
 * @author      Attilio Dona' <attilio.dona>
 *
 */

/**
 * @brief write configuration fields to flash
 */
#include "bocia.h"
#include "simplelink.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#define MAX_BIN_SIZE 128

static unsigned char _buff[MAX_BIN_SIZE];

int bocia_write_proto(const char *fname, unsigned char *buff, size_t len);

int bocia_write_proto(const char *fname, unsigned char *buff, size_t len) {
    uint32_t maxsize = 1; // 3584;
    int32_t fd = -1;
    int32_t sts;
    SlFsFileInfo_t finfo;
    uint32_t options = 0;

    // check if file exists
    sts = sl_FsGetInfo((const unsigned char *)fname, 0, &finfo);

    if (sts == 0) {
        options = FS_MODE_OPEN_WRITE;
    } else if (sts == SL_FS_ERR_FILE_NOT_EXISTS) {
        options = FS_MODE_OPEN_CREATE(maxsize, 0);
    } else {
        // TODO: fatal error
        return sts;
    }

    sts = sl_FsOpen((const unsigned char *)fname, options, NULL, &fd);
    if (sts < 0) {
        return sts;
    }

    sts = sl_FsWrite(fd, 0, (unsigned char *)buff, len);

    sl_FsClose(fd, NULL, NULL, 0);
    return sts;
}

int bocia_write_cfg(Config *cfg) {
    size_t blen;

    blen = protobuf_c_message_get_packed_size((const ProtobufCMessage *)cfg);

    DEBUG("protobuf blen: %d\n", blen);
    protobuf_c_message_pack((const ProtobufCMessage *)cfg, _buff);

    return bocia_write_proto(BOCIA_CFG_FILE, _buff, blen);
}

Config *bocia_read_cfg(void) {
    SlFsFileInfo_t finfo;
    int16_t sts;
    uint32_t blen;
    ProtobufCMessage *obj;
    int32_t fd = -1;

    // get the len of the stored config protobuf object
    sts = sl_FsGetInfo((const unsigned char *)BOCIA_CFG_FILE, 0, &finfo);
    if (sts < 0) {
        return 0;
    }
    blen = finfo.FileLen;

    sts = sl_FsOpen((const unsigned char *)BOCIA_CFG_FILE, FS_MODE_OPEN_READ,
                    NULL, &fd);
    if (sts < 0) {
        return 0;
    }
    sts = sl_FsRead(fd, 0, _buff, blen);

    sl_FsClose(fd, NULL, NULL, 0);

    obj = protobuf_c_message_unpack(&config__descriptor, NULL, blen, _buff);
    return (Config *)obj;
}

int16_t bocia_delete_file(const char *fname) {
    int16_t sts;
    sts = sl_FsDel((unsigned char *)fname, 0);
    return sts;
}
