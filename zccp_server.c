/*  =========================================================================
    zccp_server - ZCCP Server

    =========================================================================
*/

/*
@header
    Description of class for man page.
@discuss
    Detailed discussion of the class, if any.
@end
*/

#include <czmq.h>
#include "zccp_msg.h"
#include "zccp_server.h"

//  ---------------------------------------------------------------------
//  Forward declarations for the two main classes we use here

typedef struct _server_t server_t;
typedef struct _client_t client_t;

//  This structure defines the context for each running server. Store
//  whatever properties and structures you need for the server.

struct _server_t {
    //  These properties must always be present in the server_t
    //  and are set by the generated engine; do not modify them!
    zsock_t *pipe;              //  Actor pipe back to caller
    zconfig_t *config;          //  Current loaded configuration
    
    //  Add any properties you need here
    
};

//  ---------------------------------------------------------------------
//  This structure defines the state for each client connection. It will
//  be passed to each action in the 'self' argument.

struct _client_t {
    //  These properties must always be present in the client_t
    //  and are set by the generated engine; do not modify them!
    server_t *server;           //  Reference to parent server
    zccp_msg_t *request;        //  Last received request
    zccp_msg_t *reply;          //  Reply to send out, if any

    //  These properties are specific for this application
    
};

//  Include the generated server engine
#include "zccp_server_engine.inc"

//  Allocate properties and structures for a new server instance.
//  Return 0 if OK, or -1 if there was an error.

static int
server_initialize (server_t *self)
{
    //  Construct properties here
    return 0;
}

//  Free properties and structures for a server instance

static void
server_terminate (server_t *self)
{
    //  Destroy properties here
}

//  Process server API method, return reply message if any

static zmsg_t *
server_method (server_t *self, const char *method, zmsg_t *msg)
{
    return NULL;
}


//  Allocate properties and structures for a new client connection and
//  optionally engine_set_next_event (). Return 0 if OK, or -1 on error.

static int
client_initialize (client_t *self)
{
    //  Construct properties here
    return 0;
}

//  Free properties and structures for a client connection

static void
client_terminate (client_t *self)
{
    //  Destroy properties here
}


//  --------------------------------------------------------------------------
//  forward_to_everyone
//

static void
forward_to_everyone (client_t *self)
{
}


//  --------------------------------------------------------------------------
//  Selftest

void
zccp_server_test (bool verbose)
{
    printf (" * zccp_server: \n");
    if (verbose)
        printf ("\n");

    //  @selftest
    zactor_t *server = zactor_new (zccp_server, "server");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "BIND", "ipc://@/zccp_server", NULL);

    zsock_t *client = zsock_new (ZMQ_DEALER);
    assert (client);
    zsock_set_rcvtimeo (client, 2000);
    zsock_connect (client, "ipc://@/zccp_server");

    zccp_msg_t *request = zccp_msg_new (ZCCP_MSG_HELLO);
    zccp_msg_send (&request, client);
    
    zccp_msg_t *reply = zccp_msg_recv (client);
    assert (reply);
    assert (zccp_msg_id (reply) == ZCCP_MSG_READY);
    zccp_msg_destroy (&reply);
    
    zsock_destroy (&client);
    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}

