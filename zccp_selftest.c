/*  =========================================================================
    ZCCP selftest

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
    zccp_server_test (true);
    zccp_client_test (true);
    return 0;
}
