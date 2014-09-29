/*  =========================================================================
    zccp_msg - ZeroMQ Command & Control Protocol
    
    Codec header for zccp_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: zccp_msg.xml, or
     * The code generation script that built this file: zproto_codec_c
    ************************************************************************
    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of zbroker, the ZeroMQ broker project.           
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

#ifndef __ZCCP_MSG_H_INCLUDED__
#define __ZCCP_MSG_H_INCLUDED__

/*  These are the zccp_msg messages:

    HELLO - Client greets the server and provides its identifier.
        identifier          string      Client identifier
        headers             dictionary  Client properties

    HELLO_OK - Server confirms client
        headers             dictionary  Server properties

    SUBSCRIBE - Client subscribes to some set of messages
        expression          string      Regular expression
        headers             dictionary  Subscription options

    SUBSCRIBE_OK - Server confirms subscription.

    PUBLISH - Client publishes a message to the server
        address             string      Logical address
        headers             dictionary  Content header fields
        content             msg         Content, as multipart message

    DIRECT - Client sends a message to a specific client
        address             string      Client identifier
        headers             dictionary  Content header fields
        content             msg         Content, as multipart message

    DELIVER - Server delivers a message to client
        sender              string      Originating client
        address             string      Message address
        headers             dictionary  Content header fields
        content             msg         Content, as multipart message

    GOODBYE - Client says goodbye to server

    GOODBYE_OK - Server confirms client signoff
        headers             dictionary  Session statistics

    PING - Server pings the client if there's no traffic

    PING_OK - Client replies to a PING

    INVALID - Client sent a message that was not valid at this time
*/


#define ZCCP_MSG_HELLO                      1
#define ZCCP_MSG_HELLO_OK                   2
#define ZCCP_MSG_SUBSCRIBE                  3
#define ZCCP_MSG_SUBSCRIBE_OK               4
#define ZCCP_MSG_PUBLISH                    5
#define ZCCP_MSG_DIRECT                     6
#define ZCCP_MSG_DELIVER                    7
#define ZCCP_MSG_GOODBYE                    8
#define ZCCP_MSG_GOODBYE_OK                 9
#define ZCCP_MSG_PING                       10
#define ZCCP_MSG_PING_OK                    11
#define ZCCP_MSG_INVALID                    12

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zccp_msg_t zccp_msg_t;

//  @interface
//  Create a new zccp_msg
zccp_msg_t *
    zccp_msg_new (int id);

//  Destroy the zccp_msg
void
    zccp_msg_destroy (zccp_msg_t **self_p);

//  Parse a zccp_msg from zmsg_t. Returns a new object, or NULL if
//  the message could not be parsed, or was NULL. Destroys msg and 
//  nullifies the msg reference.
zccp_msg_t *
    zccp_msg_decode (zmsg_t **msg_p);

//  Encode zccp_msg into zmsg and destroy it. Returns a newly created
//  object or NULL if error. Use when not in control of sending the message.
zmsg_t *
    zccp_msg_encode (zccp_msg_t **self_p);

//  Receive and parse a zccp_msg from the socket. Returns new object, 
//  or NULL if error. Will block if there's no message waiting.
zccp_msg_t *
    zccp_msg_recv (void *input);

//  Receive and parse a zccp_msg from the socket. Returns new object, 
//  or NULL either if there was no input waiting, or the recv was interrupted.
zccp_msg_t *
    zccp_msg_recv_nowait (void *input);

//  Send the zccp_msg to the output, and destroy it
int
    zccp_msg_send (zccp_msg_t **self_p, void *output);

//  Send the zccp_msg to the output, and do not destroy it
int
    zccp_msg_send_again (zccp_msg_t *self, void *output);

//  Encode the HELLO 
zmsg_t *
    zccp_msg_encode_hello (
        const char *identifier,
        zhash_t *headers);

//  Encode the HELLO_OK 
zmsg_t *
    zccp_msg_encode_hello_ok (
        zhash_t *headers);

//  Encode the SUBSCRIBE 
zmsg_t *
    zccp_msg_encode_subscribe (
        const char *expression,
        zhash_t *headers);

//  Encode the SUBSCRIBE_OK 
zmsg_t *
    zccp_msg_encode_subscribe_ok (
);

//  Encode the PUBLISH 
zmsg_t *
    zccp_msg_encode_publish (
        const char *address,
        zhash_t *headers,
        zmsg_t *content);

//  Encode the DIRECT 
zmsg_t *
    zccp_msg_encode_direct (
        const char *address,
        zhash_t *headers,
        zmsg_t *content);

//  Encode the DELIVER 
zmsg_t *
    zccp_msg_encode_deliver (
        const char *sender,
        const char *address,
        zhash_t *headers,
        zmsg_t *content);

//  Encode the GOODBYE 
zmsg_t *
    zccp_msg_encode_goodbye (
);

//  Encode the GOODBYE_OK 
zmsg_t *
    zccp_msg_encode_goodbye_ok (
        zhash_t *headers);

//  Encode the PING 
zmsg_t *
    zccp_msg_encode_ping (
);

//  Encode the PING_OK 
zmsg_t *
    zccp_msg_encode_ping_ok (
);

//  Encode the INVALID 
zmsg_t *
    zccp_msg_encode_invalid (
);


//  Send the HELLO to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_hello (void *output,
        const char *identifier,
        zhash_t *headers);
    
//  Send the HELLO_OK to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_hello_ok (void *output,
        zhash_t *headers);
    
//  Send the SUBSCRIBE to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_subscribe (void *output,
        const char *expression,
        zhash_t *headers);
    
//  Send the SUBSCRIBE_OK to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_subscribe_ok (void *output);
    
//  Send the PUBLISH to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_publish (void *output,
        const char *address,
        zhash_t *headers,
        zmsg_t *content);
    
//  Send the DIRECT to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_direct (void *output,
        const char *address,
        zhash_t *headers,
        zmsg_t *content);
    
//  Send the DELIVER to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_deliver (void *output,
        const char *sender,
        const char *address,
        zhash_t *headers,
        zmsg_t *content);
    
//  Send the GOODBYE to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_goodbye (void *output);
    
//  Send the GOODBYE_OK to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_goodbye_ok (void *output,
        zhash_t *headers);
    
//  Send the PING to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_ping (void *output);
    
//  Send the PING_OK to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_ping_ok (void *output);
    
//  Send the INVALID to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_invalid (void *output);
    
//  Duplicate the zccp_msg message
zccp_msg_t *
    zccp_msg_dup (zccp_msg_t *self);

//  Print contents of message to stdout
void
    zccp_msg_print (zccp_msg_t *self);

//  Get/set the message routing id
zframe_t *
    zccp_msg_routing_id (zccp_msg_t *self);
void
    zccp_msg_set_routing_id (zccp_msg_t *self, zframe_t *routing_id);

//  Get the zccp_msg id and printable command
int
    zccp_msg_id (zccp_msg_t *self);
void
    zccp_msg_set_id (zccp_msg_t *self, int id);
const char *
    zccp_msg_command (zccp_msg_t *self);

//  Get/set the identifier field
const char *
    zccp_msg_identifier (zccp_msg_t *self);
void
    zccp_msg_set_identifier (zccp_msg_t *self, const char *format, ...);

//  Get/set the headers field
zhash_t *
    zccp_msg_headers (zccp_msg_t *self);
//  Get the headers field and transfer ownership to caller
zhash_t *
    zccp_msg_get_headers (zccp_msg_t *self);
//  Set the headers field, transferring ownership from caller
void
    zccp_msg_set_headers (zccp_msg_t *self, zhash_t **headers_p);
    
//  Get/set a value in the headers dictionary
const char *
    zccp_msg_headers_string (zccp_msg_t *self,
        const char *key, const char *default_value);
uint64_t
    zccp_msg_headers_number (zccp_msg_t *self,
        const char *key, uint64_t default_value);
void
    zccp_msg_headers_insert (zccp_msg_t *self,
        const char *key, const char *format, ...);
size_t
    zccp_msg_headers_size (zccp_msg_t *self);

//  Get/set the expression field
const char *
    zccp_msg_expression (zccp_msg_t *self);
void
    zccp_msg_set_expression (zccp_msg_t *self, const char *format, ...);

//  Get/set the address field
const char *
    zccp_msg_address (zccp_msg_t *self);
void
    zccp_msg_set_address (zccp_msg_t *self, const char *format, ...);

//  Get a copy of the content field
zmsg_t *
    zccp_msg_content (zccp_msg_t *self);
//  Get the content field and transfer ownership to caller
zmsg_t *
    zccp_msg_get_content (zccp_msg_t *self);
//  Set the content field, transferring ownership from caller
void
    zccp_msg_set_content (zccp_msg_t *self, zmsg_t **msg_p);

//  Get/set the sender field
const char *
    zccp_msg_sender (zccp_msg_t *self);
void
    zccp_msg_set_sender (zccp_msg_t *self, const char *format, ...);

//  Self test of this class
int
    zccp_msg_test (bool verbose);
//  @end

//  For backwards compatibility with old codecs
#define zccp_msg_dump       zccp_msg_print

#ifdef __cplusplus
}
#endif

#endif
