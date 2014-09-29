/*  =========================================================================
    zccp_client - ZCCP Client

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of zbroker, the ZeroMQ broker project.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    Description of class for man page.
@discuss
    Detailed discussion of the class, if any.
@end
*/

#include "zccp.h"

//  This structure defines the context for a client connection

typedef struct {
    //  These properties must always be present in the client_t
    //  and are set by the generated engine.
    zsock_t *pipe;              //  Actor pipe back to caller
    zsock_t *dealer;            //  Socket to talk to server
    zccp_msg_t *msgout;         //  Message to send to server
    zccp_msg_t *msgin;          //  Message received from server

    //  Own properties
    char *endpoint;             //  Last server endpoint we connected to
    int connect_timeout;        //  Timeout for connect handshake
    int presence_timeout;       //  Timeout for server presence
} client_t;

//  Include the generated client engine
#include "zccp_client_engine.inc"

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
    //  Should be 2-3 times server heartbeat interval
    self->presence_timeout = 3000;
    return 0;
}

//  Free properties and structures for a client instance

static void
client_terminate (client_t *self)
{
    //  Destroy properties here
    free (self->endpoint);
}

//  Process client API method, return event for state machine to process

static event_t
client_method (client_t *self, const char *method)
{
    if (streq (method, "CONNECT")) {
        free (self->endpoint);
        zsock_recv (self->pipe, "si", &self->endpoint, &self->connect_timeout);
        return connect_event;
    }
    else
    if (streq (method, "SUBSCRIBE")) {
        char *expression;
        zsock_recv (self->pipe, "s", &expression);
        zccp_msg_set_expression (self->msgout, expression);
        zstr_free (&expression);
        return subscribe_event;
    }
    else
    if (streq (method, "PUBLISH")) {
        char *address, *content;
        zsock_recv (self->pipe, "ss", &address, &content);
        zccp_msg_set_address (self->msgout, address);
        zstr_free (&address);
        zmsg_t *msg = zmsg_new ();
        zmsg_addstr (msg, content);
        zccp_msg_set_content (self->msgout, &msg);
        zstr_free (&content);
        return publish_event;
    }
    else
        return NULL_event;
}


//  ---------------------------------------------------------------------------
//  connect_to_server_endpoint
//

static void
connect_to_server_endpoint (client_t *self)
{
    if (zsock_connect (self->dealer, "%s", self->endpoint)) {
        engine_set_exception (self, error_event);
        zsys_warning ("could not connect to %s", self->endpoint);
    }
}


//  ---------------------------------------------------------------------------
//  use_connect_timeout
//

static void
use_connect_timeout (client_t *self)
{
    engine_set_timeout (self, self->connect_timeout);
}


//  ---------------------------------------------------------------------------
//  use_presence_timeout
//

static void
use_presence_timeout (client_t *self)
{
    engine_set_timeout (self, self->presence_timeout);
}


//  ---------------------------------------------------------------------------
//  signal_success
//

static void
signal_success (client_t *self)
{
    zsock_send (self->pipe, "si", "SUCCESS", 0);
}


//  ---------------------------------------------------------------------------
//  signal_server_not_present
//

static void
signal_server_not_present (client_t *self)
{
    zsock_send (self->pipe, "sis", "FAILURE", -1, "Server is not reachable");
}


//  ---------------------------------------------------------------------------
//  deliver_message_to_application
//

static void
deliver_message_to_application (client_t *self)
{
    char *content = zmsg_popstr (zccp_msg_content (self->msgin));
    zsock_send (self->pipe, "ssss", "MESSAGE",
                zccp_msg_sender (self->msgin),
                zccp_msg_address (self->msgin), content);
    zstr_free (&content);
}


//  ---------------------------------------------------------------------------
//  Selftest

void
zccp_client_test (bool verbose)
{
    printf (" * zccp_client: \n");
    if (verbose)
        printf ("\n");
    
    //  @selftest
    //  This is the standard IPC endpoint for the ZCCP service
    const char *endpoint = "ipc://@/zccp";

    //  Start a server to test against, and bind to endpoint
    zactor_t *server = zactor_new (zccp_server, "zccp_client_test");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "BIND", endpoint, NULL);
        
    //  Do a simple reader-writer test, using the high level API rather
    //  than the actor message interface.
    zccp_client_t *reader = zccp_client_new ();
    zccp_client_t *writer = zccp_client_new ();
    if (verbose) {
        zccp_client_verbose (reader);
        zccp_client_verbose (writer);
    }
    int rc = zccp_client_connect (reader, endpoint, 250);
    assert (rc == 0);
    rc = zccp_client_connect (writer, endpoint, 250);
    assert (rc == 0);
    
    rc = zccp_client_subscribe (reader, "^cpu\\.");
    assert (rc == 0);
    
    zccp_client_publish (writer, "cpu.1", "95");
    zccp_client_publish (writer, "cpu.2", "12");
    zccp_client_publish (writer, "cpu-count", "2");

    char *content = zccp_client_recv (reader);
    puts (content);
    content = zccp_client_recv (reader);
    puts (content);
    
    zccp_client_destroy (&writer);
    zccp_client_destroy (&reader);
    
    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}
