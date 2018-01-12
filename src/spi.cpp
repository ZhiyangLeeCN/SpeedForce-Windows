 #include "lspdef.h"
#include <strsafe.h>

#include <stdio.h>
#include <stdlib.h>

#include "socks\V5\socks_v5.h"

#pragma warning(disable:4127)       // Disable "conditional expression is constant" warning

#define DEFAULT_PRINT_BUFFER    512

////////////////////////////////////////////////////////////////////////////////
//
// Globals used across files
//
////////////////////////////////////////////////////////////////////////////////

CRITICAL_SECTION    gCriticalSection;   // Critical section to protect startup/cleanup
WSPUPCALLTABLE      gMainUpCallTable;   // Winsock upcall table
LPPROVIDER          gLayerInfo = NULL;  // Provider information for each layer under us
int                 gLayerCount = 0;    // Number of providers layered over

////////////////////////////////////////////////////////////////////////////////
//
// Globals local to this file
//
////////////////////////////////////////////////////////////////////////////////

static BOOL gDetached = FALSE;      // Indicates if process is detaching from DLL
static int  gStartupCount = 0;      // Global startup count (for every WSPStartup call)

////////////////////////////////////////////////////////////////////////////////
//
// SPI Function Implementation
//
////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI 
DllMain(
        IN HINSTANCE hinstDll, 
        IN DWORD dwReason, 
        LPVOID lpvReserved
       )
{
    UNREFERENCED_PARAMETER( hinstDll );
    UNREFERENCED_PARAMETER( lpvReserved );

    switch (dwReason)
    {

        case DLL_PROCESS_ATTACH:
            //
            // Initialize some critical section objects 
            //
            __try
            {
                InitializeCriticalSection( &gCriticalSection );
                InitializeCriticalSection( &gDebugCritSec );
            }
            __except( EXCEPTION_EXECUTE_HANDLER )
            {
                goto cleanup;
            }
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            gDetached = TRUE;

            EnterCriticalSection( &gCriticalSection );
            if ( NULL != gLayerInfo )
            {
                int Error;

                // Free LSP structures if still present as well as call WSPCleanup
                //    for all providers this LSP loaded
                FreeLspProviders( gLayerInfo, gLayerCount, &Error );
                gLayerInfo = NULL;
                gLayerCount = 0;
            }
            LeaveCriticalSection( &gCriticalSection );

            DeleteCriticalSection( &gCriticalSection );
            DeleteCriticalSection( &gDebugCritSec );

            break;
    }

    return TRUE;

cleanup:

    return FALSE;
}

int WSPAPI 
WSPCleanup(
        LPINT lpErrno  
        )
{
    int        rc = SOCKET_ERROR;

    if ( gDetached )
    {
        rc = NO_ERROR;
        goto cleanup;
    }

    //
    // Grab the DLL global critical section
    //
    EnterCriticalSection( &gCriticalSection );

    // Verify WSPStartup has been called
    if ( 0 == gStartupCount )
    {
        *lpErrno = WSANOTINITIALISED;
        goto cleanup;
    }

    // Decrement the global entry count
    gStartupCount--;

    if ( 0 == gStartupCount )
    {
        // Free LSP structures if still present as well as call WSPCleanup
        //    for all providers this LSP loaded
        FreeLspProviders( gLayerInfo, gLayerCount, lpErrno );
        gLayerInfo = NULL;
        gLayerCount = 0;
    }
    
    rc = NO_ERROR;

cleanup:

    LeaveCriticalSection( &gCriticalSection );

    return rc;
}

int WSPAPI
WSPCloseSocket(
        SOCKET s,
        LPINT  lpErrno
        )
{
    SOCKET_CONTEXT *sockContext = NULL;
    int             rc = SOCKET_ERROR;

    // Find the socket context and remove it from the provider's list of sockets
    sockContext = FindSocketContext( s, TRUE );
    if ( NULL == sockContext )
    {
        *lpErrno = WSAENOTSOCK;
        goto cleanup;
    }

    ASSERT( sockContext->Provider->NextProcTable.lpWSPCloseSocket );

    // Pass the socket down to close it
    rc = sockContext->Provider->NextProcTable.lpWSPCloseSocket(
            s,
            lpErrno
            );

    // Just free the structure as its alreayd removed from the provider's list
    LspFree( sockContext );

cleanup:

    return rc;
}

int WSPAPI 
WSPConnect(
        SOCKET                s,
        const struct sockaddr FAR * name,
        int                   namelen,
        LPWSABUF              lpCallerData,
        LPWSABUF              lpCalleeData,
        LPQOS                 lpSQOS,
        LPQOS                 lpGQOS,
        LPINT                 lpErrno
        )
{
    SOCKET_CONTEXT *sockContext = NULL;
    SOCKADDR       *proxyAddr = NULL;
    int             proxyLen = 0,
                    rc = SOCKET_ERROR, 
					socksErrono = -1;
	u_long			disabled = 0, enabled = 1;
	BOOL			isNbio;

    // Find the socket context
    sockContext = FindSocketContext( s );
    if ( NULL == sockContext )
    {
        *lpErrno = WSAENOTSOCK;
        goto cleanup;
    }

    ASSERT( sockContext->Provider->NextProcTable.lpWSPConnect );

	sockContext->AddressLength = namelen;
	memcpy(&sockContext->OriginalAddress, name, namelen);
	memcpy(&sockContext->ProxiedAddress, name, namelen);
	((SOCKADDR_IN *)&sockContext->ProxiedAddress)->sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	((SOCKADDR_IN *)&sockContext->ProxiedAddress)->sin_port = htons(1080);
	sockContext->Proxied = TRUE;

	isNbio = sockContext->Nbio;
	if (isNbio) {
		ioctlsocket(s, FIONBIO, &disabled);
	}

	rc = ConnectSocksV5ServerForTcp(
		s, 
		(SOCKADDR *)&sockContext->ProxiedAddress, 
		(SOCKADDR *)&sockContext->OriginalAddress, 
		&socksErrono
	);

	if (isNbio) {
		ioctlsocket(s, FIONBIO, &enabled);
	}

	if (0 != rc) {

		    rc = sockContext->Provider->NextProcTable.lpWSPConnect(
            s,
			name,
            namelen, 
            lpCallerData, 
            lpCalleeData,
            lpSQOS, 
            lpGQOS, 
            lpErrno
            );

	}

cleanup:

    return rc;
}

int WSPAPI
WSPIoctl(
	SOCKET          s,
	DWORD           dwIoControlCode,
	LPVOID          lpvInBuffer,
	DWORD           cbInBuffer,
	LPVOID          lpvOutBuffer,
	DWORD           cbOutBuffer,
	LPDWORD         lpcbBytesReturned,
	LPWSAOVERLAPPED lpOverlapped,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
	LPWSATHREADID   lpThreadId,
	LPINT           lpErrno
)
{
	SOCKET_CONTEXT *sockContext = NULL;
	GUID            ConnectExGuid = WSAID_CONNECTEX;
	int             rc = NO_ERROR;
	u_long			*arp;

	// Retrieve the socket context
	sockContext = FindSocketContext(s);
	if (NULL == sockContext)
	{
		*lpErrno = WSAENOTSOCK;
		rc = SOCKET_ERROR;
		goto cleanup;
	}

	ASSERT(sockContext->Provider->NextProcTable.lpWSPIoctl);

	// Pass the call to the lower layer
	rc = sockContext->Provider->NextProcTable.lpWSPIoctl(
		s,
		dwIoControlCode,
		lpvInBuffer,
		cbInBuffer,
		lpvOutBuffer,
		cbOutBuffer,
		lpcbBytesReturned,
		lpOverlapped,
		lpCompletionRoutine,
		lpThreadId,
		lpErrno
	);

	if (dwIoControlCode == FIONBIO && 0 == rc) {

		arp = (u_long *)lpvInBuffer;
		if (*arp > 0) {
			sockContext->Nbio = TRUE;
		} else {
			sockContext->Nbio = FALSE;
		}

	}

cleanup:

	return rc;
}

int WSPAPI
WSPSend(
        SOCKET          s,
        LPWSABUF        lpBuffers,
        DWORD           dwBufferCount,
        LPDWORD         lpNumberOfBytesSent,
        DWORD           dwFlags,
        LPWSAOVERLAPPED lpOverlapped,
        LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
        LPWSATHREADID   lpThreadId,
        LPINT           lpErrno
       )
{
    SOCKET_CONTEXT *sockContext = NULL;
	int             rc = SOCKET_ERROR;

    //
    // Find our provider socket corresponding to this one
    //
    sockContext = FindSocketContext( s );
    if ( NULL == sockContext )
    {
        *lpErrno = WSAENOTSOCK;
        goto cleanup;
    }

    ASSERT( sockContext->Provider->NextProcTable.lpWSPSend );

    rc = sockContext->Provider->NextProcTable.lpWSPSend(
            s,
            lpBuffers,
            dwBufferCount, 
            lpNumberOfBytesSent,
            dwFlags,
            lpOverlapped,
            lpCompletionRoutine,
            lpThreadId,
            lpErrno
            );
cleanup:

    return rc;
}

int WSPAPI
WSPSendTo(
		SOCKET s,
		LPWSABUF lpBuffers,
		DWORD dwBufferCount,
		LPDWORD lpNumberOfBytesSent,
		DWORD dwFlags,
		const struct sockaddr FAR * lpTo,
		int iTolen,
		LPWSAOVERLAPPED lpOverlapped,
		LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
		LPWSATHREADID lpThreadId,
		LPINT lpErrno
		)
{
	SOCKET_CONTEXT *sockContext = NULL;
	int             rc = SOCKET_ERROR;
	//
	// Find our provider socket corresponding to this one
	//
	sockContext = FindSocketContext(s);
	if (NULL == sockContext)
	{
		*lpErrno = WSAENOTSOCK;
		goto cleanup;
	}

	ASSERT(sockContext->Provider->NextProcTable.lpWSPSendTo);

	rc = sockContext->Provider->NextProcTable.lpWSPSendTo(
		s,
		lpBuffers,
		dwBufferCount,
		lpNumberOfBytesSent,
		dwFlags,
		lpTo,
		iTolen,
		lpOverlapped,
		lpCompletionRoutine,
		lpThreadId,
		lpErrno
	);

cleanup:

	return rc;

}

SOCKET WSPAPI
WSPSocket(
	int                 af,
	int                 type,
	int                 protocol,
	LPWSAPROTOCOL_INFOW lpProtocolInfo,
	GROUP               g,
	DWORD               dwFlags,
	LPINT               lpErrno
)
{
	WSAPROTOCOL_INFOW   InfoCopy = { 0 };
	SOCKET_CONTEXT     *sockContext = NULL;
	PROVIDER           *lowerProvider = NULL;
	SOCKET              nextProviderSocket = INVALID_SOCKET,
						sret = INVALID_SOCKET;
	int                 rc;

	// Find the LSP entry which matches the given provider info
	lowerProvider = FindMatchingLspEntryForProtocolInfo(
		lpProtocolInfo,
		gLayerInfo,
		gLayerCount
	);
	if (NULL == lowerProvider)
	{
		dbgprint("WSPSocket: FindMatchingLspEntryForProtocolInfo failed!");
		goto cleanup;
	}

	if (0 == lowerProvider->StartupCount)
	{
		rc = InitializeProvider(lowerProvider, MAKEWORD(2, 2), lpProtocolInfo,
			gMainUpCallTable, lpErrno);
		if (SOCKET_ERROR == rc)
		{
			dbgprint("WSPSocket: InitializeProvider failed: %d", *lpErrno);
			goto cleanup;
		}
	}

	// If the next layer is a base, substitute the provider structure with the
	//    base provider's
	if (BASE_PROTOCOL == lowerProvider->NextProvider.ProtocolChain.ChainLen)
	{
		memcpy(&InfoCopy, &lowerProvider->NextProvider, sizeof(InfoCopy));
		InfoCopy.dwProviderReserved = lpProtocolInfo->dwProviderReserved;
		if (af == FROM_PROTOCOL_INFO)
			InfoCopy.iAddressFamily = lpProtocolInfo->iAddressFamily;
		if (type == FROM_PROTOCOL_INFO)
			InfoCopy.iSocketType = lpProtocolInfo->iSocketType;
		if (protocol == FROM_PROTOCOL_INFO)
			InfoCopy.iProtocol = lpProtocolInfo->iProtocol;

		lpProtocolInfo = &InfoCopy;
	}

	ASSERT(lowerProvider->NextProcTable.lpWSPSocket);

	//
	// Create the socket from the lower layer
	//
	nextProviderSocket = lowerProvider->NextProcTable.lpWSPSocket(
		af,
		type,
		protocol,
		lpProtocolInfo,
		g,
		dwFlags,
		lpErrno
	);
	if (INVALID_SOCKET == nextProviderSocket)
	{
		dbgprint("WSPSocket: NextProcTable.WSPSocket failed: %d", *lpErrno);
		goto cleanup;
	}

	//
	// Create the context information to be associated with this socket
	//
	sockContext = CreateSocketContext(
		lowerProvider,
		nextProviderSocket,
		lpErrno
	);
	if (NULL == sockContext)
	{
		dbgprint("WSPSocket: CreateSocketContext failed: %d", *lpErrno);
		goto cleanup;
	}

	//
	// Associate ownership of this handle with our LSP
	//
	sret = gMainUpCallTable.lpWPUModifyIFSHandle(
		lowerProvider->LayerProvider.dwCatalogEntryId,
		nextProviderSocket,
		lpErrno
	);
	if (INVALID_SOCKET == sret)
	{
		dbgprint("WSPSocket: WPUModifyIFSHandle failed: %d", *lpErrno);
		goto cleanup;
	}

	ASSERT(sret == nextProviderSocket);

	return nextProviderSocket;

cleanup:

	// If an error occured close the socket if it was already created
	if ((NULL != sockContext) && (NULL != lowerProvider) &&
		(INVALID_SOCKET != nextProviderSocket))
	{
		rc = lowerProvider->NextProcTable.lpWSPCloseSocket(
			nextProviderSocket,
			lpErrno
		);
		if (SOCKET_ERROR == rc)
		{
			dbgprint("WSPSocket: WSPCloseSocket failed: %d", *lpErrno);
		}

	}
	if ((NULL != sockContext) && (NULL != lowerProvider))
		FreeSocketContext(lowerProvider, sockContext);

	return INVALID_SOCKET;
}

int WSPPauseEvent(SOCKET s, SOCKET_CONTEXT* context)
{
	if (context == NULL || s == INVALID_SOCKET)
	{
		return SOCKET_ERROR;
	}
	int err = 0; // THE CLEANUP BIND EVENT OBJECT
	if (context->Provider->NextProcTable.lpWSPEventSelect(s, 0, NULL, &err))
	{
		return ECONNRESET;
	}

	EnterCriticalSection(&context->Provider->ProviderCritSec);

	for (auto iter = context->HWndList.begin(); iter != context->HWndList.end(); iter++) {
		err = 0;
		if (context->Provider->NextProcTable.lpWSPAsyncSelect(s, *iter, 0, 0, &err))
		{
			LeaveCriticalSection(&context->Provider->ProviderCritSec);
			return ECONNRESET; // WSAAsyncSelect(s, hWnd, 0, 0);
		}

	}

	LeaveCriticalSection(&context->Provider->ProviderCritSec);
	return 0;
}

int WSPAPI WSPEventSelect(
	SOCKET s,
	WSAEVENT hEventObject,
	long lNetworkEvents,
	LPINT lpErrno
	)
{
	PROVIDER           *loadProvider = NULL;
	SOCKET_CONTEXT     *sockContext = NULL;
	int                 rc;

	ASSERT(sockContext->Provider->NextProcTable.lpWSPEventSelect);

	sockContext = FindSocketContext(s);
	if (NULL == sockContext)
	{
		*lpErrno = WSAENOTSOCK;
		goto cleanup;
	}

	rc = sockContext->Provider->NextProcTable.lpWSPEventSelect(
		s,
		hEventObject,
		lNetworkEvents,
		lpErrno
	);

	if (0 == rc) {

		EnterCriticalSection(&sockContext->Provider->ProviderCritSec);

		SOCKET_EVENT_CONTEXT* evt = new SOCKET_EVENT_CONTEXT;
		evt->hEvent = (__int64)hEventObject;
		evt->hWnd = NULL;
		evt->wMsg = 0;
		evt->lNetworkEvents = lNetworkEvents;

		sockContext->Events.push_back(evt);

		LeaveCriticalSection(&sockContext->Provider->ProviderCritSec);

	}

cleanup:

	return rc;

}

int WSPAPI WSPAsyncSelect(
	SOCKET s,
	HWND hWnd,
	unsigned int wMsg,
	long lEvent,
	LPINT lpErrno
	)
{
	PROVIDER           *loadProvider = NULL;
	SOCKET_CONTEXT     *sockContext = NULL;
	int                 rc;

	sockContext = FindSocketContext(s);
	if (NULL == sockContext)
	{
		*lpErrno = WSAENOTSOCK;
		goto cleanup;
	}

	ASSERT(sockContext->Provider->NextProcTable.lpWSPAsyncSelect);

	rc = sockContext->Provider->NextProcTable.lpWSPAsyncSelect(
		s,
		hWnd,
		wMsg,
		lEvent,
		lpErrno
	);

	if (0 == rc) {

		EnterCriticalSection(&sockContext->Provider->ProviderCritSec);

		SOCKET_EVENT_CONTEXT* evt = new SOCKET_EVENT_CONTEXT;
		evt->hEvent = (__int64)lEvent;
		evt->hWnd = hWnd;
		evt->wMsg = wMsg;
		evt->lNetworkEvents = NULL;

		sockContext->Events.push_back(evt);
		sockContext->HWndList.push_back(hWnd);

		LeaveCriticalSection(&sockContext->Provider->ProviderCritSec);

	}

cleanup:

	return rc;

}


int WSPAPI 
WSPStartup(
    WORD                wVersion,
    LPWSPDATA           lpWSPData,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    WSPUPCALLTABLE      UpCallTable,
    LPWSPPROC_TABLE     lpProcTable
    )
{
    PROVIDER           *loadProvider = NULL;
    int                 Error = WSAEPROVIDERFAILEDINIT,
                        rc;

    EnterCriticalSection( &gCriticalSection );

    // The first time the startup is called, create our heap and allocate some
    //    data structures for tracking the LSP providers
    if ( 0 == gStartupCount )
    {
        // Create the heap for all LSP allocations
        rc = LspCreateHeap( &Error );
        if ( SOCKET_ERROR == rc )
        {
            dbgprint("WSPStartup: LspCreateHeap failed: %d", Error );
            goto cleanup;
        }

        // Find this LSP's entries in the Winsock catalog and build a map of them
        rc = FindLspEntries( &gLayerInfo, &gLayerCount, &Error );
        if ( FALSE == rc )
        {
            dbgprint("WSPStartup: FindLspEntries failed: %d", Error );
            goto cleanup;
        }

        // Save off upcall table - this should be the same across all WSPStartup calls
        memcpy( &gMainUpCallTable, &UpCallTable, sizeof( gMainUpCallTable ) );

    }

    // Find the matching LSP provider for the requested protocol info passed in.
    //    This can either be an LSP layered over use or an entry belonging to this
    //    LSP. Note that the LSP startup gets called for each LSP layered protocol
    //    entry with a unique GUID. Because of this each layered protocol entry for
    //    the IFS LSP should be installed with its own unique GUID.
    loadProvider = FindMatchingLspEntryForProtocolInfo(
            lpProtocolInfo,
            gLayerInfo,
            gLayerCount,
            TRUE
            );
    if ( NULL == loadProvider )
    {
        dbgprint("WSPStartup: FindMatchingLspEntryForProtocolInfo failed!");
        ASSERT( 0 );
        goto cleanup;
    }

    // If this is the first time to "load" this particular provider, initialize
    //    the lower layer, etc.
    if ( 0 == loadProvider->StartupCount )
    {

        rc = InitializeProvider( loadProvider, wVersion, lpProtocolInfo, 
                UpCallTable, &Error );
        if ( SOCKET_ERROR == rc )
        {
            dbgprint("WSPStartup: InitializeProvider failed: %d", Error );
            goto cleanup;
        }

    }

    gStartupCount++;

    // Build the proc table to return to the caller
    memcpy( lpProcTable, &loadProvider->NextProcTable, sizeof( *lpProcTable ) );

    // Override only those functions the LSP wants to intercept
    lpProcTable->lpWSPCleanup       = WSPCleanup;
    lpProcTable->lpWSPCloseSocket   = WSPCloseSocket;
    lpProcTable->lpWSPConnect       = WSPConnect;
    lpProcTable->lpWSPSend          = WSPSend;
	lpProcTable->lpWSPSendTo		= WSPSendTo;
	lpProcTable->lpWSPSocket		= WSPSocket;
	lpProcTable->lpWSPIoctl			= WSPIoctl;
	lpProcTable->lpWSPAsyncSelect	= WSPAsyncSelect;
	lpProcTable->lpWSPEventSelect	= WSPEventSelect;

    memcpy( lpWSPData, &loadProvider->WinsockVersion, sizeof( *lpWSPData ) );

    Error = NO_ERROR;

cleanup:

    LeaveCriticalSection( &gCriticalSection );

    return Error;
}

////////////////////////////////////////////////////////////////////////////////
//
// Helper Function Implementation
//
////////////////////////////////////////////////////////////////////////////////


//
// Function: FindDestinationAddress
//
// Description:
//      This function is invoked whenver a connection request is made by the upper
//      layer which can occur at the WSPConnect and ConnectEx functions. This method
//      determines whether the application's destination address should be
//      redirected to another destination. Currently, this function just looks for 
//      a single IPv4 destination address and substitutes it with another.
//
void
FindDestinationAddress( 
        SOCKET_CONTEXT *context, 
        const SOCKADDR *destAddr, 
        int             destLen,
        SOCKADDR      **proxyAddr, 
        int            *proxyLen
        )
{
    UNREFERENCED_PARAMETER( context );
    UNREFERENCED_PARAMETER( destLen );
    UNREFERENCED_PARAMETER( proxyAddr );
    UNREFERENCED_PARAMETER( proxyLen );

    context->AddressLength = destLen;

    // Save destination address
    memcpy( &context->OriginalAddress, destAddr, context->AddressLength );

    *proxyAddr = (SOCKADDR *) &context->OriginalAddress;
    *proxyLen  = context->AddressLength;

    if ( destAddr->sa_family == AF_INET )
    {
        // Redirect one destination to another
        if ( ( (SOCKADDR_IN *)destAddr )->sin_addr.s_addr == inet_addr("157.56.236.201") )
        {
            memcpy( &context->ProxiedAddress, destAddr, context->AddressLength );
            ( (SOCKADDR_IN *)&context->ProxiedAddress )->sin_addr.s_addr = inet_addr(
                    "157.56.237.9"
                    );

            *proxyAddr = (SOCKADDR *) &context->ProxiedAddress;

            context->Proxied = TRUE;
        }
    }
    else if ( destAddr->sa_family == AF_INET6 )
    {
        // Perform redirection here
    }
}

//
// Function: FreeLspProviders
//
// Description:
//
//
void
FreeLspProviders(
        PROVIDER   *lspProvider,
        int         lspProviderCount,
        int        *lpErrno
        )
{
    int     i;

    if ( NULL == lspProvider )
        return;

    // Need to iterate through the LSP providers and call WSPCleanup accordingly
    for(i=0; i < lspProviderCount ;i++)
    {
        while( 0 != lspProvider[ i ].StartupCount )
        {
            lspProvider[ i ].StartupCount--;

            lspProvider[ i ].NextProcTable.lpWSPCleanup( lpErrno );
        }

        if ( NULL != lspProvider[ i ].Module )
        {
            FreeLibrary( lspProvider[ i ].Module );
            lspProvider[ i ].Module = NULL;
        }
    }

    for(i=0; i < lspProviderCount ;i++)
    {
        FreeSocketContextList( &lspProvider[ i ] );
        
        DeleteCriticalSection( &lspProvider[ i ].ProviderCritSec );
    }

    LspFree( lspProvider );
}
