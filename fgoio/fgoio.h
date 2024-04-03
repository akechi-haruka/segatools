/*
IO part of fgohook code is developed and provided by Haruka @ discord with permission. Thanks.
*/
#pragma once

#include <windows.h>

#include <stdint.h>

enum {
    FGO_IO_OPBTN_TEST = 0x01,
    FGO_IO_OPBTN_SERVICE = 0x02,
};

enum {
    IO_BTN_TREASURE = 0x01,
    IO_BTN_TARGET = 0x02,
    IO_BTN_DASH = 0x04,
    IO_BTN_ATTACK = 0x08,
    IO_BTN_CAMERA = 0x10,
};

/* Get the version of the Fate/Grand Order IO API that this DLL supports. This
   function should return a positive 16-bit integer, where the high byte is
   the major version and the low byte is the minor version (as defined by the
   Semantic Versioning standard).

   The latest API version as of this writing is 0x0100. */

uint16_t fgo_io_get_api_version(void);

/* Initialize the IO DLL. This is the second function that will be called on
   your DLL, after fgo_io_get_api_version.

   All subsequent calls to this API may originate from arbitrary threads.

   Minimum API version: 0x0100 */

HRESULT fgo_io_init(void);

/* Send any queued outputs (of which there are currently none, though this may
   change in subsequent API versions) and retrieve any new inputs.

   Minimum API version: 0x0100 */

HRESULT fgo_io_poll(void);

/* Get the state of the cabinet's operator buttons as of the last poll. See
   fgo_IO_OPBTN enum above: this contains bit mask definitions for button
   states returned in *opbtn. All buttons are active-high.

   Minimum API version: 0x0100 */

void fgo_io_get_opbtns(uint8_t *opbtn);

/* Get the state of the cabinet's gameplay buttons as of the last poll. See
   IO_BTN enum above for bit mask definitions. Inputs are split into
   a left hand side set of inputs and a right hand side set of inputs: the bit
   mappings are the same in both cases.

   All buttons are active-high, even though some buttons' electrical signals
   on a real cabinet are active-low.

   Minimum API version: 0x0100 */

void fgo_io_get_gamebtns(uint8_t *btn);

/* Get the position of the cabinet control stick as of the last poll. The center
   position should be equal to or close to 32767.

   Minimum API version: 0x0100 */

void fgo_io_get_stick(uint16_t *x, uint16_t *y);

void fgo_io_get_coin(bool *c);
