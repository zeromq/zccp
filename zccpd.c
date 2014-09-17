#include "zccp.h"

int main (void)
{
    zactor_t *server = zactor_new (zccp_server, "zccpd");
    zsock_send (server, "s", "VERBOSE");
    zsock_send (server, "sss", "SET", "server/timeout", "2000");
    zsock_send (server, "ss", "BIND", "tcp://*:50013");
    zsys_info ("binding to tcp://192.168.1.135:50013");
    zsock_wait (server);
    zactor_destroy (&server);
    return 0;
}
