/*  =========================================================================
    zccp_server_engine - ZCCP Server engine

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: zccp_server.xml, or
     * The code generation script that built this file: zproto_server_c
    ************************************************************************

    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of zbroker, the ZeroMQ broker project.           
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/


//  ---------------------------------------------------------------------------
//  State machine constants

typedef enum {
    start_state = 1,
    connected_state = 2,
    external_state = 3
} state_t;

typedef enum {
    terminate_event = -1,
    NULL_event = 0,
    hello_event = 1,
    subscribe_event = 2,
    publish_event = 3,
    direct_event = 4,
    forward_event = 5,
    goodbye_event = 6,
    expired_event = 7
} event_t;

//  Names for state machine logging and error reporting
static char *
s_state_name [] = {
    "(NONE)",
    "start",
    "connected",
    "external"
};

static char *
s_event_name [] = {
    "(NONE)",
    "HELLO",
    "SUBSCRIBE",
    "PUBLISH",
    "DIRECT",
    "forward",
    "GOODBYE",
    "expired"
};
 

//  ---------------------------------------------------------------------------
//  Context for the whole server task. This embeds the application-level
//  server context at its start (the entire structure, not a reference),
//  so we can cast a pointer between server_t and s_server_t arbitrarily.

typedef struct {
    server_t server;            //  Application-level server context
    zsock_t *pipe;              //  Socket to back to caller API
    zsock_t *router;            //  Socket to talk to clients
    int port;                   //  Server port bound to
    zloop_t *loop;              //  Reactor for server sockets
    zhash_t *clients;           //  Clients we're connected to
    zconfig_t *config;          //  Configuration tree
    uint client_id;             //  Client identifier counter
    size_t timeout;             //  Default client expiry timeout
    bool verbose;               //  Verbose logging enabled?
    char *log_prefix;           //  Default log prefix
} s_server_t;


//  ---------------------------------------------------------------------------
//  Context for each connected client. This embeds the application-level
//  client context at its start (the entire structure, not a reference),
//  so we can cast a pointer between client_t and s_client_t arbitrarily.

typedef struct {
    client_t client;            //  Application-level client context
    s_server_t *server;         //  Parent server context
    char *hashkey;              //  Key into server->clients hash
    zframe_t *routing_id;       //  Routing_id back to client
    uint unique_id;             //  Client identifier in server
    zlist_t *mailbox;           //  Incoming messages
    state_t state;              //  Current state
    event_t event;              //  Current event
    event_t next_event;         //  The next event
    event_t exception;          //  Exception event, if any
    int expiry_timer;           //  zloop timer for client timeouts
    int wakeup_timer;           //  zloop timer for client alarms
    event_t wakeup_event;       //  Wake up with this event
    char log_prefix [41];       //  Log prefix string
} s_client_t;

static int
    server_initialize (server_t *self);
static void
    server_terminate (server_t *self);
static zmsg_t *
    server_method (server_t *self, const char *method, zmsg_t *msg);
static int
    client_initialize (client_t *self);
static void
    client_terminate (client_t *self);
static void
    s_client_execute (s_client_t *client, int event);
static int
    s_client_handle_wakeup (zloop_t *loop, int timer_id, void *argument);
static void
    register_new_client (client_t *self);
static void
    store_new_subscription (client_t *self);
static void
    forward_to_subscribers (client_t *self);
static void
    get_content_to_publish (client_t *self);

//  ---------------------------------------------------------------------------
//  These methods are an internal API for actions

//  Set the next event, needed in at least one action in an internal
//  state; otherwise the state machine will wait for a message on the
//  router socket and treat that as the event.

static void
engine_set_next_event (client_t *client, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->next_event = event;
    }
}

//  Raise an exception with 'event', halting any actions in progress.
//  Continues execution of actions defined for the exception event.

static void
engine_set_exception (client_t *client, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->exception = event;
    }
}

//  Set wakeup alarm after 'delay' msecs. The next state should
//  handle the wakeup event. The alarm is cancelled on any other
//  event.

static void
engine_set_wakeup_event (client_t *client, size_t delay, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        if (self->wakeup_timer) {
            zloop_timer_end (self->server->loop, self->wakeup_timer);
            self->wakeup_timer = 0;
        }
        self->wakeup_timer = zloop_timer (
            self->server->loop, delay, 1, s_client_handle_wakeup, self);
        self->wakeup_event = event;
    }
}

//  Execute 'event' on specified client. Use this to send events to
//  other clients. Cancels any wakeup alarm on that client.

static void
engine_send_event (client_t *client, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        s_client_execute (self, event);
    }
}

//  Execute 'event' on all clients known to the server. If you pass a
//  client argument, that client will not receive the broadcast. If you
//  want to pass any arguments, store them in the server context.

static void
engine_broadcast_event (server_t *server, client_t *client, event_t event)
{
    if (server) {
        s_server_t *self = (s_server_t *) server;
        zlist_t *keys = zhash_keys (self->clients);
        char *key = (char *) zlist_first (keys);
        while (key) {
            s_client_t *target = (s_client_t *) zhash_lookup (self->clients, key);
            if (target != (s_client_t *) client)
                s_client_execute (target, event);
            key = (char *) zlist_next (keys);
        }
        zlist_destroy (&keys);
    }
}

//  Poll socket for activity, invoke handler on any received message.
//  Handler must be a CZMQ zloop_fn function; receives server as arg.

static void
engine_handle_socket (server_t *server, zsock_t *socket, zloop_reader_fn handler)
{
    if (server) {
        s_server_t *self = (s_server_t *) server;
        if (handler != NULL) {
            int rc = zloop_reader (self->loop, socket, handler, self);
            assert (rc == 0);
            zloop_reader_set_tolerant (self->loop, socket);
        }
        else
            zloop_reader_end (self->loop, socket);
    }
}

//  Register monitor function that will be called at regular intervals
//  by the server engine

static void
engine_set_monitor (server_t *server, size_t interval, zloop_timer_fn monitor)
{
    if (server) {
        s_server_t *self = (s_server_t *) server;
        int rc = zloop_timer (self->loop, interval, 0, monitor, self);
        assert (rc >= 0);
    }
}

//  Set log file prefix; this string will be added to log data, to make
//  log data more searchable. The string is truncated to ~20 chars.

static void
engine_set_log_prefix (client_t *client, const char *string)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        snprintf (self->log_prefix, sizeof (self->log_prefix) - 1,
            "%6d:%-33s", self->unique_id, string);
    }
}

//  Set a configuration value in the server's configuration tree. The
//  properties this engine uses are: server/verbose, server/timeout, and
//  server/background. You can also configure other abitrary properties.

static void
engine_configure (server_t *server, const char *path, const char *value)
{
    if (server) {
        s_server_t *self = (s_server_t *) server;
        zconfig_put (self->config, path, value);
    }
}

//  Pedantic compilers don't like unused functions, so we call the whole
//  API, passing null references. It's nasty and horrid and sufficient.

static void
s_satisfy_pedantic_compilers (void)
{
    engine_set_next_event (NULL, NULL_event);
    engine_set_exception (NULL, NULL_event);
    engine_set_wakeup_event (NULL, 0, NULL_event);
    engine_send_event (NULL, NULL_event);
    engine_broadcast_event (NULL, NULL, NULL_event);
    engine_handle_socket (NULL, 0, NULL);
    engine_set_monitor (NULL, 0, NULL);
    engine_set_log_prefix (NULL, NULL);
    engine_configure (NULL, NULL, NULL);
}


//  ---------------------------------------------------------------------------
//  Generic methods on protocol messages
//  TODO: replace with lookup table, since ID is one byte

static event_t
s_protocol_event (zccp_msg_t *request)
{
    assert (request);
    switch (zccp_msg_id (request)) {
        case ZCCP_MSG_HELLO:
            return hello_event;
            break;
        case ZCCP_MSG_SUBSCRIBE:
            return subscribe_event;
            break;
        case ZCCP_MSG_PUBLISH:
            return publish_event;
            break;
        case ZCCP_MSG_DIRECT:
            return direct_event;
            break;
        case ZCCP_MSG_GOODBYE:
            return goodbye_event;
            break;
        default:
            //  Invalid zccp_msg_t
            return terminate_event;
    }
}


//  ---------------------------------------------------------------------------
//  Client methods

static s_client_t *
s_client_new (s_server_t *server, zframe_t *routing_id)
{
    s_client_t *self = (s_client_t *) zmalloc (sizeof (s_client_t));
    assert (self);
    assert ((s_client_t *) &self->client == self);
    
    self->server = server;
    self->hashkey = zframe_strhex (routing_id);
    self->routing_id = zframe_dup (routing_id);
    self->mailbox = zlist_new ();
    self->unique_id = server->client_id++;
    engine_set_log_prefix (&self->client, server->log_prefix);

    self->client.server = (server_t *) server;
    self->client.reply = zccp_msg_new (0);
    zccp_msg_set_routing_id (self->client.reply, self->routing_id);

    //  Give application chance to initialize and set next event
    self->state = start_state;
    self->event = NULL_event;
    client_initialize (&self->client);
    return self;
}

static void
s_client_destroy (s_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        s_client_t *self = *self_p;
        zccp_msg_destroy (&self->client.request);
        zccp_msg_destroy (&self->client.reply);
        
        if (self->wakeup_timer)
            zloop_timer_end (self->server->loop, self->wakeup_timer);
        if (self->expiry_timer)
            zloop_timer_end (self->server->loop, self->expiry_timer);

        //  Empty and destroy mailbox
        zccp_msg_t *request = (zccp_msg_t *) zlist_first (self->mailbox);
        while (request) {
            zccp_msg_destroy (&request);
            request = (zccp_msg_t *) zlist_next (self->mailbox);
        }
        zlist_destroy (&self->mailbox);
        zframe_destroy (&self->routing_id);
        client_terminate (&self->client);
        free (self->hashkey);
        free (self);
        *self_p = NULL;
    }
}

//  Callback when we remove client from 'clients' hash table
static void
s_client_free (void *argument)
{
    s_client_t *client = (s_client_t *) argument;
    s_client_destroy (&client);
}

//  Do we accept a request off the mailbox? If so, return the event for
//  the message, and load the message into the current client request.
//  If not, return NULL_event;

static event_t
s_client_filter_mailbox (s_client_t *self)
{
    assert (self);
    
    zccp_msg_t *request = (zccp_msg_t *) zlist_first (self->mailbox);
    while (request) {
        //  Check whether current state can process event
        //  This should be changed to a pre-built lookup table
        event_t event = s_protocol_event (request);
        bool event_is_valid;
        if (self->state == start_state)
            event_is_valid = true;
        else
        if (self->state == connected_state)
            event_is_valid = true;
        else
        if (self->state == external_state)
            event_is_valid = true;
        else
            event_is_valid = false;
            
        if (event_is_valid) {
            zlist_remove (self->mailbox, request);
            zccp_msg_destroy (&self->client.request);
            self->client.request = request;
            return event;
        }
        request = (zccp_msg_t *) zlist_next (self->mailbox);
    }
    return NULL_event;
}


//  Execute state machine as long as we have events

static void
s_client_execute (s_client_t *self, event_t event)
{
    self->next_event = event;
    //  Cancel wakeup timer, if any was pending
    if (self->wakeup_timer) {
        zloop_timer_end (self->server->loop, self->wakeup_timer);
        self->wakeup_timer = 0;
    }
    while (self->next_event != NULL_event) {
        self->event = self->next_event;
        self->next_event = NULL_event;
        self->exception = NULL_event;
        if (self->server->verbose) {
            zsys_debug ("%s: %s:",
                self->log_prefix, s_state_name [self->state]);
            zsys_debug ("%s:     %s",
                self->log_prefix, s_event_name [self->event]);
        }
        switch (self->state) {
            case start_state:
                if (self->event == hello_event) {
                    if (!self->exception) {
                        //  register new client
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ register new client", self->log_prefix);
                        register_new_client (&self->client);
                    }
                    if (!self->exception) {
                        //  send HELLO_OK
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send HELLO_OK",
                                self->log_prefix);
                        zccp_msg_set_id (self->client.reply, ZCCP_MSG_HELLO_OK);
                        zccp_msg_send (&self->client.reply, self->server->router);
                        self->client.reply = zccp_msg_new (0);
                        zccp_msg_set_routing_id (self->client.reply, self->routing_id);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == goodbye_event) {
                    if (!self->exception) {
                        //  send GOODBYE_OK
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send GOODBYE_OK",
                                self->log_prefix);
                        zccp_msg_set_id (self->client.reply, ZCCP_MSG_GOODBYE_OK);
                        zccp_msg_send (&self->client.reply, self->server->router);
                        self->client.reply = zccp_msg_new (0);
                        zccp_msg_set_routing_id (self->client.reply, self->routing_id);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send INVALID
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send INVALID",
                                self->log_prefix);
                        zccp_msg_set_id (self->client.reply, ZCCP_MSG_INVALID);
                        zccp_msg_send (&self->client.reply, self->server->router);
                        self->client.reply = zccp_msg_new (0);
                        zccp_msg_set_routing_id (self->client.reply, self->routing_id);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                break;

            case connected_state:
                if (self->event == subscribe_event) {
                    if (!self->exception) {
                        //  store new subscription
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ store new subscription", self->log_prefix);
                        store_new_subscription (&self->client);
                    }
                    if (!self->exception) {
                        //  send SUBSCRIBE_OK
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send SUBSCRIBE_OK",
                                self->log_prefix);
                        zccp_msg_set_id (self->client.reply, ZCCP_MSG_SUBSCRIBE_OK);
                        zccp_msg_send (&self->client.reply, self->server->router);
                        self->client.reply = zccp_msg_new (0);
                        zccp_msg_set_routing_id (self->client.reply, self->routing_id);
                    }
                }
                else
                if (self->event == publish_event) {
                    if (!self->exception) {
                        //  forward to subscribers
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ forward to subscribers", self->log_prefix);
                        forward_to_subscribers (&self->client);
                    }
                }
                else
                if (self->event == direct_event) {
                }
                else
                if (self->event == forward_event) {
                    if (!self->exception) {
                        //  get content to publish
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ get content to publish", self->log_prefix);
                        get_content_to_publish (&self->client);
                    }
                    if (!self->exception) {
                        //  send DELIVER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send DELIVER",
                                self->log_prefix);
                        zccp_msg_set_id (self->client.reply, ZCCP_MSG_DELIVER);
                        zccp_msg_send (&self->client.reply, self->server->router);
                        self->client.reply = zccp_msg_new (0);
                        zccp_msg_set_routing_id (self->client.reply, self->routing_id);
                    }
                }
                else
                if (self->event == goodbye_event) {
                    if (!self->exception) {
                        //  send GOODBYE_OK
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send GOODBYE_OK",
                                self->log_prefix);
                        zccp_msg_set_id (self->client.reply, ZCCP_MSG_GOODBYE_OK);
                        zccp_msg_send (&self->client.reply, self->server->router);
                        self->client.reply = zccp_msg_new (0);
                        zccp_msg_set_routing_id (self->client.reply, self->routing_id);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send INVALID
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send INVALID",
                                self->log_prefix);
                        zccp_msg_set_id (self->client.reply, ZCCP_MSG_INVALID);
                        zccp_msg_send (&self->client.reply, self->server->router);
                        self->client.reply = zccp_msg_new (0);
                        zccp_msg_set_routing_id (self->client.reply, self->routing_id);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                break;

            case external_state:
                if (self->event == goodbye_event) {
                    if (!self->exception) {
                        //  send GOODBYE_OK
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send GOODBYE_OK",
                                self->log_prefix);
                        zccp_msg_set_id (self->client.reply, ZCCP_MSG_GOODBYE_OK);
                        zccp_msg_send (&self->client.reply, self->server->router);
                        self->client.reply = zccp_msg_new (0);
                        zccp_msg_set_routing_id (self->client.reply, self->routing_id);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  send INVALID
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send INVALID",
                                self->log_prefix);
                        zccp_msg_set_id (self->client.reply, ZCCP_MSG_INVALID);
                        zccp_msg_send (&self->client.reply, self->server->router);
                        self->client.reply = zccp_msg_new (0);
                        zccp_msg_set_routing_id (self->client.reply, self->routing_id);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                break;
        }
        //  If we had an exception event, interrupt normal programming
        if (self->exception) {
            if (self->server->verbose)
                zsys_debug ("%s:         ! %s",
                    self->log_prefix, s_event_name [self->exception]);

            self->next_event = self->exception;
        }
        if (self->next_event == terminate_event) {
            //  Automatically calls s_client_destroy
            zhash_delete (self->server->clients, self->hashkey);
            break;
        }
        else {
            if (self->server->verbose)
                zsys_debug ("%s:         > %s",
                    self->log_prefix, s_state_name [self->state]);

            if (self->next_event == NULL_event)
                //  Get next valid message from mailbox, if any
                self->next_event = s_client_filter_mailbox (self);
        }
    }
}

//  zloop callback when client inactivity timer expires

static int
s_client_handle_timeout (zloop_t *loop, int timer_id, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    s_client_execute (self, expired_event);
    return 0;
}

//  zloop callback when client wakeup timer expires

static int
s_client_handle_wakeup (zloop_t *loop, int timer_id, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    s_client_execute (self, self->wakeup_event);
    return 0;
}


//  Server methods

static void
s_server_config_self (s_server_t *self)
{
    //  Built-in server configuration options
    //  
    //  If we didn't already set verbose, check if the config tree wants it
    if (!self->verbose
    && atoi (zconfig_resolve (self->config, "server/verbose", "0")))
        self->verbose = true;
        
    //  Default client timeout is 60 seconds
    self->timeout = atoi (
        zconfig_resolve (self->config, "server/timeout", "60000"));

    //  Do we want to run server in the background?
    int background = atoi (
        zconfig_resolve (self->config, "server/background", "0"));
    if (!background)
        zsys_set_logstream (stdout);
}

static s_server_t *
s_server_new (zsock_t *pipe)
{
    s_server_t *self = (s_server_t *) zmalloc (sizeof (s_server_t));
    assert (self);
    assert ((s_server_t *) &self->server == self);

    self->pipe = pipe;
    self->router = zsock_new (ZMQ_ROUTER);
    //  By default the socket will discard outgoing messages above the
    //  HWM of 1,000. This isn't helpful for high-volume streaming. We
    //  will use a unbounded queue here. If applications need to guard
    //  against queue overflow, they should use a credit-based flow
    //  control scheme.
    zsock_set_unbounded (self->router);
    self->clients = zhash_new ();
    self->config = zconfig_new ("root", NULL);
    self->loop = zloop_new ();
    srandom ((unsigned int) zclock_time ());
    self->client_id = randof (1000);
    s_server_config_self (self);

    //  Initialize application server context
    self->server.pipe = self->pipe;
    self->server.config = self->config;
    server_initialize (&self->server);

    s_satisfy_pedantic_compilers ();
    return self;
}

static void
s_server_destroy (s_server_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        s_server_t *self = *self_p;
        //  Destroy clients before destroying the server
        zhash_destroy (&self->clients);
        server_terminate (&self->server);
        zsock_destroy (&self->router);
        zconfig_destroy (&self->config);
        zloop_destroy (&self->loop);
        free (self);
        *self_p = NULL;
    }
}

//  Apply configuration tree:
//   * apply server configuration
//   * print any echo items in top-level sections
//   * apply sections that match methods

static void
s_server_apply_config (s_server_t *self)
{
    //  Apply echo commands and class methods
    zconfig_t *section = zconfig_locate (self->config, "zccp_server");
    if (section)
        section = zconfig_child (section);

    while (section) {
        if (streq (zconfig_name (section), "echo"))
            zsys_notice ("%s", zconfig_value (section));
        else
        if (streq (zconfig_name (section), "bind")) {
            char *endpoint = zconfig_resolve (section, "endpoint", "?");
            if (zsock_bind (self->router, "%s", endpoint) == -1)
                zsys_warning ("could not bind to %s", endpoint);
        }
        section = zconfig_next (section);
    }
    s_server_config_self (self);
}

//  Process message from pipe

static int
s_server_api_message (zloop_t *loop, zsock_t *reader, void *argument)
{
    s_server_t *self = (s_server_t *) argument;
    zmsg_t *msg = zmsg_recv (self->pipe);
    if (!msg)
        return -1;              //  Interrupted; exit zloop
    char *method = zmsg_popstr (msg);
    if (self->verbose)
        zsys_debug ("%s:     API command=%s", self->log_prefix, method);
    
    if (streq (method, "VERBOSE"))
        self->verbose = true;
    else
    if (streq (method, "$TERM")) {
        //  Shutdown the engine
        free (method);
        zmsg_destroy (&msg);
        return -1;
    }
    else
    if (streq (method, "BIND")) {
        //  Bind to a specified endpoint, which may use an ephemeral port
        char *endpoint = zmsg_popstr (msg);
        self->port = zsock_bind (self->router, "%s", endpoint);
        if (self->port == -1)
            zsys_warning ("could not bind to %s", endpoint);
        free (endpoint);
    }
    else
    if (streq (method, "PORT")) {
        //  Return PORT + port number from the last bind, if any
        zstr_sendm (self->pipe, "PORT");
        zstr_sendf (self->pipe, "%d", self->port);
    }
    else
    if (streq (method, "CONFIGURE")) {
        char *config_file = zmsg_popstr (msg);
        zconfig_destroy (&self->config);
        self->config = zconfig_load (config_file);
        if (self->config) {
            s_server_apply_config (self);
            self->server.config = self->config;
        }
        else {
            zsys_warning ("cannot load config file '%s'\n", config_file);
            self->config = zconfig_new ("root", NULL);
        }
        free (config_file);
    }
    else
    if (streq (method, "SET")) {
        char *path = zmsg_popstr (msg);
        char *value = zmsg_popstr (msg);
        zconfig_put (self->config, path, value);
        //  TODO: remove after October 9, 2014
        if (streq (path, "server/animate")) {
            zsys_warning ("'%s' is deprecated, use VERBOSE command instead", path);
            self->verbose = atoi (value);
        }
        s_server_config_self (self);
        free (path);
        free (value);
    }
    else {
        //  Execute custom method
        zmsg_t *reply = server_method (&self->server, method, msg);
        //  If reply isn't null, send it to caller
        zmsg_send (&reply, self->pipe);
    }
    free (method);
    zmsg_destroy (&msg);
    return 0;
}

//  Handle a message (a protocol request) from the client

static int
s_server_client_message (zloop_t *loop, zsock_t *reader, void *argument)
{
    s_server_t *self = (s_server_t *) argument;
    zccp_msg_t *msg = zccp_msg_recv (self->router);
    if (!msg)
        return -1;              //  Interrupted; exit zloop

    char *hashkey = zframe_strhex (zccp_msg_routing_id (msg));
    s_client_t *client = (s_client_t *) zhash_lookup (self->clients, hashkey);
    if (client == NULL) {
        client = s_client_new (self, zccp_msg_routing_id (msg));
        zhash_insert (self->clients, hashkey, client);
        zhash_freefn (self->clients, hashkey, s_client_free);
    }
    free (hashkey);

    //  Any input from client counts as activity
    if (client->expiry_timer)
        zloop_timer_end (self->loop, client->expiry_timer);
    //  Reset expiry timer
    client->expiry_timer = zloop_timer (
        self->loop, self->timeout, 1, s_client_handle_timeout, client);
        
    //  Queue request and possibly pass it to client state machine
    zlist_append (client->mailbox, msg);
    event_t event = s_client_filter_mailbox (client);
    if (event != NULL_event)
        s_client_execute (client, event);
        
    return 0;
}

//  Watch server config file and reload if changed

static int
s_watch_server_config (zloop_t *loop, int timer_id, void *argument)
{
    s_server_t *self = (s_server_t *) argument;
    if (zconfig_has_changed (self->config)
    &&  zconfig_reload (&self->config) == 0) {
        s_server_config_self (self);
        self->server.config = self->config;
        zsys_notice ("reloaded configuration from %s",
            zconfig_filename (self->config));
    }
    return 0;
}


//  ---------------------------------------------------------------------------
//  This is the server actor, which polls its two sockets and processes
//  incoming messages

void
zccp_server (zsock_t *pipe, void *args)
{
    //  Initialize
    s_server_t *self = s_server_new (pipe);
    assert (self);
    zsock_signal (pipe, 0);
    //  Actor argument may be a string used for logging
    self->log_prefix = args? (char *) args: "";

    //  Set-up server monitor to watch for config file changes
    engine_set_monitor ((server_t *) self, 1000, s_watch_server_config);
    //  Set up handler for the two main sockets the server uses
    engine_handle_socket ((server_t *) self, self->pipe, s_server_api_message);
    engine_handle_socket ((server_t *) self, self->router, s_server_client_message);

    //  Run reactor until there's a termination signal
    zloop_start (self->loop);

    //  Reactor has ended
    s_server_destroy (&self);
}
