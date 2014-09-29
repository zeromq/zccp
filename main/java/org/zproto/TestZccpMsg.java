package org.zproto;

import static org.junit.Assert.*;
import org.junit.Test;
import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Socket;
import org.zeromq.ZFrame;
import org.zeromq.ZContext;

public class TestZccpMsg
{
    @Test
    public void testZccpMsg ()
    {
        System.out.printf (" * zccp_msg: ");

        //  Simple create/destroy test
        ZccpMsg self = new ZccpMsg (0);
        assert (self != null);
        self.destroy ();

        //  Create pair of sockets we can send through
        ZContext ctx = new ZContext ();
        assert (ctx != null);

        Socket output = ctx.createSocket (ZMQ.DEALER);
        assert (output != null);
        output.bind ("inproc://selftest");
        Socket input = ctx.createSocket (ZMQ.ROUTER);
        assert (input != null);
        input.connect ("inproc://selftest");

        //  Encode/send/decode and verify each message type

        self = new ZccpMsg (ZccpMsg.HELLO);
        self.setIdentifier ("Life is short but Now lasts for ever");
        self.insertHeaders ("Name", "Brutus");
        self.insertHeaders ("Age", "%d", 43);
        self.send (output);

        self = ZccpMsg.recv (input);
        assert (self != null);
        assertEquals (self.identifier (), "Life is short but Now lasts for ever");
        assertEquals (self.headers ().size (), 2);
        assertEquals (self.headersString ("Name", "?"), "Brutus");
        assertEquals (self.headersNumber ("Age", 0), 43);
        self.destroy ();

        self = new ZccpMsg (ZccpMsg.HELLO_OK);
        self.insertHeaders ("Name", "Brutus");
        self.insertHeaders ("Age", "%d", 43);
        self.send (output);

        self = ZccpMsg.recv (input);
        assert (self != null);
        assertEquals (self.headers ().size (), 2);
        assertEquals (self.headersString ("Name", "?"), "Brutus");
        assertEquals (self.headersNumber ("Age", 0), 43);
        self.destroy ();

        self = new ZccpMsg (ZccpMsg.SUBSCRIBE);
        self.setExpression ("Life is short but Now lasts for ever");
        self.insertHeaders ("Name", "Brutus");
        self.insertHeaders ("Age", "%d", 43);
        self.send (output);

        self = ZccpMsg.recv (input);
        assert (self != null);
        assertEquals (self.expression (), "Life is short but Now lasts for ever");
        assertEquals (self.headers ().size (), 2);
        assertEquals (self.headersString ("Name", "?"), "Brutus");
        assertEquals (self.headersNumber ("Age", 0), 43);
        self.destroy ();

        self = new ZccpMsg (ZccpMsg.SUBSCRIBE_OK);
        self.send (output);

        self = ZccpMsg.recv (input);
        assert (self != null);
        self.destroy ();

        self = new ZccpMsg (ZccpMsg.PUBLISH);
        self.setAddress ("Life is short but Now lasts for ever");
        self.insertHeaders ("Name", "Brutus");
        self.insertHeaders ("Age", "%d", 43);
        self.send (output);

        self = ZccpMsg.recv (input);
        assert (self != null);
        assertEquals (self.address (), "Life is short but Now lasts for ever");
        assertEquals (self.headers ().size (), 2);
        assertEquals (self.headersString ("Name", "?"), "Brutus");
        assertEquals (self.headersNumber ("Age", 0), 43);
        self.destroy ();

        self = new ZccpMsg (ZccpMsg.DIRECT);
        self.setAddress ("Life is short but Now lasts for ever");
        self.insertHeaders ("Name", "Brutus");
        self.insertHeaders ("Age", "%d", 43);
        self.send (output);

        self = ZccpMsg.recv (input);
        assert (self != null);
        assertEquals (self.address (), "Life is short but Now lasts for ever");
        assertEquals (self.headers ().size (), 2);
        assertEquals (self.headersString ("Name", "?"), "Brutus");
        assertEquals (self.headersNumber ("Age", 0), 43);
        self.destroy ();

        self = new ZccpMsg (ZccpMsg.DELIVER);
        self.setSender ("Life is short but Now lasts for ever");
        self.setAddress ("Life is short but Now lasts for ever");
        self.insertHeaders ("Name", "Brutus");
        self.insertHeaders ("Age", "%d", 43);
        self.send (output);

        self = ZccpMsg.recv (input);
        assert (self != null);
        assertEquals (self.sender (), "Life is short but Now lasts for ever");
        assertEquals (self.address (), "Life is short but Now lasts for ever");
        assertEquals (self.headers ().size (), 2);
        assertEquals (self.headersString ("Name", "?"), "Brutus");
        assertEquals (self.headersNumber ("Age", 0), 43);
        self.destroy ();

        self = new ZccpMsg (ZccpMsg.GOODBYE);
        self.send (output);

        self = ZccpMsg.recv (input);
        assert (self != null);
        self.destroy ();

        self = new ZccpMsg (ZccpMsg.GOODBYE_OK);
        self.insertHeaders ("Name", "Brutus");
        self.insertHeaders ("Age", "%d", 43);
        self.send (output);

        self = ZccpMsg.recv (input);
        assert (self != null);
        assertEquals (self.headers ().size (), 2);
        assertEquals (self.headersString ("Name", "?"), "Brutus");
        assertEquals (self.headersNumber ("Age", 0), 43);
        self.destroy ();

        self = new ZccpMsg (ZccpMsg.PING);
        self.send (output);

        self = ZccpMsg.recv (input);
        assert (self != null);
        self.destroy ();

        self = new ZccpMsg (ZccpMsg.PING_OK);
        self.send (output);

        self = ZccpMsg.recv (input);
        assert (self != null);
        self.destroy ();

        self = new ZccpMsg (ZccpMsg.INVALID);
        self.send (output);

        self = ZccpMsg.recv (input);
        assert (self != null);
        self.destroy ();

        ctx.destroy ();
        System.out.printf ("OK\n");
    }
}
