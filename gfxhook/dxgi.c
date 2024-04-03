#include <windows.h>
#include <dxgi.h>
#include <dxgi1_3.h>

#include <assert.h>
#include <stdlib.h>

#include "gfxhook/dxgi.h"
#include "gfxhook/gfx.h"

#include "hook/com-proxy.h"
#include "hook/table.h"

#include "hooklib/dll.h"

#include "util/dprintf.h"

typedef HRESULT (WINAPI *CreateDXGIFactory_t)(REFIID riid, void **factory);
typedef HRESULT (WINAPI *CreateDXGIFactory1_t)(REFIID riid, void **factory);
typedef HRESULT (WINAPI *CreateDXGIFactory2_t)(
        UINT flags,
        REFIID riid,
        void **factory);

static HRESULT hook_factory(REFIID riid, void **factory);

static HRESULT STDMETHODCALLTYPE my_IDXGIFactory_CreateSwapChain(
        IDXGIFactory *self,
        IUnknown *device,
        DXGI_SWAP_CHAIN_DESC *desc,
        IDXGISwapChain **swapchain);
static HRESULT STDMETHODCALLTYPE my_IDXGIFactory1_CreateSwapChain(
        IDXGIFactory1 *self,
        IUnknown *device,
        DXGI_SWAP_CHAIN_DESC *desc,
        IDXGISwapChain **swapchain);
static HRESULT STDMETHODCALLTYPE my_IDXGIFactory2_CreateSwapChain(
        IDXGIFactory2 *self,
        IUnknown *device,
        DXGI_SWAP_CHAIN_DESC *desc,
        IDXGISwapChain **swapchain);

static struct gfx_config gfx_config;
static CreateDXGIFactory_t next_CreateDXGIFactory;
static CreateDXGIFactory1_t next_CreateDXGIFactory1;
static CreateDXGIFactory2_t next_CreateDXGIFactory2;

static const struct hook_symbol dxgi_hooks[] = {
    {
        .name = "CreateDXGIFactory",
        .patch = CreateDXGIFactory,
        .link = (void **) &next_CreateDXGIFactory,
    }, {
        .name = "CreateDXGIFactory1",
        .patch = CreateDXGIFactory1,
        .link = (void **) &next_CreateDXGIFactory1,
    }, {
        .name = "CreateDXGIFactory2",
        .patch = CreateDXGIFactory2,
        .link = (void **) &next_CreateDXGIFactory2,
    },
};

void gfx_dxgi_hook_init(const struct gfx_config *cfg, HINSTANCE self)
{
    HMODULE dxgi;

    assert(cfg != NULL);

    if (!cfg->enable) {
        return;
    }

    memcpy(&gfx_config, cfg, sizeof(*cfg));
    hook_table_apply(NULL, "dxgi.dll", dxgi_hooks, _countof(dxgi_hooks));

    if (next_CreateDXGIFactory == NULL || next_CreateDXGIFactory1 == NULL) {
        dxgi = LoadLibraryW(L"dxgi.dll");

        if (dxgi == NULL) {
            dprintf("DXGI: dxgi.dll not found or failed initialization\n");

            goto fail;
        }

        if (next_CreateDXGIFactory == NULL) {
            next_CreateDXGIFactory = (CreateDXGIFactory_t) GetProcAddress(
                    dxgi,
                    "CreateDXGIFactory");
        }
        if (next_CreateDXGIFactory1 == NULL) {
            next_CreateDXGIFactory1 = (CreateDXGIFactory1_t) GetProcAddress(
                    dxgi,
                    "CreateDXGIFactory1");
        }
        if (next_CreateDXGIFactory2 == NULL) {
            next_CreateDXGIFactory2 = (CreateDXGIFactory2_t) GetProcAddress(
                    dxgi,
                    "CreateDXGIFactory2");
        }

        if (next_CreateDXGIFactory == NULL) {
            dprintf("DXGI: CreateDXGIFactory not found in loaded dxgi.dll\n");

            goto fail;
        }
        if (next_CreateDXGIFactory1 == NULL) {
            dprintf("DXGI: CreateDXGIFactory1 not found in loaded dxgi.dll\n");

            goto fail;
        }

        /* `CreateDXGIFactory2` was introduced in Windows 8.1 and the original
         * Nu runs Windows 8, so do not require it to exist */
    }

    if (self != NULL) {
        dll_hook_push(self, L"dxgi.dll");
    }

    return;

fail:
    if (dxgi != NULL) {
        FreeLibrary(dxgi);
    }
}

HRESULT WINAPI CreateDXGIFactory(REFIID riid, void **factory)
{
    HRESULT hr;

    dprintf("DXGI: CreateDXGIFactory hook hit\n");

    hr = next_CreateDXGIFactory(riid, factory);

    if (FAILED(hr)) {
        dprintf("DXGI: CreateDXGIFactory returned %x\n", (int) hr);

        return hr;
    }

    hr = hook_factory(riid, factory);

    if (FAILED(hr)) {
        return hr;
    }

    return hr;
}

HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void **factory)
{
    HRESULT hr;

    dprintf("DXGI: CreateDXGIFactory1 hook hit\n");

    hr = next_CreateDXGIFactory1(riid, factory);

    if (FAILED(hr)) {
        dprintf("DXGI: CreateDXGIFactory1 returned %x\n", (int) hr);

        return hr;
    }

    hr = hook_factory(riid, factory);

    if (FAILED(hr)) {
        return hr;
    }

    return hr;
}

HRESULT WINAPI CreateDXGIFactory2(UINT flags, REFIID riid, void **factory)
{
    HRESULT hr;

    dprintf("DXGI: CreateDXGIFactory2 hook hit\n");

    if (next_CreateDXGIFactory2 == NULL) {
        dprintf("DXGI: CreateDXGIFactory2 not available, forwarding to CreateDXGIFactory1\n");

        return CreateDXGIFactory1(riid, factory);
    }

    hr = next_CreateDXGIFactory2(flags, riid, factory);

    if (FAILED(hr)) {
        dprintf("DXGI: CreateDXGIFactory2 returned %x\n", (int) hr);

        return hr;
    }

    hr = hook_factory(riid, factory);

    if (FAILED(hr)) {
        return hr;
    }

    return hr;
}

static HRESULT hook_factory(REFIID riid, void **factory)
{
    struct com_proxy *proxy;
    IDXGIFactory *api_0;
    IDXGIFactory1 *api_1;
    IDXGIFactory2 *api_2;
    IDXGIFactoryVtbl *vtbl_0;
    IDXGIFactory1Vtbl *vtbl_1;
    IDXGIFactory2Vtbl *vtbl_2;
    HRESULT hr;

    api_0 = NULL;
    api_1 = NULL;
    api_2 = NULL;

    if (memcmp(riid, &IID_IDXGIFactory, sizeof(*riid)) == 0) {
        api_0 = *factory;
        hr = com_proxy_wrap(&proxy, api_0, sizeof(*api_0->lpVtbl));

        if (FAILED(hr)) {
            dprintf("DXGI: com_proxy_wrap returned %x\n", (int) hr);

            goto fail;
        }

        vtbl_0 = proxy->vptr;
        vtbl_0->CreateSwapChain = my_IDXGIFactory_CreateSwapChain;

        *factory = proxy;
    } else if (memcmp(riid, &IID_IDXGIFactory1, sizeof(*riid)) == 0) {
        api_1 = *factory;
        hr = com_proxy_wrap(&proxy, api_1, sizeof(*api_1->lpVtbl));

        if (FAILED(hr)) {
            dprintf("DXGI: com_proxy_wrap returned %x\n", (int) hr);

            goto fail;
        }

        vtbl_1 = proxy->vptr;
        vtbl_1->CreateSwapChain = my_IDXGIFactory1_CreateSwapChain;

        *factory = proxy;
    } else if (memcmp(riid, &IID_IDXGIFactory2, sizeof(*riid)) == 0) {
        api_2 = *factory;
        hr = com_proxy_wrap(&proxy, api_2, sizeof(*api_2->lpVtbl));

        if (FAILED(hr)) {
            dprintf("DXGI: com_proxy_wrap returned %x\n", (int) hr);

            goto fail;
        }

        vtbl_2 = proxy->vptr;
        vtbl_2->CreateSwapChain = my_IDXGIFactory2_CreateSwapChain;

        *factory = proxy;
    }

    return S_OK;

fail:
    if (api_0 != NULL) {
        IDXGIFactory_Release(api_0);
    }
    if (api_1 != NULL) {
        IDXGIFactory1_Release(api_1);
    }
    if (api_2 != NULL) {
        IDXGIFactory2_Release(api_2);
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE my_IDXGIFactory_CreateSwapChain(
        IDXGIFactory *self,
        IUnknown *device,
        DXGI_SWAP_CHAIN_DESC *desc,
        IDXGISwapChain **swapchain)
{
    struct com_proxy *proxy;
    IDXGIFactory *real;
    HWND hwnd;
    UINT width;
    UINT height;

    dprintf("DXGI: IDXGIFactory::CreateSwapChain hook hit\n");

    proxy = com_proxy_downcast(self);
    real = proxy->real;

    if (desc != NULL) {
        desc->Windowed = gfx_config.windowed;

        hwnd = desc->OutputWindow;
        width = desc->BufferDesc.Width;
        height = desc->BufferDesc.Height;
    } else {
        hwnd = NULL;
    }

    if (hwnd != NULL) {
        /*
        * Ensure window is maximized to avoid a Windows 10 issue where a
        * fullscreen swap chain is not created because the window is minimized
        * at the time of creation.
        */
        ShowWindow(hwnd, SW_RESTORE);

        if (!gfx_config.framed && width > 0 && height > 0) {
            dprintf("DXGI: Resizing window to %ux%u\n", width, height);

            SetWindowLongPtrW(hwnd, GWL_STYLE, WS_POPUP);
            SetWindowLongPtrW(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST);

            SetWindowPos(
                    hwnd,
                    HWND_TOP,
                    0,
                    0,
                    (int) width,
                    (int) height,
                    SWP_FRAMECHANGED | SWP_NOSENDCHANGING);

            ShowWindow(hwnd, SW_SHOWMAXIMIZED);
        }
    }

    return IDXGIFactory_CreateSwapChain(real, device, desc, swapchain);
}

static HRESULT STDMETHODCALLTYPE my_IDXGIFactory1_CreateSwapChain(
        IDXGIFactory1 *self,
        IUnknown *device,
        DXGI_SWAP_CHAIN_DESC *desc,
        IDXGISwapChain **swapchain)
{
    dprintf("DXGI: IDXGIFactory1::CreateSwapChain hook forwarding to my_IDXGIFactory_CreateSwapChain\n");

    return my_IDXGIFactory_CreateSwapChain(
            (IDXGIFactory *) self,
            device,
            desc,
            swapchain);
}

static HRESULT STDMETHODCALLTYPE my_IDXGIFactory2_CreateSwapChain(
        IDXGIFactory2 *self,
        IUnknown *device,
        DXGI_SWAP_CHAIN_DESC *desc,
        IDXGISwapChain **swapchain)
{
    dprintf("DXGI: IDXGIFactory2::CreateSwapChain hook forwarding to my_IDXGIFactory_CreateSwapChain\n");

    return my_IDXGIFactory_CreateSwapChain(
            (IDXGIFactory *) self,
            device,
            desc,
            swapchain);
}
