/*  =========================================================================
    zccp_msg - ZeroMQ Command & Control Protocol

    Codec class for zccp_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

    * The XML model used for this code generation: zccp_msg.xml
    * The code generation script that built this file: zproto_codec_c
    ************************************************************************
    
    =========================================================================
*/

/*
@header
    zccp_msg - ZeroMQ Command & Control Protocol
@discuss
@end
*/

#include "czmq.h"
#include "./zccp_msg.h"

//  Structure of our class

struct _zccp_msg_t {
    zframe_t *routing_id;               //  Routing_id from ROUTER, if any
    int id;                             //  zccp_msg message ID
    byte *needle;                       //  Read/write pointer for serialization
    byte *ceiling;                      //  Valid upper limit for read pointer
    char *identifier;                   //  Client identifier
    char *expression;                   //  Regular expression
    char *header;                       //  Header, for matching
    zchunk_t *content;                  //  Content
    char *origin;                       //  A client identifier
    char *method;                       //  Requested method
    uint16_t status;                    //  Success/failure status
};

//  --------------------------------------------------------------------------
//  Network data encoding macros

//  Put a block of octets to the frame
#define PUT_OCTETS(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
}

//  Get a block of octets from the frame
#define GET_OCTETS(host,size) { \
    if (self->needle + size > self->ceiling) \
        goto malformed; \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
}

//  Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (host); \
    self->needle++; \
}

//  Put a 2-byte number to the frame
#define PUT_NUMBER2(host) { \
    self->needle [0] = (byte) (((host) >> 8)  & 255); \
    self->needle [1] = (byte) (((host))       & 255); \
    self->needle += 2; \
}

//  Put a 4-byte number to the frame
#define PUT_NUMBER4(host) { \
    self->needle [0] = (byte) (((host) >> 24) & 255); \
    self->needle [1] = (byte) (((host) >> 16) & 255); \
    self->needle [2] = (byte) (((host) >> 8)  & 255); \
    self->needle [3] = (byte) (((host))       & 255); \
    self->needle += 4; \
}

//  Put a 8-byte number to the frame
#define PUT_NUMBER8(host) { \
    self->needle [0] = (byte) (((host) >> 56) & 255); \
    self->needle [1] = (byte) (((host) >> 48) & 255); \
    self->needle [2] = (byte) (((host) >> 40) & 255); \
    self->needle [3] = (byte) (((host) >> 32) & 255); \
    self->needle [4] = (byte) (((host) >> 24) & 255); \
    self->needle [5] = (byte) (((host) >> 16) & 255); \
    self->needle [6] = (byte) (((host) >> 8)  & 255); \
    self->needle [7] = (byte) (((host))       & 255); \
    self->needle += 8; \
}

//  Get a 1-byte number from the frame
#define GET_NUMBER1(host) { \
    if (self->needle + 1 > self->ceiling) \
        goto malformed; \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) \
        goto malformed; \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) \
        goto malformed; \
    (host) = ((uint32_t) (self->needle [0]) << 24) \
           + ((uint32_t) (self->needle [1]) << 16) \
           + ((uint32_t) (self->needle [2]) << 8) \
           +  (uint32_t) (self->needle [3]); \
    self->needle += 4; \
}

//  Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    if (self->needle + 8 > self->ceiling) \
        goto malformed; \
    (host) = ((uint64_t) (self->needle [0]) << 56) \
           + ((uint64_t) (self->needle [1]) << 48) \
           + ((uint64_t) (self->needle [2]) << 40) \
           + ((uint64_t) (self->needle [3]) << 32) \
           + ((uint64_t) (self->needle [4]) << 24) \
           + ((uint64_t) (self->needle [5]) << 16) \
           + ((uint64_t) (self->needle [6]) << 8) \
           +  (uint64_t) (self->needle [7]); \
    self->needle += 8; \
}

//  Put a string to the frame
#define PUT_STRING(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER1 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a string from the frame
#define GET_STRING(host) { \
    size_t string_size; \
    GET_NUMBER1 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

//  Put a long string to the frame
#define PUT_LONGSTR(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER4 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a long string from the frame
#define GET_LONGSTR(host) { \
    size_t string_size; \
    GET_NUMBER4 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}


//  --------------------------------------------------------------------------
//  Create a new zccp_msg

zccp_msg_t *
zccp_msg_new (int id)
{
    zccp_msg_t *self = (zccp_msg_t *) zmalloc (sizeof (zccp_msg_t));
    self->id = id;
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zccp_msg

void
zccp_msg_destroy (zccp_msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zccp_msg_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);
        free (self->identifier);
        free (self->expression);
        free (self->header);
        zchunk_destroy (&self->content);
        free (self->origin);
        free (self->method);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Parse a zccp_msg from zmsg_t. Returns a new object, or NULL if
//  the message could not be parsed, or was NULL. Destroys msg and 
//  nullifies the msg reference.

zccp_msg_t *
zccp_msg_decode (zmsg_t **msg_p)
{
    assert (msg_p);
    zmsg_t *msg = *msg_p;
    if (msg == NULL)
        return NULL;
        
    zccp_msg_t *self = zccp_msg_new (0);
    //  Read and parse command in frame
    zframe_t *frame = zmsg_pop (msg);
    if (!frame) 
        goto empty;             //  Malformed or empty

    //  Get and check protocol signature
    self->needle = zframe_data (frame);
    self->ceiling = self->needle + zframe_size (frame);
    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 10))
        goto empty;             //  Invalid signature

    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case ZCCP_MSG_HELLO:
            GET_STRING (self->identifier);
            break;

        case ZCCP_MSG_GOODBYE:
            break;

        case ZCCP_MSG_READY:
            break;

        case ZCCP_MSG_SUBSCRIBE:
            GET_STRING (self->expression);
            break;

        case ZCCP_MSG_PUBLISH:
            GET_STRING (self->header);
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling))
                    goto malformed;
                self->content = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            break;

        case ZCCP_MSG_DELIVER:
            GET_STRING (self->origin);
            GET_STRING (self->header);
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling))
                    goto malformed;
                self->content = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            break;

        case ZCCP_MSG_REQUEST:
            GET_STRING (self->method);
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling))
                    goto malformed;
                self->content = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            break;

        case ZCCP_MSG_REPLY:
            GET_NUMBER2 (self->status);
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling))
                    goto malformed;
                self->content = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            break;

        case ZCCP_MSG_INVALID:
            break;

        default:
            goto malformed;
    }
    //  Successful return
    zframe_destroy (&frame);
    zmsg_destroy (msg_p);
    return self;

    //  Error returns
    malformed:
        zsys_error ("malformed message '%d'\n", self->id);
    empty:
        zframe_destroy (&frame);
        zmsg_destroy (msg_p);
        zccp_msg_destroy (&self);
        return (NULL);
}


//  --------------------------------------------------------------------------
//  Encode zccp_msg into zmsg and destroy it. Returns a newly created
//  object or NULL if error. Use when not in control of sending the message.

zmsg_t *
zccp_msg_encode (zccp_msg_t **self_p)
{
    assert (self_p);
    assert (*self_p);
    
    zccp_msg_t *self = *self_p;
    zmsg_t *msg = zmsg_new ();

    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case ZCCP_MSG_HELLO:
            //  identifier is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->identifier)
                frame_size += strlen (self->identifier);
            break;
            
        case ZCCP_MSG_GOODBYE:
            break;
            
        case ZCCP_MSG_READY:
            break;
            
        case ZCCP_MSG_SUBSCRIBE:
            //  expression is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->expression)
                frame_size += strlen (self->expression);
            break;
            
        case ZCCP_MSG_PUBLISH:
            //  header is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->header)
                frame_size += strlen (self->header);
            //  content is a chunk with 4-byte length
            frame_size += 4;
            if (self->content)
                frame_size += zchunk_size (self->content);
            break;
            
        case ZCCP_MSG_DELIVER:
            //  origin is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->origin)
                frame_size += strlen (self->origin);
            //  header is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->header)
                frame_size += strlen (self->header);
            //  content is a chunk with 4-byte length
            frame_size += 4;
            if (self->content)
                frame_size += zchunk_size (self->content);
            break;
            
        case ZCCP_MSG_REQUEST:
            //  method is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->method)
                frame_size += strlen (self->method);
            //  content is a chunk with 4-byte length
            frame_size += 4;
            if (self->content)
                frame_size += zchunk_size (self->content);
            break;
            
        case ZCCP_MSG_REPLY:
            //  status is a 2-byte integer
            frame_size += 2;
            //  content is a chunk with 4-byte length
            frame_size += 4;
            if (self->content)
                frame_size += zchunk_size (self->content);
            break;
            
        case ZCCP_MSG_INVALID:
            break;
            
        default:
            zsys_error ("bad message type '%d', not sent\n", self->id);
            //  No recovery, this is a fatal application error
            assert (false);
    }
    //  Now serialize message into the frame
    zframe_t *frame = zframe_new (NULL, frame_size);
    self->needle = zframe_data (frame);
    PUT_NUMBER2 (0xAAA0 | 10);
    PUT_NUMBER1 (self->id);

    switch (self->id) {
        case ZCCP_MSG_HELLO:
            if (self->identifier) {
                PUT_STRING (self->identifier);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case ZCCP_MSG_GOODBYE:
            break;

        case ZCCP_MSG_READY:
            break;

        case ZCCP_MSG_SUBSCRIBE:
            if (self->expression) {
                PUT_STRING (self->expression);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case ZCCP_MSG_PUBLISH:
            if (self->header) {
                PUT_STRING (self->header);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->content) {
                PUT_NUMBER4 (zchunk_size (self->content));
                memcpy (self->needle,
                        zchunk_data (self->content),
                        zchunk_size (self->content));
                self->needle += zchunk_size (self->content);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
            break;

        case ZCCP_MSG_DELIVER:
            if (self->origin) {
                PUT_STRING (self->origin);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->header) {
                PUT_STRING (self->header);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->content) {
                PUT_NUMBER4 (zchunk_size (self->content));
                memcpy (self->needle,
                        zchunk_data (self->content),
                        zchunk_size (self->content));
                self->needle += zchunk_size (self->content);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
            break;

        case ZCCP_MSG_REQUEST:
            if (self->method) {
                PUT_STRING (self->method);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->content) {
                PUT_NUMBER4 (zchunk_size (self->content));
                memcpy (self->needle,
                        zchunk_data (self->content),
                        zchunk_size (self->content));
                self->needle += zchunk_size (self->content);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
            break;

        case ZCCP_MSG_REPLY:
            PUT_NUMBER2 (self->status);
            if (self->content) {
                PUT_NUMBER4 (zchunk_size (self->content));
                memcpy (self->needle,
                        zchunk_data (self->content),
                        zchunk_size (self->content));
                self->needle += zchunk_size (self->content);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
            break;

        case ZCCP_MSG_INVALID:
            break;

    }
    //  Now send the data frame
    if (zmsg_append (msg, &frame)) {
        zmsg_destroy (&msg);
        zccp_msg_destroy (self_p);
        return NULL;
    }
    //  Destroy zccp_msg object
    zccp_msg_destroy (self_p);
    return msg;
}


//  --------------------------------------------------------------------------
//  Receive and parse a zccp_msg from the socket. Returns new object or
//  NULL if error. Will block if there's no message waiting.

zccp_msg_t *
zccp_msg_recv (void *input)
{
    assert (input);
    zmsg_t *msg = zmsg_recv (input);
    if (!msg)
        return NULL;            //  Interrupted
    //  If message came from a router socket, first frame is routing_id
    zframe_t *routing_id = NULL;
    if (zsocket_type (zsock_resolve (input)) == ZMQ_ROUTER) {
        routing_id = zmsg_pop (msg);
        //  If message was not valid, forget about it
        if (!routing_id || !zmsg_next (msg))
            return NULL;        //  Malformed or empty
    }
    zccp_msg_t *zccp_msg = zccp_msg_decode (&msg);
    if (zccp_msg && zsocket_type (zsock_resolve (input)) == ZMQ_ROUTER)
        zccp_msg->routing_id = routing_id;

    return zccp_msg;
}


//  --------------------------------------------------------------------------
//  Receive and parse a zccp_msg from the socket. Returns new object,
//  or NULL either if there was no input waiting, or the recv was interrupted.

zccp_msg_t *
zccp_msg_recv_nowait (void *input)
{
    assert (input);
    zmsg_t *msg = zmsg_recv_nowait (input);
    if (!msg)
        return NULL;            //  Interrupted
    //  If message came from a router socket, first frame is routing_id
    zframe_t *routing_id = NULL;
    if (zsocket_type (zsock_resolve (input)) == ZMQ_ROUTER) {
        routing_id = zmsg_pop (msg);
        //  If message was not valid, forget about it
        if (!routing_id || !zmsg_next (msg))
            return NULL;        //  Malformed or empty
    }
    zccp_msg_t *zccp_msg = zccp_msg_decode (&msg);
    if (zccp_msg && zsocket_type (zsock_resolve (input)) == ZMQ_ROUTER)
        zccp_msg->routing_id = routing_id;

    return zccp_msg;
}


//  --------------------------------------------------------------------------
//  Send the zccp_msg to the socket, and destroy it
//  Returns 0 if OK, else -1

int
zccp_msg_send (zccp_msg_t **self_p, void *output)
{
    assert (self_p);
    assert (*self_p);
    assert (output);

    //  Save routing_id if any, as encode will destroy it
    zccp_msg_t *self = *self_p;
    zframe_t *routing_id = self->routing_id;
    self->routing_id = NULL;

    //  Encode zccp_msg message to a single zmsg
    zmsg_t *msg = zccp_msg_encode (self_p);
    
    //  If we're sending to a ROUTER, send the routing_id first
    if (zsocket_type (zsock_resolve (output)) == ZMQ_ROUTER) {
        assert (routing_id);
        zmsg_prepend (msg, &routing_id);
    }
    else
        zframe_destroy (&routing_id);
        
    if (msg && zmsg_send (&msg, output) == 0)
        return 0;
    else
        return -1;              //  Failed to encode, or send
}


//  --------------------------------------------------------------------------
//  Send the zccp_msg to the output, and do not destroy it

int
zccp_msg_send_again (zccp_msg_t *self, void *output)
{
    assert (self);
    assert (output);
    self = zccp_msg_dup (self);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Encode HELLO message

zmsg_t * 
zccp_msg_encode_hello (
    const char *identifier)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_HELLO);
    zccp_msg_set_identifier (self, identifier);
    return zccp_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode GOODBYE message

zmsg_t * 
zccp_msg_encode_goodbye (
)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_GOODBYE);
    return zccp_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode READY message

zmsg_t * 
zccp_msg_encode_ready (
)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_READY);
    return zccp_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode SUBSCRIBE message

zmsg_t * 
zccp_msg_encode_subscribe (
    const char *expression)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_SUBSCRIBE);
    zccp_msg_set_expression (self, expression);
    return zccp_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode PUBLISH message

zmsg_t * 
zccp_msg_encode_publish (
    const char *header,
    zchunk_t *content)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_PUBLISH);
    zccp_msg_set_header (self, header);
    zchunk_t *content_copy = zchunk_dup (content);
    zccp_msg_set_content (self, &content_copy);
    return zccp_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode DELIVER message

zmsg_t * 
zccp_msg_encode_deliver (
    const char *origin,
    const char *header,
    zchunk_t *content)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_DELIVER);
    zccp_msg_set_origin (self, origin);
    zccp_msg_set_header (self, header);
    zchunk_t *content_copy = zchunk_dup (content);
    zccp_msg_set_content (self, &content_copy);
    return zccp_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode REQUEST message

zmsg_t * 
zccp_msg_encode_request (
    const char *method,
    zchunk_t *content)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_REQUEST);
    zccp_msg_set_method (self, method);
    zchunk_t *content_copy = zchunk_dup (content);
    zccp_msg_set_content (self, &content_copy);
    return zccp_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode REPLY message

zmsg_t * 
zccp_msg_encode_reply (
    uint16_t status,
    zchunk_t *content)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_REPLY);
    zccp_msg_set_status (self, status);
    zchunk_t *content_copy = zchunk_dup (content);
    zccp_msg_set_content (self, &content_copy);
    return zccp_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode INVALID message

zmsg_t * 
zccp_msg_encode_invalid (
)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_INVALID);
    return zccp_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Send the HELLO to the socket in one step

int
zccp_msg_send_hello (
    void *output,
    const char *identifier)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_HELLO);
    zccp_msg_set_identifier (self, identifier);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the GOODBYE to the socket in one step

int
zccp_msg_send_goodbye (
    void *output)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_GOODBYE);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the READY to the socket in one step

int
zccp_msg_send_ready (
    void *output)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_READY);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the SUBSCRIBE to the socket in one step

int
zccp_msg_send_subscribe (
    void *output,
    const char *expression)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_SUBSCRIBE);
    zccp_msg_set_expression (self, expression);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the PUBLISH to the socket in one step

int
zccp_msg_send_publish (
    void *output,
    const char *header,
    zchunk_t *content)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_PUBLISH);
    zccp_msg_set_header (self, header);
    zchunk_t *content_copy = zchunk_dup (content);
    zccp_msg_set_content (self, &content_copy);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the DELIVER to the socket in one step

int
zccp_msg_send_deliver (
    void *output,
    const char *origin,
    const char *header,
    zchunk_t *content)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_DELIVER);
    zccp_msg_set_origin (self, origin);
    zccp_msg_set_header (self, header);
    zchunk_t *content_copy = zchunk_dup (content);
    zccp_msg_set_content (self, &content_copy);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the REQUEST to the socket in one step

int
zccp_msg_send_request (
    void *output,
    const char *method,
    zchunk_t *content)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_REQUEST);
    zccp_msg_set_method (self, method);
    zchunk_t *content_copy = zchunk_dup (content);
    zccp_msg_set_content (self, &content_copy);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the REPLY to the socket in one step

int
zccp_msg_send_reply (
    void *output,
    uint16_t status,
    zchunk_t *content)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_REPLY);
    zccp_msg_set_status (self, status);
    zchunk_t *content_copy = zchunk_dup (content);
    zccp_msg_set_content (self, &content_copy);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the INVALID to the socket in one step

int
zccp_msg_send_invalid (
    void *output)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_INVALID);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Duplicate the zccp_msg message

zccp_msg_t *
zccp_msg_dup (zccp_msg_t *self)
{
    if (!self)
        return NULL;
        
    zccp_msg_t *copy = zccp_msg_new (self->id);
    if (self->routing_id)
        copy->routing_id = zframe_dup (self->routing_id);
    switch (self->id) {
        case ZCCP_MSG_HELLO:
            copy->identifier = self->identifier? strdup (self->identifier): NULL;
            break;

        case ZCCP_MSG_GOODBYE:
            break;

        case ZCCP_MSG_READY:
            break;

        case ZCCP_MSG_SUBSCRIBE:
            copy->expression = self->expression? strdup (self->expression): NULL;
            break;

        case ZCCP_MSG_PUBLISH:
            copy->header = self->header? strdup (self->header): NULL;
            copy->content = self->content? zchunk_dup (self->content): NULL;
            break;

        case ZCCP_MSG_DELIVER:
            copy->origin = self->origin? strdup (self->origin): NULL;
            copy->header = self->header? strdup (self->header): NULL;
            copy->content = self->content? zchunk_dup (self->content): NULL;
            break;

        case ZCCP_MSG_REQUEST:
            copy->method = self->method? strdup (self->method): NULL;
            copy->content = self->content? zchunk_dup (self->content): NULL;
            break;

        case ZCCP_MSG_REPLY:
            copy->status = self->status;
            copy->content = self->content? zchunk_dup (self->content): NULL;
            break;

        case ZCCP_MSG_INVALID:
            break;

    }
    return copy;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
zccp_msg_print (zccp_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZCCP_MSG_HELLO:
            zsys_debug ("ZCCP_MSG_HELLO:");
            if (self->identifier)
                zsys_debug ("    identifier='%s'", self->identifier);
            else
                zsys_debug ("    identifier=");
            break;
            
        case ZCCP_MSG_GOODBYE:
            zsys_debug ("ZCCP_MSG_GOODBYE:");
            break;
            
        case ZCCP_MSG_READY:
            zsys_debug ("ZCCP_MSG_READY:");
            break;
            
        case ZCCP_MSG_SUBSCRIBE:
            zsys_debug ("ZCCP_MSG_SUBSCRIBE:");
            if (self->expression)
                zsys_debug ("    expression='%s'", self->expression);
            else
                zsys_debug ("    expression=");
            break;
            
        case ZCCP_MSG_PUBLISH:
            zsys_debug ("ZCCP_MSG_PUBLISH:");
            if (self->header)
                zsys_debug ("    header='%s'", self->header);
            else
                zsys_debug ("    header=");
            zsys_debug ("    content=[ ... ]");
            break;
            
        case ZCCP_MSG_DELIVER:
            zsys_debug ("ZCCP_MSG_DELIVER:");
            if (self->origin)
                zsys_debug ("    origin='%s'", self->origin);
            else
                zsys_debug ("    origin=");
            if (self->header)
                zsys_debug ("    header='%s'", self->header);
            else
                zsys_debug ("    header=");
            zsys_debug ("    content=[ ... ]");
            break;
            
        case ZCCP_MSG_REQUEST:
            zsys_debug ("ZCCP_MSG_REQUEST:");
            if (self->method)
                zsys_debug ("    method='%s'", self->method);
            else
                zsys_debug ("    method=");
            zsys_debug ("    content=[ ... ]");
            break;
            
        case ZCCP_MSG_REPLY:
            zsys_debug ("ZCCP_MSG_REPLY:");
            zsys_debug ("    status=%ld", (long) self->status);
            zsys_debug ("    content=[ ... ]");
            break;
            
        case ZCCP_MSG_INVALID:
            zsys_debug ("ZCCP_MSG_INVALID:");
            break;
            
    }
}


//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
zccp_msg_routing_id (zccp_msg_t *self)
{
    assert (self);
    return self->routing_id;
}

void
zccp_msg_set_routing_id (zccp_msg_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the zccp_msg id

int
zccp_msg_id (zccp_msg_t *self)
{
    assert (self);
    return self->id;
}

void
zccp_msg_set_id (zccp_msg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
zccp_msg_command (zccp_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZCCP_MSG_HELLO:
            return ("HELLO");
            break;
        case ZCCP_MSG_GOODBYE:
            return ("GOODBYE");
            break;
        case ZCCP_MSG_READY:
            return ("READY");
            break;
        case ZCCP_MSG_SUBSCRIBE:
            return ("SUBSCRIBE");
            break;
        case ZCCP_MSG_PUBLISH:
            return ("PUBLISH");
            break;
        case ZCCP_MSG_DELIVER:
            return ("DELIVER");
            break;
        case ZCCP_MSG_REQUEST:
            return ("REQUEST");
            break;
        case ZCCP_MSG_REPLY:
            return ("REPLY");
            break;
        case ZCCP_MSG_INVALID:
            return ("INVALID");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the identifier field

const char *
zccp_msg_identifier (zccp_msg_t *self)
{
    assert (self);
    return self->identifier;
}

void
zccp_msg_set_identifier (zccp_msg_t *self, const char *format, ...)
{
    //  Format identifier from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->identifier);
    self->identifier = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the expression field

const char *
zccp_msg_expression (zccp_msg_t *self)
{
    assert (self);
    return self->expression;
}

void
zccp_msg_set_expression (zccp_msg_t *self, const char *format, ...)
{
    //  Format expression from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->expression);
    self->expression = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the header field

const char *
zccp_msg_header (zccp_msg_t *self)
{
    assert (self);
    return self->header;
}

void
zccp_msg_set_header (zccp_msg_t *self, const char *format, ...)
{
    //  Format header from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->header);
    self->header = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get the content field without transferring ownership

zchunk_t *
zccp_msg_content (zccp_msg_t *self)
{
    assert (self);
    return self->content;
}

//  Get the content field and transfer ownership to caller

zchunk_t *
zccp_msg_get_content (zccp_msg_t *self)
{
    zchunk_t *content = self->content;
    self->content = NULL;
    return content;
}

//  Set the content field, transferring ownership from caller

void
zccp_msg_set_content (zccp_msg_t *self, zchunk_t **chunk_p)
{
    assert (self);
    assert (chunk_p);
    zchunk_destroy (&self->content);
    self->content = *chunk_p;
    *chunk_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get/set the origin field

const char *
zccp_msg_origin (zccp_msg_t *self)
{
    assert (self);
    return self->origin;
}

void
zccp_msg_set_origin (zccp_msg_t *self, const char *format, ...)
{
    //  Format origin from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->origin);
    self->origin = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the method field

const char *
zccp_msg_method (zccp_msg_t *self)
{
    assert (self);
    return self->method;
}

void
zccp_msg_set_method (zccp_msg_t *self, const char *format, ...)
{
    //  Format method from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->method);
    self->method = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the status field

uint16_t
zccp_msg_status (zccp_msg_t *self)
{
    assert (self);
    return self->status;
}

void
zccp_msg_set_status (zccp_msg_t *self, uint16_t status)
{
    assert (self);
    self->status = status;
}



//  --------------------------------------------------------------------------
//  Selftest

int
zccp_msg_test (bool verbose)
{
    printf (" * zccp_msg: ");

    //  @selftest
    //  Simple create/destroy test
    zccp_msg_t *self = zccp_msg_new (0);
    assert (self);
    zccp_msg_destroy (&self);

    //  Create pair of sockets we can send through
    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    zsock_connect (input, "inproc://selftest-zccp_msg");

    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    zsock_bind (output, "inproc://selftest-zccp_msg");

    //  Encode/send/decode and verify each message type
    int instance;
    zccp_msg_t *copy;
    self = zccp_msg_new (ZCCP_MSG_HELLO);
    
    //  Check that _dup works on empty message
    copy = zccp_msg_dup (self);
    assert (copy);
    zccp_msg_destroy (&copy);

    zccp_msg_set_identifier (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    zccp_msg_send_again (self, output);
    zccp_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zccp_msg_recv (input);
        assert (self);
        assert (zccp_msg_routing_id (self));
        
        assert (streq (zccp_msg_identifier (self), "Life is short but Now lasts for ever"));
        zccp_msg_destroy (&self);
    }
    self = zccp_msg_new (ZCCP_MSG_GOODBYE);
    
    //  Check that _dup works on empty message
    copy = zccp_msg_dup (self);
    assert (copy);
    zccp_msg_destroy (&copy);

    //  Send twice from same object
    zccp_msg_send_again (self, output);
    zccp_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zccp_msg_recv (input);
        assert (self);
        assert (zccp_msg_routing_id (self));
        
        zccp_msg_destroy (&self);
    }
    self = zccp_msg_new (ZCCP_MSG_READY);
    
    //  Check that _dup works on empty message
    copy = zccp_msg_dup (self);
    assert (copy);
    zccp_msg_destroy (&copy);

    //  Send twice from same object
    zccp_msg_send_again (self, output);
    zccp_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zccp_msg_recv (input);
        assert (self);
        assert (zccp_msg_routing_id (self));
        
        zccp_msg_destroy (&self);
    }
    self = zccp_msg_new (ZCCP_MSG_SUBSCRIBE);
    
    //  Check that _dup works on empty message
    copy = zccp_msg_dup (self);
    assert (copy);
    zccp_msg_destroy (&copy);

    zccp_msg_set_expression (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    zccp_msg_send_again (self, output);
    zccp_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zccp_msg_recv (input);
        assert (self);
        assert (zccp_msg_routing_id (self));
        
        assert (streq (zccp_msg_expression (self), "Life is short but Now lasts for ever"));
        zccp_msg_destroy (&self);
    }
    self = zccp_msg_new (ZCCP_MSG_PUBLISH);
    
    //  Check that _dup works on empty message
    copy = zccp_msg_dup (self);
    assert (copy);
    zccp_msg_destroy (&copy);

    zccp_msg_set_header (self, "Life is short but Now lasts for ever");
    zchunk_t *publish_content = zchunk_new ("Captcha Diem", 12);
    zccp_msg_set_content (self, &publish_content);
    //  Send twice from same object
    zccp_msg_send_again (self, output);
    zccp_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zccp_msg_recv (input);
        assert (self);
        assert (zccp_msg_routing_id (self));
        
        assert (streq (zccp_msg_header (self), "Life is short but Now lasts for ever"));
        assert (memcmp (zchunk_data (zccp_msg_content (self)), "Captcha Diem", 12) == 0);
        zccp_msg_destroy (&self);
    }
    self = zccp_msg_new (ZCCP_MSG_DELIVER);
    
    //  Check that _dup works on empty message
    copy = zccp_msg_dup (self);
    assert (copy);
    zccp_msg_destroy (&copy);

    zccp_msg_set_origin (self, "Life is short but Now lasts for ever");
    zccp_msg_set_header (self, "Life is short but Now lasts for ever");
    zchunk_t *deliver_content = zchunk_new ("Captcha Diem", 12);
    zccp_msg_set_content (self, &deliver_content);
    //  Send twice from same object
    zccp_msg_send_again (self, output);
    zccp_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zccp_msg_recv (input);
        assert (self);
        assert (zccp_msg_routing_id (self));
        
        assert (streq (zccp_msg_origin (self), "Life is short but Now lasts for ever"));
        assert (streq (zccp_msg_header (self), "Life is short but Now lasts for ever"));
        assert (memcmp (zchunk_data (zccp_msg_content (self)), "Captcha Diem", 12) == 0);
        zccp_msg_destroy (&self);
    }
    self = zccp_msg_new (ZCCP_MSG_REQUEST);
    
    //  Check that _dup works on empty message
    copy = zccp_msg_dup (self);
    assert (copy);
    zccp_msg_destroy (&copy);

    zccp_msg_set_method (self, "Life is short but Now lasts for ever");
    zchunk_t *request_content = zchunk_new ("Captcha Diem", 12);
    zccp_msg_set_content (self, &request_content);
    //  Send twice from same object
    zccp_msg_send_again (self, output);
    zccp_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zccp_msg_recv (input);
        assert (self);
        assert (zccp_msg_routing_id (self));
        
        assert (streq (zccp_msg_method (self), "Life is short but Now lasts for ever"));
        assert (memcmp (zchunk_data (zccp_msg_content (self)), "Captcha Diem", 12) == 0);
        zccp_msg_destroy (&self);
    }
    self = zccp_msg_new (ZCCP_MSG_REPLY);
    
    //  Check that _dup works on empty message
    copy = zccp_msg_dup (self);
    assert (copy);
    zccp_msg_destroy (&copy);

    zccp_msg_set_status (self, 123);
    zchunk_t *reply_content = zchunk_new ("Captcha Diem", 12);
    zccp_msg_set_content (self, &reply_content);
    //  Send twice from same object
    zccp_msg_send_again (self, output);
    zccp_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zccp_msg_recv (input);
        assert (self);
        assert (zccp_msg_routing_id (self));
        
        assert (zccp_msg_status (self) == 123);
        assert (memcmp (zchunk_data (zccp_msg_content (self)), "Captcha Diem", 12) == 0);
        zccp_msg_destroy (&self);
    }
    self = zccp_msg_new (ZCCP_MSG_INVALID);
    
    //  Check that _dup works on empty message
    copy = zccp_msg_dup (self);
    assert (copy);
    zccp_msg_destroy (&copy);

    //  Send twice from same object
    zccp_msg_send_again (self, output);
    zccp_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zccp_msg_recv (input);
        assert (self);
        assert (zccp_msg_routing_id (self));
        
        zccp_msg_destroy (&self);
    }

    zsock_destroy (&input);
    zsock_destroy (&output);
    //  @end

    printf ("OK\n");
    return 0;
}
