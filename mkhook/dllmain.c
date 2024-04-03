#include <windows.h>
#include <iphlpapi.h>

#include "api/api.h"


#include "hook/process.h"
#include "hook/table.h"

#include "hooklib/path.h"
#include "hooklib/serial.h"
#include "hooklib/window.h"

#include "mkhook/config.h"
#include "mkhook/jvs.h"

#include "namco/bngrw.h"
#include "namco/keychip.h"
#include "namco/strpcb.h"
#include "namco/namco-jvs-usb.h"

#include "platform/platform.h"

#include "util/dprintf.h"

#include <iphlpapi.h>
#include <icmpapi.h>
#include <wininet.h>

static HMODULE mk_hook_mod;
static process_entry_t mk_startup;
static struct mk_hook_config mk_hook_cfg;

DWORD __stdcall hook_SetIpForwardEntry(PMIB_IPFORWARDROW pRoute){
    dprintf("MK: blocking setting of gateway\n");
    return NO_ERROR;
}

DWORD __stdcall hook_DeleteIpForwardEntry(PMIB_IPFORWARDROW pRoute){
    dprintf("MK: blocking deletion of gateway\n");
    return NO_ERROR;
}

static const struct hook_symbol mk_syms[] = {
	{
		.name   = "SetIpForwardEntry",
		.patch  = hook_SetIpForwardEntry,
	},
	{
		.name   = "DeleteIpForwardEntry",
		.patch  = hook_DeleteIpForwardEntry,
	},
};

static __stdcall BOOL (*next_WinHttpSetOption)(HINTERNET hInternet, DWORD dwOption, LPVOID    lpBuffer, DWORD dwBufferLength);

static __stdcall BOOL hook_WinHttpSetOption(HINTERNET hInternet, DWORD dwOption, LPVOID    lpBuffer, DWORD dwBufferLength){
    if (dwOption == 31){
        dprintf("MK: disabling security\n");
        int dwFlags =
                    SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                    SECURITY_FLAG_IGNORE_UNKNOWN_CA
        ;
        int* ptr = (int*)lpBuffer;
        (*ptr) |= dwFlags;
        return next_WinHttpSetOption(hInternet, dwOption, ptr, dwBufferLength);
    }
    return next_WinHttpSetOption(hInternet, dwOption, lpBuffer, dwBufferLength);
}

static const struct hook_symbol wh_syms[] = {
    {
        .name   = "WinHttpSetOption",
        .patch  = hook_WinHttpSetOption,
        .link   = (void **) &next_WinHttpSetOption,
    }
};

static DWORD CALLBACK mk_pre_startup(void)
{
    HRESULT hr;

    dprintf("--- Begin mk_pre_startup ---\n");

    mk_hook_config_load(&mk_hook_cfg, L".\\segatools.ini");

    show_banner(__TIMESTAMP__);

    hr = dns_platform_hook_init(&mk_hook_cfg.platform.dns);

    if (FAILED(hr)) {
        dprintf("Init: dns_platform_hook_init failed\n");
        return hr;
    }

    gfx_hook_init(&mk_hook_cfg.gfx, mk_hook_mod);

    hr = netenv_hook_init(&mk_hook_cfg.platform.netenv, &mk_hook_cfg.platform.nusec);

    if (FAILED(hr)) {
        dprintf("Init: netenv_hook_init failed\n");
        return hr;
    }

    hr = keychip_init(&mk_hook_cfg.namsec);

    if (FAILED(hr)) {
        dprintf("Init: keychip_init failed\n");
        return hr;
    }

    hr = clock_hook_init(&mk_hook_cfg.clock);

    if (FAILED(hr)) {
        dprintf("Init: clock_hook_init failed\n");
        return hr;
    }

    mk_jvs_init(&mk_hook_cfg.io);

    hr = jvs_hook_init(mk_jvs_node);

    if (FAILED(hr)) {
        dprintf("Init: jvs_hook_init failed\n");
        return hr;
    }

    hr = strpcb_hook_init();

    if (FAILED(hr)) {
        dprintf("Init: jvs_hook_init failed\n");
        return hr;
    }

    hr = bngrw_init(&mk_hook_cfg.bngrw, true);

    if (FAILED(hr)) {
        dprintf("Init: bngrw_init failed\n");
        return hr;
    }

    path_hook_init();
    serial_hook_init();
    window_hook_init(&mk_hook_cfg.window, L"mkart3");

    hook_table_apply(
                                NULL,
                                "IPHLPAPI.dll",
                                mk_syms,
                                _countof(mk_syms));
    hook_table_apply(
                                NULL,
                                "winhttp.dll",
                                wh_syms,
                                _countof(wh_syms));

	if (FAILED(hr)) {
        return hr;
    }

    dprintf("---  End  mk_pre_startup ---\n");

    GetAdaptersInfo(NULL, NULL); // trigger network info

    return mk_startup();
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    mk_hook_mod = mod;

    hr = process_hijack_startup(mk_pre_startup, &mk_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
