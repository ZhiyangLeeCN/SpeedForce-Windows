#include "lspdef.h"
#include <windows.h>


//
// This is the hardcoded guid for our dummy (hidden) catalog entry
//
GUID gProviderGuid = { //f345d288-21e2-441d-a7d1-eb4c6e001e3d
    0xf345d288,
    0x21e2,
    0x441d,
    {0xa7, 0xd1, 0xeb, 0x4c, 0x6e, 0x00, 0x1e, 0x3d}
};

void
WSPAPI
GetLspGuid(
    LPGUID lpGuid
    )
{
    memcpy( lpGuid, &gProviderGuid, sizeof( GUID ) );
}
