package msg

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"

	zmq "github.com/pebbe/zmq4"
)

// Client greets the server and provides its identifier.
type Hello struct {
	routingId  []byte
	Identifier string
	Headers    map[string]string
}

// New creates new Hello message.
func NewHello() *Hello {
	hello := &Hello{}
	hello.Headers = make(map[string]string)
	return hello
}

// String returns print friendly name.
func (h *Hello) String() string {
	str := "ZCCP_MSG_HELLO:\n"
	str += fmt.Sprintf("    Identifier = %v\n", h.Identifier)
	str += fmt.Sprintf("    Headers = %v\n", h.Headers)
	return str
}

// Marshal serializes the message.
func (h *Hello) Marshal() ([]byte, error) {
	// Calculate size of serialized data
	bufferSize := 2 + 1 // Signature and message ID

	// Identifier is a string with 1-byte length
	bufferSize++ // Size is one byte
	bufferSize += len(h.Identifier)

	// Headers is a hash table
	bufferSize += 4 // Size is 4 bytes
	for key, val := range h.Headers {
		bufferSize += 1 + len(key)
		bufferSize += 4 + len(val)
	}

	// Now serialize the message
	tmpBuf := make([]byte, bufferSize)
	tmpBuf = tmpBuf[:0]
	buffer := bytes.NewBuffer(tmpBuf)
	binary.Write(buffer, binary.BigEndian, Signature)
	binary.Write(buffer, binary.BigEndian, HelloId)

	// Identifier
	putString(buffer, h.Identifier)

	// Headers
	binary.Write(buffer, binary.BigEndian, uint32(len(h.Headers)))
	for key, val := range h.Headers {
		putString(buffer, key)
		putLongString(buffer, val)
	}

	return buffer.Bytes(), nil
}

// Unmarshals the message.
func (h *Hello) Unmarshal(frames ...[]byte) error {
	if frames == nil {
		return errors.New("Can't unmarshal empty message")
	}

	frame := frames[0]
	frames = frames[1:]

	buffer := bytes.NewBuffer(frame)

	// Get and check protocol signature
	var signature uint16
	binary.Read(buffer, binary.BigEndian, &signature)
	if signature != Signature {
		return errors.New("invalid signature")
	}

	// Get message id and parse per message type
	var id uint8
	binary.Read(buffer, binary.BigEndian, &id)
	if id != HelloId {
		return errors.New("malformed Hello message")
	}
	// Identifier
	h.Identifier = getString(buffer)
	// Headers
	var headersSize uint32
	binary.Read(buffer, binary.BigEndian, &headersSize)
	for ; headersSize != 0; headersSize-- {
		key := getString(buffer)
		val := getLongString(buffer)
		h.Headers[key] = val
	}

	return nil
}

// Sends marshaled data through 0mq socket.
func (h *Hello) Send(socket *zmq.Socket) (err error) {
	frame, err := h.Marshal()
	if err != nil {
		return err
	}

	socType, err := socket.GetType()
	if err != nil {
		return err
	}

	// If we're sending to a ROUTER, we send the routingId first
	if socType == zmq.ROUTER {
		_, err = socket.SendBytes(h.routingId, zmq.SNDMORE)
		if err != nil {
			return err
		}
	}

	// Now send the data frame
	_, err = socket.SendBytes(frame, 0)
	if err != nil {
		return err
	}

	return err
}

// RoutingId returns the routingId for this message, routingId should be set
// whenever talking to a ROUTER.
func (h *Hello) RoutingId() []byte {
	return h.routingId
}

// SetRoutingId sets the routingId for this message, routingId should be set
// whenever talking to a ROUTER.
func (h *Hello) SetRoutingId(routingId []byte) {
	h.routingId = routingId
}
