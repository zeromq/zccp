#include "zccp.h"

int main (void)
{
    zactor_t *server = zactor_new (zccp_server, "zccpd");
    zsock_send (server, "s", "VERBOSE");
    zsock_send (server, "sss", "SET", "server/timeout", "2000");
    zsock_send (server, "ss", "BIND", "ipc://@/zccp");
    zsock_wait (server);
    zactor_destroy (&server);
    return 0;
}
