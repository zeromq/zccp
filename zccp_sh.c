//  ZCCP shell API

#include "zccp.h"

int main (int argc, char *argv [])
{
    if (argc < 2) {
        printf ("syntax: zccp_sh type body\n");
        exit;
    }
    zccp_client_t *client = zccp_client_new ("zccp_sh", "ipc://@/zccp");
    if (client) {
        zccp_client_send (client, argv [1], argv [2]);
        zccp_client_destroy (&client);
    }
    else
        printf ("zccp_sh: server not running at ipc://@/zccp\n");

    return 0;
}
