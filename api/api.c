#include <stdbool.h>
#include <stdint.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>

#include "api/api.h"
#include "api/config.h"

#include "util/dprintf.h"

#define DEFAULT_BUFLEN 512

static struct api_config api_cfg;
static unsigned int __stdcall api_socket_thread_proc(void *ctx);
static HANDLE api_socket_thread;
static SOCKET ListenSocket = INVALID_SOCKET;
static SOCKET SendSocket = INVALID_SOCKET;
static struct sockaddr_in SendAddr;
static struct sockaddr_in RecvAddr;
static struct sockaddr_in SendAddrBind;
static bool threadExitFlag = false;
static bool send_initialized = false;

static bool api_test_button = false;
static bool api_service_button = false;
static bool api_credit_button = false;
static bool api_felica_sent = false;
static bool api_aime_sent = false;
static bool allow_shutdown = false;
static uint8_t* api_felica_id;
static uint8_t* api_aime_id;

HRESULT api_init(){

    WSADATA wsa;

	if (api_socket_thread != NULL) {
		dprintf("API: already running\n");
        return E_FAIL;
    }

    api_config_load(&api_cfg, L".\\segatools.ini");

	if (!api_cfg.enable){
		return S_OK;
	}
	dprintf("API: Initializing using port %d\n", api_cfg.port);

    allow_shutdown = api_cfg.allowShutdown;

    int err = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (err != 0) {
		dprintf("API: Failed to initialize, error %d\n", err);
        return E_FAIL;
    }

    // Create the UDP status socket
    ListenSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (ListenSocket == SOCKET_ERROR) {
        dprintf("API: Failed to open listen socket: %d\n", WSAGetLastError());
        return E_FAIL;
    }

	const char opt = 1;
	setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	//setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
	setsockopt(ListenSocket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

	RecvAddr.sin_family = AF_INET;
    RecvAddr.sin_port = htons(api_cfg.port);
    RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(ListenSocket, (SOCKADDR *) & RecvAddr, sizeof (RecvAddr))) {
        dprintf("API: bind (recv) failed with error %d\n", WSAGetLastError());
        return E_FAIL;
    }

	api_initialize_send();

	api_felica_id = malloc(32);
	api_aime_id = malloc(32);

	threadExitFlag = false;
	api_socket_thread = (HANDLE) _beginthreadex(NULL, 0, api_socket_thread_proc, NULL, 0, NULL);

	return S_OK;
}

static unsigned int __stdcall api_socket_thread_proc(void *ctx)
{
	struct sockaddr_in SenderAddr;
    int SenderAddrSize = sizeof(SenderAddr);

    int err = SOCKET_ERROR;
    char ucBuffer[PACKET_MAX_SIZE];

    // Loop while the thread is not aborting
    while (!threadExitFlag) {
        err = recvfrom(ListenSocket, ucBuffer, PACKET_MAX_SIZE, 0, (SOCKADDR *) &SenderAddr, &SenderAddrSize);

        if (err != SOCKET_ERROR) {
            int id = ucBuffer[PACKET_HEADER_FIELD_ID];
			int len = ucBuffer[PACKET_HEADER_FIELD_LEN];
			int target = ucBuffer[PACKET_HEADER_FIELD_MACHINEID];
			if (target != api_cfg.networkId){
				dprintf("API: Received packet designated for machine %d, but we're %d\n", target, api_cfg.networkId);
			}
			if (len > PACKET_CONTENT_MAX_SIZE){
				len = PACKET_CONTENT_MAX_SIZE;
			}
			char data[PACKET_CONTENT_MAX_SIZE];
			for (int i = 0; i < len - PACKET_HEADER_LEN; i++){
				data[i] = ucBuffer[PACKET_HEADER_LEN + i];
			}
			dprintf("API: Received Packet: %d\n", id);
			api_parse(id, len, data);
        }
        else if (err != WSAEINTR) {
            err = WSAGetLastError();
            dprintf("API: Receive error: %d\n", err);
			threadExitFlag = true;
        }
    }

    dprintf("API: Exiting\n");
	threadExitFlag = true;
	send_initialized = false;

	closesocket(ListenSocket);
	closesocket(SendSocket);
    WSACleanup();

    return 0;
}

int api_parse(int id, int len, const char * data){

	char blank_data[0];
	switch (id){
		case PACKET_00_PING:
			api_send(PACKET_01_ACK, 0, blank_data);
			break;
		case PACKET_02_TEST:
			api_test_button = true;
			api_send(PACKET_01_ACK, 0, blank_data);
			break;
		case PACKET_03_SERVICE:
			api_service_button = true;
			api_send(PACKET_01_ACK, 0, blank_data);
			break;
		case PACKET_04_CREDIT:
			api_credit_button = true;
			api_send(PACKET_01_ACK, 0, blank_data);
			break;
		case PACKET_05_CARD_FELICA:
			{
				for (int i = 0; i < CARD_LEN_FELICA; i++){
					api_felica_id[i] = data[i];
				}
				api_felica_sent = true;
				api_send(PACKET_01_ACK, 0, blank_data);
				break;
			}
        case 6:
        case PACKET_10_SEQUENCE:
        case PACKET_11_VFD:
            break;
		case PACKET_07_CARD_AIME:
			{
				for (int i = 0; i < CARD_LEN_AIME; i++){
					api_aime_id[i] = data[i];
				}
				api_aime_sent = true;
				api_send(PACKET_01_ACK, 0, blank_data);
				break;
			}
        case PACKET_08_SEARCH:
            {
                api_send(PACKET_00_PING, 0, blank_data);
                break;
            }
        case PACKET_09_SHUTDOWN:
            {
                if (allow_shutdown){
                    dprintf("API: shutting down this PC\n");

                    if (!InitiateSystemShutdown(NULL, NULL, 0, TRUE, FALSE)){
                        dprintf("API: shutdown failed: %Id\n", GetLastError());
                    }
                } else {
                    dprintf("API: shutdown not permitted from config\n");
                }
				api_send(PACKET_01_ACK, 0, blank_data);
                break;
            }
		default:
			dprintf("API: Unknown packet: %d\n", id);
			return API_PACKET_ID_UNKNOWN;
	}

	return API_COMMAND_OK;
}

// HACK: I have no idea why this needs to exist in the first place
// somehow there are different instances of SendSocket depending on where they're called from??
HRESULT api_initialize_send(){
	dprintf("API: Initializing send\n");

	const char opt = 1;
	SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (SendSocket == SOCKET_ERROR) {
        dprintf("API: Failed to open send socket: %d\n", WSAGetLastError());
        return E_FAIL;
    }
	setsockopt(SendSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	//setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
	setsockopt(SendSocket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
	SendAddrBind.sin_family = AF_INET;
    SendAddrBind.sin_port = htons(api_cfg.port);
    SendAddrBind.sin_addr.s_addr = htonl(INADDR_ANY);
	SendAddr.sin_family = AF_INET;
    SendAddr.sin_port = htons(api_cfg.port);
    SendAddr.sin_addr.s_addr = inet_addr(api_cfg.bindAddr);
    if (bind(SendSocket, (SOCKADDR *) &SendAddrBind, sizeof (SendAddrBind))) {
        dprintf("API: bind (send) failed with error %d\n", WSAGetLastError());
        return E_FAIL;
    }
	dprintf("API: Send initialized on port %d\n", api_cfg.port+1);
	send_initialized = true;
	return S_OK;
}

int api_send(int id, int len, const char * data){

    if (!api_cfg.enable){
        return API_DISABLED;
    }
	if (threadExitFlag){
		return API_STATE_ERROR;
	}
	if (len > PACKET_CONTENT_MAX_SIZE){
		return API_PACKET_TOO_LONG;
	}
	if (!send_initialized){
		if (FAILED(api_init())){
			threadExitFlag = true;
			return API_STATE_ERROR;
		}
	}
	dprintf("API: Sending Packet: %d\n", id);
	int packetLen = PACKET_HEADER_LEN + len;
	char packet[packetLen];
	packet[PACKET_HEADER_FIELD_ID] = id;
	packet[PACKET_HEADER_FIELD_LEN] = len;
	packet[PACKET_HEADER_FIELD_MACHINEID] = api_cfg.networkId;
	for (int i = 0; i < len; i++){
		packet[PACKET_HEADER_LEN + i] = data[i];
	}

    if (sendto(SendSocket, packet, packetLen, 0, (SOCKADDR *) &SendAddr, sizeof(SendAddr)) == SOCKET_ERROR) {
        dprintf("API: sendto failed with error: %d\n", WSAGetLastError());
        return API_SOCKET_OPERATION_FAIL;
    }

	return API_COMMAND_OK;
}

void api_stop(){
	dprintf("API: shutdown\n");
	threadExitFlag = true;
	closesocket(ListenSocket);
	closesocket(SendSocket);
	WaitForSingleObject(api_socket_thread, INFINITE);
    CloseHandle(api_socket_thread);
    api_socket_thread = NULL;
}

bool api_get_test_button(){
	return api_test_button;
}
bool api_get_service_button(){
	return api_service_button;
}
bool api_get_credit_button(){
	return api_credit_button;
}
bool api_get_felica_inserted(){
	return api_felica_sent;
}
bool api_get_aime_inserted(){
	return api_aime_sent;
}
uint8_t* api_get_felica_id(){
	return api_felica_id;
}
uint8_t* api_get_aime_id(){
	return api_aime_id;
}
void api_clear_buttons(){
	api_test_button = false;
	api_service_button = false;
	api_credit_button = false;
}
void api_clear_cards(){
	api_aime_sent = false;
	api_felica_sent = false;
}
void api_send_vfd(const wchar_t* string){
    char str[1024];
    wcstombs(str, string, 1024);
    api_send(PACKET_11_VFD, strlen(str), str);
}
void api_send_vfd_sj(const char* string){
    api_send(PACKET_12_VFD_SHIFTJIS, strlen(string), string);
}
