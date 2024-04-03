#pragma once

#include <windows.h>

#include <stddef.h>

struct dns_hook_entry {
    wchar_t *from;
    wchar_t *to;
};

// if to_src is NULL, all lookups for from_src will fail
HRESULT dns_hook_push(const wchar_t *from_src, const wchar_t *to_src);

wchar_t* dns_hook_check(const wchar_t *from_src);

