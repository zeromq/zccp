/*  =========================================================================
    ZCCP daemon

    Accepts either one or two arguments:
    zccp_sh pattern         -- show all matching notifications
    zccp_sh address body    -- send one notification to this address

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of zbroker, the ZeroMQ broker project.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#include "zccp.h"

int main (void)
{
    zactor_t *server = zactor_new (zccp_server, "zccpd");
    zsock_send (server, "s", "VERBOSE");
    zsock_send (server, "ss", "BIND", "ipc://@/zccp");
    zsock_wait (server);
    zactor_destroy (&server);
    return 0;
}
