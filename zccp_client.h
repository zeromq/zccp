/*  =========================================================================
    zccp_client - ZCCP Client

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

#ifndef __ZCCP_CLIENT_H_INCLUDED__
#define __ZCCP_CLIENT_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zccp_client_t zccp_client_t;

//  @interface
//  Create a new zccp_client
zccp_client_t *
    zccp_client_new (void);

//  Destroy the zccp_client
void
    zccp_client_destroy (zccp_client_t **self_p);

//  Enable verbose logging of client activity
void
    zccp_client_verbose (zccp_client_t *self);

//  Return actor for low-level command control and polling
zactor_t *
    zccp_client_actor (zccp_client_t *self);

//  Connect to server and return only when there's a successful connection or the   
//  timeout in msecs expires. Returns 0 if successfully connected, else -1.         
//  Returns >= 0 if successful, -1 if interrupted.
int
    zccp_client_connect (zccp_client_t *self, const char *endpoint, int timeout);

//  Subscribe to all messages sent to matching addresses. The expression is a       
//  regular expression using the CZMQ zrex syntax. The most useful elements are: ^  
//  and $ to match the start and end, . to match any character, \s and \S to match  
//  whitespace and non-whitespace, \d and \D to match a digit and non-digit, \a and 
//  \A to match alphabetic and non-alphabetic, \w and \W to match alphanumeric and  
//  non-alphanumeric, + for one or more repetitions, * for zero or more repetitions,
//  and ( ) to create groups. Returns 0 if subscription was successful, else -1.    
//  Returns >= 0 if successful, -1 if interrupted.
int
    zccp_client_subscribe (zccp_client_t *self, const char *expression);

//  Publish a message on the server, using a logical address. All subscribers to    
//  that address will receive a copy of the message. The server does not store      
//  messages. If a message is published before subscribers arrive, they will miss   
//  it. Currently only supports string contents. Does not return a status value;    
//  publish commands are asynchronous and unconfirmed.                              
int
    zccp_client_publish (zccp_client_t *self, const char *address, const char *content);

//  Receive next message from server. Returns the message content, as a string, if  
//  any. The caller should not modify or free this string.                          
//  Returns NULL on an interrupt.
char *
    zccp_client_recv (zccp_client_t *self);

//  Return last received status
int
    zccp_client_status (zccp_client_t *self);

//  Return last received reason
char *
    zccp_client_reason (zccp_client_t *self);

//  Return last received sender
char *
    zccp_client_sender (zccp_client_t *self);

//  Return last received address
char *
    zccp_client_address (zccp_client_t *self);

//  Return last received content
char *
    zccp_client_content (zccp_client_t *self);

//  Self test of this class
void
    zccp_client_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
