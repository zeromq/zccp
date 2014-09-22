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
    zccp_client_t *client = zccp_client_new ("zccp_sh", "ipc://@/zccp", NULL);
    if (!client) {
        printf ("zccp_sh: server not running at ipc://@/zccp\n");
        return 0;
    }
    if (argc == 2) {
        zccp_client_subscribe (client, argv [1], NULL);
        while (true) {
            char *type, *body;
            char *sender, *address;
            zhash_t *headers;
            zmsg_t *content;
            if (zccp_client_recv (client, &sender, &address, &headers, &content)) {
                break;
            }
            printf ("%s: %s\n", sender, address);
        }
    }
    else
        zccp_client_send (client, argv [1], NULL, argv [2]);
    
    zccp_client_destroy (&client);
    return 0;
}
