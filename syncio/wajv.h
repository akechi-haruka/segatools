#pragma once
#include <stdint.h>
#include "syncio/config.h"

HRESULT wajv_init(struct wajv_config* cfg);

struct WAJVOutput
{
  /* 0x0000 */ unsigned short CoinDec[4];
  /* 0x0008 */ unsigned short ServiceDec[4];
  /* 0x0010 */ BOOL GenOut[40];
  /* 0x0038 */ unsigned short AnalogOut[8];
}; /* size: 0x0048 */

struct UnusedFunc
{
  /* 0x0000 */ unsigned char dummy1;
  /* 0x0001 */ unsigned char dummy2;
  /* 0x0002 */ unsigned char dummy3;
  /* 0x0003 */ unsigned char dummy4;
}; /* size: 0x0004 */

struct SwInFunc
{
  /* 0x0000 */ unsigned char Code;
  /* 0x0001 */ unsigned char Players;
  /* 0x0002 */ unsigned char Buttons;
  /* 0x0003 */ unsigned char dummy1;
}; /* size: 0x0004 */

struct CoinInFunc
{
  /* 0x0000 */ unsigned char Code;
  /* 0x0001 */ unsigned char Slots;
  /* 0x0002 */ unsigned char dummy1;
  /* 0x0003 */ unsigned char dummy2;
}; /* size: 0x0004 */

struct AnalogInFunc
{
  /* 0x0000 */ unsigned char Code;
  /* 0x0001 */ unsigned char Channels;
  /* 0x0002 */ unsigned char Bits;
  /* 0x0003 */ unsigned char dummy1;
}; /* size: 0x0004 */

struct RotInFunc
{
  /* 0x0000 */ unsigned char Code;
  /* 0x0001 */ unsigned char Channels;
  /* 0x0002 */ unsigned char dummy1;
  /* 0x0003 */ unsigned char dummy2;
}; /* size: 0x0004 */

struct KeyInFunc
{
  /* 0x0000 */ unsigned char Code;
  /* 0x0001 */ unsigned char dummy1;
  /* 0x0002 */ unsigned char dummy2;
  /* 0x0003 */ unsigned char dummy3;
}; /* size: 0x0004 */

struct DispInFunc
{
  /* 0x0000 */ unsigned char Code;
  /* 0x0001 */ unsigned char XBits;
  /* 0x0002 */ unsigned char YBits;
  /* 0x0003 */ unsigned char Channels;
}; /* size: 0x0004 */

struct GpSwInFunc
{
  /* 0x0000 */ unsigned char Code;
  /* 0x0001 */ unsigned char SwitchesUpper;
  /* 0x0002 */ unsigned char SwitchesLower;
  /* 0x0003 */ unsigned char dummy1;
}; /* size: 0x0004 */

struct CardFunc
{
  /* 0x0000 */ unsigned char Code;
  /* 0x0001 */ unsigned char Slots;
  /* 0x0002 */ unsigned char dummy1;
  /* 0x0003 */ unsigned char dummy2;
}; /* size: 0x0004 */

struct MedalFunc
{
  /* 0x0000 */ unsigned char Code;
  /* 0x0001 */ unsigned char Channels;
  /* 0x0002 */ unsigned char dummy1;
  /* 0x0003 */ unsigned char dummy2;
}; /* size: 0x0004 */

struct GpOutFunc
{
  /* 0x0000 */ unsigned char Code;
  /* 0x0001 */ unsigned char Slots;
  /* 0x0002 */ unsigned char dummy1;
  /* 0x0003 */ unsigned char dummy2;
}; /* size: 0x0004 */

struct DaOutFunc
{
  /* 0x0000 */ unsigned char Code;
  /* 0x0001 */ unsigned char Channels;
  /* 0x0002 */ unsigned char dummy1;
  /* 0x0003 */ unsigned char dummy2;
}; /* size: 0x0004 */

struct CharOutFunc
{
  /* 0x0000 */ unsigned char Code;
  /* 0x0001 */ unsigned char Columns;
  /* 0x0002 */ unsigned char Lines;
  /* 0x0003 */ unsigned char CharCode;
}; /* size: 0x0004 */

struct BackupFunc
{
  /* 0x0000 */ unsigned char Code;
  /* 0x0001 */ unsigned char dummy1;
  /* 0x0002 */ unsigned char dummy2;
  /* 0x0003 */ unsigned char dummy3;
}; /* size: 0x0004 */

struct WAJVFunctions
{
  /* 0x0000 */ struct UnusedFunc Unused0;
  /* 0x0004 */ struct SwInFunc SwIn;
  /* 0x0008 */ struct CoinInFunc CoinIn;
  /* 0x000c */ struct AnalogInFunc AnalogIn;
  /* 0x0010 */ struct RotInFunc RotIn;
  /* 0x0014 */ struct KeyInFunc KeyIn;
  /* 0x0018 */ struct DispInFunc DispIn;
  /* 0x001c */ struct GpSwInFunc GpSwIn;
  /* 0x0020 */ struct UnusedFunc Unused8;
  /* 0x0024 */ struct UnusedFunc Unused9;
  /* 0x0028 */ struct UnusedFunc UnusedA;
  /* 0x002c */ struct UnusedFunc UnusedB;
  /* 0x0030 */ struct UnusedFunc UnusedC;
  /* 0x0034 */ struct UnusedFunc UnusedD;
  /* 0x0038 */ struct UnusedFunc UnusedE;
  /* 0x003c */ struct UnusedFunc UnusedF;
  /* 0x0040 */ struct CardFunc Card;
  /* 0x0044 */ struct MedalFunc Medal;
  /* 0x0048 */ struct GpOutFunc GpOut;
  /* 0x004c */ struct DaOutFunc DaOut;
  /* 0x0050 */ struct CharOutFunc CharOut;
  /* 0x0054 */ struct BackupFunc Backup;
}; /* size: 0x0058 */

enum WAJVCoinStatus {
    WAJVCoinOK,
    WAJVCoinJam,
    WAJVCoinDisconnected,
    WAJVCoinBusy
};

struct WAJVCoin
{
  /* 0x0000 */ unsigned short Count;
  /* 0x0002 */ unsigned short Dec;
  /* 0x0004 */ enum WAJVCoinStatus Status;
}; /* size: 0x0008 */

enum WAJVServiceStatus {
    WAJVServiceOK,
    WAJVServiceJam
};

struct WAJVService
{
  /* 0x0000 */ unsigned short Count;
  /* 0x0002 */ unsigned short Dec;
  /* 0x0004 */ enum WAJVServiceStatus Status;
}; /* size: 0x0008 */

struct WAJVInput
{
  /* 0x0000 */ char IdString[256];
  /* 0x0100 */ unsigned char CmdRev;
  /* 0x0101 */ unsigned char JvRev;
  /* 0x0102 */ unsigned char ComRev;
  union
  {
    /* 0x0103 */ unsigned char Func[4][32];
    /* 0x0103 */ struct WAJVFunctions Functions;
  }; /* size: 0x0080 */
  /* 0x0183 */ char DipSw1;
  /* 0x0184 */ char DipSw2;
  /* 0x0185 */ char DipSw3;
  /* 0x0186 */ char DipSw4;
  /* 0x0187 */ char SwTest;
  /* 0x0188 */ char SwIn[64][4];
  /* 0x0288 */ unsigned short AnalogIn[16];
  /* 0x02a8 */ struct WAJVCoin Coin[4];
  /* 0x02c8 */ struct WAJVService Service[4];
  /* 0x02e8 */ BOOL SwTilt[3];
  /* 0x02eb */ char Padding_1342[5];
  /* 0x02f0 */ __int64 Timestamp;
}; /* size: 0x02f8 */

typedef struct WAJVInput WAJVInput;
typedef struct WAJVOutput WAJVOutput;
