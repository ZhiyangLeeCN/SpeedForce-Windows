 #include "lspdef.h"

SOCKET_CONTEXT *
FindSocketContext(
    SOCKET  s,
    BOOL    Remove
    )
{
    SOCKET_CONTEXT  *SocketContext = NULL,
               *info = NULL;
    LIST_ENTRY *lptr = NULL;
    int         i;

    EnterCriticalSection( &gCriticalSection );

    for(i=0; i < gLayerCount ;i++)
    {
        EnterCriticalSection( &gLayerInfo[ i ].ProviderCritSec );

        for(lptr = gLayerInfo[ i ].SocketList.Flink ;
            lptr != &gLayerInfo[ i ].SocketList ;
            lptr = lptr->Flink )
        {
            info = CONTAINING_RECORD( lptr, SOCKET_CONTEXT, Link );

            if ( s == info->Socket )
            {
                SocketContext = info;
                
                if ( TRUE == Remove )
                {
                    RemoveEntryList( &info->Link );
                }
                break;
            }
        }

        LeaveCriticalSection( &gLayerInfo[ i ].ProviderCritSec );

        if ( NULL != SocketContext )
            break;
    }

    LeaveCriticalSection( &gCriticalSection );

    return SocketContext;
}

SOCKET_CONTEXT *
CreateSocketContext(
    PROVIDER  *Provider, 
    SOCKET     Socket, 
    int       *lpErrno
    )
{
    SOCKET_CONTEXT   *newContext = NULL;

	newContext = new SOCKET_CONTEXT;
    if ( NULL == newContext )
    {
        dbgprint("CreateSocketContext: LspAlloc failed: %d", *lpErrno );
        goto cleanup;
    }

    newContext->Socket			 = Socket;
    newContext->Provider		 = Provider;
	newContext->Nbio			 = FALSE;
    newContext->Proxied			 = FALSE;
	newContext->RequireHandshake = TRUE;

    EnterCriticalSection( &Provider->ProviderCritSec );

    InsertHeadList( &Provider->SocketList, &newContext->Link );

    LeaveCriticalSection( &Provider->ProviderCritSec );

    return newContext;

cleanup:

    return NULL;
}

void 
FreeSocketContext(
    PROVIDER       *Provider,
    SOCKET_CONTEXT *Context
    )
{
    EnterCriticalSection( &Provider->ProviderCritSec );

    RemoveEntryList( &Context->Link );

	auto iter = Context->Events.begin();
	for (; iter != Context->Events.end(); iter++) {
		delete *iter;
	}

	delete Context;

    LeaveCriticalSection( &Provider->ProviderCritSec );

    return;
}

void 
FreeSocketContextList(
        PROVIDER *provider
        )
{
    LIST_ENTRY     *lptr = NULL;
    SOCKET_CONTEXT *context = NULL;

    ASSERT( provider );

    // Walk the list of sockets
    while ( !IsListEmpty( &provider->SocketList ) )
    {
        lptr = RemoveHeadList( &provider->SocketList );

        ASSERT( lptr );

        context = CONTAINING_RECORD( lptr, SOCKET_CONTEXT, Link );

		auto iter = context->Events.begin();
		for (; iter != context->Events.end(); iter++) {
			delete *iter;
		}

		delete context;
    }

    return;
}
