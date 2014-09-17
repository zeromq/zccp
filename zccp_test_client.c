#include "zccp.h"

int main (void)
{
    zccp_client_t *client = zccp_client_new ("Pieter", "tcp://192.168.1.135:50013");
    assert (client);
    zccp_client_destroy (&client);
    return 0;
}