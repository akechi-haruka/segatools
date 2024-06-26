#include <windows.h>
#include <wincrypt.h>
#include <iphlpapi.h>
#include <winsock.h>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hook/table.h"

#include "platform/netenv.h"
#include "platform/nusec.h"

#include "util/dprintf.h"

struct netenv {
    IP_ADAPTER_ADDRESSES head;
    char name[64];
    wchar_t dns_suffix[64];
    wchar_t description[64];
    wchar_t friendly_name[64];
    IP_ADAPTER_PREFIX prefix;
    IP_ADAPTER_UNICAST_ADDRESS iface;
    IP_ADAPTER_GATEWAY_ADDRESS router;
    IP_ADAPTER_DNS_SERVER_ADDRESS dns;
    struct sockaddr_in prefix_sa;
    struct sockaddr_in iface_sa;
    struct sockaddr_in router_sa;
    struct sockaddr_in dns_sa;
};

/* Hook functions */

static uint32_t WINAPI hook_GetAdaptersAddresses(
        uint32_t Family,
        uint32_t Flags,
        void *Reserved,
        IP_ADAPTER_ADDRESSES *AdapterAddresses,
        uint32_t *SizePointer);

static uint32_t WINAPI hook_GetAdaptersInfo(
        IP_ADAPTER_INFO *AdapterInfo,
        uint32_t *SizePointer);

static uint32_t WINAPI hook_IpRenewAddress(
        PIP_ADAPTER_INDEX_MAP AdapterInfo);

static uint32_t WINAPI hook_GetBestRoute(
        uint32_t src_ip,
        uint32_t dest_ip,
        MIB_IPFORWARDROW *route);

static uint32_t WINAPI hook_GetIfTable(
        MIB_IFTABLE *pIfTable,
        uint32_t *pdwSize,
        BOOL bOrder);

static uint32_t WINAPI hook_IcmpSendEcho(
        HANDLE IcmpHandle,
        IPAddr DestinationAddress,
        LPVOID RequestData,
        WORD RequestSize,
        PIP_OPTION_INFORMATION RequestOptions,
        LPVOID ReplyBuffer,
        DWORD ReplySize,
        DWORD Timeout);

static uint32_t WINAPI hook_IcmpSendEcho2(
        HANDLE IcmpHandle,
        HANDLE Event,
        PIO_APC_ROUTINE ApcRoutine,
        void *ApcContext,
        uint32_t DestinationAddress,
        void *RequestData,
        uint16_t RequestSize,
        IP_OPTION_INFORMATION *RequestOptions,
        void *ReplyBuffer,
        uint32_t ReplySize,
        uint32_t Timeout);

static int hook_setsockopt(
        SOCKET     s,
        int        level,
        int        optname,
        const char *optval,
        int        optlen
);

static __stdcall BOOL hook_GetRTTAndHopCount(
 IPAddr DestIpAddress,
 PULONG HopCount,
 ULONG  MaxHops,
 PULONG RTT);

static int WSAAPI hook_WSAIoctl(
  SOCKET                             s,
  DWORD                              dwIoControlCode,
  LPVOID                             lpvInBuffer,
  DWORD                              cbInBuffer,
  LPVOID                             lpvOutBuffer,
  DWORD                              cbOutBuffer,
  LPDWORD                            lpcbBytesReturned,
  LPWSAOVERLAPPED                    lpOverlapped,
  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

/* Link pointers */

static uint32_t (WINAPI *next_GetAdaptersAddresses)(
        uint32_t Family,
        uint32_t Flags,
        void *Reserved,
        IP_ADAPTER_ADDRESSES *AdapterAddresses,
        uint32_t *SizePointer);

static uint32_t (WINAPI *next_GetAdaptersInfo)(
        IP_ADAPTER_INFO *AdapterInfo,
        uint32_t *SizePointer);

static uint32_t (WINAPI *next_IpRenewAddress)(
        PIP_ADAPTER_INDEX_MAP AdapterInfo);

static uint32_t (WINAPI *next_GetBestRoute)(
        uint32_t src_ip,
        uint32_t dest_ip,
        MIB_IPFORWARDROW *route);

static uint32_t (WINAPI *next_GetIfTable)(
        MIB_IFTABLE *pIfTable,
        uint32_t *pdwSize,
        BOOL bOrder);

static uint32_t (WINAPI *next_IcmpSendEcho)(
        HANDLE IcmpHandle,
        IPAddr DestinationAddress,
        LPVOID RequestData,
        WORD RequestSize,
        PIP_OPTION_INFORMATION RequestOptions,
        LPVOID ReplyBuffer,
        DWORD ReplySize,
        DWORD Timeout);

static uint32_t (WINAPI *next_IcmpSendEcho2)(
        HANDLE IcmpHandle,
        HANDLE Event,
        PIO_APC_ROUTINE ApcRoutine,
        void *ApcContext,
        uint32_t DestinationAddress,
        void *RequestData,
        uint16_t RequestSize,
        IP_OPTION_INFORMATION *RequestOptions,
        void *ReplyBuffer,
        uint32_t ReplySize,
        uint32_t Timeout);

static int (*next_setsockopt)(
        SOCKET     s,
        int        level,
        int        optname,
        const char *optval,
        int        optlen
);

static __stdcall BOOL (*next_GetRTTAndHopCount)(
 IPAddr DestIpAddress,
 PULONG HopCount,
 ULONG  MaxHops,
 PULONG RTT);

static int WSAAPI (*next_WSAIoctl)(
         SOCKET                             s,
         DWORD                              dwIoControlCode,
         LPVOID                             lpvInBuffer,
         DWORD                              cbInBuffer,
         LPVOID                             lpvOutBuffer,
         DWORD                              cbOutBuffer,
         LPDWORD                            lpcbBytesReturned,
         LPWSAOVERLAPPED                    lpOverlapped,
         LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
       );

static const struct hook_symbol netenv_hook_syms[] = {
    {
        .name   = "GetAdaptersAddresses",
        .patch  = hook_GetAdaptersAddresses,
        .link   = (void **) &next_GetAdaptersAddresses,
    }, {
        .name   = "GetAdaptersInfo",
        .patch  = hook_GetAdaptersInfo,
        .link   = (void **) &next_GetAdaptersInfo,
    },{
        .name   = "IpRenewAddress",
        .patch  = hook_IpRenewAddress,
        .link   = (void **) &next_IpRenewAddress,
    }, {
        .name   = "GetBestRoute",
        .patch  = hook_GetBestRoute,
        .link   = (void **) &next_GetBestRoute,
    }, {
        .name   = "GetIfTable",
        .patch  = hook_GetIfTable,
        .link   = (void **) &next_GetIfTable,
    }, {
       .name   = "IcmpSendEcho",
       .patch  = hook_IcmpSendEcho,
       .link   = (void **) &next_IcmpSendEcho,
    }, {
        .name   = "IcmpSendEcho2",
        .patch  = hook_IcmpSendEcho2,
        .link   = (void **) &next_IcmpSendEcho2,
    }, {
        .name   = "GetRTTAndHopCount",
        .patch  = hook_GetRTTAndHopCount,
        .link   = (void **) &next_GetRTTAndHopCount,
    },
};

static const struct hook_symbol netenv_hook_syms_soft[] = {
    {
        .name   = "IpRenewAddress",
        .patch  = hook_IpRenewAddress,
        .link   = (void **) &next_IpRenewAddress,
    }, {
       .name   = "IcmpSendEcho",
       .patch  = hook_IcmpSendEcho,
       .link   = (void **) &next_IcmpSendEcho,
    }, {
        .name   = "IcmpSendEcho2",
        .patch  = hook_IcmpSendEcho2,
        .link   = (void **) &next_IcmpSendEcho2,
    }, {
        .name   = "GetRTTAndHopCount",
        .patch  = hook_GetRTTAndHopCount,
        .link   = (void **) &next_GetRTTAndHopCount,
    },
};

static const struct hook_symbol netenv_ws2_hook_syms[] = {
    {
        .name   = "setsockopt_",
        .patch  = hook_setsockopt,
        .link   = (void **) &next_setsockopt,
    },
    {
        .name   = "WSAIoctl",
        .patch  = hook_WSAIoctl,
        .link   = (void **) &next_WSAIoctl,
    },
};

static uint32_t netenv_ip_prefix;
static uint32_t netenv_ip_iface;
static uint32_t netenv_ip_router;
static uint32_t netenv_ip_bcast;
static uint8_t netenv_mac_addr[6];
static uint8_t use_tracert_bypass;

HRESULT netenv_hook_init(
        const struct netenv_config *cfg,
        const struct nusec_config *kc_cfg)
{
    assert(cfg != NULL);
    assert(kc_cfg != NULL);

    if (!cfg->enable) {
        return S_FALSE;
    }

    dprintf("Init: netenv_hook_init\n");

    if (!kc_cfg->enable && !cfg->force) {
        dprintf("Netenv: Keychip emu is off? Disabling Netenv emu.\n");

        return S_FALSE;
    }

    netenv_ip_prefix = kc_cfg->subnet;
    netenv_ip_iface = kc_cfg->subnet | cfg->addr_suffix;
    netenv_ip_router = kc_cfg->subnet | cfg->router_suffix;
    if (cfg->full_addr){
        netenv_ip_iface = cfg->virtual_ipaddr;
        netenv_ip_router = cfg->virtual_router;
        netenv_ip_bcast = cfg->virtual_bcast;
    }
    use_tracert_bypass = cfg->use_tracert_bypass;
    memcpy(netenv_mac_addr, cfg->mac_addr, sizeof(netenv_mac_addr));

    if (cfg->enable == 2){
        hook_table_apply(
            NULL,
            "iphlpapi.dll",
            netenv_hook_syms_soft,
            _countof(netenv_hook_syms_soft));
    } else {
        hook_table_apply(
                NULL,
                "iphlpapi.dll",
                netenv_hook_syms,
                _countof(netenv_hook_syms));
    }
    hook_table_apply(
            NULL,
            "Ws2_32.dll",
            netenv_ws2_hook_syms,
            _countof(netenv_ws2_hook_syms));

    return S_OK;
}

static uint32_t WINAPI hook_GetAdaptersAddresses(
        uint32_t Family,
        uint32_t Flags,
        void *Reserved,
        IP_ADAPTER_ADDRESSES *AdapterAddresses,
        uint32_t *SizePointer)
{
    /* This hook errs on the side of caution and returns a lot more
       information than the ALLNET lib cares about. MSVC mangles the main
       call site for this API quite aggressively, so by the time we decompile
       the code in question it's a little difficult to tell which pieces the
       ALLNET lib pays attention to. */

    uint32_t nbytes;
    struct netenv *env;

    if (Reserved != NULL || SizePointer == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    nbytes = *SizePointer;
    *SizePointer = sizeof(*env);

    if (AdapterAddresses == NULL || nbytes < sizeof(*env)) {
        return ERROR_BUFFER_OVERFLOW;
    }

    env = CONTAINING_RECORD(AdapterAddresses, struct netenv, head);
    memset(env, 0, sizeof(*env));

    env->head.Length = sizeof(env->head);
    env->head.IfIndex = 1;
    env->head.AdapterName = env->name;
    env->head.FirstUnicastAddress = &env->iface;
    env->head.FirstDnsServerAddress = &env->dns;
    env->head.DnsSuffix = env->dns_suffix;
    env->head.Description = env->description;
    env->head.FriendlyName = env->friendly_name;
    memcpy( env->head.PhysicalAddress,
            netenv_mac_addr,
            sizeof(netenv_mac_addr));
    env->head.PhysicalAddressLength = sizeof(netenv_mac_addr);
    env->head.Flags = IP_ADAPTER_DHCP_ENABLED | IP_ADAPTER_IPV4_ENABLED;
    env->head.Mtu = 4200; /* idk what's typical here */
    env->head.IfType = IF_TYPE_ETHERNET_CSMACD;
    env->head.OperStatus = IfOperStatusUp;
    env->head.FirstPrefix = &env->prefix;
    env->head.FirstGatewayAddress = &env->router;

    strcpy_s(
            env->name,
            _countof(env->name),
            "{00000000-0000-0000-0000-000000000000}");

    wcscpy_s(
            env->dns_suffix,
            _countof(env->dns_suffix),
            L"local");

    wcscpy_s(
            env->description,
            _countof(env->description),
            L"Interface Description");

    wcscpy_s(
            env->friendly_name,
            _countof(env->friendly_name),
            L"Fake Ethernet");

    env->iface.Length = sizeof(env->iface);
    env->iface.Flags = 0;
    env->iface.Address.lpSockaddr = (struct sockaddr *) &env->iface_sa;
    env->iface.Address.iSockaddrLength = sizeof(env->iface_sa);
    env->iface.PrefixOrigin = IpPrefixOriginDhcp;
    env->iface.SuffixOrigin = IpSuffixOriginDhcp;
    env->iface.DadState = IpDadStatePreferred;
    env->iface.ValidLifetime = UINT32_MAX;
    env->iface.PreferredLifetime = UINT32_MAX;
    env->iface.LeaseLifetime = 86400;
    env->iface.OnLinkPrefixLength = 24;

    env->prefix.Length = sizeof(env->prefix);
    env->prefix.Address.lpSockaddr = (struct sockaddr *) &env->prefix_sa;
    env->prefix.Address.iSockaddrLength = sizeof(env->prefix_sa);
    env->prefix.PrefixLength = 24;

    env->router.Length = sizeof(env->router);
    env->router.Address.lpSockaddr = (struct sockaddr *) &env->router_sa;
    env->router.Address.iSockaddrLength = sizeof(env->router_sa);

    env->dns.Length = sizeof(env->dns);
    env->dns.Address.lpSockaddr = (struct sockaddr *) &env->dns_sa;
    env->dns.Address.iSockaddrLength = sizeof(env->dns_sa);

    env->prefix_sa.sin_family = AF_INET;
    env->prefix_sa.sin_addr.s_addr = _byteswap_ulong(netenv_ip_prefix);

    env->iface_sa.sin_family = AF_INET;
    env->iface_sa.sin_addr.s_addr = _byteswap_ulong(netenv_ip_iface);

    env->router_sa.sin_family = AF_INET;
    env->router_sa.sin_addr.s_addr = _byteswap_ulong(netenv_ip_router);

    env->dns_sa.sin_family = AF_INET;
    env->dns_sa.sin_addr.s_addr = _byteswap_ulong(netenv_ip_router);

    return ERROR_SUCCESS;
}

static uint32_t WINAPI hook_IpRenewAddress(
    PIP_ADAPTER_INDEX_MAP AdapterInfo)
{
    dprintf("Netenv: Blocked IPRenewAddress\n");
    return 0;
}

static uint32_t WINAPI hook_GetAdaptersInfo(
        IP_ADAPTER_INFO *ai,
        uint32_t *nbytes_inout)
{
    IP_ADDR_STRING iface;
    IP_ADDR_STRING router;
    uint32_t nbytes;

    dprintf("Netenv: GetAdaptersInfo: Virtualized LAN configuration:\n");
    dprintf("Netenv: Interface IP :   %3i.%3i.%3i.%3i\n",
            (uint8_t) (netenv_ip_iface >> 24),
            (uint8_t) (netenv_ip_iface >> 16),
            (uint8_t) (netenv_ip_iface >>  8),
            (uint8_t) (netenv_ip_iface      ));
    dprintf("Netenv: Router IP    :   %3i.%3i.%3i.%3i\n",
            (uint8_t) (netenv_ip_router >> 24),
            (uint8_t) (netenv_ip_router >> 16),
            (uint8_t) (netenv_ip_router >>  8),
            (uint8_t) (netenv_ip_router      ));
    dprintf("Netenv: MAC Address  : %02x:%02x:%02x:%02x:%02x:%02x\n",
            netenv_mac_addr[0],
            netenv_mac_addr[1],
            netenv_mac_addr[2],
            netenv_mac_addr[3],
            netenv_mac_addr[4],
            netenv_mac_addr[5]);

    if (nbytes_inout == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    nbytes = *nbytes_inout;
    *nbytes_inout = sizeof(*ai);

    if (ai == NULL || nbytes < sizeof(*ai)) {
        return ERROR_BUFFER_OVERFLOW;
    }

    memset(&iface, 0, sizeof(iface));
    memset(&router, 0, sizeof(router));

    sprintf_s(
            iface.IpAddress.String,
            _countof(iface.IpAddress.String),
            "%i.%i.%i.%i",
            (uint8_t) (netenv_ip_iface >> 24),
            (uint8_t) (netenv_ip_iface >> 16),
            (uint8_t) (netenv_ip_iface >>  8),
            (uint8_t) (netenv_ip_iface      ));

    strcpy_s(
            iface.IpMask.String,
            _countof(iface.IpMask.String),
            "255.255.255.0");

    sprintf_s(
            router.IpAddress.String,
            _countof(iface.IpAddress.String),
            "%i.%i.%i.%i",
            (uint8_t) (netenv_ip_router >> 24),
            (uint8_t) (netenv_ip_router >> 16),
            (uint8_t) (netenv_ip_router >>  8),
            (uint8_t) (netenv_ip_router      ));

    strcpy_s(
            router.IpMask.String,
            _countof(router.IpMask.String),
            "255.255.255.0");

    memset(ai, 0, sizeof(*ai));
    strcpy_s(
            ai->AdapterName,
            _countof(ai->AdapterName),
            "Fake Ethernet");
    strcpy_s(ai->Description,
            _countof(ai->Description),
            "Adapter Description");
    ai->AddressLength = sizeof(netenv_mac_addr);
    memcpy(ai->Address, netenv_mac_addr, sizeof(netenv_mac_addr));
    ai->Index = 1;
    ai->Type = MIB_IF_TYPE_ETHERNET;
    ai->DhcpEnabled = 1;
    memcpy(&ai->IpAddressList, &iface, sizeof(iface));
    memcpy(&ai->GatewayList, &router, sizeof(router));
    memcpy(&ai->DhcpServer, &router, sizeof(router));
    ai->LeaseObtained = time(NULL) - 3600;
    ai->LeaseExpires = time(NULL) + 86400;

    return ERROR_SUCCESS;
}

static uint32_t WINAPI hook_GetBestRoute(
        uint32_t src_ip,
        uint32_t dest_ip,
        MIB_IPFORWARDROW *route)
{
    if (route == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    dprintf("Netenv: GetBestRoute ip4 %x -> ip4 %x\n",
            (int) _byteswap_ulong(src_ip),
            (int) _byteswap_ulong(dest_ip));

    memset(route, 0, sizeof(*route));

    /* This doesn't seem to get read? It just needs to succeed. */

    route->dwForwardDest = 0x00000000;
    route->dwForwardMask = 0xFFFFFFFF;
    route->dwForwardPolicy = 0; /* idk */
    route->dwForwardNextHop = _byteswap_ulong(netenv_ip_router);
    route->dwForwardIfIndex = 1;
    route->dwForwardType = MIB_IPROUTE_TYPE_INDIRECT;
    route->dwForwardProto = MIB_IPPROTO_NETMGMT;

    return ERROR_SUCCESS;
}

static uint32_t WINAPI hook_GetIfTable(
        MIB_IFTABLE *pIfTable,
        uint32_t *pdwSize,
        BOOL bOrder)
{
    MIB_IFROW *row;
    uint32_t nbytes;

    if (pdwSize == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    nbytes = *pdwSize;
    *pdwSize = sizeof(*row) + sizeof(DWORD);

    if (pIfTable == NULL || nbytes < sizeof(*row) + sizeof(DWORD)) {
        return ERROR_BUFFER_OVERFLOW;
    }

    pIfTable->dwNumEntries = 1;

    row = pIfTable->table;
    memset(row, 0, sizeof(*row));

    wcscpy_s(row->wszName, _countof(row->wszName), L"Fake Ethernet");
    row->dwIndex = 1; /* Should match other IF_INDEX fields we return */
    row->dwType = IF_TYPE_ETHERNET_CSMACD;
    row->dwMtu = 4200; /* I guess? */
    row->dwSpeed = 1000000000;
    row->dwPhysAddrLen = sizeof(netenv_mac_addr);
    memcpy(row->bPhysAddr, netenv_mac_addr, sizeof(netenv_mac_addr));
    row->dwAdminStatus = 1;
    row->dwOperStatus = IF_OPER_STATUS_OPERATIONAL;

    return ERROR_SUCCESS;
}

static uint32_t WINAPI hook_IcmpSendEcho(
        HANDLE IcmpHandle,
        IPAddr DestinationAddress,
        LPVOID RequestData,
        WORD RequestSize,
        PIP_OPTION_INFORMATION RequestOptions,
        LPVOID ReplyBuffer,
        DWORD ReplySize,
        DWORD Timeout)
{
    ICMP_ECHO_REPLY *pong;

    if (IcmpHandle == NULL || IcmpHandle == INVALID_HANDLE_VALUE) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return 0;
    }

    if (ReplyBuffer == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return 0;
    }

    if (ReplySize < sizeof(ICMP_ECHO_REPLY)) {
        SetLastError(IP_BUF_TOO_SMALL);

        return 0;
    }

    dprintf("Netenv: Virtualized ICMP Ping to ip4 %lx (in=%d)\n", DestinationAddress, RequestSize);

    pong = (ICMP_ECHO_REPLY *) ReplyBuffer;
    memset(pong, 0, sizeof(*pong));
    pong->Address = DestinationAddress;
    pong->Status = IP_SUCCESS;
    pong->RoundTripTime = 1;
    pong->DataSize = RequestSize;
    pong->Reserved = 1; /* Number of ICMP_ECHO_REPLY structs in ReplyBuffer */
    pong->Data = RequestData;

    SetLastError(ERROR_SUCCESS);

    return 1;
}

static uint32_t WINAPI hook_IcmpSendEcho2(
        HANDLE IcmpHandle,
        HANDLE Event,
        PIO_APC_ROUTINE ApcRoutine,
        void *ApcContext,
        uint32_t DestinationAddress,
        void *RequestData,
        uint16_t RequestSize,
        IP_OPTION_INFORMATION *RequestOptions,
        void *ReplyBuffer,
        uint32_t ReplySize,
        uint32_t Timeout)
{
    ICMP_ECHO_REPLY *pong;
    BOOL ok;

    if (IcmpHandle == NULL || IcmpHandle == INVALID_HANDLE_VALUE) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return 0;
    }

    if (ApcRoutine != NULL) {
        dprintf("%s: Got APC routine...\n", __func__);
        SetLastError(ERROR_NOT_SUPPORTED);

        return 0;
    }

    if (ReplyBuffer == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return 0;
    }

    if (ReplySize < sizeof(ICMP_ECHO_REPLY)) {
        SetLastError(IP_BUF_TOO_SMALL);

        return 0;
    }

    dprintf("Netenv: Virtualized ICMP Ping to ip4 %x (in=%d)\n",
            (int) _byteswap_ulong(DestinationAddress), RequestSize);

    pong = (ICMP_ECHO_REPLY *) ReplyBuffer;
    memset(pong, 0, sizeof(*pong));
    pong->Address = DestinationAddress;
    pong->Status = IP_SUCCESS;
    pong->RoundTripTime = 1;
    pong->DataSize = RequestSize;
    pong->Reserved = 1; /* Number of ICMP_ECHO_REPLY structs in ReplyBuffer */
    pong->Data = RequestData;

    if (Event != NULL) {
        ok = SetEvent(Event);

        if (ok) {
            SetLastError(ERROR_IO_PENDING);
        }

        return 0;
    }

    dprintf("%s: Unexpected synchronous call...\n", __func__);
    SetLastError(ERROR_SUCCESS);

    return 1;
}

static int hook_setsockopt(
    SOCKET     s,
    int        level,
    int        optname,
    const char *optval,
    int        optlen
){
     dprintf("%s: %d %d %s %d\n", __func__, level, optname, optval, optlen);
     int ret = next_setsockopt(s, level, optname, optval, optlen);
     if (ret == -1){
        dprintf("%s: error: %d\n", __func__, WSAGetLastError());
     }
     return ret;
}

static __stdcall BOOL hook_GetRTTAndHopCount(
 IPAddr DestIpAddress,
 PULONG HopCount,
 ULONG  MaxHops,
 PULONG RTT){
    if (use_tracert_bypass){
        dprintf("Netenv: Virtualizing tracert\n");
        if (HopCount != NULL){
            *HopCount = 1;
        }
        if (RTT != NULL){
            *RTT = 0;
        }
        return true;
    } else {
        return next_GetRTTAndHopCount(DestIpAddress, HopCount, MaxHops, RTT);
    }
}

static int WSAAPI hook_WSAIoctl(
  SOCKET                             s,
  DWORD                              dwIoControlCode,
  LPVOID                             lpvInBuffer,
  DWORD                              cbInBuffer,
  LPVOID                             lpvOutBuffer,
  DWORD                              cbOutBuffer,
  LPDWORD                            lpcbBytesReturned,
  LPWSAOVERLAPPED                    lpOverlapped,
  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
){
    if (dwIoControlCode != SIO_GET_INTERFACE_LIST){
        return next_WSAIoctl(s, dwIoControlCode, lpvInBuffer, cbInBuffer, lpvOutBuffer, cbOutBuffer, lpcbBytesReturned, lpOverlapped, lpCompletionRoutine);
    }

    dprintf("Netenv: WSAIoctl: SIO_GET_INTERFACE_LIST\n");

    INTERFACE_INFO interfaces[1];

    interfaces[0].iiFlags = IFF_UP | IFF_BROADCAST | IFF_MULTICAST;
    interfaces[0].iiAddress.AddressIn.sin_family = AF_INET;
    interfaces[0].iiAddress.AddressIn.sin_port = 0;
    interfaces[0].iiAddress.AddressIn.sin_addr.s_addr = _byteswap_ulong(netenv_ip_iface);
    interfaces[0].iiBroadcastAddress.AddressIn.sin_family = AF_INET;
    interfaces[0].iiBroadcastAddress.AddressIn.sin_port = 0;
    interfaces[0].iiBroadcastAddress.AddressIn.sin_addr.s_addr = _byteswap_ulong(netenv_ip_prefix | 0xFF);
    interfaces[0].iiNetmask.AddressIn.sin_family = AF_INET;
    interfaces[0].iiNetmask.AddressIn.sin_port = 0;
    interfaces[0].iiNetmask.AddressIn.sin_addr.s_addr = _byteswap_ulong(netenv_ip_bcast);

    size_t size = sizeof(interfaces);

    if (size > cbOutBuffer){
        dprintf("Netenv: ERROR: buffer too small for WSAIoCtl\n");
        WSASetLastError(WSAEFAULT);
        return WSAEFAULT;
    }

    memcpy(lpvOutBuffer, interfaces, size);
    *lpcbBytesReturned = size;

    return 0;
}
