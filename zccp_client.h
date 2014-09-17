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
//  Constructor
CZMQ_EXPORT zccp_client_t *
    zccp_client_new (const char *server);

//  Destructor
CZMQ_EXPORT void
    zccp_client_destroy (zccp_client_t **self_p);

//  Returns last error number, if any
CZMQ_EXPORT int
    zccp_client_error (zccp_client_t *self);

// Self test of this class
CZMQ_EXPORT void
    zccp_client_test (bool verbose);
// @end

#ifdef __cplusplus
}
#endif

#endif
