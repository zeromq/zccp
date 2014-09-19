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
    zhash_t *headers;                   //  Client properties
    size_t headers_bytes;               //  Size of dictionary content
    char *expression;                   //  Regular expression
    char *address;                      //  Destination routing key
    zmsg_t *content;                    //  Content, as multipart message
    char *sender;                       //  Originating client
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
        zhash_destroy (&self->headers);
        free (self->expression);
        free (self->address);
        zmsg_destroy (&self->content);
        free (self->sender);

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
            {
                size_t hash_size;
                GET_NUMBER4 (hash_size);
                self->headers = zhash_new ();
                zhash_autofree (self->headers);
                while (hash_size--) {
                    char *key, *value;
                    GET_STRING (key);
                    GET_LONGSTR (value);
                    zhash_insert (self->headers, key, value);
                    free (key);
                    free (value);
                }
            }
            break;

        case ZCCP_MSG_HELLO_OK:
            {
                size_t hash_size;
                GET_NUMBER4 (hash_size);
                self->headers = zhash_new ();
                zhash_autofree (self->headers);
                while (hash_size--) {
                    char *key, *value;
                    GET_STRING (key);
                    GET_LONGSTR (value);
                    zhash_insert (self->headers, key, value);
                    free (key);
                    free (value);
                }
            }
            break;

        case ZCCP_MSG_SUBSCRIBE:
            GET_STRING (self->expression);
            {
                size_t hash_size;
                GET_NUMBER4 (hash_size);
                self->headers = zhash_new ();
                zhash_autofree (self->headers);
                while (hash_size--) {
                    char *key, *value;
                    GET_STRING (key);
                    GET_LONGSTR (value);
                    zhash_insert (self->headers, key, value);
                    free (key);
                    free (value);
                }
            }
            break;

        case ZCCP_MSG_PUBLISH:
            GET_STRING (self->address);
            {
                size_t hash_size;
                GET_NUMBER4 (hash_size);
                self->headers = zhash_new ();
                zhash_autofree (self->headers);
                while (hash_size--) {
                    char *key, *value;
                    GET_STRING (key);
                    GET_LONGSTR (value);
                    zhash_insert (self->headers, key, value);
                    free (key);
                    free (value);
                }
            }
            //  Get zero or more remaining frames, leaving current
            //  frame untouched
            self->content = zmsg_new ();
            while (zmsg_size (msg))
                zmsg_add (self->content, zmsg_pop (msg));
            break;

        case ZCCP_MSG_DIRECT:
            GET_STRING (self->address);
            {
                size_t hash_size;
                GET_NUMBER4 (hash_size);
                self->headers = zhash_new ();
                zhash_autofree (self->headers);
                while (hash_size--) {
                    char *key, *value;
                    GET_STRING (key);
                    GET_LONGSTR (value);
                    zhash_insert (self->headers, key, value);
                    free (key);
                    free (value);
                }
            }
            //  Get zero or more remaining frames, leaving current
            //  frame untouched
            self->content = zmsg_new ();
            while (zmsg_size (msg))
                zmsg_add (self->content, zmsg_pop (msg));
            break;

        case ZCCP_MSG_DELIVER:
            GET_STRING (self->sender);
            GET_STRING (self->address);
            {
                size_t hash_size;
                GET_NUMBER4 (hash_size);
                self->headers = zhash_new ();
                zhash_autofree (self->headers);
                while (hash_size--) {
                    char *key, *value;
                    GET_STRING (key);
                    GET_LONGSTR (value);
                    zhash_insert (self->headers, key, value);
                    free (key);
                    free (value);
                }
            }
            //  Get zero or more remaining frames, leaving current
            //  frame untouched
            self->content = zmsg_new ();
            while (zmsg_size (msg))
                zmsg_add (self->content, zmsg_pop (msg));
            break;

        case ZCCP_MSG_GOODBYE:
            break;

        case ZCCP_MSG_GOODBYE_OK:
            {
                size_t hash_size;
                GET_NUMBER4 (hash_size);
                self->headers = zhash_new ();
                zhash_autofree (self->headers);
                while (hash_size--) {
                    char *key, *value;
                    GET_STRING (key);
                    GET_LONGSTR (value);
                    zhash_insert (self->headers, key, value);
                    free (key);
                    free (value);
                }
            }
            break;

        case ZCCP_MSG_PING:
            break;

        case ZCCP_MSG_PING_OK:
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
            //  headers is an array of key=value strings
            frame_size += 4;    //  Size is 4 octets
            if (self->headers) {
                self->headers_bytes = 0;
                //  Add up size of dictionary contents
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    self->headers_bytes += 1 + strlen (zhash_cursor (self->headers));
                    self->headers_bytes += 4 + strlen (item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            frame_size += self->headers_bytes;
            break;
            
        case ZCCP_MSG_HELLO_OK:
            //  headers is an array of key=value strings
            frame_size += 4;    //  Size is 4 octets
            if (self->headers) {
                self->headers_bytes = 0;
                //  Add up size of dictionary contents
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    self->headers_bytes += 1 + strlen (zhash_cursor (self->headers));
                    self->headers_bytes += 4 + strlen (item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            frame_size += self->headers_bytes;
            break;
            
        case ZCCP_MSG_SUBSCRIBE:
            //  expression is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->expression)
                frame_size += strlen (self->expression);
            //  headers is an array of key=value strings
            frame_size += 4;    //  Size is 4 octets
            if (self->headers) {
                self->headers_bytes = 0;
                //  Add up size of dictionary contents
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    self->headers_bytes += 1 + strlen (zhash_cursor (self->headers));
                    self->headers_bytes += 4 + strlen (item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            frame_size += self->headers_bytes;
            break;
            
        case ZCCP_MSG_PUBLISH:
            //  address is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->address)
                frame_size += strlen (self->address);
            //  headers is an array of key=value strings
            frame_size += 4;    //  Size is 4 octets
            if (self->headers) {
                self->headers_bytes = 0;
                //  Add up size of dictionary contents
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    self->headers_bytes += 1 + strlen (zhash_cursor (self->headers));
                    self->headers_bytes += 4 + strlen (item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            frame_size += self->headers_bytes;
            break;
            
        case ZCCP_MSG_DIRECT:
            //  address is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->address)
                frame_size += strlen (self->address);
            //  headers is an array of key=value strings
            frame_size += 4;    //  Size is 4 octets
            if (self->headers) {
                self->headers_bytes = 0;
                //  Add up size of dictionary contents
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    self->headers_bytes += 1 + strlen (zhash_cursor (self->headers));
                    self->headers_bytes += 4 + strlen (item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            frame_size += self->headers_bytes;
            break;
            
        case ZCCP_MSG_DELIVER:
            //  sender is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->sender)
                frame_size += strlen (self->sender);
            //  address is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->address)
                frame_size += strlen (self->address);
            //  headers is an array of key=value strings
            frame_size += 4;    //  Size is 4 octets
            if (self->headers) {
                self->headers_bytes = 0;
                //  Add up size of dictionary contents
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    self->headers_bytes += 1 + strlen (zhash_cursor (self->headers));
                    self->headers_bytes += 4 + strlen (item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            frame_size += self->headers_bytes;
            break;
            
        case ZCCP_MSG_GOODBYE:
            break;
            
        case ZCCP_MSG_GOODBYE_OK:
            //  headers is an array of key=value strings
            frame_size += 4;    //  Size is 4 octets
            if (self->headers) {
                self->headers_bytes = 0;
                //  Add up size of dictionary contents
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    self->headers_bytes += 1 + strlen (zhash_cursor (self->headers));
                    self->headers_bytes += 4 + strlen (item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            frame_size += self->headers_bytes;
            break;
            
        case ZCCP_MSG_PING:
            break;
            
        case ZCCP_MSG_PING_OK:
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
            if (self->headers) {
                PUT_NUMBER4 (zhash_size (self->headers));
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    PUT_STRING (zhash_cursor (self->headers));
                    PUT_LONGSTR (item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty dictionary
            break;

        case ZCCP_MSG_HELLO_OK:
            if (self->headers) {
                PUT_NUMBER4 (zhash_size (self->headers));
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    PUT_STRING (zhash_cursor (self->headers));
                    PUT_LONGSTR (item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty dictionary
            break;

        case ZCCP_MSG_SUBSCRIBE:
            if (self->expression) {
                PUT_STRING (self->expression);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->headers) {
                PUT_NUMBER4 (zhash_size (self->headers));
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    PUT_STRING (zhash_cursor (self->headers));
                    PUT_LONGSTR (item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty dictionary
            break;

        case ZCCP_MSG_PUBLISH:
            if (self->address) {
                PUT_STRING (self->address);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->headers) {
                PUT_NUMBER4 (zhash_size (self->headers));
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    PUT_STRING (zhash_cursor (self->headers));
                    PUT_LONGSTR (item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty dictionary
            break;

        case ZCCP_MSG_DIRECT:
            if (self->address) {
                PUT_STRING (self->address);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->headers) {
                PUT_NUMBER4 (zhash_size (self->headers));
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    PUT_STRING (zhash_cursor (self->headers));
                    PUT_LONGSTR (item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty dictionary
            break;

        case ZCCP_MSG_DELIVER:
            if (self->sender) {
                PUT_STRING (self->sender);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->address) {
                PUT_STRING (self->address);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->headers) {
                PUT_NUMBER4 (zhash_size (self->headers));
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    PUT_STRING (zhash_cursor (self->headers));
                    PUT_LONGSTR (item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty dictionary
            break;

        case ZCCP_MSG_GOODBYE:
            break;

        case ZCCP_MSG_GOODBYE_OK:
            if (self->headers) {
                PUT_NUMBER4 (zhash_size (self->headers));
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    PUT_STRING (zhash_cursor (self->headers));
                    PUT_LONGSTR (item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty dictionary
            break;

        case ZCCP_MSG_PING:
            break;

        case ZCCP_MSG_PING_OK:
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
    //  Now send the content field if set
    if (self->id == ZCCP_MSG_PUBLISH) {
        zframe_t *content_part = zmsg_pop (self->content);
        while (content_part) {
            zmsg_append (msg, &content_part);
            content_part = zmsg_pop (self->content);
        }
    }
    //  Now send the content field if set
    if (self->id == ZCCP_MSG_DIRECT) {
        zframe_t *content_part = zmsg_pop (self->content);
        while (content_part) {
            zmsg_append (msg, &content_part);
            content_part = zmsg_pop (self->content);
        }
    }
    //  Now send the content field if set
    if (self->id == ZCCP_MSG_DELIVER) {
        zframe_t *content_part = zmsg_pop (self->content);
        while (content_part) {
            zmsg_append (msg, &content_part);
            content_part = zmsg_pop (self->content);
        }
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
    const char *identifier,
    zhash_t *headers)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_HELLO);
    zccp_msg_set_identifier (self, identifier);
    zhash_t *headers_copy = zhash_dup (headers);
    zccp_msg_set_headers (self, &headers_copy);
    return zccp_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode HELLO_OK message

zmsg_t * 
zccp_msg_encode_hello_ok (
    zhash_t *headers)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_HELLO_OK);
    zhash_t *headers_copy = zhash_dup (headers);
    zccp_msg_set_headers (self, &headers_copy);
    return zccp_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode SUBSCRIBE message

zmsg_t * 
zccp_msg_encode_subscribe (
    const char *expression,
    zhash_t *headers)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_SUBSCRIBE);
    zccp_msg_set_expression (self, expression);
    zhash_t *headers_copy = zhash_dup (headers);
    zccp_msg_set_headers (self, &headers_copy);
    return zccp_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode PUBLISH message

zmsg_t * 
zccp_msg_encode_publish (
    const char *address,
    zhash_t *headers,
    zmsg_t *content)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_PUBLISH);
    zccp_msg_set_address (self, address);
    zhash_t *headers_copy = zhash_dup (headers);
    zccp_msg_set_headers (self, &headers_copy);
    zmsg_t *content_copy = zmsg_dup (content);
    zccp_msg_set_content (self, &content_copy);
    return zccp_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode DIRECT message

zmsg_t * 
zccp_msg_encode_direct (
    const char *address,
    zhash_t *headers,
    zmsg_t *content)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_DIRECT);
    zccp_msg_set_address (self, address);
    zhash_t *headers_copy = zhash_dup (headers);
    zccp_msg_set_headers (self, &headers_copy);
    zmsg_t *content_copy = zmsg_dup (content);
    zccp_msg_set_content (self, &content_copy);
    return zccp_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode DELIVER message

zmsg_t * 
zccp_msg_encode_deliver (
    const char *sender,
    const char *address,
    zhash_t *headers,
    zmsg_t *content)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_DELIVER);
    zccp_msg_set_sender (self, sender);
    zccp_msg_set_address (self, address);
    zhash_t *headers_copy = zhash_dup (headers);
    zccp_msg_set_headers (self, &headers_copy);
    zmsg_t *content_copy = zmsg_dup (content);
    zccp_msg_set_content (self, &content_copy);
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
//  Encode GOODBYE_OK message

zmsg_t * 
zccp_msg_encode_goodbye_ok (
    zhash_t *headers)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_GOODBYE_OK);
    zhash_t *headers_copy = zhash_dup (headers);
    zccp_msg_set_headers (self, &headers_copy);
    return zccp_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode PING message

zmsg_t * 
zccp_msg_encode_ping (
)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_PING);
    return zccp_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode PING_OK message

zmsg_t * 
zccp_msg_encode_ping_ok (
)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_PING_OK);
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
    const char *identifier,
    zhash_t *headers)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_HELLO);
    zccp_msg_set_identifier (self, identifier);
    zhash_t *headers_copy = zhash_dup (headers);
    zccp_msg_set_headers (self, &headers_copy);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the HELLO_OK to the socket in one step

int
zccp_msg_send_hello_ok (
    void *output,
    zhash_t *headers)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_HELLO_OK);
    zhash_t *headers_copy = zhash_dup (headers);
    zccp_msg_set_headers (self, &headers_copy);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the SUBSCRIBE to the socket in one step

int
zccp_msg_send_subscribe (
    void *output,
    const char *expression,
    zhash_t *headers)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_SUBSCRIBE);
    zccp_msg_set_expression (self, expression);
    zhash_t *headers_copy = zhash_dup (headers);
    zccp_msg_set_headers (self, &headers_copy);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the PUBLISH to the socket in one step

int
zccp_msg_send_publish (
    void *output,
    const char *address,
    zhash_t *headers,
    zmsg_t *content)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_PUBLISH);
    zccp_msg_set_address (self, address);
    zhash_t *headers_copy = zhash_dup (headers);
    zccp_msg_set_headers (self, &headers_copy);
    zmsg_t *content_copy = zmsg_dup (content);
    zccp_msg_set_content (self, &content_copy);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the DIRECT to the socket in one step

int
zccp_msg_send_direct (
    void *output,
    const char *address,
    zhash_t *headers,
    zmsg_t *content)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_DIRECT);
    zccp_msg_set_address (self, address);
    zhash_t *headers_copy = zhash_dup (headers);
    zccp_msg_set_headers (self, &headers_copy);
    zmsg_t *content_copy = zmsg_dup (content);
    zccp_msg_set_content (self, &content_copy);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the DELIVER to the socket in one step

int
zccp_msg_send_deliver (
    void *output,
    const char *sender,
    const char *address,
    zhash_t *headers,
    zmsg_t *content)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_DELIVER);
    zccp_msg_set_sender (self, sender);
    zccp_msg_set_address (self, address);
    zhash_t *headers_copy = zhash_dup (headers);
    zccp_msg_set_headers (self, &headers_copy);
    zmsg_t *content_copy = zmsg_dup (content);
    zccp_msg_set_content (self, &content_copy);
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
//  Send the GOODBYE_OK to the socket in one step

int
zccp_msg_send_goodbye_ok (
    void *output,
    zhash_t *headers)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_GOODBYE_OK);
    zhash_t *headers_copy = zhash_dup (headers);
    zccp_msg_set_headers (self, &headers_copy);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the PING to the socket in one step

int
zccp_msg_send_ping (
    void *output)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_PING);
    return zccp_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the PING_OK to the socket in one step

int
zccp_msg_send_ping_ok (
    void *output)
{
    zccp_msg_t *self = zccp_msg_new (ZCCP_MSG_PING_OK);
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
            copy->headers = self->headers? zhash_dup (self->headers): NULL;
            break;

        case ZCCP_MSG_HELLO_OK:
            copy->headers = self->headers? zhash_dup (self->headers): NULL;
            break;

        case ZCCP_MSG_SUBSCRIBE:
            copy->expression = self->expression? strdup (self->expression): NULL;
            copy->headers = self->headers? zhash_dup (self->headers): NULL;
            break;

        case ZCCP_MSG_PUBLISH:
            copy->address = self->address? strdup (self->address): NULL;
            copy->headers = self->headers? zhash_dup (self->headers): NULL;
            copy->content = self->content? zmsg_dup (self->content): NULL;
            break;

        case ZCCP_MSG_DIRECT:
            copy->address = self->address? strdup (self->address): NULL;
            copy->headers = self->headers? zhash_dup (self->headers): NULL;
            copy->content = self->content? zmsg_dup (self->content): NULL;
            break;

        case ZCCP_MSG_DELIVER:
            copy->sender = self->sender? strdup (self->sender): NULL;
            copy->address = self->address? strdup (self->address): NULL;
            copy->headers = self->headers? zhash_dup (self->headers): NULL;
            copy->content = self->content? zmsg_dup (self->content): NULL;
            break;

        case ZCCP_MSG_GOODBYE:
            break;

        case ZCCP_MSG_GOODBYE_OK:
            copy->headers = self->headers? zhash_dup (self->headers): NULL;
            break;

        case ZCCP_MSG_PING:
            break;

        case ZCCP_MSG_PING_OK:
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
            zsys_debug ("    headers=");
            if (self->headers) {
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    zsys_debug ("        %s=%s", zhash_cursor (self->headers), item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            else
                zsys_debug ("(NULL)");
            break;
            
        case ZCCP_MSG_HELLO_OK:
            zsys_debug ("ZCCP_MSG_HELLO_OK:");
            zsys_debug ("    headers=");
            if (self->headers) {
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    zsys_debug ("        %s=%s", zhash_cursor (self->headers), item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            else
                zsys_debug ("(NULL)");
            break;
            
        case ZCCP_MSG_SUBSCRIBE:
            zsys_debug ("ZCCP_MSG_SUBSCRIBE:");
            if (self->expression)
                zsys_debug ("    expression='%s'", self->expression);
            else
                zsys_debug ("    expression=");
            zsys_debug ("    headers=");
            if (self->headers) {
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    zsys_debug ("        %s=%s", zhash_cursor (self->headers), item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            else
                zsys_debug ("(NULL)");
            break;
            
        case ZCCP_MSG_PUBLISH:
            zsys_debug ("ZCCP_MSG_PUBLISH:");
            if (self->address)
                zsys_debug ("    address='%s'", self->address);
            else
                zsys_debug ("    address=");
            zsys_debug ("    headers=");
            if (self->headers) {
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    zsys_debug ("        %s=%s", zhash_cursor (self->headers), item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            else
                zsys_debug ("(NULL)");
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;
            
        case ZCCP_MSG_DIRECT:
            zsys_debug ("ZCCP_MSG_DIRECT:");
            if (self->address)
                zsys_debug ("    address='%s'", self->address);
            else
                zsys_debug ("    address=");
            zsys_debug ("    headers=");
            if (self->headers) {
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    zsys_debug ("        %s=%s", zhash_cursor (self->headers), item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            else
                zsys_debug ("(NULL)");
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;
            
        case ZCCP_MSG_DELIVER:
            zsys_debug ("ZCCP_MSG_DELIVER:");
            if (self->sender)
                zsys_debug ("    sender='%s'", self->sender);
            else
                zsys_debug ("    sender=");
            if (self->address)
                zsys_debug ("    address='%s'", self->address);
            else
                zsys_debug ("    address=");
            zsys_debug ("    headers=");
            if (self->headers) {
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    zsys_debug ("        %s=%s", zhash_cursor (self->headers), item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            else
                zsys_debug ("(NULL)");
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;
            
        case ZCCP_MSG_GOODBYE:
            zsys_debug ("ZCCP_MSG_GOODBYE:");
            break;
            
        case ZCCP_MSG_GOODBYE_OK:
            zsys_debug ("ZCCP_MSG_GOODBYE_OK:");
            zsys_debug ("    headers=");
            if (self->headers) {
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    zsys_debug ("        %s=%s", zhash_cursor (self->headers), item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            else
                zsys_debug ("(NULL)");
            break;
            
        case ZCCP_MSG_PING:
            zsys_debug ("ZCCP_MSG_PING:");
            break;
            
        case ZCCP_MSG_PING_OK:
            zsys_debug ("ZCCP_MSG_PING_OK:");
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
        case ZCCP_MSG_HELLO_OK:
            return ("HELLO_OK");
            break;
        case ZCCP_MSG_SUBSCRIBE:
            return ("SUBSCRIBE");
            break;
        case ZCCP_MSG_PUBLISH:
            return ("PUBLISH");
            break;
        case ZCCP_MSG_DIRECT:
            return ("DIRECT");
            break;
        case ZCCP_MSG_DELIVER:
            return ("DELIVER");
            break;
        case ZCCP_MSG_GOODBYE:
            return ("GOODBYE");
            break;
        case ZCCP_MSG_GOODBYE_OK:
            return ("GOODBYE_OK");
            break;
        case ZCCP_MSG_PING:
            return ("PING");
            break;
        case ZCCP_MSG_PING_OK:
            return ("PING_OK");
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
//  Get the headers field without transferring ownership

zhash_t *
zccp_msg_headers (zccp_msg_t *self)
{
    assert (self);
    return self->headers;
}

//  Get the headers field and transfer ownership to caller

zhash_t *
zccp_msg_get_headers (zccp_msg_t *self)
{
    zhash_t *headers = self->headers;
    self->headers = NULL;
    return headers;
}

//  Set the headers field, transferring ownership from caller

void
zccp_msg_set_headers (zccp_msg_t *self, zhash_t **headers_p)
{
    assert (self);
    assert (headers_p);
    zhash_destroy (&self->headers);
    self->headers = *headers_p;
    *headers_p = NULL;
}

//  --------------------------------------------------------------------------
//  Get/set a value in the headers dictionary

const char *
zccp_msg_headers_string (zccp_msg_t *self, const char *key, const char *default_value)
{
    assert (self);
    const char *value = NULL;
    if (self->headers)
        value = (const char *) (zhash_lookup (self->headers, key));
    if (!value)
        value = default_value;

    return value;
}

uint64_t
zccp_msg_headers_number (zccp_msg_t *self, const char *key, uint64_t default_value)
{
    assert (self);
    uint64_t value = default_value;
    char *string = NULL;
    if (self->headers)
        string = (char *) (zhash_lookup (self->headers, key));
    if (string)
        value = atol (string);

    return value;
}

void
zccp_msg_headers_insert (zccp_msg_t *self, const char *key, const char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    //  Store string in hash table
    if (!self->headers) {
        self->headers = zhash_new ();
        zhash_autofree (self->headers);
    }
    zhash_update (self->headers, key, string);
    free (string);
}

size_t
zccp_msg_headers_size (zccp_msg_t *self)
{
    return zhash_size (self->headers);
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
//  Get/set the address field

const char *
zccp_msg_address (zccp_msg_t *self)
{
    assert (self);
    return self->address;
}

void
zccp_msg_set_address (zccp_msg_t *self, const char *format, ...)
{
    //  Format address from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->address);
    self->address = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get the content field without transferring ownership

zmsg_t *
zccp_msg_content (zccp_msg_t *self)
{
    assert (self);
    return self->content;
}

//  Get the content field and transfer ownership to caller

zmsg_t *
zccp_msg_get_content (zccp_msg_t *self)
{
    zmsg_t *content = self->content;
    self->content = NULL;
    return content;
}

//  Set the content field, transferring ownership from caller

void
zccp_msg_set_content (zccp_msg_t *self, zmsg_t **msg_p)
{
    assert (self);
    assert (msg_p);
    zmsg_destroy (&self->content);
    self->content = *msg_p;
    *msg_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get/set the sender field

const char *
zccp_msg_sender (zccp_msg_t *self)
{
    assert (self);
    return self->sender;
}

void
zccp_msg_set_sender (zccp_msg_t *self, const char *format, ...)
{
    //  Format sender from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->sender);
    self->sender = zsys_vprintf (format, argptr);
    va_end (argptr);
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
    zccp_msg_headers_insert (self, "Name", "Brutus");
    zccp_msg_headers_insert (self, "Age", "%d", 43);
    //  Send twice from same object
    zccp_msg_send_again (self, output);
    zccp_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zccp_msg_recv (input);
        assert (self);
        assert (zccp_msg_routing_id (self));
        
        assert (streq (zccp_msg_identifier (self), "Life is short but Now lasts for ever"));
        assert (zccp_msg_headers_size (self) == 2);
        assert (streq (zccp_msg_headers_string (self, "Name", "?"), "Brutus"));
        assert (zccp_msg_headers_number (self, "Age", 0) == 43);
        zccp_msg_destroy (&self);
    }
    self = zccp_msg_new (ZCCP_MSG_HELLO_OK);
    
    //  Check that _dup works on empty message
    copy = zccp_msg_dup (self);
    assert (copy);
    zccp_msg_destroy (&copy);

    zccp_msg_headers_insert (self, "Name", "Brutus");
    zccp_msg_headers_insert (self, "Age", "%d", 43);
    //  Send twice from same object
    zccp_msg_send_again (self, output);
    zccp_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zccp_msg_recv (input);
        assert (self);
        assert (zccp_msg_routing_id (self));
        
        assert (zccp_msg_headers_size (self) == 2);
        assert (streq (zccp_msg_headers_string (self, "Name", "?"), "Brutus"));
        assert (zccp_msg_headers_number (self, "Age", 0) == 43);
        zccp_msg_destroy (&self);
    }
    self = zccp_msg_new (ZCCP_MSG_SUBSCRIBE);
    
    //  Check that _dup works on empty message
    copy = zccp_msg_dup (self);
    assert (copy);
    zccp_msg_destroy (&copy);

    zccp_msg_set_expression (self, "Life is short but Now lasts for ever");
    zccp_msg_headers_insert (self, "Name", "Brutus");
    zccp_msg_headers_insert (self, "Age", "%d", 43);
    //  Send twice from same object
    zccp_msg_send_again (self, output);
    zccp_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zccp_msg_recv (input);
        assert (self);
        assert (zccp_msg_routing_id (self));
        
        assert (streq (zccp_msg_expression (self), "Life is short but Now lasts for ever"));
        assert (zccp_msg_headers_size (self) == 2);
        assert (streq (zccp_msg_headers_string (self, "Name", "?"), "Brutus"));
        assert (zccp_msg_headers_number (self, "Age", 0) == 43);
        zccp_msg_destroy (&self);
    }
    self = zccp_msg_new (ZCCP_MSG_PUBLISH);
    
    //  Check that _dup works on empty message
    copy = zccp_msg_dup (self);
    assert (copy);
    zccp_msg_destroy (&copy);

    zccp_msg_set_address (self, "Life is short but Now lasts for ever");
    zccp_msg_headers_insert (self, "Name", "Brutus");
    zccp_msg_headers_insert (self, "Age", "%d", 43);
    zmsg_t *publish_content = zmsg_new ();
    zccp_msg_set_content (self, &publish_content);
    zmsg_addstr (zccp_msg_content (self), "Hello, World");
    //  Send twice from same object
    zccp_msg_send_again (self, output);
    zccp_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zccp_msg_recv (input);
        assert (self);
        assert (zccp_msg_routing_id (self));
        
        assert (streq (zccp_msg_address (self), "Life is short but Now lasts for ever"));
        assert (zccp_msg_headers_size (self) == 2);
        assert (streq (zccp_msg_headers_string (self, "Name", "?"), "Brutus"));
        assert (zccp_msg_headers_number (self, "Age", 0) == 43);
        assert (zmsg_size (zccp_msg_content (self)) == 1);
        zccp_msg_destroy (&self);
    }
    self = zccp_msg_new (ZCCP_MSG_DIRECT);
    
    //  Check that _dup works on empty message
    copy = zccp_msg_dup (self);
    assert (copy);
    zccp_msg_destroy (&copy);

    zccp_msg_set_address (self, "Life is short but Now lasts for ever");
    zccp_msg_headers_insert (self, "Name", "Brutus");
    zccp_msg_headers_insert (self, "Age", "%d", 43);
    zmsg_t *direct_content = zmsg_new ();
    zccp_msg_set_content (self, &direct_content);
    zmsg_addstr (zccp_msg_content (self), "Hello, World");
    //  Send twice from same object
    zccp_msg_send_again (self, output);
    zccp_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zccp_msg_recv (input);
        assert (self);
        assert (zccp_msg_routing_id (self));
        
        assert (streq (zccp_msg_address (self), "Life is short but Now lasts for ever"));
        assert (zccp_msg_headers_size (self) == 2);
        assert (streq (zccp_msg_headers_string (self, "Name", "?"), "Brutus"));
        assert (zccp_msg_headers_number (self, "Age", 0) == 43);
        assert (zmsg_size (zccp_msg_content (self)) == 1);
        zccp_msg_destroy (&self);
    }
    self = zccp_msg_new (ZCCP_MSG_DELIVER);
    
    //  Check that _dup works on empty message
    copy = zccp_msg_dup (self);
    assert (copy);
    zccp_msg_destroy (&copy);

    zccp_msg_set_sender (self, "Life is short but Now lasts for ever");
    zccp_msg_set_address (self, "Life is short but Now lasts for ever");
    zccp_msg_headers_insert (self, "Name", "Brutus");
    zccp_msg_headers_insert (self, "Age", "%d", 43);
    zmsg_t *deliver_content = zmsg_new ();
    zccp_msg_set_content (self, &deliver_content);
    zmsg_addstr (zccp_msg_content (self), "Hello, World");
    //  Send twice from same object
    zccp_msg_send_again (self, output);
    zccp_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zccp_msg_recv (input);
        assert (self);
        assert (zccp_msg_routing_id (self));
        
        assert (streq (zccp_msg_sender (self), "Life is short but Now lasts for ever"));
        assert (streq (zccp_msg_address (self), "Life is short but Now lasts for ever"));
        assert (zccp_msg_headers_size (self) == 2);
        assert (streq (zccp_msg_headers_string (self, "Name", "?"), "Brutus"));
        assert (zccp_msg_headers_number (self, "Age", 0) == 43);
        assert (zmsg_size (zccp_msg_content (self)) == 1);
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
    self = zccp_msg_new (ZCCP_MSG_GOODBYE_OK);
    
    //  Check that _dup works on empty message
    copy = zccp_msg_dup (self);
    assert (copy);
    zccp_msg_destroy (&copy);

    zccp_msg_headers_insert (self, "Name", "Brutus");
    zccp_msg_headers_insert (self, "Age", "%d", 43);
    //  Send twice from same object
    zccp_msg_send_again (self, output);
    zccp_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zccp_msg_recv (input);
        assert (self);
        assert (zccp_msg_routing_id (self));
        
        assert (zccp_msg_headers_size (self) == 2);
        assert (streq (zccp_msg_headers_string (self, "Name", "?"), "Brutus"));
        assert (zccp_msg_headers_number (self, "Age", 0) == 43);
        zccp_msg_destroy (&self);
    }
    self = zccp_msg_new (ZCCP_MSG_PING);
    
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
    self = zccp_msg_new (ZCCP_MSG_PING_OK);
    
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
