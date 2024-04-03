/* This is some sort of LCD display found on various cabinets. It is driven
   directly by amdaemon, and it has something to do with displaying the status
   of electronic payments.

   Part number in schematics is "VFD GP1232A02A FUTABA".

   Little else about this board is known. Black-holing the RS232 comms that it
   receives seems to be sufficient for the time being. */

#include <windows.h>

#include <assert.h>
#include <stdint.h>

#include "api/api.h"

#include "board/config.h"
#include "board/vfd.h"
#include "board/vfd-cmd.h"

#include "hook/iohook.h"

#include "hooklib/uart.h"

#include "util/dprintf.h"
#include "util/dump.h"

#define SUPER_VERBOSE 1

static HRESULT vfd_handle_irp(struct irp *irp);

static struct uart vfd_uart;
static uint8_t vfd_written[4096];
static uint8_t vfd_readable[4096];

static int encoding = VFD_ENC_SHIFT_JIS;

HRESULT vfd_handle_get_version(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);
HRESULT vfd_handle_reset(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);
HRESULT vfd_handle_clear_screen(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);
HRESULT vfd_handle_set_brightness(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);
HRESULT vfd_handle_set_screen_on(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);
HRESULT vfd_handle_set_h_scroll(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);
HRESULT vfd_handle_draw_image(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);
HRESULT vfd_handle_set_cursor(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);
HRESULT vfd_handle_set_encoding(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);
HRESULT vfd_handle_set_text_wnd(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);
HRESULT vfd_handle_set_text_speed(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);
HRESULT vfd_handle_write_text(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);
HRESULT vfd_handle_enable_scroll(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);
HRESULT vfd_handle_disable_scroll(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);
HRESULT vfd_handle_rotate(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);
HRESULT vfd_handle_create_char(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);
HRESULT vfd_handle_create_char2(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart);

static bool api_enabled;
static bool utf_enabled;

HRESULT vfd_hook_init(struct vfd_config *cfg)
{
    if (!cfg->enable){
        return S_FALSE;
    }

    api_enabled = cfg->send_to_api;
    utf_enabled = cfg->utf_conversion;

    dprintf("VFD: enabling (port=%d)\n", cfg->port);
    uart_init(&vfd_uart, cfg->port);
    vfd_uart.written.bytes = vfd_written;
    vfd_uart.written.nbytes = sizeof(vfd_written);
    vfd_uart.readable.bytes = vfd_readable;
    vfd_uart.readable.nbytes = sizeof(vfd_readable);

    return iohook_push_handler(vfd_handle_irp);
}

static HRESULT vfd_handle_irp(struct irp *irp)
{
    HRESULT hr;

    assert(irp != NULL);

    if (!uart_match_irp(&vfd_uart, irp)) {
        return iohook_invoke_next(irp);
    }

    if (irp->op == IRP_OP_OPEN){
        dprintf("VFD: Open\n");
    } else if (irp->op == IRP_OP_CLOSE){
        dprintf("VFD: Close\n");
    }

    hr = uart_handle_irp(&vfd_uart, irp);

    if (FAILED(hr) || irp->op != IRP_OP_WRITE) {
        return hr;
    }

#if SUPER_VERBOSE
    dprintf("VFD TX:\n");
    dump_iobuf(&vfd_uart.written);
#endif

    struct const_iobuf reader;
    iobuf_flip(&reader, &vfd_uart.written);

    struct iobuf* writer = &vfd_uart.readable;
    for (; reader.pos < reader.nbytes ; ){

        vfd_frame_sync(&reader);

        uint8_t cmd;
        iobuf_read_8(&reader, &cmd);

        if (cmd == VFD_CMD_GET_VERSION){
            hr = vfd_handle_get_version(&reader, writer, &vfd_uart);
        } else if (cmd == VFD_CMD_RESET){
            hr = vfd_handle_reset(&reader, writer, &vfd_uart);
        } else if (cmd == VFD_CMD_CLEAR_SCREEN){
            hr = vfd_handle_clear_screen(&reader, writer, &vfd_uart);
        } else if (cmd == VFD_CMD_SET_BRIGHTNESS){
            hr = vfd_handle_set_brightness(&reader, writer, &vfd_uart);
        } else if (cmd == VFD_CMD_SET_SCREEN_ON){
            hr = vfd_handle_set_screen_on(&reader, writer, &vfd_uart);
        } else if (cmd == VFD_CMD_SET_H_SCROLL){
            hr = vfd_handle_set_h_scroll(&reader, writer, &vfd_uart);
        } else if (cmd == VFD_CMD_DRAW_IMAGE){
            hr = vfd_handle_draw_image(&reader, writer, &vfd_uart);
        } else if (cmd == VFD_CMD_SET_CURSOR){
            hr = vfd_handle_set_cursor(&reader, writer, &vfd_uart);
        } else if (cmd == VFD_CMD_SET_ENCODING){
            hr = vfd_handle_set_encoding(&reader, writer, &vfd_uart);
        } else if (cmd == VFD_CMD_SET_TEXT_WND){
            hr = vfd_handle_set_text_wnd(&reader, writer, &vfd_uart);
        } else if (cmd == VFD_CMD_SET_TEXT_SPEED){
            hr = vfd_handle_set_text_speed(&reader, writer, &vfd_uart);
        } else if (cmd == VFD_CMD_WRITE_TEXT){
            hr = vfd_handle_write_text(&reader, writer, &vfd_uart);
        } else if (cmd == VFD_CMD_ENABLE_SCROLL){
            hr = vfd_handle_enable_scroll(&reader, writer, &vfd_uart);
        } else if (cmd == VFD_CMD_DISABLE_SCROLL){
            hr = vfd_handle_disable_scroll(&reader, writer, &vfd_uart);
        } else if (cmd == VFD_CMD_ROTATE){
            hr = vfd_handle_rotate(&reader, writer, &vfd_uart);
        } else if (cmd == VFD_CMD_CREATE_CHAR){
            hr = vfd_handle_create_char(&reader, writer, &vfd_uart);
        } else if (cmd == VFD_CMD_CREATE_CHAR2){
            hr = vfd_handle_create_char2(&reader, writer, &vfd_uart);
        } else {
            dprintf("VFD: Unknown command 0x%x\n", cmd);
            dump_const_iobuf(&reader);
            hr = S_FALSE;
        }

        if (!SUCCEEDED(hr)){
            return hr;
        }

    }

    vfd_uart.written.pos = 0;

    return hr;
}

HRESULT vfd_handle_get_version(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    dprintf("VFD: Get Version\n");

    struct vfd_resp_board_info resp;

    memset(&resp, 0, sizeof(resp));
    resp.unk1 = 2;
    strcpy(resp.version, "01.20");
    resp.unk2 = 1;

    return vfd_frame_encode(writer, &resp, sizeof(resp));
}
HRESULT vfd_handle_reset(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    dprintf("VFD: Reset\n");

    encoding = VFD_ENC_SHIFT_JIS;
    if (api_enabled){
        api_send_vfd(L"");
    }

    return S_FALSE;
}
HRESULT vfd_handle_clear_screen(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    dprintf("VFD: Clear Screen\n");

    if (api_enabled){
        api_send_vfd(L"");
    }

    return S_FALSE;
}
HRESULT vfd_handle_set_brightness(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    uint8_t b;
    iobuf_read_8(reader, &b);

    dprintf("VFD: Brightness, %d\n", b);

    return S_FALSE;
}
HRESULT vfd_handle_set_screen_on(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    uint8_t b;
    iobuf_read_8(reader, &b);

    dprintf("VFD: Screen Power, %d\n", b);
    return S_FALSE;
}
HRESULT vfd_handle_set_h_scroll(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    uint8_t x;
    iobuf_read_8(reader, &x);

    dprintf("VFD: Horizontal Scroll, X=%d\n", x);
    return S_FALSE;
}
HRESULT vfd_handle_draw_image(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    int w, h;
    uint16_t x0, x1;
    uint8_t y0, y1;
    uint8_t image[2048];

    iobuf_read_be16(reader, &x0);
    iobuf_read_8(reader, &y0);
    iobuf_read_be16(reader, &x1);
    iobuf_read_8(reader, &y1);
    w = x1 - x0;
    h = y1 - y0;
    iobuf_read(reader, image, w*h);

    dprintf("VFD: Draw image, %dx%d\n", w, h);
    return S_FALSE;
}

const char* enctostr(int b){
    switch (b){
        case 0: return "gb2312";
        case 1: return "big5";
        case 2: return "shift-jis";
        case 3: return "ks_c_5601-1987";
        default: return "unknown";
    }
}

void print_vfd_text(const char* str, int len){

    if (utf_enabled){

        wchar_t strenc[1024];
        memset(strenc, 0, 1024 * sizeof(wchar_t));

        int codepage = 0;
        if (encoding == VFD_ENC_GB2312){
            codepage = 936;
        } else if (encoding == VFD_ENC_BIG5){
            codepage = 950;
        } else if (encoding == VFD_ENC_SHIFT_JIS){
            codepage = 932;
        } else if (encoding == VFD_ENC_KSC5601) {
            codepage = 949;
        }

        if (!MultiByteToWideChar(codepage, MB_USEGLYPHCHARS, str, len, strenc, 1024)){
            dprintf("VFD: Text conversion failed: %ld", GetLastError());
            return;
        }

        if (api_enabled){
            api_send_vfd(strenc);
        }

        dprintf("VFD: Text: %ls\n", strenc);
    } else {

        if (api_enabled){
            api_send_vfd_sj(str);
        }

        dprintf("VFD: Text: %s\n", str);

    }
}
HRESULT vfd_handle_set_cursor(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    uint16_t x;
    uint8_t y;

    iobuf_read_be16(reader, &x);
    iobuf_read_8(reader, &y);

    dprintf("VFD: Set Cursor, x=%d,y=%d\n", x, y);

    if (reader->pos < reader->nbytes){
        int len = 0;
        while (reader->pos + len + 1 < reader->nbytes && reader->bytes[reader->pos + len] != VFD_SYNC_BYTE){
            len++;
        }
        //int len = reader->nbytes - reader->pos;
        dprintf("VFD: %d trailing characters?\n", len);
        char* str = malloc(len);
        iobuf_read(reader, str, len);
        print_vfd_text(str, len);
        free(str);
    }

    return S_FALSE;
}

HRESULT vfd_handle_set_encoding(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    uint8_t b;
    iobuf_read_8(reader, &b);

    dprintf("VFD: Set Encoding, %d (%s)\n", b, enctostr(b));

    if (b < 0 || b > VFD_ENC_MAX){
        dprintf("Invalid encoding specified\n");
        return E_FAIL;
    }

    encoding = b;

    return S_FALSE;
}
HRESULT vfd_handle_set_text_wnd(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    uint16_t x0, x1;
    uint8_t y0, y1;

    iobuf_read_be16(reader, &x0);
    iobuf_read_8(reader, &y0);
    iobuf_read_be16(reader, &x1);
    iobuf_read_8(reader, &y1);

    dprintf("VFD: Set Text Window, p0:%d,%d, p1:%d,%d\n", x0, y0, x1, y1);
    return S_FALSE;
}
HRESULT vfd_handle_set_text_speed(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    uint8_t b;
    iobuf_read_8(reader, &b);

    dprintf("VFD: Set Text Speed, %d\n", b);
    return S_FALSE;
}
HRESULT vfd_handle_write_text(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    uint8_t len;
    iobuf_read_8(reader, &len);

    char* str = malloc(len);
    iobuf_read(reader, str, len);

    print_vfd_text(str, len);
    free(str);

    return S_FALSE;
}
HRESULT vfd_handle_enable_scroll(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    dprintf("VFD: Enable Scrolling\n");
    return S_FALSE;
}
HRESULT vfd_handle_disable_scroll(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    dprintf("VFD: Disable Scrolling\n");
    return S_FALSE;
}
HRESULT vfd_handle_rotate(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    uint8_t b;
    iobuf_read_8(reader, &b);

    dprintf("VFD: Rotate, %d\n", b);
    return S_FALSE;
}
HRESULT vfd_handle_create_char(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    uint8_t b;
    iobuf_read_8(reader, &b);
    char buf[32];

    iobuf_read(reader, buf, 32);

    dprintf("VFD: Create character, %d\n", b);
    return S_FALSE;
}
HRESULT vfd_handle_create_char2(struct const_iobuf* reader, struct iobuf* writer, struct uart* vfd_uart){
    uint8_t b, b2;
    iobuf_read_8(reader, &b);
    iobuf_read_8(reader, &b2);
    char buf[16];

    iobuf_read(reader, buf, 16);

    dprintf("VFD: Create character, %d, %d\n", b, b2);
    return S_FALSE;
}
