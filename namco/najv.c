/*
Based on IO3
*/
#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#include "jvs/jvs-bus.h"
#include "jvs/jvs-cmd.h"
#include "jvs/jvs-util.h"

#include "namco/najv.h"

#include "util/dprintf.h"
#include "util/dump.h"

static void najv_transact(
        struct jvs_node *node,
        const void *bytes,
        size_t nbytes,
        struct iobuf *resp);

static bool najv_sense(struct jvs_node *node);

static HRESULT najv_cmd(
        void *ctx,
        struct const_iobuf *req,
        struct iobuf *resp);

static HRESULT najv_cmd_read_id(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT najv_cmd_get_cmd_version(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT najv_cmd_get_jvs_version(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT najv_cmd_get_comm_version(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT najv_cmd_get_features(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT najv_cmd_read_switches(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT najv_cmd_read_coin(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT najv_cmd_read_analogs(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT najv_cmd_read_rotary(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT najv_cmd_write_gpio(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT najv_cmd_convey_main_board(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT najv_cmd_0x70(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT najv_cmd_0x78(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT najv_cmd_0x7a(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT najv_cmd_0x4e(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT najv_cmd_0x50(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT najv_cmd_reset(struct najv *najv, struct const_iobuf *buf);

static HRESULT najv_cmd_assign_addr(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static const uint8_t najv_ident[] =
        "NBGI.;NA-JV;Ver6.01;JPN,MK3100-1-NA-APR0-A01";

static uint8_t najv_features[] = {
    /* Feature : 0x01 : Players and switches
       Param1  :    2 : Number of players
       Param2  :   14 : Number of switches per player
       Param3  :    0 : N/A */

    0x01, 2, 0x10, 0,

    /* Feature : 0x02 : Coin slots
       Param1  :    2 : Number of coin slots
       Param2  :    0 : N/A
       Param3  :    0 : N/A */

    0x02, 2, 0, 0,

    /* Feature : 0x03 : Analog inputs
       Param1  :    8 : Number of ADC channels
       Param2  :   10 : Effective bits of resolution per ADC
       Param3  :    0 : N/A */

    0x03, 8, 10, 0,

    /* Feature : 0x12 : GPIO outputs
       Param1  :    3 : Number of ports (8 bits per port)
       Param2  :    0 : N/A
       Param3  :    0 : N/A

       NOTE: This particular port count is what an IO-4 attached over JVS
       advertises, an IO-3 only advertises 3. Still, this seems to be backwards
       compatible with games that expect an IO-3, and the protocols seem to be
       identical otherwise. */

    0x12, 0x14, 0, 0,

    /* Feature : 0x00 : End of capabilities */

    0x00,
};

void najv_init(
        struct najv *najv,
        struct jvs_node *next,
        const struct najv_ops *ops,
        void *ops_ctx)
{
    assert(najv != NULL);
    assert(ops != NULL);

    najv->jvs.next = next;
    najv->jvs.transact = najv_transact;
    najv->jvs.sense = najv_sense;
    najv->addr = 0xFF;
    najv->ops = ops;
    najv->ops_ctx = ops_ctx;
    dprintf("JVS (NAJV): Initialized\n");
}

struct jvs_node *najv_to_jvs_node(struct najv *najv)
{
    assert(najv != NULL);

    return &najv->jvs;
}

static void najv_transact(
        struct jvs_node *node,
        const void *bytes,
        size_t nbytes,
        struct iobuf *resp)
{
    struct najv *najv;

    assert(node != NULL);
    assert(bytes != NULL);
    assert(resp != NULL);

    najv = CONTAINING_RECORD(node, struct najv, jvs);

    jvs_crack_request(bytes, nbytes, resp, najv->addr, najv_cmd, najv);
}

static bool najv_sense(struct jvs_node *node)
{
    struct najv *najv;

    assert(node != NULL);

    najv = CONTAINING_RECORD(node, struct najv, jvs);

    return najv->addr == 0xFF;
}

static HRESULT najv_cmd(
        void *ctx,
        struct const_iobuf *req,
        struct iobuf *resp)
{
    struct najv *najv;

    najv = ctx;

    switch (req->bytes[req->pos]) {
    case JVS_CMD_READ_ID:
        return najv_cmd_read_id(najv, req, resp);

    case JVS_CMD_GET_CMD_VERSION:
        return najv_cmd_get_cmd_version(najv, req, resp);

    case JVS_CMD_GET_JVS_VERSION:
        return najv_cmd_get_jvs_version(najv, req, resp);

    case JVS_CMD_GET_COMM_VERSION:
        return najv_cmd_get_comm_version(najv, req, resp);

    case JVS_CMD_GET_FEATURES:
        return najv_cmd_get_features(najv, req, resp);

    case JVS_CMD_READ_SWITCHES:
        return najv_cmd_read_switches(najv, req, resp);

    case JVS_CMD_READ_COIN:
        return najv_cmd_read_coin(najv, req, resp);

    case JVS_CMD_READ_ANALOGS:
        return najv_cmd_read_analogs(najv, req, resp);

    case JVS_CMD_READ_ROTARY:
        return najv_cmd_read_rotary(najv, req, resp);

    case JVS_CMD_WRITE_GPIO:
        return najv_cmd_write_gpio(najv, req, resp);

    case JVS_CMD_RESET:
        return najv_cmd_reset(najv, req);

    case JVS_CMD_ASSIGN_ADDR:
        return najv_cmd_assign_addr(najv, req, resp);

    case JVS_CMD_CONVEY_MAIN_BOARD:
        return najv_cmd_convey_main_board(najv, req, resp);

    case 0x70:
        return najv_cmd_0x70(najv, req, resp);

    case 0x50:
        return najv_cmd_0x50(najv, req, resp);

    case 0x78:
        return najv_cmd_0x78(najv, req, resp);

    case 0x7a:
        return najv_cmd_0x7a(najv, req, resp);

    case 0x4e:
        return najv_cmd_0x4e(najv, req, resp);

    default:
        dprintf("JVS I/O: Node %02x: Unhandled command byte %02x\n",
                najv->addr,
                req->bytes[req->pos]);
        dump_const_iobuf(req);

        return E_NOTIMPL;
    }
}

static HRESULT najv_cmd_read_id(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    uint8_t req;
    HRESULT hr;

    hr = iobuf_read_8(req_buf, &req);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("JVS I/O: Read ID\n");

    /* Write report byte */

    hr = iobuf_write_8(resp_buf, 0x01);

    if (FAILED(hr)) {
        return hr;
    }

    /* Write the identification string. The NUL terminator at the end of this C
       string is also sent, and it naturally terminates the response chunk. */

    return iobuf_write(resp_buf, najv_ident, sizeof(najv_ident));
}

static HRESULT najv_cmd_get_cmd_version(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    uint8_t req;
    uint8_t resp[2];
    HRESULT hr;

    hr = iobuf_read_8(req_buf, &req);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("JVS I/O: Get command format version\n");
    resp[0] = 0x01; /* Report byte */
    resp[1] = 0x31; /* Command format version BCD */

    return iobuf_write(resp_buf, resp, sizeof(resp));
}

static HRESULT najv_cmd_get_jvs_version(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    uint8_t req;
    uint8_t resp[2];
    HRESULT hr;

    hr = iobuf_read_8(req_buf, &req);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("JVS I/O: Get JVS version\n");
    resp[0] = 0x01; /* Report byte */
    resp[1] = 0x31; /* JVS version BCD */

    return iobuf_write(resp_buf, resp, sizeof(resp));
}

static HRESULT najv_cmd_get_comm_version(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    uint8_t req;
    uint8_t resp[2];
    HRESULT hr;

    hr = iobuf_read_8(req_buf, &req);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("JVS I/O: Get communication version\n");
    resp[0] = 0x01; /* Report byte */
    resp[1] = 0x31; /* "Communication version" BCD */

    return iobuf_write(resp_buf, resp, sizeof(resp));
}

static HRESULT najv_cmd_get_features(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    uint8_t req;
    HRESULT hr;

    hr = iobuf_read_8(req_buf, &req);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("JVS I/O: Get features\n");

    hr = iobuf_write_8(resp_buf, 0x01); /* Write report byte */

    if (FAILED(hr)) {
        return hr;
    }

    return iobuf_write(resp_buf, najv_features, sizeof(najv_features));
}

static HRESULT najv_cmd_read_switches(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    struct jvs_req_read_switches req;
    struct najv_switch_state state;
    HRESULT hr;

    /* Read req */

    hr = iobuf_read(req_buf, &req, sizeof(req));

    if (FAILED(hr)) {
        return hr;
    }

#if 0
    dprintf("JVS I/O: Read switches, np=%i, bpp=%i\n",
            req.num_players,
            req.bytes_per_player);
#endif

    if (req.num_players > 2 || req.bytes_per_player > 4) {
        dprintf("JVS I/O: Invalid read size "
                        "num_players=%i "
                        "bytes_per_player=%i\n",
                req.num_players,
                req.bytes_per_player);
        hr = iobuf_write_8(resp_buf, 0x02);

        if (FAILED(hr)) {
            return hr;
        }

        return E_FAIL;
    }

    /* Build response */

    hr = iobuf_write_8(resp_buf, 0x01); /* Report byte */

    if (FAILED(hr)) {
        return hr;
    }

    memset(&state, 0, sizeof(state));

    if (najv->ops != NULL) {
        najv->ops->read_switches(najv->ops_ctx, &state);
    }

    hr = iobuf_write_8(resp_buf, state.system); /* Test, Tilt lines */

    if (FAILED(hr)) {
        return hr;
    }

    if (req.num_players > 0) {
        hr = iobuf_write_be16(resp_buf, state.p1);

        if (FAILED(hr)) {
            return hr;
        }
    }

    if (req.num_players > 1) {
        hr = iobuf_write_8(resp_buf, 0);
        hr = iobuf_write_8(resp_buf, 0);

        if (FAILED(hr)) {
            return hr;
        }
    }

    return hr;
}

static HRESULT najv_cmd_read_coin(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    struct jvs_req_read_coin req;
    uint16_t ncoins;
    uint8_t i;
    HRESULT hr;

    /* Read req */

    hr = iobuf_read(req_buf, &req, sizeof(req));

    if (FAILED(hr)) {
        return hr;
    }

    //dprintf("JVS I/O: Read coin, nslots=%i\n", req.nslots);

    /* Write report byte */

    hr = iobuf_write_8(resp_buf, 0x01);

    if (FAILED(hr)) {
        return hr;
    }

    /* Write slot detail */

    for (i = 0 ; i < req.nslots ; i++) {
        ncoins = 0;

        if (najv->ops->read_coin_counter != NULL) {
            najv->ops->read_coin_counter(najv->ops_ctx, i, &ncoins);
        }

        hr = iobuf_write_be16(resp_buf, ncoins);

        if (FAILED(hr)) {
            return hr;
        }
    }

    return hr;
}

static HRESULT najv_cmd_read_analogs(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    struct jvs_req_read_analogs req;
    uint16_t analogs[8];
    uint8_t i;
    HRESULT hr;

    /* Read req */

    hr = iobuf_read(req_buf, &req, sizeof(req));

    if (FAILED(hr)) {
        return hr;
    }

    if (req.nanalogs > _countof(analogs)) {
        dprintf("JVS I/O: Invalid analog count %i\n", req.nanalogs);

        return E_FAIL;
    }

    //dprintf("JVS I/O: Read analogs, nanalogs=%i\n", req.nanalogs);

    /* Write report byte */

    hr = iobuf_write_8(resp_buf, 0x01);

    if (FAILED(hr)) {
        return hr;
    }

    /* Write analogs */

    memset(analogs, 0, sizeof(analogs));

    if (najv->ops->read_analogs != NULL) {
        najv->ops->read_analogs(najv->ops_ctx, analogs, req.nanalogs);
    }

    for (i = 0 ; i < req.nanalogs ; i++) {
        hr = iobuf_write_be16(resp_buf, analogs[i]);

        if (FAILED(hr)) {
            return hr;
        }
    }

    return hr;

}

static HRESULT najv_cmd_read_rotary(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    struct jvs_req_read_rotary req;
    uint16_t rotary[4];
    uint8_t i;
    HRESULT hr;

    /* Read req */

    hr = iobuf_read(req_buf, &req, sizeof(req));

    if (FAILED(hr)) {
        return hr;
    }

    if (req.nrotary > _countof(rotary)) {
        dprintf("JVS I/O: Invalid rotary count %i\n", req.nrotary);

        return E_FAIL;
    }

    //dprintf("JVS I/O: Read rotary, nrotary=%i\n", req.nrotary);

    /* Write report byte */

    hr = iobuf_write_8(resp_buf, 0x01);

    if (FAILED(hr)) {
        return hr;
    }

    /* Write rotary */

    memset(rotary, 0, sizeof(rotary));

    if (FAILED(hr)) {
        return hr;
    }

    if (najv->ops->read_rotary != NULL) {
        najv->ops->read_rotary(najv->ops_ctx, rotary, req.nrotary);
    }

    for (i = 0 ; i < req.nrotary ; i++) {
        hr = iobuf_write_le16(resp_buf, rotary[i]);

        if (FAILED(hr)) {
            return hr;
        }
    }

    return hr;

}

static HRESULT najv_cmd_write_gpio(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    uint8_t cmd;
    uint8_t nbytes;
    uint8_t bytes[3];
    HRESULT hr;

    /* Read request header */

    hr = iobuf_read_8(req_buf, &cmd);

    if (FAILED(hr)) {
        return hr;
    }

    hr = iobuf_read_8(req_buf, &nbytes);

    if (FAILED(hr)) {
        return hr;
    }

    if (nbytes > 3) {
        dprintf("JVS I/O: Invalid GPIO write size %i\n", nbytes);
        hr = iobuf_write_8(resp_buf, 0x02);

        if (FAILED(hr)) {
            return hr;
        }

        return E_FAIL;
    }

    /* Read payload */

    memset(bytes, 0, sizeof(bytes));
    hr = iobuf_read(req_buf, bytes, nbytes);

    if (FAILED(hr)) {
        return hr;
    }

    if (najv->ops->write_gpio != NULL) {
        najv->ops->write_gpio(
                najv->ops_ctx,
                bytes[0] | (bytes[1] << 8) | (bytes[2] << 16));
    }

    /* Write report byte */

    return iobuf_write_8(resp_buf, 0x01);
}

static HRESULT najv_cmd_reset(struct najv *najv, struct const_iobuf *req_buf)
{
    struct jvs_req_reset req;
    HRESULT hr;

    hr = iobuf_read(req_buf, &req, sizeof(req));

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("JVS I/O: Reset (param %02x)\n", req.unknown);
    najv->addr = 0xFF;

    if (najv->ops->reset != NULL) {
        najv->ops->reset(najv->ops_ctx);
    }

    /* No ack for this since it really is addressed to everybody */

    return S_FALSE;
}

static HRESULT najv_cmd_assign_addr(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    struct jvs_req_assign_addr req;
    bool sense;
    HRESULT hr;

    hr = iobuf_read(req_buf, &req, sizeof(req));

    if (FAILED(hr)) {
        return hr;
    }

    sense = jvs_node_sense(najv->jvs.next);
    dprintf("JVS I/O: Assign addr %02x sense %i\n", req.addr, sense);

    if (sense) {
        /* That address is for somebody else */
        return S_OK;
    }

    najv->addr = req.addr;

    return iobuf_write_8(resp_buf, 0x01);
}

static HRESULT najv_cmd_convey_main_board(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{

    uint8_t resp[2];

    req_buf->pos = req_buf->nbytes; // skip packet

    dprintf("JVS I/O: Convey main board name\n");
    resp[0] = 0x01; /* Report byte */
    resp[1] = 0x01; /* Status byte */

    return iobuf_write(resp_buf, resp, sizeof(resp));
}

static HRESULT najv_cmd_0x70(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{

    req_buf->pos = req_buf->nbytes; // skip packet

    //dprintf("JVS I/O: unknown 0x70\n");

    iobuf_write_8(resp_buf, 0x01);
    return iobuf_write_8(resp_buf, 0x01);
}

static HRESULT najv_cmd_0x78(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    //dprintf("JVS I/O: unknown 0x78\n");
    req_buf->pos = req_buf->nbytes; // skip packet

    return S_OK;
}

static HRESULT najv_cmd_0x7a(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    //dprintf("JVS I/O: unknown 0x7a\n");
    req_buf->pos = req_buf->nbytes; // skip packet

    return S_OK;
}

static HRESULT najv_cmd_0x4e(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    dprintf("JVS I/O: unknown 0x4e\n");
    req_buf->pos = req_buf->nbytes; // skip packet

    return S_OK;
}

static HRESULT najv_cmd_0x50(
        struct najv *najv,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    dprintf("JVS I/O: unknown 0x50\n");
    req_buf->pos = req_buf->nbytes; // skip packet

    return S_OK;
}
