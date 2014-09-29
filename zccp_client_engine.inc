/*  =========================================================================
    zccp_client_engine - ZCCP Client engine

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: zccp_client.xml, or
     * The code generation script that built this file: zproto_client_c
    ************************************************************************

    =========================================================================
*/


//  ---------------------------------------------------------------------------
//  State machine constants

typedef enum {
    start_state = 1,
    connecting_state = 2,
    connected_state = 3,
    subscribing_state = 4,
    disconnecting_state = 5,
    defaults_state = 6,
    reconnecting_state = 7
} state_t;

typedef enum {
    NULL_event = 0,
    connect_event = 1,
    hello_ok_event = 2,
    expired_event = 3,
    subscribe_event = 4,
    publish_event = 5,
    direct_event = 6,
    deliver_event = 7,
    ping_event = 8,
    close_connection_event = 9,
    error_event = 10,
    subscribe_ok_event = 11,
    goodbye_ok_event = 12,
    invalid_event = 13
} event_t;

//  Names for state machine logging and error reporting
static char *
s_state_name [] = {
    "(NONE)",
    "start",
    "connecting",
    "connected",
    "subscribing",
    "disconnecting",
    "defaults",
    "reconnecting"
};

static char *
s_event_name [] = {
    "(NONE)",
    "connect",
    "HELLO_OK",
    "expired",
    "SUBSCRIBE",
    "PUBLISH",
    "DIRECT",
    "DELIVER",
    "PING",
    "close_connection",
    "error",
    "SUBSCRIBE_OK",
    "GOODBYE_OK",
    "INVALID"
};
 

//  ---------------------------------------------------------------------------
//  Context for the client. This embeds the application-level client context
//  at its start (the entire structure, not a reference), so we can cast a
//  pointer between client_t and s_client_t arbitrarily.

typedef struct {
    client_t client;            //  Application-level client context
    zsock_t *pipe;              //  Socket to back to caller API
    zsock_t *dealer;            //  Socket to talk to server
    zloop_t *loop;              //  Listen to pipe and dealer
    bool terminated;            //  True if client is shutdown
    size_t timeout;             //  inactivity timeout, msecs
    state_t state;              //  Current state
    event_t event;              //  Current event
    event_t next_event;         //  The next event
    event_t exception;          //  Exception event, if any
    int expiry_timer;           //  zloop timer for timeouts
    int wakeup_timer;           //  zloop timer for alarms
    event_t wakeup_event;       //  Wake up with this event
    bool verbose;               //  Verbose logging enabled?
} s_client_t;


static int
    client_initialize (client_t *self);
static void
    client_terminate (client_t *self);
static event_t
    client_method (client_t *self, const char *method);
static void
    s_client_destroy (s_client_t **self_p);
static void
    s_client_execute (s_client_t *self, event_t event);
static int
    s_client_handle_wakeup (zloop_t *loop, int timer_id, void *argument);
static void
    s_satisfy_pedantic_compilers (void);
static void
    connect_to_server_endpoint (client_t *self);
static void
    use_connect_timeout (client_t *self);
static void
    signal_success (client_t *self);
static void
    use_presence_timeout (client_t *self);
static void
    signal_server_not_present (client_t *self);
static void
    deliver_message_to_application (client_t *self);

//  Create a new client connection

static s_client_t *
s_client_new (zsock_t *pipe)
{
    s_client_t *self = (s_client_t *) zmalloc (sizeof (s_client_t));
    if (self) {
        assert ((s_client_t *) &self->client == self);
        self->pipe = pipe;
        self->dealer = zsock_new (ZMQ_DEALER);
        if (self->dealer)
            self->loop = zloop_new ();
        if (self->loop)
            self->client.msgout = zccp_msg_new (0);
        if (self->client.msgout) {
            //  Give application chance to initialize and set next event
            self->state = start_state;
            self->event = NULL_event;
            self->client.pipe = self->pipe;
            self->client.dealer = self->dealer;
            if (client_initialize (&self->client))
                s_client_destroy (&self);
        }
        else
            s_client_destroy (&self);
    }
    s_satisfy_pedantic_compilers ();
    return self;
}

//  Destroy the client connection

static void
s_client_destroy (s_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        s_client_t *self = *self_p;
        client_terminate (&self->client);
        zccp_msg_destroy (&self->client.msgout);
        zccp_msg_destroy (&self->client.msgin);
        zsock_destroy (&self->dealer);
        zloop_destroy (&self->loop);
        free (self);
        *self_p = NULL;
    }
}

//  ---------------------------------------------------------------------------
//  These methods are an internal API for actions

//  Set the next event, needed in at least one action in an internal
//  state; otherwise the state machine will wait for a message on the
//  dealer socket and treat that as the event.

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

//  Set wakeup alarm after 'delay' msecs. The next state should handle the
//  wakeup event. The alarm is cancelled on any other event.

static void
engine_set_wakeup_event (client_t *client, size_t delay, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        if (self->wakeup_timer) {
            zloop_timer_end (self->loop, self->wakeup_timer);
            self->wakeup_timer = 0;
        }
        self->wakeup_timer = zloop_timer (
            self->loop, delay, 1, s_client_handle_wakeup, self);
        self->wakeup_event = event;
    }
}

//  Set timeout for next protocol read. By default, will wait forever
//  or until the process is interrupted. The timeout is in milliseconds.
//  The state machine must handle the "expired" event.

static void
engine_set_timeout (client_t *client, int timeout)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->timeout = timeout;
    }
}

//  Poll socket for activity, invoke handler on any received message.
//  Handler must be a CZMQ zloop_fn function; receives client as arg.

static void
engine_handle_socket (client_t *client, zsock_t *socket, zloop_reader_fn handler)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        if (handler != NULL) {
            int rc = zloop_reader (self->loop, socket, handler, self);
            assert (rc == 0);
            zloop_reader_set_tolerant (self->loop, socket);
        }
        else
            zloop_reader_end (self->loop, socket);
    }
}

//  Pedantic compilers don't like unused functions, so we call the whole
//  API, passing null references. It's nasty and horrid and sufficient.

static void
s_satisfy_pedantic_compilers (void)
{
    engine_set_next_event (NULL, NULL_event);
    engine_set_exception (NULL, NULL_event);
    engine_set_timeout (NULL, 0);
    engine_set_wakeup_event (NULL, 0, NULL_event);
    engine_handle_socket (NULL, 0, NULL);
}


//  ---------------------------------------------------------------------------
//  Generic methods on protocol messages
//  TODO: replace with lookup table, since ID is one byte

static event_t
s_protocol_event (s_client_t *self, zccp_msg_t *msgin)
{
    assert (msgin);
    switch (zccp_msg_id (msgin)) {
        case ZCCP_MSG_HELLO_OK:
            return hello_ok_event;
            break;
        case ZCCP_MSG_SUBSCRIBE:
            return subscribe_event;
            break;
        case ZCCP_MSG_SUBSCRIBE_OK:
            return subscribe_ok_event;
            break;
        case ZCCP_MSG_PUBLISH:
            return publish_event;
            break;
        case ZCCP_MSG_DIRECT:
            return direct_event;
            break;
        case ZCCP_MSG_DELIVER:
            return deliver_event;
            break;
        case ZCCP_MSG_GOODBYE_OK:
            return goodbye_ok_event;
            break;
        case ZCCP_MSG_PING:
            return ping_event;
            break;
        case ZCCP_MSG_INVALID:
            return invalid_event;
            break;
        default:
            zsys_error ("zccp_client: invalid protocol event, halting");
            self->terminated = true;
            return NULL_event;
    }
}


//  Execute state machine as long as we have events; if event is NULL_event,
//  or state machine is terminated, do nothing.

static void
s_client_execute (s_client_t *self, event_t event)
{
    self->next_event = event;
    //  Cancel wakeup timer, if any was pending
    if (self->wakeup_timer) {
        zloop_timer_end (self->loop, self->wakeup_timer);
        self->wakeup_timer = 0;
    }
    while (!self->terminated && self->next_event != NULL_event) {
        self->event = self->next_event;
        self->next_event = NULL_event;
        self->exception = NULL_event;
        if (self->verbose) {
            zsys_debug ("zccp_client: %s:", s_state_name [self->state]);
            zsys_debug ("zccp_client:       %s", s_event_name [self->event]);
        }
        switch (self->state) {
            case start_state:
                if (self->event == connect_event) {
                    if (!self->exception) {
                        //  connect to server endpoint
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ connect to server endpoint");
                        connect_to_server_endpoint (&self->client);
                    }
                    if (!self->exception) {
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ use connect timeout");
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ send HELLO");
                        zccp_msg_set_id (self->client.msgout, ZCCP_MSG_HELLO);
                        zccp_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = zccp_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = connecting_state;
                }
                else {
                    //  Handle unexpected internal events
                    zsys_warning ("zccp_client: unhandled event %s in %s",
                        s_event_name [self->event], s_state_name [self->state]);
                    assert (false);
                }
                break;

            case connecting_state:
                if (self->event == hello_ok_event) {
                    if (!self->exception) {
                        //  signal success
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ signal success");
                        signal_success (&self->client);
                    }
                    if (!self->exception) {
                        //  use presence timeout
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ use presence timeout");
                        use_presence_timeout (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  signal server not present
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ signal server not present");
                        signal_server_not_present (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ terminate");
                        self->terminated = true;
                    }
                }
                else
                if (self->event == invalid_event) {
                    if (!self->exception) {
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ use connect timeout");
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ send HELLO");
                        zccp_msg_set_id (self->client.msgout, ZCCP_MSG_HELLO);
                        zccp_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = zccp_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = reconnecting_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case connected_state:
                if (self->event == subscribe_event) {
                    if (!self->exception) {
                        //  send SUBSCRIBE
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ send SUBSCRIBE");
                        zccp_msg_set_id (self->client.msgout, ZCCP_MSG_SUBSCRIBE);
                        zccp_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = zccp_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = subscribing_state;
                }
                else
                if (self->event == publish_event) {
                    if (!self->exception) {
                        //  send PUBLISH
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ send PUBLISH");
                        zccp_msg_set_id (self->client.msgout, ZCCP_MSG_PUBLISH);
                        zccp_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = zccp_msg_new (0);
                    }
                }
                else
                if (self->event == direct_event) {
                    if (!self->exception) {
                        //  send DIRECT
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ send DIRECT");
                        zccp_msg_set_id (self->client.msgout, ZCCP_MSG_DIRECT);
                        zccp_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = zccp_msg_new (0);
                    }
                }
                else
                if (self->event == deliver_event) {
                    if (!self->exception) {
                        //  deliver message to application
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ deliver message to application");
                        deliver_message_to_application (&self->client);
                    }
                }
                else
                if (self->event == ping_event) {
                    if (!self->exception) {
                        //  send PING_OK
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ send PING_OK");
                        zccp_msg_set_id (self->client.msgout, ZCCP_MSG_PING_OK);
                        zccp_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = zccp_msg_new (0);
                    }
                }
                else
                if (self->event == close_connection_event) {
                    if (!self->exception) {
                        //  send GOODBYE
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ send GOODBYE");
                        zccp_msg_set_id (self->client.msgout, ZCCP_MSG_GOODBYE);
                        zccp_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = zccp_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = disconnecting_state;
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ terminate");
                        self->terminated = true;
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ use connect timeout");
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ send HELLO");
                        zccp_msg_set_id (self->client.msgout, ZCCP_MSG_HELLO);
                        zccp_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = zccp_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = reconnecting_state;
                }
                else
                if (self->event == invalid_event) {
                    if (!self->exception) {
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ use connect timeout");
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ send HELLO");
                        zccp_msg_set_id (self->client.msgout, ZCCP_MSG_HELLO);
                        zccp_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = zccp_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = reconnecting_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case subscribing_state:
                if (self->event == subscribe_ok_event) {
                    if (!self->exception) {
                        //  signal success
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ signal success");
                        signal_success (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ use connect timeout");
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ send HELLO");
                        zccp_msg_set_id (self->client.msgout, ZCCP_MSG_HELLO);
                        zccp_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = zccp_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = reconnecting_state;
                }
                else
                if (self->event == invalid_event) {
                    if (!self->exception) {
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ use connect timeout");
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ send HELLO");
                        zccp_msg_set_id (self->client.msgout, ZCCP_MSG_HELLO);
                        zccp_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = zccp_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = reconnecting_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case disconnecting_state:
                if (self->event == goodbye_ok_event) {
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ terminate");
                        self->terminated = true;
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ terminate");
                        self->terminated = true;
                    }
                }
                else
                if (self->event == invalid_event) {
                    if (!self->exception) {
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ use connect timeout");
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ send HELLO");
                        zccp_msg_set_id (self->client.msgout, ZCCP_MSG_HELLO);
                        zccp_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = zccp_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = reconnecting_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case defaults_state:
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ use connect timeout");
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ send HELLO");
                        zccp_msg_set_id (self->client.msgout, ZCCP_MSG_HELLO);
                        zccp_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = zccp_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = reconnecting_state;
                }
                else
                if (self->event == invalid_event) {
                    if (!self->exception) {
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ use connect timeout");
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ send HELLO");
                        zccp_msg_set_id (self->client.msgout, ZCCP_MSG_HELLO);
                        zccp_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = zccp_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = reconnecting_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case reconnecting_state:
                if (self->event == hello_ok_event) {
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ use connect timeout");
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ send HELLO");
                        zccp_msg_set_id (self->client.msgout, ZCCP_MSG_HELLO);
                        zccp_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = zccp_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = reconnecting_state;
                }
                else
                if (self->event == invalid_event) {
                    if (!self->exception) {
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ use connect timeout");
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send HELLO
                        if (self->verbose)
                            zsys_debug ("zccp_client:           $ send HELLO");
                        zccp_msg_set_id (self->client.msgout, ZCCP_MSG_HELLO);
                        zccp_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = zccp_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = reconnecting_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;
        }
        //  If we had an exception event, interrupt normal programming
        if (self->exception) {
            if (self->verbose)
                zsys_debug ("zccp_client:           ! %s", s_event_name [self->exception]);
            self->next_event = self->exception;
        }
        else
        if (self->verbose)
            zsys_debug ("zccp_client:           > %s", s_state_name [self->state]);
    }
}

//  zloop callback when client inactivity timer expires

static int
s_client_handle_timeout (zloop_t *loop, int timer_id, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    s_client_execute (self, expired_event);
    return self->terminated? -1: 0;
}

//  zloop callback when client wakeup timer expires

static int
s_client_handle_wakeup (zloop_t *loop, int timer_id, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    s_client_execute (self, self->wakeup_event);
    return 0;
}


//  Process message from pipe

static int
s_client_handle_pipe (zloop_t *loop, zsock_t *reader, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    char *method = zstr_recv (self->pipe);
    if (!method)
        return -1;              //  Interrupted; exit zloop
    if (self->verbose)
        zsys_debug ("zccp_client:       API command=%s", method);
    
    if (streq (method, "VERBOSE"))
        self->verbose = true;
    else
    if (streq (method, "$TERM")) {
        //  Shutdown the engine
        zstr_free (&method);
        self->terminated = true;
    }
    else
        //  Execute custom method
        s_client_execute (self, client_method (&self->client, method));

    if (zsock_rcvmore (self->pipe)) {
        zsys_error ("zccp_client: trailing API command frames (%s)", method);
        zmsg_t *more = zmsg_recv (self->pipe);
        zmsg_print (more);
        zmsg_destroy (&more);
    }
    zstr_free (&method);
    return self->terminated? -1: 0;
}

//  Handle a message (a protocol reply) from the server

static int
s_client_handle_protocol (zloop_t *loop, zsock_t *reader, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    zccp_msg_t *msgin = zccp_msg_recv (self->dealer);
    if (!msgin)
        return -1;              //  Interrupted; exit zloop

    zccp_msg_destroy (&self->client.msgin);
    self->client.msgin = msgin;

    s_client_execute (self, s_protocol_event (self, msgin));
    return self->terminated? -1: 0;
}


//  ---------------------------------------------------------------------------
//  This is the client actor, which polls its two sockets and processes
//  incoming messages

void
zccp_client (zsock_t *pipe, void *args)
{
    //  Initialize
    s_client_t *self = s_client_new (pipe);
    if (self) {
        zsock_signal (pipe, 0);
        
        //  Set up handler for the two main sockets the client uses
        engine_handle_socket ((client_t *) self, self->pipe, s_client_handle_pipe);
        engine_handle_socket ((client_t *) self, self->dealer, s_client_handle_protocol);

        //  Run reactor until there's a termination signal
        zloop_start (self->loop);

        //  Reactor has ended
        s_client_destroy (&self);
    }
}


//  ---------------------------------------------------------------------------
//  Class interface

struct _zccp_client_t {
    zactor_t *actor;            //  Client actor
    int status;                 //  Returned by actor reply
    char *reason;               //  Returned by actor reply
    char *sender;               //  Returned by actor reply
    char *address;              //  Returned by actor reply
    char *content;              //  Returned by actor reply
};


//  ---------------------------------------------------------------------------
//  Create a new zccp_client

zccp_client_t *
zccp_client_new (void)
{
    zccp_client_t *self = (zccp_client_t *) zmalloc (sizeof (zccp_client_t));
    if (self) {
        self->actor = zactor_new (zccp_client, NULL);
        if (!self->actor)
            zccp_client_destroy (&self);
    }
    return self;
}


//  ---------------------------------------------------------------------------
//  Destroy the zccp_client

void
zccp_client_destroy (zccp_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zccp_client_t *self = *self_p;
        zactor_destroy (&self->actor);
        zstr_free (&self->reason);
        zstr_free (&self->sender);
        zstr_free (&self->address);
        zstr_free (&self->content);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------------
//  Enable verbose logging of client activity

void
zccp_client_verbose (zccp_client_t *self)
{
    assert (self);
    zsock_send (self->actor, "s", "VERBOSE");
}


//  ---------------------------------------------------------------------------
//  Return actor for low-level command control and polling

zactor_t *
zccp_client_actor (zccp_client_t *self)
{
    assert (self);
    return self->actor;
}


//  ---------------------------------------------------------------------------
//  Get valid reply from actor; discard replies that does not match. Current
//  implementation filters on first frame of message. Blocks until a valid
//  reply is received, and properties can be loaded from it. Returns 0 if
//  matched, -1 if interrupted or timed-out.

static int
s_accept_reply (zccp_client_t *self, ...)
{
    assert (self);
    while (true) {
        char *reply = zstr_recv (self->actor);
        if (!reply)
            return -1;          //  Interrupted or timed-out
        
        va_list args;
        va_start (args, self);
        char *filter = va_arg (args, char *);
        while (filter) {
            if (streq (reply, filter)) {
                if (streq (reply, "SUCCESS")) {
                    zsock_recv (self->actor, "i", &self->status);
                }
                else
                if (streq (reply, "FAILURE")) {
                    zstr_free (&self->reason);
                    zsock_recv (self->actor, "is", &self->status, &self->reason);
                }
                else
                if (streq (reply, "MESSAGE")) {
                    zstr_free (&self->sender);
                    zstr_free (&self->address);
                    zstr_free (&self->content);
                    zsock_recv (self->actor, "sss", &self->sender, &self->address, &self->content);
                }
                break;
            }
            //  Get next filter, and discard rest of message on the pipe
            filter = va_arg (args, char *);
            zsock_flush (self->actor);
        }
        va_end (args);
        if (filter)
            return 0;           //  We matched one of the filters
    }
}


//  ---------------------------------------------------------------------------
//  Connect to server and return only when there's a successful connection or the   
//  timeout in msecs expires. Returns 0 if successfully connected, else -1.         
//  Returns >= 0 if successful, -1 if interrupted.

int
zccp_client_connect (zccp_client_t *self, const char *endpoint, int timeout)
{
    assert (self);
    zsock_send (self->actor, "ssi", "CONNECT", endpoint, timeout);
    if (s_accept_reply (self, "SUCCESS", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Subscribe to all messages sent to matching addresses. The expression is a       
//  regular expression using the CZMQ zrex syntax. The most useful elements are: ^  
//  and $ to match the start and end, . to match any character, \s and \S to match  
//  whitespace and non-whitespace, \d and \D to match a digit and non-digit, \a and 
//  \A to match alphabetic and non-alphabetic, \w and \W to match alphanumeric and  
//  non-alphanumeric, + for one or more repetitions, * for zero or more repetitions,
//  and ( ) to create groups. Returns 0 if subscription was successful, else -1.    
//  Returns >= 0 if successful, -1 if interrupted.

int
zccp_client_subscribe (zccp_client_t *self, const char *expression)
{
    assert (self);
    zsock_send (self->actor, "ss", "SUBSCRIBE", expression);
    if (s_accept_reply (self, "SUCCESS", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Publish a message on the server, using a logical address. All subscribers to    
//  that address will receive a copy of the message. The server does not store      
//  messages. If a message is published before subscribers arrive, they will miss   
//  it. Currently only supports string contents. Does not return a status value;    
//  publish commands are asynchronous and unconfirmed.                              

int
zccp_client_publish (zccp_client_t *self, const char *address, const char *content)
{
    assert (self);
    zsock_send (self->actor, "sss", "PUBLISH", address, content);
}


//  ---------------------------------------------------------------------------
//  Receive next message from server. Returns the message content, as a string, if  
//  any. The caller should not modify or free this string.                          
//  Returns NULL on an interrupt.

char *
zccp_client_recv (zccp_client_t *self)
{
    assert (self);
    if (s_accept_reply (self, "MESSAGE", NULL))
        return NULL;            //  Interrupted or timed-out
    return self->content;
}


//  ---------------------------------------------------------------------------
//  Return last received status

int
zccp_client_status (zccp_client_t *self)
{
    assert (self);
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Return last received reason

char *
zccp_client_reason (zccp_client_t *self)
{
    assert (self);
    return self->reason;
}


//  ---------------------------------------------------------------------------
//  Return last received sender

char *
zccp_client_sender (zccp_client_t *self)
{
    assert (self);
    return self->sender;
}


//  ---------------------------------------------------------------------------
//  Return last received address

char *
zccp_client_address (zccp_client_t *self)
{
    assert (self);
    return self->address;
}


//  ---------------------------------------------------------------------------
//  Return last received content

char *
zccp_client_content (zccp_client_t *self)
{
    assert (self);
    return self->content;
}
