/*  =========================================================================
    zccp_client.c - API for zccp client applications

    =========================================================================
*/

/*
@header
    Provides an API to a ZCCP server.
@discuss
@end
*/

#include "zccp.h"

//  ---------------------------------------------------------------------
//  Structure of zccp_client class

struct _zccp_client_t {
    zsock_t *dealer;            //  Dealer socket to zccp server
    char *type;                 //  Last received message type
    char *body;                 //  Last received message body
};


//  ---------------------------------------------------------------------
//  Constructor: creates new connection to server, sending HELLO and
//  expecting READY back. The client identifier is a string used for
//  REQUEST/REPLY exchanges.

zccp_client_t *
zccp_client_new (const char *identifier, const char *server)
{
    zccp_client_t *self = (zccp_client_t *) zmalloc (sizeof (zccp_client_t));
    assert (self);
    
    self->dealer = zsock_new (ZMQ_DEALER);
    assert (self->dealer);
    if (zsock_connect (self->dealer, "%s", server) == 0) {
        zsock_set_rcvtimeo (self->dealer, 2000);
        zccp_msg_send_hello (self->dealer, identifier);
        zccp_msg_t *msg = zccp_msg_recv (self->dealer);
        if (!msg || zccp_msg_id (msg) != ZCCP_MSG_READY)
            zccp_client_destroy (&self);
            
        zccp_msg_destroy (&msg);
    }
    else
        zccp_client_destroy (&self);
    
    return self;
}


//  ---------------------------------------------------------------------
//  Destructor

void
zccp_client_destroy (zccp_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zccp_client_t *self = *self_p;

        //  Handshake GOODBYE to the server just to be sure anything
        //  we sent does not get lost when we destroy our dealer socket.
        zsock_set_rcvtimeo (self->dealer, 500);
        zccp_msg_send_goodbye (self->dealer);
        zccp_msg_t *msg = zccp_msg_recv (self->dealer);
        zccp_msg_destroy (&msg);
        
        zsock_destroy (&self->dealer);
        free (self->type);
        free (self->body);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------
//  Subscribe to some set of notifications

void
zccp_client_subscribe (zccp_client_t *self, const char *subscription)
{
    zccp_msg_send_subscribe (self->dealer, subscription);
}


//  ---------------------------------------------------------------------
//  Send a notification message of some type

void
zccp_client_send (zccp_client_t *self, const char *type, const char *message)
{
    zchunk_t *content = zchunk_new (message, strlen (message));
    zccp_msg_send_publish (self->dealer, type, content);
    zchunk_destroy (&content);
}


//  ---------------------------------------------------------------------
//  Wait for and receive next notification message; returns 0 if OK,
//  else returns -1.

int
zccp_client_recv (zccp_client_t *self, char **type, char **body)
{
    zsock_set_rcvtimeo (self->dealer, -1);
    zccp_msg_t *msg = zccp_msg_recv (self->dealer);
    if (!msg)
        return -1;              //  Interrupted
        
    assert (zccp_msg_id (msg) == ZCCP_MSG_DELIVER);
    free (self->type);
    self->type = strdup (zccp_msg_header (msg));
    free (self->body);
    self->body = zchunk_strdup (zccp_msg_content (msg));
    zccp_msg_destroy (&msg);
    
    *type = self->type;
    *body = self->body;
    return 0;
}


//  ---------------------------------------------------------------------
// Self test of this class

void
zccp_client_test (bool verbose)
{
    printf (" * zccp_client: ");
    if (verbose)
        printf ("\n");
    
    //  @selftest
    ///
    const char *server_endpoint = "ipc://@/zccp_server";
    zactor_t *server = zactor_new (zccp_server, "zccp_client_test");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "BIND", server_endpoint, NULL);
    ///

    //  We'll simulate a network with a sensor device and a logger app
    zccp_client_t *logger = zccp_client_new ("logger", server_endpoint);
    assert (logger);
    zccp_client_subscribe (logger, "^temp\\.");
    zccp_client_subscribe (logger, "^fan\\.speed$");
    zccp_client_subscribe (logger, "^special");
    //  Artificial delay to ensure subscriptions arrive before we continue
    zclock_sleep (100);

    zccp_client_t *sensor = zccp_client_new ("sensor", server_endpoint);
    assert (sensor);
    zccp_client_send (sensor, "temp.cpu.1", "33");
    zccp_client_send (sensor, "temp.cpu.2", "200");
    zccp_client_send (sensor, "temp.cpu.3", "40");
    zccp_client_send (sensor, "temp.cpu.4", "56");
    zccp_client_send (sensor, "fan.speed", "1200");
    zccp_client_send (sensor, "fan.direction", "normal");
    zccp_client_send (sensor, "power.total", "15.3");
    zccp_client_send (sensor, "special", "FINISHED");

    while (true) {
        char *type, *body;
        int rc = zccp_client_recv (logger, &type, &body);
        assert (rc == 0);
        if (verbose)
            printf ("%s=%s\n", type, body);
        if (streq (body, "FINISHED"))
            break;
    }
    zccp_client_destroy (&logger);
    zccp_client_destroy (&sensor);

    ///
    zactor_destroy (&server);
    ///
    //  @end
    printf ("OK\n");
}
