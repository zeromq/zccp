//  ZCCP shell API
//  Accepts either one or two arguments:
//  zccp_sh type      -- show all notifications of this type
//  zccp_sh type body -- send one notification of this type

#include "zccp.h"

int main (int argc, char *argv [])
{
    if (argc < 2) {
        printf ("syntax: zccp_sh type [ body ]\n");
        return 0;
    }
    zccp_client_t *client = zccp_client_new ("zccp_sh", "ipc://@/zccp");
    if (!client) {
        printf ("zccp_sh: server not running at ipc://@/zccp\n");
        return 0;
    }
    if (argc == 2) {
        zccp_client_subscribe (client, argv [1]);
        while (true) {
            char *type, *body;
            if (zccp_client_recv (client, &type, &body))
                break;
            printf ("%s: %s\n", type, body);
        }
    }
    else
        zccp_client_send (client, argv [1], argv [2]);
    
    zccp_client_destroy (&client);
    return 0;
}
