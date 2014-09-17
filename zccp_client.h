/*  =========================================================================
    zccp_client.h - simple API for zccp client applications

    =========================================================================
*/

#ifndef __ZCCP_CLIENT_H_INCLUDED__
#define __ZCCP_CLIENT_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zccp_client_t zccp_client_t;

// @interface
//  Constructor: creates new connection to server, sending HELLO and
//  expecting READY back. The client identifier is a string used for
//  REQUEST/REPLY exchanges.
zccp_client_t *
    zccp_client_new (const char *server, const char *identifier);

//  Destructor
void
    zccp_client_destroy (zccp_client_t **self_p);

//  Subscribe to some set of notifications
void
    zccp_client_subscribe (zccp_client_t *self, const char *subscription);

//  Send a notification message of some type
void
    zccp_client_send (zccp_client_t *self, const char *type, const char *message);

//  Wait for and receive next notification message; returns 0 if OK,
//  else returns -1.
int
    zccp_client_recv (zccp_client_t *self, char **type, char **body);

// Self test of this class
void
    zccp_client_test (bool verbose);
// @end

#ifdef __cplusplus
}
#endif

#endif
