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

    newContext = (SOCKET_CONTEXT *) LspAlloc(
            sizeof( SOCKET_CONTEXT ),
            lpErrno
            );
    if ( NULL == newContext )
    {
        dbgprint("CreateSocketContext: LspAlloc failed: %d", *lpErrno );
        goto cleanup;
    }

    newContext->Socket     = Socket;
    newContext->Provider   = Provider;
    newContext->Proxied    = FALSE;

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
    LspFree( Context );

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

        // Context is already removed so just free it
        LspFree( context );
    }

    return;
}