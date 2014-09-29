/*  =========================================================================
    ZccpMsg - ZeroMQ Command & Control Protocol

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

    * The XML model used for this code generation: zccp_msg.xml
    * The code generation script that built this file: zproto_codec_c
    ************************************************************************
    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of zbroker, the ZeroMQ broker project.           
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

/*  These are the ZccpMsg messages:

    HELLO - Client greets the server and provides its identifier.
        identifier          string      Client identifier
        headers             dictionary  Client properties

    HELLO_OK - Server confirms client
        headers             dictionary  Server properties

    SUBSCRIBE - Client subscribes to some set of messages
        expression          string      Regular expression
        headers             dictionary  Subscription options

    SUBSCRIBE_OK - Server confirms subscription.

    PUBLISH - Client publishes a message to the server
        address             string      Logical address
        headers             dictionary  Content header fields
        content             msg         Content, as multipart message

    DIRECT - Client sends a message to a specific client
        address             string      Client identifier
        headers             dictionary  Content header fields
        content             msg         Content, as multipart message

    DELIVER - Server delivers a message to client
        sender              string      Originating client
        address             string      Message address
        headers             dictionary  Content header fields
        content             msg         Content, as multipart message

    GOODBYE - Client says goodbye to server

    GOODBYE_OK - Server confirms client signoff
        headers             dictionary  Session statistics

    PING - Server pings the client if there's no traffic

    PING_OK - Client replies to a PING

    INVALID - Client sent a message that was not valid at this time
*/

package org.zproto;

import java.util.*;
import java.nio.ByteBuffer;

import org.zeromq.ZFrame;
import org.zeromq.ZMsg;
import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Socket;

public class ZccpMsg implements java.io.Closeable
{

    public static final int HELLO                 = 1;
    public static final int HELLO_OK              = 2;
    public static final int SUBSCRIBE             = 3;
    public static final int SUBSCRIBE_OK          = 4;
    public static final int PUBLISH               = 5;
    public static final int DIRECT                = 6;
    public static final int DELIVER               = 7;
    public static final int GOODBYE               = 8;
    public static final int GOODBYE_OK            = 9;
    public static final int PING                  = 10;
    public static final int PING_OK               = 11;
    public static final int INVALID               = 12;

    //  Structure of our class
    private ZFrame routingId;           // Routing_id from ROUTER, if any
    private int id;                     //  ZccpMsg message ID
    private ByteBuffer needle;          //  Read/write pointer for serialization

    private String identifier;
    private Map <String, String> headers;
    private int headersBytes;
    private String expression;
    private String address;
    private ZMsg content;
    private String sender;

    public ZccpMsg( int id )
    {
        this.id = id;
    }

    public void destroy()
    {
        close();
    }

    @Override
    public void close()
    {
        //  Destroy frame fields
    }
    //  --------------------------------------------------------------------------
    //  Network data encoding macros


    //  Put a 1-byte number to the frame
    private final void putNumber1 (int value)
    {
        needle.put ((byte) value);
    }

    //  Get a 1-byte number to the frame
    //  then make it unsigned
    private int getNumber1 ()
    {
        int value = needle.get ();
        if (value < 0)
            value = (0xff) & value;
        return value;
    }

    //  Put a 2-byte number to the frame
    private final void putNumber2 (int value)
    {
        needle.putShort ((short) value);
    }

    //  Get a 2-byte number to the frame
    private int getNumber2 ()
    {
        int value = needle.getShort ();
        if (value < 0)
            value = (0xffff) & value;
        return value;
    }

    //  Put a 4-byte number to the frame
    private final void putNumber4 (long value)
    {
        needle.putInt ((int) value);
    }

    //  Get a 4-byte number to the frame
    //  then make it unsigned
    private long getNumber4 ()
    {
        long value = needle.getInt ();
        if (value < 0)
            value = (0xffffffff) & value;
        return value;
    }

    //  Put a 8-byte number to the frame
    public void putNumber8 (long value)
    {
        needle.putLong (value);
    }

    //  Get a 8-byte number to the frame
    public long getNumber8 ()
    {
        return needle.getLong ();
    }


    //  Put a block to the frame
    private void putBlock (byte [] value, int size)
    {
        needle.put (value, 0, size);
    }

    private byte [] getBlock (int size)
    {
        byte [] value = new byte [size];
        needle.get (value);

        return value;
    }

    //  Put a string to the frame
    public void putString (String value)
    {
        needle.put ((byte) value.length ());
        needle.put (value.getBytes());
    }

    //  Get a string from the frame
    public String getString ()
    {
        int size = getNumber1 ();
        byte [] value = new byte [size];
        needle.get (value);

        return new String (value);
    }

        //  Put a string to the frame
    public void putLongString (String value)
    {
        needle.putInt (value.length ());
        needle.put (value.getBytes());
    }

    //  Get a string from the frame
    public String getLongString ()
    {
        long size = getNumber4 ();
        byte [] value = new byte [(int) size];
        needle.get (value);

        return new String (value);
    }
    //  --------------------------------------------------------------------------
    //  Receive and parse a ZccpMsg from the socket. Returns new object or
    //  null if error. Will block if there's no message waiting.

    public static ZccpMsg recv (Socket input)
    {
        assert (input != null);
        ZccpMsg self = new ZccpMsg (0);
        ZFrame frame = null;

        try {
            //  Read valid message frame from socket; we loop over any
            //  garbage data we might receive from badly-connected peers
            while (true) {
                //  If we're reading from a ROUTER socket, get routingId
                if (input.getType () == ZMQ.ROUTER) {
                    self.routingId = ZFrame.recvFrame (input);
                    if (self.routingId == null)
                        return null;         //  Interrupted
                    if (!input.hasReceiveMore ())
                        throw new IllegalArgumentException ();
                }
                //  Read and parse command in frame
                frame = ZFrame.recvFrame (input);
                if (frame == null)
                    return null;             //  Interrupted

                //  Get and check protocol signature
                self.needle = ByteBuffer.wrap (frame.getData ());
                int signature = self.getNumber2 ();
                if (signature == (0xAAA0 | 10))
                    break;                  //  Valid signature

                //  Protocol assertion, drop message
                while (input.hasReceiveMore ()) {
                    frame.destroy ();
                    frame = ZFrame.recvFrame (input);
                }
                frame.destroy ();
            }

            //  Get message id, which is first byte in frame
            self.id = self.getNumber1 ();
            int listSize;
            int hashSize;

            switch (self.id) {
            case HELLO:
                self.identifier = self.getString ();
                hashSize = (int) self.getNumber4 ();
                self.headers = new HashMap <String, String> ();
                while (hashSize-- > 0) {
                    String key = self.getString ();
                    String value = self.getLongString ();

                    self.headers.put(key, value);
                }
                break;

            case HELLO_OK:
                hashSize = (int) self.getNumber4 ();
                self.headers = new HashMap <String, String> ();
                while (hashSize-- > 0) {
                    String key = self.getString ();
                    String value = self.getLongString ();

                    self.headers.put(key, value);
                }
                break;

            case SUBSCRIBE:
                self.expression = self.getString ();
                hashSize = (int) self.getNumber4 ();
                self.headers = new HashMap <String, String> ();
                while (hashSize-- > 0) {
                    String key = self.getString ();
                    String value = self.getLongString ();

                    self.headers.put(key, value);
                }
                break;

            case SUBSCRIBE_OK:
                break;

            case PUBLISH:
                self.address = self.getString ();
                hashSize = (int) self.getNumber4 ();
                self.headers = new HashMap <String, String> ();
                while (hashSize-- > 0) {
                    String key = self.getString ();
                    String value = self.getLongString ();

                    self.headers.put(key, value);
                }
                self.content = new ZMsg();
                if (input.hasReceiveMore ())
                    self.content.add(ZFrame.recvFrame (input));
                break;

            case DIRECT:
                self.address = self.getString ();
                hashSize = (int) self.getNumber4 ();
                self.headers = new HashMap <String, String> ();
                while (hashSize-- > 0) {
                    String key = self.getString ();
                    String value = self.getLongString ();

                    self.headers.put(key, value);
                }
                self.content = new ZMsg();
                if (input.hasReceiveMore ())
                    self.content.add(ZFrame.recvFrame (input));
                break;

            case DELIVER:
                self.sender = self.getString ();
                self.address = self.getString ();
                hashSize = (int) self.getNumber4 ();
                self.headers = new HashMap <String, String> ();
                while (hashSize-- > 0) {
                    String key = self.getString ();
                    String value = self.getLongString ();

                    self.headers.put(key, value);
                }
                self.content = new ZMsg();
                if (input.hasReceiveMore ())
                    self.content.add(ZFrame.recvFrame (input));
                break;

            case GOODBYE:
                break;

            case GOODBYE_OK:
                hashSize = (int) self.getNumber4 ();
                self.headers = new HashMap <String, String> ();
                while (hashSize-- > 0) {
                    String key = self.getString ();
                    String value = self.getLongString ();

                    self.headers.put(key, value);
                }
                break;

            case PING:
                break;

            case PING_OK:
                break;

            case INVALID:
                break;

            default:
                throw new IllegalArgumentException ();
            }

            return self;

        } catch (Exception e) {
            //  Error returns
            System.out.printf ("E: malformed message '%d'\n", self.id);
            self.destroy ();
            return null;
        } finally {
            if (frame != null)
                frame.destroy ();
        }
    }

    //  --------------------------------------------------------------------------
    //  Send the ZccpMsg to the socket, and destroy it

    public boolean send (Socket socket)
    {
        assert (socket != null);

        ZMsg msg = new ZMsg();

        int frameSize = 2 + 1;          //  Signature and message ID
        switch (id) {
        case HELLO:
            //  identifier is a string with 1-byte length
            frameSize ++;
            frameSize += (identifier != null) ? identifier.length() : 0;
            //  headers is an array of key=value strings
            frameSize += 4;
            if (headers != null) {
                headersBytes = 0;
                for (Map.Entry <String, String> entry: headers.entrySet ()) {
                    headersBytes += 1 + entry.getKey().length();
                    headersBytes += 4 + entry.getValue().length();
                }
                frameSize += headersBytes;
            }
            break;

        case HELLO_OK:
            //  headers is an array of key=value strings
            frameSize += 4;
            if (headers != null) {
                headersBytes = 0;
                for (Map.Entry <String, String> entry: headers.entrySet ()) {
                    headersBytes += 1 + entry.getKey().length();
                    headersBytes += 4 + entry.getValue().length();
                }
                frameSize += headersBytes;
            }
            break;

        case SUBSCRIBE:
            //  expression is a string with 1-byte length
            frameSize ++;
            frameSize += (expression != null) ? expression.length() : 0;
            //  headers is an array of key=value strings
            frameSize += 4;
            if (headers != null) {
                headersBytes = 0;
                for (Map.Entry <String, String> entry: headers.entrySet ()) {
                    headersBytes += 1 + entry.getKey().length();
                    headersBytes += 4 + entry.getValue().length();
                }
                frameSize += headersBytes;
            }
            break;

        case SUBSCRIBE_OK:
            break;

        case PUBLISH:
            //  address is a string with 1-byte length
            frameSize ++;
            frameSize += (address != null) ? address.length() : 0;
            //  headers is an array of key=value strings
            frameSize += 4;
            if (headers != null) {
                headersBytes = 0;
                for (Map.Entry <String, String> entry: headers.entrySet ()) {
                    headersBytes += 1 + entry.getKey().length();
                    headersBytes += 4 + entry.getValue().length();
                }
                frameSize += headersBytes;
            }
            break;

        case DIRECT:
            //  address is a string with 1-byte length
            frameSize ++;
            frameSize += (address != null) ? address.length() : 0;
            //  headers is an array of key=value strings
            frameSize += 4;
            if (headers != null) {
                headersBytes = 0;
                for (Map.Entry <String, String> entry: headers.entrySet ()) {
                    headersBytes += 1 + entry.getKey().length();
                    headersBytes += 4 + entry.getValue().length();
                }
                frameSize += headersBytes;
            }
            break;

        case DELIVER:
            //  sender is a string with 1-byte length
            frameSize ++;
            frameSize += (sender != null) ? sender.length() : 0;
            //  address is a string with 1-byte length
            frameSize ++;
            frameSize += (address != null) ? address.length() : 0;
            //  headers is an array of key=value strings
            frameSize += 4;
            if (headers != null) {
                headersBytes = 0;
                for (Map.Entry <String, String> entry: headers.entrySet ()) {
                    headersBytes += 1 + entry.getKey().length();
                    headersBytes += 4 + entry.getValue().length();
                }
                frameSize += headersBytes;
            }
            break;

        case GOODBYE:
            break;

        case GOODBYE_OK:
            //  headers is an array of key=value strings
            frameSize += 4;
            if (headers != null) {
                headersBytes = 0;
                for (Map.Entry <String, String> entry: headers.entrySet ()) {
                    headersBytes += 1 + entry.getKey().length();
                    headersBytes += 4 + entry.getValue().length();
                }
                frameSize += headersBytes;
            }
            break;

        case PING:
            break;

        case PING_OK:
            break;

        case INVALID:
            break;

        default:
            System.out.printf ("E: bad message type '%d', not sent\n", id);
            assert (false);
        }
        //  Now serialize message into the frame
        ZFrame frame = new ZFrame (new byte [frameSize]);
        needle = ByteBuffer.wrap (frame.getData ());
        int frameFlags = 0;
        putNumber2 (0xAAA0 | 10);
        putNumber1 ((byte) id);

        switch (id) {
        case HELLO:
            if (identifier != null)
                putString (identifier);
            else
                putNumber1 ((byte) 0);      //  Empty string
            if (headers != null) {
                putNumber4 (headers.size ());
                for (Map.Entry <String, String> entry: headers.entrySet ()) {
                    putString(entry.getKey());
                    putLongString(entry.getValue());
                }
            }
            else
                putNumber4 (0);      //  Empty dictionary
            break;

        case HELLO_OK:
            if (headers != null) {
                putNumber4 (headers.size ());
                for (Map.Entry <String, String> entry: headers.entrySet ()) {
                    putString(entry.getKey());
                    putLongString(entry.getValue());
                }
            }
            else
                putNumber4 (0);      //  Empty dictionary
            break;

        case SUBSCRIBE:
            if (expression != null)
                putString (expression);
            else
                putNumber1 ((byte) 0);      //  Empty string
            if (headers != null) {
                putNumber4 (headers.size ());
                for (Map.Entry <String, String> entry: headers.entrySet ()) {
                    putString(entry.getKey());
                    putLongString(entry.getValue());
                }
            }
            else
                putNumber4 (0);      //  Empty dictionary
            break;

        case SUBSCRIBE_OK:
            break;

        case PUBLISH:
            if (address != null)
                putString (address);
            else
                putNumber1 ((byte) 0);      //  Empty string
            if (headers != null) {
                putNumber4 (headers.size ());
                for (Map.Entry <String, String> entry: headers.entrySet ()) {
                    putString(entry.getKey());
                    putLongString(entry.getValue());
                }
            }
            else
                putNumber4 (0);      //  Empty dictionary
            break;

        case DIRECT:
            if (address != null)
                putString (address);
            else
                putNumber1 ((byte) 0);      //  Empty string
            if (headers != null) {
                putNumber4 (headers.size ());
                for (Map.Entry <String, String> entry: headers.entrySet ()) {
                    putString(entry.getKey());
                    putLongString(entry.getValue());
                }
            }
            else
                putNumber4 (0);      //  Empty dictionary
            break;

        case DELIVER:
            if (sender != null)
                putString (sender);
            else
                putNumber1 ((byte) 0);      //  Empty string
            if (address != null)
                putString (address);
            else
                putNumber1 ((byte) 0);      //  Empty string
            if (headers != null) {
                putNumber4 (headers.size ());
                for (Map.Entry <String, String> entry: headers.entrySet ()) {
                    putString(entry.getKey());
                    putLongString(entry.getValue());
                }
            }
            else
                putNumber4 (0);      //  Empty dictionary
            break;

        case GOODBYE:
            break;

        case GOODBYE_OK:
            if (headers != null) {
                putNumber4 (headers.size ());
                for (Map.Entry <String, String> entry: headers.entrySet ()) {
                    putString(entry.getKey());
                    putLongString(entry.getValue());
                }
            }
            else
                putNumber4 (0);      //  Empty dictionary
            break;

        case PING:
            break;

        case PING_OK:
            break;

        case INVALID:
            break;

        }
        //  Now send the data frame
        msg.add(frame);

        //  Now send any frame fields, in order
        switch (id) {
        }
        switch (id) {
        case PUBLISH:
            if( content == null )
                content = new ZMsg();
            for (ZFrame contentPart : content) {
                msg.add(contentPart);
            }
            break;
        case DIRECT:
            if( content == null )
                content = new ZMsg();
            for (ZFrame contentPart : content) {
                msg.add(contentPart);
            }
            break;
        case DELIVER:
            if( content == null )
                content = new ZMsg();
            for (ZFrame contentPart : content) {
                msg.add(contentPart);
            }
            break;
        }
        //  Destroy ZccpMsg object
        msg.send(socket);
        destroy ();
        return true;
    }


//  --------------------------------------------------------------------------
//  Send the HELLO to the socket in one step

    public static void sendHello (
        Socket output,
        String identifier,
        Map <String, String> headers)
    {
        ZccpMsg self = new ZccpMsg (ZccpMsg.HELLO);
        self.setIdentifier (identifier);
        self.setHeaders (new HashMap <String, String> (headers));
        self.send (output);
    }

//  --------------------------------------------------------------------------
//  Send the HELLO_OK to the socket in one step

    public static void sendHello_Ok (
        Socket output,
        Map <String, String> headers)
    {
        ZccpMsg self = new ZccpMsg (ZccpMsg.HELLO_OK);
        self.setHeaders (new HashMap <String, String> (headers));
        self.send (output);
    }

//  --------------------------------------------------------------------------
//  Send the SUBSCRIBE to the socket in one step

    public static void sendSubscribe (
        Socket output,
        String expression,
        Map <String, String> headers)
    {
        ZccpMsg self = new ZccpMsg (ZccpMsg.SUBSCRIBE);
        self.setExpression (expression);
        self.setHeaders (new HashMap <String, String> (headers));
        self.send (output);
    }

//  --------------------------------------------------------------------------
//  Send the SUBSCRIBE_OK to the socket in one step

    public static void sendSubscribe_Ok (
        Socket output)
    {
        ZccpMsg self = new ZccpMsg (ZccpMsg.SUBSCRIBE_OK);
        self.send (output);
    }

//  --------------------------------------------------------------------------
//  Send the PUBLISH to the socket in one step

    public static void sendPublish (
        Socket output,
        String address,
        Map <String, String> headers,
        ZMsg content)
    {
        ZccpMsg self = new ZccpMsg (ZccpMsg.PUBLISH);
        self.setAddress (address);
        self.setHeaders (new HashMap <String, String> (headers));
        self.setContent (content.duplicate ());
        self.send (output);
    }

//  --------------------------------------------------------------------------
//  Send the DIRECT to the socket in one step

    public static void sendDirect (
        Socket output,
        String address,
        Map <String, String> headers,
        ZMsg content)
    {
        ZccpMsg self = new ZccpMsg (ZccpMsg.DIRECT);
        self.setAddress (address);
        self.setHeaders (new HashMap <String, String> (headers));
        self.setContent (content.duplicate ());
        self.send (output);
    }

//  --------------------------------------------------------------------------
//  Send the DELIVER to the socket in one step

    public static void sendDeliver (
        Socket output,
        String sender,
        String address,
        Map <String, String> headers,
        ZMsg content)
    {
        ZccpMsg self = new ZccpMsg (ZccpMsg.DELIVER);
        self.setSender (sender);
        self.setAddress (address);
        self.setHeaders (new HashMap <String, String> (headers));
        self.setContent (content.duplicate ());
        self.send (output);
    }

//  --------------------------------------------------------------------------
//  Send the GOODBYE to the socket in one step

    public static void sendGoodbye (
        Socket output)
    {
        ZccpMsg self = new ZccpMsg (ZccpMsg.GOODBYE);
        self.send (output);
    }

//  --------------------------------------------------------------------------
//  Send the GOODBYE_OK to the socket in one step

    public static void sendGoodbye_Ok (
        Socket output,
        Map <String, String> headers)
    {
        ZccpMsg self = new ZccpMsg (ZccpMsg.GOODBYE_OK);
        self.setHeaders (new HashMap <String, String> (headers));
        self.send (output);
    }

//  --------------------------------------------------------------------------
//  Send the PING to the socket in one step

    public static void sendPing (
        Socket output)
    {
        ZccpMsg self = new ZccpMsg (ZccpMsg.PING);
        self.send (output);
    }

//  --------------------------------------------------------------------------
//  Send the PING_OK to the socket in one step

    public static void sendPing_Ok (
        Socket output)
    {
        ZccpMsg self = new ZccpMsg (ZccpMsg.PING_OK);
        self.send (output);
    }

//  --------------------------------------------------------------------------
//  Send the INVALID to the socket in one step

    public static void sendInvalid (
        Socket output)
    {
        ZccpMsg self = new ZccpMsg (ZccpMsg.INVALID);
        self.send (output);
    }


    //  --------------------------------------------------------------------------
    //  Duplicate the ZccpMsg message

    public ZccpMsg dup ()
    {
        ZccpMsg copy = new ZccpMsg (this.id);
        if (this.routingId != null)
            copy.routingId = this.routingId.duplicate ();
        switch (this.id) {
        case HELLO:
            copy.identifier = this.identifier;
            copy.headers = new HashMap <String, String> (this.headers);
        break;
        case HELLO_OK:
            copy.headers = new HashMap <String, String> (this.headers);
        break;
        case SUBSCRIBE:
            copy.expression = this.expression;
            copy.headers = new HashMap <String, String> (this.headers);
        break;
        case SUBSCRIBE_OK:
        break;
        case PUBLISH:
            copy.address = this.address;
            copy.headers = new HashMap <String, String> (this.headers);
        break;
        case DIRECT:
            copy.address = this.address;
            copy.headers = new HashMap <String, String> (this.headers);
        break;
        case DELIVER:
            copy.sender = this.sender;
            copy.address = this.address;
            copy.headers = new HashMap <String, String> (this.headers);
        break;
        case GOODBYE:
        break;
        case GOODBYE_OK:
            copy.headers = new HashMap <String, String> (this.headers);
        break;
        case PING:
        break;
        case PING_OK:
        break;
        case INVALID:
        break;
        }
        return copy;
    }

    //  Dump headers key=value pair to stdout
    public static void headersDump (Map.Entry <String, String> entry, ZccpMsg self)
    {
        System.out.printf ("        %s=%s\n", entry.getKey (), entry.getValue ());
    }


    //  --------------------------------------------------------------------------
    //  Print contents of message to stdout

    public void dump ()
    {
        switch (id) {
        case HELLO:
            System.out.println ("HELLO:");
            if (identifier != null)
                System.out.printf ("    identifier='%s'\n", identifier);
            else
                System.out.printf ("    identifier=\n");
            System.out.printf ("    headers={\n");
            if (headers != null) {
                for (Map.Entry <String, String> entry : headers.entrySet ())
                    headersDump (entry, this);
            }
            System.out.printf ("    }\n");
            break;

        case HELLO_OK:
            System.out.println ("HELLO_OK:");
            System.out.printf ("    headers={\n");
            if (headers != null) {
                for (Map.Entry <String, String> entry : headers.entrySet ())
                    headersDump (entry, this);
            }
            System.out.printf ("    }\n");
            break;

        case SUBSCRIBE:
            System.out.println ("SUBSCRIBE:");
            if (expression != null)
                System.out.printf ("    expression='%s'\n", expression);
            else
                System.out.printf ("    expression=\n");
            System.out.printf ("    headers={\n");
            if (headers != null) {
                for (Map.Entry <String, String> entry : headers.entrySet ())
                    headersDump (entry, this);
            }
            System.out.printf ("    }\n");
            break;

        case SUBSCRIBE_OK:
            System.out.println ("SUBSCRIBE_OK:");
            break;

        case PUBLISH:
            System.out.println ("PUBLISH:");
            if (address != null)
                System.out.printf ("    address='%s'\n", address);
            else
                System.out.printf ("    address=\n");
            System.out.printf ("    headers={\n");
            if (headers != null) {
                for (Map.Entry <String, String> entry : headers.entrySet ())
                    headersDump (entry, this);
            }
            System.out.printf ("    }\n");
            break;

        case DIRECT:
            System.out.println ("DIRECT:");
            if (address != null)
                System.out.printf ("    address='%s'\n", address);
            else
                System.out.printf ("    address=\n");
            System.out.printf ("    headers={\n");
            if (headers != null) {
                for (Map.Entry <String, String> entry : headers.entrySet ())
                    headersDump (entry, this);
            }
            System.out.printf ("    }\n");
            break;

        case DELIVER:
            System.out.println ("DELIVER:");
            if (sender != null)
                System.out.printf ("    sender='%s'\n", sender);
            else
                System.out.printf ("    sender=\n");
            if (address != null)
                System.out.printf ("    address='%s'\n", address);
            else
                System.out.printf ("    address=\n");
            System.out.printf ("    headers={\n");
            if (headers != null) {
                for (Map.Entry <String, String> entry : headers.entrySet ())
                    headersDump (entry, this);
            }
            System.out.printf ("    }\n");
            break;

        case GOODBYE:
            System.out.println ("GOODBYE:");
            break;

        case GOODBYE_OK:
            System.out.println ("GOODBYE_OK:");
            System.out.printf ("    headers={\n");
            if (headers != null) {
                for (Map.Entry <String, String> entry : headers.entrySet ())
                    headersDump (entry, this);
            }
            System.out.printf ("    }\n");
            break;

        case PING:
            System.out.println ("PING:");
            break;

        case PING_OK:
            System.out.println ("PING_OK:");
            break;

        case INVALID:
            System.out.println ("INVALID:");
            break;

        }
    }


    //  --------------------------------------------------------------------------
    //  Get/set the message routing id

    public ZFrame routingId ()
    {
        return routingId;
    }

    public void setRoutingId (ZFrame routingId)
    {
        if (this.routingId != null)
            this.routingId.destroy ();
        this.routingId = routingId.duplicate ();
    }


    //  --------------------------------------------------------------------------
    //  Get/set the zccp_msg id

    public int id ()
    {
        return id;
    }

    public void setId (int id)
    {
        this.id = id;
    }

    //  --------------------------------------------------------------------------
    //  Get/set the identifier field

    public String identifier ()
    {
        return identifier;
    }

    public void setIdentifier (String format, Object ... args)
    {
        //  Format into newly allocated string
        identifier = String.format (format, args);
    }

    //  --------------------------------------------------------------------------
    //  Get/set a value in the headers dictionary

    public Map <String, String> headers ()
    {
        return headers;
    }

    public String headersString (String key, String defaultValue)
    {
        String value = null;
        if (headers != null)
            value = headers.get (key);
        if (value == null)
            value = defaultValue;

        return value;
    }

    public long headersNumber (String key, long defaultValue)
    {
        long value = defaultValue;
        String string = null;
        if (headers != null)
            string = headers.get (key);
        if (string != null)
            value = Long.valueOf (string);

        return value;
    }

    public void insertHeaders (String key, String format, Object ... args)
    {
        //  Format string into buffer
        String string = String.format (format, args);

        //  Store string in hash table
        if (headers == null)
            headers = new HashMap <String, String> ();
        headers.put (key, string);
        headersBytes += key.length () + 1 + string.length ();
    }

    public void setHeaders (Map <String, String> value)
    {
        if (value != null)
            headers = new HashMap <String, String> (value);
        else
            headers = value;
    }

    //  --------------------------------------------------------------------------
    //  Get/set the expression field

    public String expression ()
    {
        return expression;
    }

    public void setExpression (String format, Object ... args)
    {
        //  Format into newly allocated string
        expression = String.format (format, args);
    }

    //  --------------------------------------------------------------------------
    //  Get/set the address field

    public String address ()
    {
        return address;
    }

    public void setAddress (String format, Object ... args)
    {
        //  Format into newly allocated string
        address = String.format (format, args);
    }

    //  --------------------------------------------------------------------------
    //  Get/set the content field

    public ZMsg content ()
    {
        return content;
    }

    //  Takes ownership of supplied frame
    public void setContent (ZMsg frame)
    {
        if (content != null)
            content.destroy ();
        content = frame;
    }
    //  --------------------------------------------------------------------------
    //  Get/set the sender field

    public String sender ()
    {
        return sender;
    }

    public void setSender (String format, Object ... args)
    {
        //  Format into newly allocated string
        sender = String.format (format, args);
    }

}

