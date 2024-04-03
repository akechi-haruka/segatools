#pragma once
#include <stdint.h>

#include "namco/config.h"


// dummy types
enum BngRwCardType {
    unknownct
};
enum BngRwIdType {
    unknownit
};
enum BngRwMCarrier {
    unknownmc
};

// banapass reader card object
struct BngRwCardHandle
{
  /* 0x0000 */ enum BngRwCardType eCardType;
  /* 0x0004 */ int iIdLen;
  /* 0x0008 */ int iFelicaOS;
  /* 0x000c */ unsigned char ucChipId[16];
}; /* size: 0x001c */

// banapass reader read action result object
struct BngRwResWaitTouch
{
  /* 0x0000 */ struct BngRwCardHandle hCardHandle;
  /* 0x001c */ unsigned char ucChipId[16];
  /* 0x002c */ char strChipId[36];
  /* 0x0050 */ char strAccessCode[24];
  /* 0x0068 */ enum BngRwIdType eIdType;
  /* 0x006c */ enum BngRwCardType eCardType;
  /* 0x0070 */ enum BngRwMCarrier eCarrierType;
  /* 0x0074 */ int iFelicaOS;
  /* 0x0078 */ char strBngProductId[8];
  /* 0x0080 */ unsigned int uiBngId;
  /* 0x0084 */ unsigned short usBngTypeCode;
  /* 0x0086 */ unsigned char ucBngRegionCode;
  /* 0x0087 */ unsigned char ucBlock1[16];
  /* 0x0097 */ unsigned char ucBlock2[16];
  /* 0x00a7 */ char __PADDING__[1];
}; /* size: 0x00a8 */

// callback function when reader finishes
typedef void (*BngRwReqCallback)(int iDev, int response, struct BngRwResWaitTouch *result, void *arg);
typedef __cdecl void* (*BngRwCallback)(int iDev, int response, void *arg);

// segatools data object for read operation
typedef struct CardReaderData {
	int timeout;
	BngRwReqCallback on_scan;
	struct BngRwResWaitTouch response;
	void* card_reader; // don't care
} CRDATA, *PCRDATA;

HRESULT bngrw_init(struct bngrw_config* cfg, bool dev_reset_must_callback);
