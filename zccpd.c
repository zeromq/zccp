#include "zccp.h"

int main (void)
{
    zactor_t *server = zactor_new (zccp_server, "zccpd");
    zsock_send (server, "s", "VERBOSE");
    zsock_send (server, "ss", "BIND", "tcp://*:50013");
    zsock_wait (server);
    zactor_destroy (&server);
    return 0;
}
