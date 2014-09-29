// Package msg is 100% generated. If you edit this file,
// you will lose your changes at the next build cycle.
// DO NOT MAKE ANY CHANGES YOU WISH TO KEEP.
//
// The correct places for commits are:
//  - The XML model used for this code generation: zccp_msg.xml
//  - The code generation script that built this file: zproto_codec_c
package msg

import (
	"bytes"
	"encoding/binary"
	"errors"

	zmq "github.com/pebbe/zmq4"
)

const (
	Signature uint16 = 0xAAA0 | 10
)

const (
	HelloId       uint8 = 1
	HelloOkId     uint8 = 2
	SubscribeId   uint8 = 3
	SubscribeOkId uint8 = 4
	PublishId     uint8 = 5
	DirectId      uint8 = 6
	DeliverId     uint8 = 7
	GoodbyeId     uint8 = 8
	GoodbyeOkId   uint8 = 9
	PingId        uint8 = 10
	PingOkId      uint8 = 11
	InvalidId     uint8 = 12
)

type Transit interface {
	Marshal() ([]byte, error)
	Unmarshal(...[]byte) error
	String() string
	Send(*zmq.Socket) error
	SetRoutingId([]byte)
	RoutingId() []byte
}

// Unmarshals data from raw frames.
func Unmarshal(frames ...[]byte) (t Transit, err error) {
	if frames == nil {
		return nil, errors.New("can't unmarshal an empty message")
	}
	var buffer *bytes.Buffer

	// Check the signature
	var signature uint16
	buffer = bytes.NewBuffer(frames[0])
	binary.Read(buffer, binary.BigEndian, &signature)
	if signature != Signature {
		// Invalid signature
		return nil, errors.New("invalid signature")
	}

	// Get message id and parse per message type
	var id uint8
	binary.Read(buffer, binary.BigEndian, &id)

	switch id {
	case HelloId:
		t = NewHello()
	case HelloOkId:
		t = NewHelloOk()
	case SubscribeId:
		t = NewSubscribe()
	case SubscribeOkId:
		t = NewSubscribeOk()
	case PublishId:
		t = NewPublish()
	case DirectId:
		t = NewDirect()
	case DeliverId:
		t = NewDeliver()
	case GoodbyeId:
		t = NewGoodbye()
	case GoodbyeOkId:
		t = NewGoodbyeOk()
	case PingId:
		t = NewPing()
	case PingOkId:
		t = NewPingOk()
	case InvalidId:
		t = NewInvalid()
	}
	err = t.Unmarshal(frames...)

	return t, err
}

// Receives marshaled data from 0mq socket.
func Recv(socket *zmq.Socket) (t Transit, err error) {
	return recv(socket, 0)
}

// Receives marshaled data from 0mq socket. It won't wait for input.
func RecvNoWait(socket *zmq.Socket) (t Transit, err error) {
	return recv(socket, zmq.DONTWAIT)
}

// Receives marshaled data from 0mq socket.
func recv(socket *zmq.Socket, flag zmq.Flag) (t Transit, err error) {
	// Read all frames
	frames, err := socket.RecvMessageBytes(flag)
	if err != nil {
		return nil, err
	}

	sType, err := socket.GetType()
	if err != nil {
		return nil, err
	}

	var routingId []byte
	// If message came from a router socket, first frame is routingId
	if sType == zmq.ROUTER {
		if len(frames) <= 1 {
			return nil, errors.New("no routingId")
		}
		routingId = frames[0]
		frames = frames[1:]
	}

	t, err = Unmarshal(frames...)
	if err != nil {
		return nil, err
	}

	if sType == zmq.ROUTER {
		t.SetRoutingId(routingId)
	}
	return t, err
}

// Clones a message.
func Clone(t Transit) Transit {

	switch msg := t.(type) {
	case *Hello:
		cloned := NewHello()
		routingId := make([]byte, len(msg.RoutingId()))
		copy(routingId, msg.RoutingId())
		cloned.SetRoutingId(routingId)
		cloned.Identifier = msg.Identifier
		for key, val := range msg.Headers {
			cloned.Headers[key] = val
		}
		return cloned

	case *HelloOk:
		cloned := NewHelloOk()
		routingId := make([]byte, len(msg.RoutingId()))
		copy(routingId, msg.RoutingId())
		cloned.SetRoutingId(routingId)
		for key, val := range msg.Headers {
			cloned.Headers[key] = val
		}
		return cloned

	case *Subscribe:
		cloned := NewSubscribe()
		routingId := make([]byte, len(msg.RoutingId()))
		copy(routingId, msg.RoutingId())
		cloned.SetRoutingId(routingId)
		cloned.Expression = msg.Expression
		for key, val := range msg.Headers {
			cloned.Headers[key] = val
		}
		return cloned

	case *SubscribeOk:
		cloned := NewSubscribeOk()
		routingId := make([]byte, len(msg.RoutingId()))
		copy(routingId, msg.RoutingId())
		cloned.SetRoutingId(routingId)
		return cloned

	case *Publish:
		cloned := NewPublish()
		routingId := make([]byte, len(msg.RoutingId()))
		copy(routingId, msg.RoutingId())
		cloned.SetRoutingId(routingId)
		cloned.Address = msg.Address
		for key, val := range msg.Headers {
			cloned.Headers[key] = val
		}
		cloned.Content = append(cloned.Content, msg.Content...)
		return cloned

	case *Direct:
		cloned := NewDirect()
		routingId := make([]byte, len(msg.RoutingId()))
		copy(routingId, msg.RoutingId())
		cloned.SetRoutingId(routingId)
		cloned.Address = msg.Address
		for key, val := range msg.Headers {
			cloned.Headers[key] = val
		}
		cloned.Content = append(cloned.Content, msg.Content...)
		return cloned

	case *Deliver:
		cloned := NewDeliver()
		routingId := make([]byte, len(msg.RoutingId()))
		copy(routingId, msg.RoutingId())
		cloned.SetRoutingId(routingId)
		cloned.Sender = msg.Sender
		cloned.Address = msg.Address
		for key, val := range msg.Headers {
			cloned.Headers[key] = val
		}
		cloned.Content = append(cloned.Content, msg.Content...)
		return cloned

	case *Goodbye:
		cloned := NewGoodbye()
		routingId := make([]byte, len(msg.RoutingId()))
		copy(routingId, msg.RoutingId())
		cloned.SetRoutingId(routingId)
		return cloned

	case *GoodbyeOk:
		cloned := NewGoodbyeOk()
		routingId := make([]byte, len(msg.RoutingId()))
		copy(routingId, msg.RoutingId())
		cloned.SetRoutingId(routingId)
		for key, val := range msg.Headers {
			cloned.Headers[key] = val
		}
		return cloned

	case *Ping:
		cloned := NewPing()
		routingId := make([]byte, len(msg.RoutingId()))
		copy(routingId, msg.RoutingId())
		cloned.SetRoutingId(routingId)
		return cloned

	case *PingOk:
		cloned := NewPingOk()
		routingId := make([]byte, len(msg.RoutingId()))
		copy(routingId, msg.RoutingId())
		cloned.SetRoutingId(routingId)
		return cloned

	case *Invalid:
		cloned := NewInvalid()
		routingId := make([]byte, len(msg.RoutingId()))
		copy(routingId, msg.RoutingId())
		cloned.SetRoutingId(routingId)
		return cloned
	}

	return nil
}

// putString marshals a string into the buffer.
func putString(buffer *bytes.Buffer, str string) {
	size := len(str)
	binary.Write(buffer, binary.BigEndian, byte(size))
	binary.Write(buffer, binary.BigEndian, []byte(str[0:size]))
}

// getString unmarshals a string from the buffer.
func getString(buffer *bytes.Buffer) string {
	var size byte
	binary.Read(buffer, binary.BigEndian, &size)
	str := make([]byte, size)
	binary.Read(buffer, binary.BigEndian, &str)
	return string(str)
}

// putLongString marshals a string into the buffer.
func putLongString(buffer *bytes.Buffer, str string) {
	size := len(str)
	binary.Write(buffer, binary.BigEndian, uint32(size))
	binary.Write(buffer, binary.BigEndian, []byte(str[0:size]))
}

// getLongString unmarshals a string from the buffer.
func getLongString(buffer *bytes.Buffer) string {
	var size uint32
	binary.Read(buffer, binary.BigEndian, &size)
	str := make([]byte, size)
	binary.Read(buffer, binary.BigEndian, &str)
	return string(str)
}

// putBytes marshals []byte into the buffer.
func putBytes(buffer *bytes.Buffer, data []byte) {
	size := uint64(len(data))
	binary.Write(buffer, binary.BigEndian, size)
	binary.Write(buffer, binary.BigEndian, data)
}

// getBytes unmarshals []byte from the buffer.
func getBytes(buffer *bytes.Buffer) []byte {
	var size uint64
	binary.Read(buffer, binary.BigEndian, &size)
	data := make([]byte, size)
	binary.Read(buffer, binary.BigEndian, &data)
	return data
}
