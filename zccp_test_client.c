#include "zccp.h"

int main (void)
{
    zccp_client_t *client = zccp_client_new ();
    assert (client);
    zccp_client_destroy (&client);
    return 0;
}