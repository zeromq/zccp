//  ZCCP shell API
//  Accepts either one or two arguments:
//  zccp_sh pattern         -- show all matching notifications
//  zccp_sh address body    -- send one notification to this address

#include "zccp.h"

int main (int argc, char *argv [])
{
    printf ("ARGS:%d\n", argc);
    if (argc < 2) {
        printf ("syntax: zccp_sh type [ body ]\n");
        return 0;
    }
    zccp_client_t *client = zccp_client_new ();
    zccp_client_verbose (client);
    if (zccp_client_connect (client, "ipc://@/zccp", 1000)) {
        zsys_error ("zccp_sh: server not reachable at ipc://@/zccp");
        zccp_client_destroy (&client);
        return 0;
    }
    if (argc == 2) {
        //  Subscribe to the event addresses specified by the pattern
        zccp_client_subscribe (client, argv [1]);
        while (true) {
            //  Now receive and print any messages we get
            char *content = zccp_client_recv (client);
            if (!content)
                break;          //  Interrupted
            printf ("Content=%s sender=%s address=%s\n",
                content, zccp_client_sender (client), zccp_client_address (client));
        }
    }
    else
    if (argc == 3)
        zccp_client_publish (client, argv [1], argv [2]);

    zccp_client_destroy (&client);
    return 0;
}
