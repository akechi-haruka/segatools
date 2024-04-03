#include <windows.h>

#include <assert.h>

#include "hooklib/dns.h"

#include "platform/dns.h"

HRESULT dns_platform_hook_init(const struct dns_config *cfg)
{
    HRESULT hr;

    assert(cfg != NULL);

    if (!cfg->enable) {
        return S_FALSE;
    }

    hr = dns_hook_push(L"tenporouter.loc", cfg->router);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"bbrouter.loc", cfg->router);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"naominet.jp", cfg->startup);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"anbzvarg.wc", cfg->startup);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"ib.naominet.jp", cfg->billing);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"vo.anbzvarg.wc", cfg->billing);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"aime.naominet.jp", cfg->aimedb);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"nvzr.anbzvarg.wc", cfg->aimedb);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"p.eagate.573.jp", cfg->eagate);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"p.eagate.dev.573.jp", cfg->eagate);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"konmai-emu.locl", cfg->eagate);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"at.auth.sys-all.net", cfg->startup);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"at.sys-all.net", cfg->startup);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"op.auth.sys-all.net", cfg->startup);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"sdhd.amlog.auth.sys-all.net", cfg->startup);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"sdhd.d-amlog.auth.sys-all.net", cfg->startup);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"SDDP.amlog.sys-all.net", cfg->startup);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"SDDP.d-amlog.auth.sys-all.net", cfg->startup);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"SDEC.d-amlog.auth.sys-all.net", cfg->startup);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"SDEC.amlog.auth.sys-all.net", cfg->startup);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"amk3-stg.nbgi-amnet.jp", cfg->startup);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"api-aime.am-all.net", cfg->startup);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"tasms-api-basis.thincacloud.com", cfg->startup);

    if (FAILED(hr)) {
        return hr;
    }

    // if your ISP resolves bad domains, it will kill the network. These 2
    // *cannot* resolve

    hr = dns_hook_push(L"mobirouter.loc", NULL);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"dslrouter.loc", NULL);

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}
