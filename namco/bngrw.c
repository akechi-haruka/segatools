#include <windows.h>

#include <stdint.h>

#include "api/api.h"

#include "hook/table.h"

#include "namco/bngrw.h"
#include "namco/config.h"

#include "util/dprintf.h"

static struct bngrw_config* bngrw_cfg;
static PCRDATA cr_operation;
static char* bngrw_card_id = NULL;
static bool is_scanning = false;
static bool dev_reset_hook = false;

static _stdcall int hook_BngRwAttach(int a1, char *Str2, int a3, int a4, BngRwCallback func, void* a6);
static _stdcall int hook_BngRwDevReset(int a1, BngRwCallback a2, void* a3);
static _stdcall int hook_BngRwExReadMifareAllBlock(int a1, int a2);
static _stdcall int hook_BngRwFin();
static _stdcall int hook_BngRwGetFwVersion(int a1);
static _stdcall int hook_BngRwGetStationID(int a1);
static _stdcall int hook_BngRwGetTotalRetryCount(int a1);
static const char* hook_BngRwGetVersion();
static _stdcall int hook_BngRwInit();
static _stdcall int hook_BngRwIsCmdExec(int a1);
static _stdcall int hook_BngRwReqAction(int a1, int a2, int a3, int a4);
static _stdcall int hook_BngRwReqAiccAuth(unsigned int a1, signed int a2, int a3, DWORD *a4, int a5, int a6, int *a7);
static _stdcall int hook_BngRwReqBeep(int a1, int a2, int a3, int a4);
static _stdcall int hook_BngRwReqCancel(int a1);
static _stdcall int hook_BngRwReqFwCleanup(int a1, int a2, int a3);
static _stdcall int hook_BngRwReqFwVersionup(int a1, int a2, int a3, int a4);
static _stdcall int hook_BngRwReqLatchID(int a1, int a2, int a3);
static _stdcall int hook_BngRwReqLed(unsigned int a1, unsigned int a2, int a3, int a4);
static _stdcall int hook_BngRwReqSendMailTo(int a1, int a2, int a3, int a4, void *Src, void *a6, void *a7, void *a8, int a9, int a10);
static _stdcall int hook_BngRwReqSendUrlTo(int a1, int a2, int a3, int a4, void *Src, void *a6, int a7, int a8);
static _stdcall int hook_BngRwReqWaitTouch(int a1, int a2, int a3, BngRwReqCallback a4, void* a5);
static _stdcall int hook_BngRwSetLedPower(int a1, int a2, int a3);


static const struct hook_symbol sync_card_reader_syms[] = {
	{
		.name   = "_BngRwAttach@24",
		.patch  = hook_BngRwAttach,
		.ordinal= 1,
	},
	{
		.name   = "_BngRwDevReset@12",
		.patch  = hook_BngRwDevReset,
		.ordinal= 2,
	},
	{
		.name   = "_BngRwExReadMifareAllBlock@8",
		.patch  = hook_BngRwExReadMifareAllBlock,
		.ordinal= 3,
	},
	{
		.name   = "_BngRwFin@0",
		.patch  = hook_BngRwFin,
		.ordinal= 4,
	},
	{
		.name   = "_BngRwGetFwVersion@4",
		.patch  = hook_BngRwGetFwVersion,
		.ordinal= 5,
	},
	{
		.name   = "_BngRwGetStationID@4",
		.patch  = hook_BngRwGetStationID,
		.ordinal= 6,
	},
	{
		.name   = "_BngRwGetTotalRetryCount@4",
		.patch  = hook_BngRwGetTotalRetryCount,
		.ordinal= 7,
	},
	{
        .name   = "_BngRwGetVersion@0",
        .patch  = hook_BngRwGetVersion,
		.ordinal= 8,
    },
	{
		.name   = "_BngRwInit@0",
		.patch  = hook_BngRwInit,
		.ordinal= 9,
	},
	{
		.name   = "_BngRwIsCmdExec@4",
		.patch  = hook_BngRwIsCmdExec,
		.ordinal= 10,
	},
	{
		.name   = "_BngRwReqAction@16",
		.patch  = hook_BngRwReqAction,
		.ordinal= 11,
	},
	{
		.name   = "_BngRwReqAiccAuth@28",
		.patch  = hook_BngRwReqAiccAuth,
		.ordinal= 12,
	},
	{
		.name   = "_BngRwReqBeep@16",
		.patch  = hook_BngRwReqBeep,
		.ordinal= 13,
	},
	{
		.name   = "_BngRwReqCancel@4",
		.patch  = hook_BngRwReqCancel,
		.ordinal= 14,
	},
	{
		.name   = "_BngRwReqFwCleanup@12",
		.patch  = hook_BngRwReqFwCleanup,
		.ordinal= 15,
	},
	{
		.name   = "_BngRwReqFwVersionup@16",
		.patch  = hook_BngRwReqFwVersionup,
		.ordinal= 16,
	},
	{
		.name   = "_BngRwReqLatchID@12",
		.patch  = hook_BngRwReqLatchID,
		.ordinal= 17,
	},
	{
		.name   = "_BngRwReqLed@16",
		.patch  = hook_BngRwReqLed,
		.ordinal= 18,
	},
	{
		.name   = "_BngRwReqSendMailTo@40",
		.patch  = hook_BngRwReqSendMailTo,
		.ordinal= 19,
	},
	{
		.name   = "_BngRwReqSendUrlTo@32",
		.patch  = hook_BngRwReqSendUrlTo,
		.ordinal= 20,
	},
	{
		.name   = "_BngRwReqWaitTouch@20",
		.patch  = hook_BngRwReqWaitTouch,
		.ordinal= 21,
	},
	{
		.name   = "_BngRwSetLedPower@12",
		.patch  = hook_BngRwSetLedPower,
		.ordinal= 22,
	},
};

HRESULT bngrw_init(struct bngrw_config* cfg, bool dev_reset_must_callback)
{

    if (!cfg->enable){
        return S_FALSE;
    }

    bngrw_cfg = cfg;
    dev_reset_hook = dev_reset_must_callback;

	hook_table_apply(
        NULL,
        "bngrw.dll",
        sync_card_reader_syms,
        _countof(sync_card_reader_syms));

    dprintf("BNGRW: initialized\n");

    return S_OK;
}

// used
static _stdcall int hook_BngRwAttach(int a1, char *Str2, int a3, int a4, BngRwCallback func, void* arg){
	dprintf("BNGRW: hook_BngRwAttach\n");
	if (func != NULL){
        func(0, 0, arg);
	}
	return 1;
}

// used
static _stdcall int hook_BngRwDevReset(int a1, BngRwCallback func, void* arg){
	dprintf("BNGRW: hook_BngRwDevReset\n");
	if (func != NULL && dev_reset_hook){
	    //DebugBreak();
	    dprintf("BNGRW: callback function\n");
        func(0, 0, arg);
        return 1;
    }
	return 0;
}

static _stdcall int hook_BngRwExReadMifareAllBlock(int a1, int a2){
	dprintf("BNGRW: hook_BngRwExReadMifareAllBlock (stub)\n");
	return -101;
}

// used
static _stdcall int hook_BngRwFin(){
	dprintf("BNGRW: hook_BngRwFin\n");
	free(bngrw_card_id);
	bngrw_card_id = NULL;
	return 0;
}

static _stdcall int hook_BngRwGetFwVersion(int a1){
	dprintf("BNGRW: hook_BngRwGetFwVersion (stub)\n");
	return -101;
}

static _stdcall int hook_BngRwGetStationID(int a1){
	dprintf("BNGRW: hook_BngRwGetStationID (stub)\n");
	return -101;
}

static _stdcall int hook_BngRwGetTotalRetryCount(int a1){
	dprintf("BNGRW: hook_BngRwGetTotalRetryCount (stub)\n");
	return -101;
}

static const char* hook_BngRwGetVersion(){
	return "Ver 1.6.0";
}

// used
static _stdcall int hook_BngRwInit(){
	dprintf("BNGRW: Initializing\n");
	bngrw_card_id = malloc(22);
	return 0;
}

// used
static _stdcall int hook_BngRwIsCmdExec(int a1){
	if (a1 != 0){
		//dprintf("BNGRW: hook_BngRwIsCmdExec: %d\n", a1);
		if (a1 == 1 && is_scanning){
		    return 1;
		}
	}
	return 0;
}

static _stdcall int hook_BngRwReqAction(int a1, int a2, int a3, int a4){
	dprintf("BNGRW: hook_BngRwReqAction (stub)\n");
	return -101;
}

static _stdcall int hook_BngRwReqAiccAuth(unsigned int a1, signed int a2, int a3, DWORD *a4, int a5, int a6, int *a7){
	dprintf("BNGRW: hook_BngRwReqAiccAuth (stub)\n");
	return -101;
}

// used
static _stdcall int hook_BngRwReqBeep(int a1, int a2, int a3, int a4){
	dprintf("BNGRW: hook_BngRwReqBeep: %d, %d, %d, %d\n", a1, a2, a3, a4);
	return 1;
}

// used
static _stdcall int hook_BngRwReqCancel(int a1){
	dprintf("BNGRW: hook_BngRwReqCancel\n");
    if (cr_operation != NULL){
	    dprintf("BNGRW: stopping reader\n");
        cr_operation->timeout = 5;
    }
	return 1;
}

static _stdcall int hook_BngRwReqFwCleanup(int a1, int a2, int a3){
	dprintf("BNGRW: hook_BngRwReqFwCleanup (stub)\n");
	return -101;
}

static _stdcall int hook_BngRwReqFwVersionup(int a1, int a2, int a3, int a4){
	dprintf("BNGRW: hook_BngRwReqFwVersionup (stub)\n");
	return -101;
}

static _stdcall int hook_BngRwReqLatchID(int a1, int a2, int a3){
	dprintf("BNGRW: hook_BngRwReqLatchID\n");
	return -101;
}

// used
static _stdcall int hook_BngRwReqLed(unsigned int a1, unsigned int a2, int a3, int a4){
	dprintf("BNGRW: hook_BngRwReqLed: %d, %d, %d, %d\n", a1, a2, a3, a4);
	return 1;
}

static _stdcall int hook_BngRwReqSendMailTo(int a1, int a2, int a3, int a4, void *Src, void *a6, void *a7, void *a8, int a9, int a10){
	dprintf("BNGRW: hook_BngRwReqSendMailTo\n");
	return -101;
}

// used
static _stdcall int hook_BngRwReqSendUrlTo(int a1, int a2, int a3, int a4, void *Src, void *a6, int a7, int a8){
	dprintf("BNGRW: hook_BngRwReqSendUrlTo\n");
	return 0;
}

static HRESULT bngrw_read_id_file(
        const wchar_t *path,
        char *bytes,
        size_t nbytes)
{
    FILE *f = _wfopen(path, L"r");

    if (f == NULL) {
        return S_FALSE;
    }

    char* ret = fgets(bytes, nbytes, f);
    if (ret == NULL){
        dprintf("BNGRW: %S: failed reading\n", path);
        return S_FALSE;
    }

    if (f != NULL) {
        fclose(f);
    }

    return S_OK;
}

void tohex(unsigned char * in, size_t insz, char * out, size_t outsz)
{
    unsigned char * pin = in;
    const char * hex = "0123456789ABCDEF";
    char * pout = out;
    for(; pin < in+insz; pout +=2, pin++){
        pout[0] = hex[(*pin>>4) & 0xF];
        pout[1] = hex[ *pin     & 0xF];
        if (pout + 2 - out > outsz){
            /* Better to truncate output string than overflow buffer */
            /* it would be still better to either return a status */
            /* or ensure the target buffer is large enough and it never happen */
            break;
        }
    }
    pout[-1] = 0;
}

DWORD WINAPI BngRwReqWaitTouchThread(void* data) {
	PCRDATA arg = (PCRDATA)data;
	if (arg->on_scan != NULL){
		dprintf("BNGRW: scanning (with timeout %d)...\n", arg->timeout);
		while (arg->timeout == -1 || arg->timeout --> 0){
			if (GetAsyncKeyState(bngrw_cfg->vk_scan) & 0x8000 || api_get_felica_inserted() || api_get_aime_inserted()){
				dprintf("BNGRW: detected key down\n");

				if (api_get_felica_inserted()){
					dprintf("API: Getting FeliCA from API\n");
					uint8_t* felica = api_get_felica_id();
					tohex(felica, 32, bngrw_card_id, 22);
					bngrw_card_id[CARD_LEN_FELICA] = 0;
					api_clear_cards();
				} else if (api_get_aime_inserted()){
					dprintf("API: Getting AIME from API\n");
					uint8_t* aime = api_get_aime_id();
                    tohex(aime, 32, bngrw_card_id, 22);
					bngrw_card_id[CARD_LEN_AIME] = 0;
					api_clear_cards();
				} else {
					HRESULT hr = bngrw_read_id_file(
							bngrw_cfg->banapass_path,
							bngrw_card_id,
							21);

					if (!SUCCEEDED(hr) || hr == S_FALSE) {
						dprintf("BNGRW: %S read failure: %ld\n", bngrw_cfg->banapass_path, hr);
						Sleep(1000);
						continue;
					}
				}

                dprintf("BNGRW: card id: %s\n", bngrw_card_id);

                arg->response.hCardHandle.eCardType = 3;
                arg->response.hCardHandle.iIdLen = 16;
                arg->response.hCardHandle.iFelicaOS = 1;
                strcpy((char*)arg->response.hCardHandle.ucChipId, "0AAAAAAAAAAAAAAA");
                strcpy((char*)arg->response.ucChipId, "0AAAAAAAAAAAAAAA");
                strcpy((char*)arg->response.strChipId, "ABCDWHATABCDWHATABCDWHATABCDWHAT1234");
                strcpy((char*)arg->response.strAccessCode, bngrw_card_id);
                arg->response.eIdType = 0;
                arg->response.eCardType = 3;
                arg->response.eCarrierType = 1;
                arg->response.iFelicaOS = 1;
                strcpy((char*)arg->response.strBngProductId, "12345678");
                arg->response.uiBngId = 123456789;
                arg->response.usBngTypeCode = 1;
                arg->response.ucBngRegionCode = 1;
                strcpy((char*)arg->response.ucBlock1, "0AAAAAAAAAAAAAAA");
                strcpy((char*)arg->response.ucBlock2, "0AAAAAAAAAAAAAAA");

				dprintf("BNGRW: sending to game\n");
				arg->on_scan(0, 0, (void*)&(arg->response), arg->card_reader);
				is_scanning = false;
				return 0;
			}
			Sleep(10);
		}
		dprintf("BNGRW: timed out (%d)\n", arg->timeout);
		arg->on_scan(0, 3, (void*)&(arg->response), arg->card_reader);
	} else {
		dprintf("BNGRW: no scan function\n");
	}
	is_scanning = false;
    return 0;
}


// used
static _stdcall int hook_BngRwReqWaitTouch(int a1, int timeout, int a3, BngRwReqCallback on_scan, void* a5){
	dprintf("BNGRW: hook_BngRwReqWaitTouch: %d, %d, %x, %p, %p\n", a1, timeout, a3, on_scan, a5);
	PCRDATA arg = (PCRDATA) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CRDATA));
	cr_operation = arg;
	arg->timeout = timeout;
	arg->on_scan = on_scan;
	arg->card_reader = a5;
	if (CreateThread(NULL, 0, BngRwReqWaitTouchThread, arg, 0, NULL) == NULL){
		dprintf("BNGRW: thread start fail");
	} else {
	    is_scanning = true;
	}
	return 1;
}

static _stdcall int hook_BngRwSetLedPower(int a1, int a2, int a3){
	dprintf("BNGRW: hook_BngRwSetLedPower (stub)\n");
	return -101;
}

