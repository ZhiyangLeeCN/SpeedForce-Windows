#include "lspdef.h"
#include <windows.h>


//
// This is the hardcoded guid for our dummy (hidden) catalog entry
//
GUID gProviderGuid = { //8DCA2997-7BD5-4455-8327-40E293B4ACC2
	0X8DCA2997,
	0X7BD5,
	0X4455,
	{ 0X83,0X27,0X40,0XE2,0X93,0XB4,0XAC,0XC2 } 
};

void
WSPAPI
GetLspGuid(
    LPGUID lpGuid
    )
{
    memcpy( lpGuid, &gProviderGuid, sizeof( GUID ) );
}
