#include <windows.h>

#include <assert.h>
#include <stdlib.h>

#include "fgohook/fgo-dll.h"

#include "util/dll-bind.h"
#include "util/dprintf.h"

const struct dll_bind_sym fgo_dll_syms[] = {
    {
        .sym = "fgo_io_init",
        .off = offsetof(struct fgo_dll, init),
    }, {
        .sym = "fgo_io_poll",
        .off = offsetof(struct fgo_dll, poll),
    }, {
        .sym = "fgo_io_get_opbtns",
        .off = offsetof(struct fgo_dll, get_opbtns),
    }, {
        .sym = "fgo_io_get_gamebtns",
        .off = offsetof(struct fgo_dll, get_gamebtns),
    }, {
        .sym = "fgo_io_get_stick",
        .off = offsetof(struct fgo_dll, get_stick),
    }, {
      .sym = "fgo_io_get_coin",
      .off = offsetof(struct fgo_dll, get_coin),
    }
};

struct fgo_dll fgo_dll;

// Copypasta DLL binding and diagnostic message boilerplate.
// Not much of this lends itself to being easily factored out. Also there
// will be a lot of API-specific branching code here eventually as new API
// versions get defined, so even though these functions all look the same
// now this won't remain the case forever.

HRESULT fgo_dll_init(const struct fgo_dll_config *cfg, HINSTANCE self)
{
    uint16_t (*get_api_version)(void);
    const struct dll_bind_sym *sym;
    HINSTANCE owned;
    HINSTANCE src;
    HRESULT hr;

    assert(cfg != NULL);
    assert(self != NULL);

    if (cfg->path[0] != L'\0') {
        owned = LoadLibraryW(cfg->path);

        if (owned == NULL) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            dprintf("FGO IO: Failed to load IO DLL: %lx: %S\n",
                    hr,
                    cfg->path);

            goto end;
        }

        dprintf("FGO IO: Using custom IO DLL: %S\n", cfg->path);
        src = owned;
    } else {
        owned = NULL;
        src = self;
    }

    get_api_version = (void *) GetProcAddress(src, "fgo_io_get_api_version");

    if (get_api_version != NULL) {
        fgo_dll.api_version = get_api_version();
    } else {
        fgo_dll.api_version = 0x0100;
        dprintf("Custom IO DLL does not expose fgo_io_get_api_version, "
                "assuming API version 1.0.\n"
                "Please ask the developer to update their DLL.\n");
    }

    if (fgo_dll.api_version >= 0x0200) {
        hr = E_NOTIMPL;
        dprintf("FGO IO: Custom IO DLL implements an unsupported "
                "API version (%#04x). Please update Segatools.\n",
                fgo_dll.api_version);

        goto end;
    }

    sym = fgo_dll_syms;
    hr = dll_bind(&fgo_dll, src, &sym, _countof(fgo_dll_syms));

    if (FAILED(hr)) {
        if (src != self) {
            dprintf("FGO IO: Custom IO DLL does not provide function "
                    "\"%s\". Please contact your IO DLL's developer for "
                    "further assistance.\n",
                    sym->sym);

            goto end;
        } else {
            dprintf("Internal error: could not reflect \"%s\"\n", sym->sym);
        }
    }

    owned = NULL;

end:
    if (owned != NULL) {
        FreeLibrary(owned);
    }

    return hr;
}
