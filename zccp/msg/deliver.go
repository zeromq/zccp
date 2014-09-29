package msg

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"

	zmq "github.com/pebbe/zmq4"
)

// Server delivers a message to client
type Deliver struct {
	routingId []byte
	Sender    string
	Address   string
	Headers   map[string]string
	Content   []byte
}

// New creates new Deliver message.
func NewDeliver() *Deliver {
	deliver := &Deliver{}
	deliver.Headers = make(map[string]string)
	return deliver
}

// String returns print friendly name.
func (d *Deliver) String() string {
	str := "ZCCP_MSG_DELIVER:\n"
	str += fmt.Sprintf("    Sender = %v\n", d.Sender)
	str += fmt.Sprintf("    Address = %v\n", d.Address)
	str += fmt.Sprintf("    Headers = %v\n", d.Headers)
	str += fmt.Sprintf("    Content = %v\n", d.Content)
	return str
}

// Marshal serializes the message.
func (d *Deliver) Marshal() ([]byte, error) {
	// Calculate size of serialized data
	bufferSize := 2 + 1 // Signature and message ID

	// Sender is a string with 1-byte length
	bufferSize++ // Size is one byte
	bufferSize += len(d.Sender)

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
	binary.Write(buffer, binary.BigEndian, DeliverId)

	// Sender
	putString(buffer, d.Sender)

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
func (d *Deliver) Unmarshal(frames ...[]byte) error {
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
	if id != DeliverId {
		return errors.New("malformed Deliver message")
	}
	// Sender
	d.Sender = getString(buffer)
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
func (d *Deliver) Send(socket *zmq.Socket) (err error) {
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
func (d *Deliver) RoutingId() []byte {
	return d.routingId
}

// SetRoutingId sets the routingId for this message, routingId should be set
// whenever talking to a ROUTER.
func (d *Deliver) SetRoutingId(routingId []byte) {
	d.routingId = routingId
}
