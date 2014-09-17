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
    zlist_t *subscribers;       //  List of known subscribers
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
    self->subscribers = zlist_new ();
    return 0;
}

//  Free properties and structures for a server instance

static void
server_terminate (server_t *self)
{
    zlist_destroy (&self->subscribers);
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
    zlist_remove (self->server->subscribers, self);
}


//  --------------------------------------------------------------------------
//  register_new_client
//

static void
register_new_client (client_t *self)
{
    zsys_info ("register client identifier=%s",
               zccp_msg_identifier (self->request));
}


//  --------------------------------------------------------------------------
//  subscribe_client_to_events
//

static void
subscribe_client_to_events (client_t *self)
{
    zlist_append (self->server->subscribers, self);
}


//  --------------------------------------------------------------------------
//  forward_to_subscribers
//

static void
forward_to_subscribers (client_t *self)
{
    client_t *client = (client_t *) zlist_first (self->server->subscribers);
    while (client) {
        engine_send_event (client, forward_event);
        client = (client_t *) zlist_next (self->server->subscribers);
    }
}


//  --------------------------------------------------------------------------
//  Selftest

void
zccp_server_test (bool verbose)
{
    printf (" * zccp_server: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    zactor_t *server = zactor_new (zccp_server, "zccp_server_test");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "BIND", "ipc://@/zccp_server", NULL);

    zsock_t *client = zsock_new (ZMQ_DEALER);
    assert (client);
    zsock_connect (client, "ipc://@/zccp_server");

    zccp_msg_t *message;
    
    //  Check HELLO/READY and INVALID
    
    zccp_msg_send_request (client, "START", NULL);
    message = zccp_msg_recv (client);
    assert (message);
    assert (zccp_msg_id (message) == ZCCP_MSG_INVALID);
    zccp_msg_destroy (&message);
    
    zccp_msg_send_hello (client, "step 1");
    message = zccp_msg_recv (client);
    assert (message);
    assert (zccp_msg_id (message) == ZCCP_MSG_READY);
    zccp_msg_destroy (&message);
    
    zccp_msg_send_request (client, "START", NULL);
    message = zccp_msg_recv (client);
    assert (message);
    assert (zccp_msg_id (message) == ZCCP_MSG_REPLY);
    zccp_msg_destroy (&message);

    zccp_msg_send_hello (client, "step 2");
    message = zccp_msg_recv (client);
    assert (message);
    assert (zccp_msg_id (message) == ZCCP_MSG_INVALID);
    zccp_msg_destroy (&message);

    //  Let's try some SUBSCRIBE and PUBLISH commands
    zccp_msg_send_hello (client, "step 3 - client");
    message = zccp_msg_recv (client);
    assert (message);
    assert (zccp_msg_id (message) == ZCCP_MSG_READY);
    zccp_msg_destroy (&message);
    
    zsock_t *device = zsock_new (ZMQ_DEALER);
    assert (device);
    zsock_connect (device, "ipc://@/zccp_server");
    
    zccp_msg_send_hello (device, "step 3 - device");
    message = zccp_msg_recv (device);
    assert (message);
    assert (zccp_msg_id (message) == ZCCP_MSG_READY);
    zccp_msg_destroy (&message);
    
    zccp_msg_send_subscribe (client, "*");
    zccp_msg_send_publish (device, "WARNING!!", NULL);
    
    message = zccp_msg_recv (client);
    assert (message);
    assert (zccp_msg_id (message) == ZCCP_MSG_PUBLISH);
    zccp_msg_destroy (&message);

    //  Finished, we can clean up
    zsock_destroy (&device);
    zsock_destroy (&client);
    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}

