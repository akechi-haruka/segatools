/*
Bandai Namco ES3/BNA1 USB HID emulator
2023 Haruka
please for your own sanity do not look at this shitty code
*/

#include <windows.h>

#include <devioctl.h>
#include <hidclass.h>
#include <cfgmgr32.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "board/guid.h"
#include "board/io4.h"

#include "hook/iobuf.h"
#include "hook/iohook.h"
#include "hook/table.h"

#include "hooklib/setupapi.h"

#include "namco/keychip.h"

#include "util/async.h"
#include "util/dprintf.h"
#include "util/dump.h"
#include "util/str.h"

static const wchar_t* keychip_usb_dev = L"$namcokeychip";
static const wchar_t* keychip_usb_fd = L"\\\\.\\$namcokeychip";
static struct namsec_config config;
static HANDLE keychip_fd;

static __stdcall CONFIGRET hook_CM_Get_DevNode_Registry_PropertyW(
    DEVINST dnDevInst,
    ULONG   ulProperty,
    PULONG  pulRegDataType,
    PVOID   Buffer,
    PULONG  pulLength,
    ULONG   ulFlags
);
static __stdcall CONFIGRET (*next_CM_Get_DevNode_Registry_PropertyW)(
    DEVINST dnDevInst,
    ULONG   ulProperty,
    PULONG  pulRegDataType,
    PVOID   Buffer,
    PULONG  pulLength,
    ULONG   ulFlags
);

static const struct hook_symbol devnode_syms[] = {
	{
		.name   = "CM_Get_DevNode_Registry_PropertyW",
		.patch  = hook_CM_Get_DevNode_Registry_PropertyW,
		.link   = (void*) &next_CM_Get_DevNode_Registry_PropertyW
	},
};

static HRESULT keychip_handle_open(struct irp *irp)
{
    if (!wstr_ieq(irp->open_filename, keychip_usb_dev) && !wstr_ieq(irp->open_filename, keychip_usb_fd)){
        return iohook_invoke_next(irp);
    }

    dprintf("NamSec: Opened handle (%p)\n", keychip_fd);
    irp->fd = keychip_fd;

    return S_OK;
}

static HRESULT keychip_handle_close(struct irp *irp)
{
    dprintf("NamSec: Closed handle\n");

    return S_OK;
}

static HRESULT keychip_GetHCDDriverKeyName_0x220424(struct irp *irp){
    //dprintf("NamSec: %s\n", __func__);
    size_t len = (wcslen(keychip_usb_dev) + 1) * sizeof(wchar_t);
    if (irp->read.nbytes == 6){
        return iobuf_write_le16(&irp->read, len + 4);
    }
    iobuf_write_8(&irp->read, 1);
    iobuf_write_8(&irp->read, 1);
    iobuf_write_8(&irp->read, 1);
    iobuf_write_8(&irp->read, 1);
    return iobuf_write(&irp->read, keychip_usb_dev, len);
}

static HRESULT keychip_GetRootHubName_0x220408(struct irp *irp){
    //dprintf("NamSec: %s\n", __func__);
    size_t len = (wcslen(keychip_usb_dev) + 1) * sizeof(wchar_t);
    if (irp->read.nbytes == 6){
        return iobuf_write_le16(&irp->read, len + 4);
    } else if (irp->write.nbytes > 0){
        iobuf_write_le16(&irp->read, 1);
        iobuf_write_le16(&irp->read, 1);
        iobuf_write_le16(&irp->read, 1);
        return iobuf_write_le16(&irp->read, 1);
    }
    iobuf_write_8(&irp->read, 1);
    iobuf_write_8(&irp->read, 1);
    iobuf_write_8(&irp->read, 1);
    iobuf_write_8(&irp->read, 1);
    return iobuf_write(&irp->read, keychip_usb_dev, len);
}

static __stdcall CONFIGRET hook_CM_Get_DevNode_Registry_PropertyW(
    DEVINST dnDevInst,
    ULONG   ulProperty,
    PULONG  pulRegDataType,
    PVOID   Buffer,
    PULONG  pulLength,
    ULONG   ulFlags
){
    //dprintf("CM_Get_DevNode_Registry_PropertyW (%ld)\n", ulProperty);
    if (ulProperty == 10 || ulProperty == 1){
        wcscpy(Buffer, keychip_usb_dev);
        return 0;
    }
    return next_CM_Get_DevNode_Registry_PropertyW(dnDevInst, ulProperty, pulRegDataType, Buffer, pulLength, ulFlags);
}

static HRESULT keychip_GetExternalHubName_0x220414(struct irp *irp){
    //dprintf("NamSec: %s\n", __func__);
    size_t len = (wcslen(keychip_usb_dev) + 1) * sizeof(wchar_t);
    if (irp->read.nbytes == 6){
        return iobuf_write_le16(&irp->read, len);
    }
    return iobuf_write(&irp->read, keychip_usb_dev, len);
}

static HRESULT keychip_GetStringDescriptor_0x220410(struct irp *irp){
    //dprintf("NamSec: %s\n", __func__);

    wchar_t str[16];

    dprintf("NamSec: Get Keychip ID (%ls)\n", config.keychip);
    wcscpy_s(str, 16, config.keychip);
    size_t len = (wcslen(str) + 1) * sizeof(wchar_t);

    if (irp->read.nbytes == 21){
        iobuf_write_be32(&irp->read, 1);
        iobuf_write_be32(&irp->read, 2);
        iobuf_write_be32(&irp->read, 3);
        iobuf_write_be16(&irp->read, 4);
        iobuf_write_le16(&irp->read, len + 2); // must be > 9
        iobuf_write_be32(&irp->read, 5);
        return iobuf_write_8(&irp->read, 6);
    } else if (irp->read.nbytes > 50){
        iobuf_write_be32(&irp->read, 1);
        iobuf_write_be32(&irp->read, 2);
        iobuf_write_be32(&irp->read, 3);
        iobuf_write_8(&irp->read, len);
        iobuf_write_8(&irp->read, 3);
        iobuf_write(&irp->read, str, len - 2);
        return S_OK;
    } else if (irp->read.nbytes > 21){
         iobuf_write_be32(&irp->read, 1);
         iobuf_write_be32(&irp->read, 2);
         iobuf_write_be32(&irp->read, 3);
         iobuf_write_be16(&irp->read, len);
         iobuf_write_le16(&irp->read, len + 2);
         iobuf_write(&irp->read, str, len - 2);
         iobuf_write_le32(&irp->read, len);
         iobuf_write_le32(&irp->read, len);
         iobuf_write_le32(&irp->read, len);
         return S_OK;
     }
    iobuf_write_be32(&irp->read, irp->read.nbytes);
    iobuf_write_be32(&irp->read, 1);
    iobuf_write_be32(&irp->read, 1);
    iobuf_write_le16(&irp->read, 0);
    iobuf_write_le16(&irp->read, len);
    iobuf_write(&irp->read, str, len);
    iobuf_write_be32(&irp->read, 0);
    iobuf_write_be32(&irp->read, 0);
    iobuf_write_be32(&irp->read, 0);
    iobuf_write_be32(&irp->read, 0);
    return iobuf_write_8(&irp->read, 0);
}

static HRESULT keychip_GetNodeConnectionInformationEx_0x220448(struct irp *irp){
    //dprintf("NamSec: %s\n", __func__);
    if (true){return E_FAIL;}

    return S_OK;
}

static HRESULT keychip_GetNodeConnectionInformation_0x22040C(struct irp *irp){
    //dprintf("NamSec: %s\n", __func__);
    iobuf_write_be32(&irp->read, 1);
    iobuf_write_be32(&irp->read, 2);
    iobuf_write_be32(&irp->read, 0xF000);
    iobuf_write_le32(&irp->read, 0x00C100B9A);
    iobuf_write_8(&irp->read, 1);
    iobuf_write_8(&irp->read, 1);
    iobuf_write_be16(&irp->read, 0);
    iobuf_write_8(&irp->read, 1);
    iobuf_write_8(&irp->read, 1);
    iobuf_write_be16(&irp->read, 0);
    iobuf_write_8(&irp->read, 1);
    iobuf_write_8(&irp->read, 1);
    iobuf_write_be16(&irp->read, 0);
    iobuf_write_be32(&irp->read, 1);
    iobuf_write_8(&irp->read, 0);
    iobuf_write_be16(&irp->read, 0x0000);
    iobuf_write_be16(&irp->read, 0xFFFF);
    iobuf_write_be32(&irp->read, 0xFFFFFFFF);
    iobuf_write_be32(&irp->read, 0xFFFFFFFF);
    iobuf_write_be32(&irp->read, 0xFFFFFFFF);
    iobuf_write_be32(&irp->read, 0xFFFFFFFF);
    iobuf_write_be32(&irp->read, 0xFFFFFFFF);
    iobuf_write_be32(&irp->read, 0xFFFFFFFF);
    iobuf_write_be32(&irp->read, 0xFFFFFFFF);
    iobuf_write_be32(&irp->read, 0xFFFFFFFF);
    iobuf_write_be32(&irp->read, 0xFFFFFFFF);
    iobuf_write_be32(&irp->read, 0xFFFFFFFF);
    iobuf_write_be32(&irp->read, 0xFFFFFFFF);
    for (int i = 0; i < 100; i++){
        iobuf_write_be32(&irp->read, 0xFFFFFFFF);
    }
    return S_OK;
}

static HRESULT keychip_GetBusInfo_0x220420(struct irp *irp){
    //dprintf("NamSec: %s\n", __func__);
    size_t len = (wcslen(keychip_usb_dev) + 1) * sizeof(wchar_t);
    if (irp->read.nbytes == 10){
        iobuf_write_le16(&irp->read, len);
        iobuf_write_le16(&irp->read, 1);
        iobuf_write_8(&irp->read, 0xA + len);
        iobuf_write_8(&irp->read, 0);
        iobuf_write_le32(&irp->read, 0);
        return S_OK;
    }
    iobuf_write_le32(&irp->read, len);
    iobuf_write_le32(&irp->read, 0);
    return iobuf_write(&irp->read, keychip_usb_dev, len);
}

static HRESULT keychip_handle_ioctl(struct irp *irp)
{
    /*dprintf("NamSec: Command %#08x, write %i read %i\n",
       irp->ioctl,
       (int) irp->write.nbytes,
       (int) irp->read.nbytes);*/
    //dprintf("IN:\n");
    //dump_const_iobuf(&irp->write);

    switch (irp->ioctl) {
    case 0x220424: return keychip_GetHCDDriverKeyName_0x220424(irp);
    case 0x220408: return keychip_GetRootHubName_0x220408(irp);
    case 0x220414: return keychip_GetExternalHubName_0x220414(irp);
    case 0x220448: return keychip_GetNodeConnectionInformationEx_0x220448(irp);
    case 0x22040C: return keychip_GetNodeConnectionInformation_0x22040C(irp);
    case 0x220410: return keychip_GetStringDescriptor_0x220410(irp);
    case 0x220420: return keychip_GetBusInfo_0x220420(irp);
    default:
        dprintf("NamSec: Unknown ioctl %#08x, write %i read %i\n",
                irp->ioctl,
                (int) irp->write.nbytes,
                (int) irp->read.nbytes);

        return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT keychip_handle_irp(struct irp *irp)
{
    assert(irp != NULL);

    if (irp->op != IRP_OP_OPEN && irp->fd != keychip_fd) {
        //dprintf("NamSec: Rejecting %p, %ls\n", irp->fd, irp->open_filename);
        return iohook_invoke_next(irp);
    }

    switch (irp->op) {
        case IRP_OP_OPEN:   return keychip_handle_open(irp);
        case IRP_OP_CLOSE:  return keychip_handle_close(irp);
        case IRP_OP_IOCTL:
            ;
            HRESULT hr = keychip_handle_ioctl(irp);
            //dprintf("NamSec: %ld/%ld bytes written on bus\n", irp->read.pos, irp->read.nbytes);

            //dprintf("OUT:\n");
            //dump_iobuf(&irp->read);

            return hr;
        default:
            dprintf("NamSec: Unimplemented interrupt routine: %d\n", irp->op);
            return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

HRESULT keychip_init(struct namsec_config *cfg){

    assert(cfg != NULL);

    if (!cfg->enable){
        dprintf("NamSec: disabled?\n");
        return S_FALSE;
    }

    dprintf("NamSec: initializing\n");
    memcpy(&config, cfg, sizeof(config));

    HRESULT hr = iohook_open_nul_fd(&keychip_fd);

    if (FAILED(hr)) {
        dprintf("NamSec: iohook_open_nul_fd fail: %lx\n", hr);
        return hr;
    }

    hook_table_apply(
        NULL,
        "setupapi.dll",
        devnode_syms,
        _countof(devnode_syms));

    hr = iohook_push_handler(keychip_handle_irp);
    if (FAILED(hr)) {
        dprintf("NamSec: iohook_push_handler failed: %lx\n", hr);
        return hr;
    }

    hr = setupapi_add_phantom_dev(&keychip_guid, keychip_usb_dev);

    if (FAILED(hr)) {
        dprintf("NamSec: setupapi_add_phantom_dev failed: %lx\n", hr);
        return hr;
    }

    return S_OK;
}
