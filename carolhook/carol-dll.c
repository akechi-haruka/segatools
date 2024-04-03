#include <windows.h>

#include <assert.h>
#include <stdlib.h>

#include "carolhook/carol-dll.h"

#include "util/dll-bind.h"
#include "util/dprintf.h"

const struct dll_bind_sym carol_dll_syms[] = {
    {
        .sym = "carol_io_jvs_init",
        .off = offsetof(struct carol_dll, jvs_init),
    }, {
        .sym = "carol_io_jvs_poll",
        .off = offsetof(struct carol_dll, jvs_poll),
    }, {
        .sym = "carol_io_jvs_read_coin_counter",
        .off = offsetof(struct carol_dll, jvs_read_coin_counter),
    }, {
        .sym = "carol_io_touch_init",
        .off = offsetof(struct carol_dll, touch_init),
    }, {
        .sym = "carol_io_controlbd_init",
        .off = offsetof(struct carol_dll, controlbd_init),
    }
};

struct carol_dll carol_dll;

// Copypasta DLL binding and diagnostic message boilerplate.
// Not much of this lends itself to being easily factored out. Also there
// will be a lot of API-specific branching code here eventually as new API
// versions get defined, so even though these functions all look the same
// now this won't remain the case forever.

HRESULT carol_dll_init(const struct carol_dll_config *cfg, HINSTANCE self)
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
            dprintf("carol IO: Failed to load IO DLL: %lx: %S\n",
                    hr,
                    cfg->path);

            goto end;
        }

        dprintf("carol IO: Using custom IO DLL: %S\n", cfg->path);
        src = owned;
    } else {
        owned = NULL;
        src = self;
    }

    get_api_version = (void *) GetProcAddress(src, "carol_io_get_api_version");

    if (get_api_version != NULL) {
        carol_dll.api_version = get_api_version();
    } else {
        carol_dll.api_version = 0x0100;
        dprintf("Custom IO DLL does not expose carol_io_get_api_version, "
                "assuming API version 1.0.\n"
                "Please ask the developer to update their DLL.\n");
    }

    if (carol_dll.api_version >= 0x0200) {
        hr = E_NOTIMPL;
        dprintf("carol IO: Custom IO DLL implements an unsupported "
                "API version (%#04x). Please update Segatools.\n",
                carol_dll.api_version);

        goto end;
    }

    sym = carol_dll_syms;
    hr = dll_bind(&carol_dll, src, &sym, _countof(carol_dll_syms));

    if (FAILED(hr)) {
        if (src != self) {
            dprintf("carol IO: Custom IO DLL does not provide function "
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
