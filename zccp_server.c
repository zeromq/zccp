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

//  ---------------------------------------------------------------------------
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
    
    zlist_t *patterns;          //  List of patterns subscribed to

    //  When we're forwarding a message, we need these
    const char *sender;         //  Identifier of client sender
    const char *address;        //  Message address
    zhash_t *headers;           //  Message headers
    zmsg_t *content;            //  Message content
};

//  ---------------------------------------------------------------------------
//  This structure defines the state for each client connection. It will
//  be passed to each action in the 'self' argument.

struct _client_t {
    //  These properties must always be present in the client_t
    //  and are set by the generated engine; do not modify them!
    server_t *server;           //  Reference to parent server
    zccp_msg_t *request;        //  Last received request
    zccp_msg_t *reply;          //  Reply to send out, if any

    //  These properties are specific for this application
    char *identifier;           //  Identifier of client
};

//  Include the generated server engine
#include "zccp_server_engine.inc"


//  This is a simple pattern class

typedef struct {
    char *expression;           //  Regular expression to match on
    zrex_t *rex;                //  Expression, compiled as a zrex object
    zlist_t *clients;           //  All clients that asked for this pattern
} pattern_t;

static void
s_pattern_destroy (pattern_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        pattern_t *self = *self_p;
        zrex_destroy (&self->rex);
        zlist_destroy (&self->clients);
        free (self->expression);
        free (self);
        *self_p = NULL;
    }
}

static pattern_t *
s_pattern_new (const char *expression, client_t *client)
{
    pattern_t *self = (pattern_t *) zmalloc (sizeof (pattern_t));
    if (self) {
        self->rex = zrex_new (expression);
        if (self->rex)
            self->expression = strdup (expression);
        if (self->expression)
            self->clients = zlist_new ();
        if (self->clients)
            zlist_append (self->clients, client);
        else
            s_pattern_destroy (&self);
    }
    return self;
}


//  Allocate properties and structures for a new server instance.
//  Return 0 if OK, or -1 if there was an error.

static int
server_initialize (server_t *self)
{
    self->patterns = zlist_new ();
    zlist_set_destructor (self->patterns, (czmq_destructor *) s_pattern_destroy);
    return 0;
}

//  Free properties and structures for a server instance

static void
server_terminate (server_t *self)
{
    zlist_destroy (&self->patterns);
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
    pattern_t *pattern = (pattern_t *) zlist_first (self->server->patterns);
    while (pattern) {
        zlist_remove (pattern->clients, self);
        pattern = (pattern_t *) zlist_next (self->server->patterns);
    }
    free (self->identifier);
}


//  ---------------------------------------------------------------------------
//  register_new_client
//

static void
register_new_client (client_t *self)
{
    self->identifier = strdup (zccp_msg_identifier (self->request));
}


//  ---------------------------------------------------------------------------
//  store_new_subscription
//

static void
store_new_subscription (client_t *self)
{
    const char *expression = zccp_msg_expression (self->request);
    pattern_t *pattern = (pattern_t *) zlist_first (self->server->patterns);
    while (pattern) {
        if (streq (pattern->expression, expression)) {
            client_t *client = (client_t *) zlist_first (pattern->clients);
            while (client) {
                if (client == self)
                    break;      //  This client is already on the list
                client = (client_t *) zlist_next (pattern->clients);
            }
            //  Add client, if it's new
            if (!client)
                zlist_append (pattern->clients, self);
            break;
        }
        pattern = (pattern_t *) zlist_next (self->server->patterns);
    }
    //  Add pattern, if it's new
    if (!pattern)
        zlist_append (self->server->patterns, s_pattern_new (expression, self));
}


//  ---------------------------------------------------------------------------
//  forward_to_subscribers
//

static void
forward_to_subscribers (client_t *self)
{
    //  Keep track of the message we're sending out to subscribers
    self->server->sender = self->identifier;
    self->server->address = zccp_msg_address (self->request);
    self->server->content = zccp_msg_content (self->request);
    
    //  Now find all matching subscribers
    pattern_t *pattern = (pattern_t *) zlist_first (self->server->patterns);
    while (pattern) {
        if (zrex_matches (pattern->rex, zccp_msg_address (self->request))) {
            client_t *client = (client_t *) zlist_first (pattern->clients);
            while (client) {
                if (client != self)
                    engine_send_event (client, forward_event);
                client = (client_t *) zlist_next (pattern->clients);
            }
        }
        pattern = (pattern_t *) zlist_next (self->server->patterns);
    }
}



//  --------------------------------------------------------------------------
//  get_content_to_publish
//

static void
get_content_to_publish (client_t *self)
{
    zhash_t *headers = zhash_dup (self->server->headers);
    zmsg_t *content = zmsg_dup (self->server->content);
    
    zccp_msg_set_sender  (self->reply, self->server->sender);
    zccp_msg_set_address (self->reply, self->server->address);
    zccp_msg_set_headers (self->reply, &headers);
    zccp_msg_set_content (self->reply, &content);
}


//  ---------------------------------------------------------------------------
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
    zsock_set_rcvtimeo (client, 200);

    zccp_msg_t *message;
    
    //  Check HELLO/HELLO-OK and INVALID
    
    zccp_msg_send_publish (client, "(HELLO, WORLD)", NULL, NULL);
    message = zccp_msg_recv (client);
    assert (message);
    assert (zccp_msg_id (message) == ZCCP_MSG_INVALID);
    zccp_msg_destroy (&message);
    
    zccp_msg_send_hello (client, "step 1", NULL);
    message = zccp_msg_recv (client);
    assert (message);
    assert (zccp_msg_id (message) == ZCCP_MSG_HELLO_OK);
    zccp_msg_destroy (&message);
    
    zccp_msg_send_hello (client, "step 2", NULL);
    message = zccp_msg_recv (client);
    assert (message);
    assert (zccp_msg_id (message) == ZCCP_MSG_INVALID);
    zccp_msg_destroy (&message);

    //  Let's try some SUBSCRIBE and PUBLISH commands
    zccp_msg_send_hello (client, "step 3 - client", NULL);
    message = zccp_msg_recv (client);
    assert (message);
    assert (zccp_msg_id (message) == ZCCP_MSG_HELLO_OK);
    zccp_msg_destroy (&message);
    
    zsock_t *device = zsock_new (ZMQ_DEALER);
    assert (device);
    zsock_connect (device, "ipc://@/zccp_server");
    
    zccp_msg_send_hello (device, "step 3 - device", NULL);
    message = zccp_msg_recv (device);
    assert (message);
    assert (zccp_msg_id (message) == ZCCP_MSG_HELLO_OK);
    zccp_msg_destroy (&message);

    //  Check our subscription engine; this should deliver us 2 messages
    zccp_msg_send_subscribe (client, "H.*O", NULL);
    zccp_msg_send_subscribe (client, "H.*O", NULL);
    zccp_msg_send_subscribe (client, "HELLO", NULL);
    //  Artificial delay to ensure subscriptions all arrive before we continue
    zclock_sleep (100);
    zmsg_t *content = zmsg_new ();
    zmsg_pushstr (content, "TEST");
    zccp_msg_send_publish (device, "(HELLO, WORLD)", NULL, content);
    zmsg_destroy (&content);
    
    message = zccp_msg_recv (client);
    assert (message);
    assert (zccp_msg_id (message) == ZCCP_MSG_DELIVER);
    assert (streq (zccp_msg_address (message), "(HELLO, WORLD)"));
    char *string = zmsg_popstr (zccp_msg_content (message));
    assert (streq (string, "TEST"));
    free (string);
    zccp_msg_destroy (&message);

    message = zccp_msg_recv (client);
    assert (message);
    assert (zccp_msg_id (message) == ZCCP_MSG_DELIVER);
    assert (streq (zccp_msg_address (message), "(HELLO, WORLD)"));
    string = zmsg_popstr (zccp_msg_content (message));
    assert (streq (string, "TEST"));
    free (string);
    zccp_msg_destroy (&message);

    message = zccp_msg_recv (client);
    assert (!message);
    
    //  Finished, we can clean up
    zsock_destroy (&device);
    zsock_destroy (&client);
    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}
