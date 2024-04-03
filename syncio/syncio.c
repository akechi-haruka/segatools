#include <windows.h>

#include <limits.h>
#include <stdint.h>

#include "api/api.h"


#include "hook/table.h"

#include "namco/bngrw.h"
#include "namco/config.h"
#include "namco/keychip.h"

#include "syncio/syncio.h"
#include "syncio/config.h"
#include "syncio/wajv.h"

#include "util/dprintf.h"

static struct bngrw_config sync_io_cfg;
static struct wajv_config wajv_cfg;
static struct namsec_config namsec_cfg;

HRESULT sync_io_init(void)
{

    bngrw_config_load(&sync_io_cfg, L".\\segatools.ini");
    wajv_config_load(&wajv_cfg, L".\\segatools.ini");
    namsec_config_load(&namsec_cfg, L".\\segatools.ini");

    bngrw_init(&sync_io_cfg, false);

    keychip_init(&namsec_cfg);

    if (wajv_cfg.enable){
	    wajv_init(&wajv_cfg);
    }

	api_init();

    return S_OK;
}
