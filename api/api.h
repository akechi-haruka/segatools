#define PACKET_00_PING 0
#define PACKET_01_ACK 1
#define PACKET_02_TEST 2
#define PACKET_03_SERVICE 3
#define PACKET_04_CREDIT 4
#define PACKET_05_CARD_FELICA 5
#define PACKET_06_AIME_RGB 6
#define PACKET_07_CARD_AIME 7
#define PACKET_08_SEARCH 8
#define PACKET_09_SHUTDOWN 9
#define PACKET_10_SEQUENCE 10
#define PACKET_11_VFD 11
#define PACKET_12_VFD_SHIFTJIS 12

#define PACKET_HEADER_LEN 3
#define PACKET_HEADER_FIELD_ID 0
#define PACKET_HEADER_FIELD_LEN 1
#define PACKET_HEADER_FIELD_MACHINEID 2

#define PACKET_CONTENT_MAX_SIZE 128
#define PACKET_MAX_SIZE PACKET_CONTENT_MAX_SIZE + PACKET_HEADER_LEN

#define CARD_LEN_FELICA 16
#define CARD_LEN_AIME 20

#define API_DISABLED 0
#define API_COMMAND_OK 1
#define API_IS_ERROR(x) x < API_DISABLED
#define API_PACKET_TOO_LONG -1
#define API_STATE_ERROR -2
#define API_PACKET_ID_UNKNOWN -3
#define API_SOCKET_OPERATION_FAIL -4

HRESULT api_init();
void api_stop();
int api_parse(int id, int len, const char* data);
int api_send(int id, int len, const char* data);
bool api_get_test_button();
bool api_get_service_button();
bool api_get_credit_button();
bool api_get_felica_inserted();
uint8_t* api_get_felica_id();
bool api_get_aime_inserted();
uint8_t* api_get_aime_id();
void api_clear_buttons();
void api_clear_cards();
void api_send_vfd(const wchar_t* string);
void api_send_vfd_sj(const char* string);
HRESULT api_initialize_send();
