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
    int error;                  //  Last error cause
};


//  Return 0 if the reply from the broker is what we expect, else return
//  -1. This includes interrupts.
//  TODO: implement timeout when broker doesn't reply at all
//  TODO: this loop should also PING the broker every second

static int
s_expect_reply (zccp_client_t *self, int message_id)
{
    zccp_msg_t *reply = zccp_msg_recv (self->dealer);
    if (!reply)
        return -1;
    int rc = zccp_msg_id (reply) == message_id? 0: -1;
    zccp_msg_destroy (&reply);
    return rc;
}


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
        zccp_msg_send_hello (self->dealer, identifier);
        if (s_expect_reply (self, ZCCP_MSG_READY))
            zccp_client_destroy (&self);
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
        zsock_destroy (&self->dealer);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------
//  Returns last error number, if any

int
zccp_client_error (zccp_client_t *self)
{
    assert (self);
    return self->error;
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
    zactor_t *server = zactor_new (zccp_server, "zccp_client_test");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "BIND", "ipc://@/zccp_server", NULL);

    zccp_client_t *client = zccp_client_new ("zccp_client", "ipc://@/zccp_server");
    assert (client);
    zccp_client_destroy (&client);
    
    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}
