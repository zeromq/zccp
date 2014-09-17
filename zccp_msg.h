/*  =========================================================================
    zccp_msg - ZeroMQ Command & Control Protocol
    
    Codec header for zccp_msg.

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

#ifndef __ZCCP_MSG_H_INCLUDED__
#define __ZCCP_MSG_H_INCLUDED__

/*  These are the zccp_msg messages:

    HELLO - Client says hello to server

    READY - Server accepts client

    SUBSCRIBE - Client subscribes to some set of events
        header              string      Header, for matching

    PUBLISH - Client publishes an event, or server delivers to client
        header              string      Header, for matching
        content             chunk       Event content

    REQUEST - Request some action
        method              string      Requested method
        content             chunk       Event content

    REPLY - Reply to a command request
        status              number 2    Success/failure status
        content             chunk       Event content

    INVALID - Client sent a message that was not valid at this time
*/


#define ZCCP_MSG_HELLO                      1
#define ZCCP_MSG_READY                      2
#define ZCCP_MSG_SUBSCRIBE                  3
#define ZCCP_MSG_PUBLISH                    4
#define ZCCP_MSG_REQUEST                    5
#define ZCCP_MSG_REPLY                      6
#define ZCCP_MSG_INVALID                    7

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
);

//  Encode the READY 
zmsg_t *
    zccp_msg_encode_ready (
);

//  Encode the SUBSCRIBE 
zmsg_t *
    zccp_msg_encode_subscribe (
        const char *header);

//  Encode the PUBLISH 
zmsg_t *
    zccp_msg_encode_publish (
        const char *header,
        zchunk_t *content);

//  Encode the REQUEST 
zmsg_t *
    zccp_msg_encode_request (
        const char *method,
        zchunk_t *content);

//  Encode the REPLY 
zmsg_t *
    zccp_msg_encode_reply (
        uint16_t status,
        zchunk_t *content);

//  Encode the INVALID 
zmsg_t *
    zccp_msg_encode_invalid (
);


//  Send the HELLO to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_hello (void *output);
    
//  Send the READY to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_ready (void *output);
    
//  Send the SUBSCRIBE to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_subscribe (void *output,
        const char *header);
    
//  Send the PUBLISH to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_publish (void *output,
        const char *header,
        zchunk_t *content);
    
//  Send the REQUEST to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_request (void *output,
        const char *method,
        zchunk_t *content);
    
//  Send the REPLY to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    zccp_msg_send_reply (void *output,
        uint16_t status,
        zchunk_t *content);
    
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

//  Get/set the header field
const char *
    zccp_msg_header (zccp_msg_t *self);
void
    zccp_msg_set_header (zccp_msg_t *self, const char *format, ...);

//  Get a copy of the content field
zchunk_t *
    zccp_msg_content (zccp_msg_t *self);
//  Get the content field and transfer ownership to caller
zchunk_t *
    zccp_msg_get_content (zccp_msg_t *self);
//  Set the content field, transferring ownership from caller
void
    zccp_msg_set_content (zccp_msg_t *self, zchunk_t **chunk_p);

//  Get/set the method field
const char *
    zccp_msg_method (zccp_msg_t *self);
void
    zccp_msg_set_method (zccp_msg_t *self, const char *format, ...);

//  Get/set the status field
uint16_t
    zccp_msg_status (zccp_msg_t *self);
void
    zccp_msg_set_status (zccp_msg_t *self, uint16_t status);

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
