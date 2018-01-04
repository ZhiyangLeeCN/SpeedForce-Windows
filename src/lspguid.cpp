#include "lspdef.h"
#include "lspguid.h"

extern GUID gProviderGuid;

void
GetLspGuid(
    LPGUID lpGuid
    )
{
    memcpy( lpGuid, &gProviderGuid, sizeof( GUID ) );
}
