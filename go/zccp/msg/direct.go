package msg

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"

	zmq "github.com/pebbe/zmq4"
)

// Client sends a message to a specific client
type Direct struct {
	routingId []byte
	Address   string
	Headers   map[string]string
	Content   []byte
}

// New creates new Direct message.
func NewDirect() *Direct {
	direct := &Direct{}
	direct.Headers = make(map[string]string)
	return direct
}

// String returns print friendly name.
func (d *Direct) String() string {
	str := "ZCCP_MSG_DIRECT:\n"
	str += fmt.Sprintf("    Address = %v\n", d.Address)
	str += fmt.Sprintf("    Headers = %v\n", d.Headers)
	str += fmt.Sprintf("    Content = %v\n", d.Content)
	return str
}

// Marshal serializes the message.
func (d *Direct) Marshal() ([]byte, error) {
	// Calculate size of serialized data
	bufferSize := 2 + 1 // Signature and message ID

	// Address is a string with 1-byte length
	bufferSize++ // Size is one byte
	bufferSize += len(d.Address)

	// Headers is a hash table
	bufferSize += 4 // Size is 4 bytes
	for key, val := range d.Headers {
		bufferSize += 1 + len(key)
		bufferSize += 4 + len(val)
	}

	// Now serialize the message
	tmpBuf := make([]byte, bufferSize)
	tmpBuf = tmpBuf[:0]
	buffer := bytes.NewBuffer(tmpBuf)
	binary.Write(buffer, binary.BigEndian, Signature)
	binary.Write(buffer, binary.BigEndian, DirectId)

	// Address
	putString(buffer, d.Address)

	// Headers
	binary.Write(buffer, binary.BigEndian, uint32(len(d.Headers)))
	for key, val := range d.Headers {
		putString(buffer, key)
		putLongString(buffer, val)
	}

	return buffer.Bytes(), nil
}

// Unmarshals the message.
func (d *Direct) Unmarshal(frames ...[]byte) error {
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
	if id != DirectId {
		return errors.New("malformed Direct message")
	}
	// Address
	d.Address = getString(buffer)
	// Headers
	var headersSize uint32
	binary.Read(buffer, binary.BigEndian, &headersSize)
	for ; headersSize != 0; headersSize-- {
		key := getString(buffer)
		val := getLongString(buffer)
		d.Headers[key] = val
	}
	// Content
	if 0 <= len(frames)-1 {
		d.Content = frames[0]
	}

	return nil
}

// Sends marshaled data through 0mq socket.
func (d *Direct) Send(socket *zmq.Socket) (err error) {
	frame, err := d.Marshal()
	if err != nil {
		return err
	}

	socType, err := socket.GetType()
	if err != nil {
		return err
	}

	// If we're sending to a ROUTER, we send the routingId first
	if socType == zmq.ROUTER {
		_, err = socket.SendBytes(d.routingId, zmq.SNDMORE)
		if err != nil {
			return err
		}
	}

	// Now send the data frame
	_, err = socket.SendBytes(frame, zmq.SNDMORE)
	if err != nil {
		return err
	}
	// Now send any frame fields, in order
	_, err = socket.SendBytes(d.Content, 0)

	return err
}

// RoutingId returns the routingId for this message, routingId should be set
// whenever talking to a ROUTER.
func (d *Direct) RoutingId() []byte {
	return d.routingId
}

// SetRoutingId sets the routingId for this message, routingId should be set
// whenever talking to a ROUTER.
func (d *Direct) SetRoutingId(routingId []byte) {
	d.routingId = routingId
}
