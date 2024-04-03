#include <windows.h>
#include <d3d9.h>

#include <assert.h>
#include <stdlib.h>

#include "hook/com-proxy.h"
#include "hook/table.h"

#include "hooklib/dll.h"

#include "gfxhook/gfx.h"
#include "gfxhook/util.h"

#include "util/dprintf.h"

typedef IDirect3D9 * (WINAPI *Direct3DCreate9_t)(UINT sdk_ver);
typedef HRESULT (WINAPI *Direct3DCreate9Ex_t)(UINT sdk_ver, IDirect3D9Ex **d3d9ex);

static HRESULT STDMETHODCALLTYPE my_IDirect3D9_CreateDevice(
        IDirect3D9 *self,
        UINT adapter,
        D3DDEVTYPE type,
        HWND hwnd,
        DWORD flags,
        D3DPRESENT_PARAMETERS *pp,
        IDirect3DDevice9 **pdev);
static HRESULT STDMETHODCALLTYPE my_IDirect3D9Ex_CreateDevice(
        IDirect3D9Ex *self,
        UINT adapter,
        D3DDEVTYPE type,
        HWND hwnd,
        DWORD flags,
        D3DPRESENT_PARAMETERS *pp,
        IDirect3DDevice9 **pdev);

static struct gfx_config gfx_config;
static Direct3DCreate9_t next_Direct3DCreate9;
static Direct3DCreate9Ex_t next_Direct3DCreate9Ex;

static const struct hook_symbol gfx_hooks[] = {
    {
        .name   = "Direct3DCreate9",
        .patch  = Direct3DCreate9,
        .link   = (void **) &next_Direct3DCreate9,
    }, {
        .name   = "Direct3DCreate9Ex",
        .patch  = Direct3DCreate9Ex,
        .link   = (void **) &next_Direct3DCreate9Ex,
    },
};

void gfx_d3d9_hook_init(const struct gfx_config *cfg, HINSTANCE self)
{
    HMODULE d3d9;

    assert(cfg != NULL);

    if (!cfg->enable) {
        return;
    }

    memcpy(&gfx_config, cfg, sizeof(*cfg));
    hook_table_apply(NULL, "d3d9.dll", gfx_hooks, _countof(gfx_hooks));

    if (next_Direct3DCreate9 == NULL || next_Direct3DCreate9Ex == NULL) {
        d3d9 = LoadLibraryW(L"d3d9.dll");

        if (d3d9 == NULL) {
            dprintf("Gfx: d3d9.dll not found or failed initialization\n");

            goto fail;
        }

        if (next_Direct3DCreate9 == NULL) {
            next_Direct3DCreate9 = (Direct3DCreate9_t) GetProcAddress(d3d9, "Direct3DCreate9");
        }
        if (next_Direct3DCreate9Ex == NULL) {
            next_Direct3DCreate9Ex = (Direct3DCreate9Ex_t) GetProcAddress(d3d9, "Direct3DCreate9Ex");
        }

        if (next_Direct3DCreate9 == NULL) {
            dprintf("Gfx: Direct3DCreate9 not found in loaded d3d9.dll\n");

            goto fail;
        }
        if (next_Direct3DCreate9Ex == NULL) {
            dprintf("Gfx: Direct3DCreate9Ex not found in loaded d3d9.dll\n");

            goto fail;
        }
    }

    if (self != NULL) {
        dll_hook_push(self, L"d3d9.dll");
    }

    return;

fail:
    if (d3d9 != NULL) {
        FreeLibrary(d3d9);
    }
}

IDirect3D9 * WINAPI Direct3DCreate9(UINT sdk_ver)
{
    struct com_proxy *proxy;
    IDirect3D9Vtbl *vtbl;
    IDirect3D9 *api;
    HRESULT hr;

    dprintf("Gfx: Direct3DCreate9 hook hit\n");

    api = NULL;

    if (next_Direct3DCreate9 == NULL) {
        dprintf("Gfx: next_Direct3DCreate9 == NULL\n");

        goto fail;
    }

    api = next_Direct3DCreate9(sdk_ver);

    if (api == NULL) {
        dprintf("Gfx: next_Direct3DCreate9 returned NULL\n");

        goto fail;
    }

    hr = com_proxy_wrap(&proxy, api, sizeof(*api->lpVtbl));

    if (FAILED(hr)) {
        dprintf("Gfx: com_proxy_wrap returned %x\n", (int) hr);

        goto fail;
    }

    vtbl = proxy->vptr;
    vtbl->CreateDevice = my_IDirect3D9_CreateDevice;

    return (IDirect3D9 *) proxy;

fail:
    if (api != NULL) {
        IDirect3D9_Release(api);
    }

    return NULL;
}

HRESULT WINAPI Direct3DCreate9Ex(UINT sdk_ver, IDirect3D9Ex **d3d9ex)
{
    struct com_proxy *proxy;
    IDirect3D9ExVtbl *vtbl;
    IDirect3D9Ex *api;
    HRESULT hr;

    dprintf("Gfx: Direct3DCreate9Ex hook hit\n");

    api = NULL;

    if (next_Direct3DCreate9Ex == NULL) {
        dprintf("Gfx: next_Direct3DCreate9Ex == NULL\n");

        goto fail;
    }

    hr = next_Direct3DCreate9Ex(sdk_ver, d3d9ex);

    if (FAILED(hr)) {
        dprintf("Gfx: next_Direct3DCreate9Ex returned %x\n", (int) hr);

        goto fail;
    }

    api = *d3d9ex;
    hr = com_proxy_wrap(&proxy, api, sizeof(*api->lpVtbl));

    if (FAILED(hr)) {
        dprintf("Gfx: com_proxy_wrap returned %x\n", (int) hr);

        goto fail;
    }

    vtbl = proxy->vptr;
    vtbl->CreateDevice = my_IDirect3D9Ex_CreateDevice;

    *d3d9ex = (IDirect3D9Ex *) proxy;

    return S_OK;

fail:
    if (api != NULL) {
        IDirect3D9Ex_Release(api);
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE my_IDirect3D9_CreateDevice(
        IDirect3D9 *self,
        UINT adapter,
        D3DDEVTYPE type,
        HWND hwnd,
        DWORD flags,
        D3DPRESENT_PARAMETERS *pp,
        IDirect3DDevice9 **pdev)
{
    struct com_proxy *proxy;
    IDirect3D9 *real;

    dprintf("Gfx: IDirect3D9::CreateDevice hook hit\n");

    proxy = com_proxy_downcast(self);
    real = proxy->real;

    if (gfx_config.windowed) {
        pp->Windowed = TRUE;
        pp->FullScreen_RefreshRateInHz = 0;
    }

    if (gfx_config.framed) {
        gfx_util_frame_window(hwnd);
    }

    dprintf("Gfx: Using adapter %d\n", gfx_config.monitor);

    return IDirect3D9_CreateDevice(real, gfx_config.monitor, type, hwnd, flags, pp, pdev);
}

static HRESULT STDMETHODCALLTYPE my_IDirect3D9Ex_CreateDevice(
        IDirect3D9Ex *self,
        UINT adapter,
        D3DDEVTYPE type,
        HWND hwnd,
        DWORD flags,
        D3DPRESENT_PARAMETERS *pp,
        IDirect3DDevice9 **pdev)
{
    dprintf("Gfx: IDirect3D9Ex::CreateDevice hook forwarding to my_IDirect3D9_CreateDevice\n");

    return my_IDirect3D9_CreateDevice(
            (IDirect3D9 *) self,
            adapter,
            type,
            hwnd,
            flags,
            pp,
            pdev);
}
