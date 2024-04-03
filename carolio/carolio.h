#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stdint.h>

/* Get the version of the Project carol IO API that this DLL supports. This
   function should return a positive 16-bit integer, where the high byte is
   the major version and the low byte is the minor version (as defined by the
   Semantic Versioning standard).

   The latest API version as of this writing is 0x0100. */

uint16_t carol_io_get_api_version(void);

/* Initialize JVS-based input. This function will be called before any other
   carol_io_jvs_*() function calls. Errors returned from this function will
   manifest as a disconnected JVS bus.

   All subsequent calls may originate from arbitrary threads and some may
   overlap with each other. Ensuring synchronization inside your IO DLL is
   your responsibility.

   Minimum API version: 0x0100 */

HRESULT carol_io_jvs_init(void);

/* Poll JVS input.

   opbtn returns the cabinet test/service state, where bit 0 is Test and Bit 1
   is Service.

   gamebtn bits, from least significant to most significant, are:

   Circle Cross Square Triangle Start UNUSED UNUSED UNUSED

   Minimum API version: 0x0100 */

void carol_io_jvs_poll(uint8_t *opbtn, uint8_t *gamebtn);

/* Read the current state of the coin counter. This value should be incremented
   for every coin detected by the coin acceptor mechanism. This count does not
   need to persist beyond the lifetime of the process.

   Minimum API Version: 0x0100 */

void carol_io_jvs_read_coin_counter(uint16_t *out);

HRESULT carol_io_touch_init();

HRESULT carol_io_controlbd_init();