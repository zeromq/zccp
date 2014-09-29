package msg

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"

	zmq "github.com/pebbe/zmq4"
)

// Client sent a message that was not valid at this time
type Invalid struct {
	routingId []byte
}

// New creates new Invalid message.
func NewInvalid() *Invalid {
	invalid := &Invalid{}
	return invalid
}

// String returns print friendly name.
func (i *Invalid) String() string {
	str := "ZCCP_MSG_INVALID:\n"
	return str
}

// Marshal serializes the message.
func (i *Invalid) Marshal() ([]byte, error) {
	// Calculate size of serialized data
	bufferSize := 2 + 1 // Signature and message ID

	// Now serialize the message
	tmpBuf := make([]byte, bufferSize)
	tmpBuf = tmpBuf[:0]
	buffer := bytes.NewBuffer(tmpBuf)
	binary.Write(buffer, binary.BigEndian, Signature)
	binary.Write(buffer, binary.BigEndian, InvalidId)

	return buffer.Bytes(), nil
}

// Unmarshals the message.
func (i *Invalid) Unmarshal(frames ...[]byte) error {
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
	if id != InvalidId {
		return errors.New("malformed Invalid message")
	}

	return nil
}

// Sends marshaled data through 0mq socket.
func (i *Invalid) Send(socket *zmq.Socket) (err error) {
	frame, err := i.Marshal()
	if err != nil {
		return err
	}

	socType, err := socket.GetType()
	if err != nil {
		return err
	}

	// If we're sending to a ROUTER, we send the routingId first
	if socType == zmq.ROUTER {
		_, err = socket.SendBytes(i.routingId, zmq.SNDMORE)
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
func (i *Invalid) RoutingId() []byte {
	return i.routingId
}

// SetRoutingId sets the routingId for this message, routingId should be set
// whenever talking to a ROUTER.
func (i *Invalid) SetRoutingId(routingId []byte) {
	i.routingId = routingId
}
