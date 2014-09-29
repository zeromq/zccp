package msg

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"

	zmq "github.com/pebbe/zmq4"
)

// Server confirms subscription.
type SubscribeOk struct {
	routingId []byte
}

// New creates new SubscribeOk message.
func NewSubscribeOk() *SubscribeOk {
	subscribeok := &SubscribeOk{}
	return subscribeok
}

// String returns print friendly name.
func (s *SubscribeOk) String() string {
	str := "ZCCP_MSG_SUBSCRIBEOK:\n"
	return str
}

// Marshal serializes the message.
func (s *SubscribeOk) Marshal() ([]byte, error) {
	// Calculate size of serialized data
	bufferSize := 2 + 1 // Signature and message ID

	// Now serialize the message
	tmpBuf := make([]byte, bufferSize)
	tmpBuf = tmpBuf[:0]
	buffer := bytes.NewBuffer(tmpBuf)
	binary.Write(buffer, binary.BigEndian, Signature)
	binary.Write(buffer, binary.BigEndian, SubscribeOkId)

	return buffer.Bytes(), nil
}

// Unmarshals the message.
func (s *SubscribeOk) Unmarshal(frames ...[]byte) error {
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
	if id != SubscribeOkId {
		return errors.New("malformed SubscribeOk message")
	}

	return nil
}

// Sends marshaled data through 0mq socket.
func (s *SubscribeOk) Send(socket *zmq.Socket) (err error) {
	frame, err := s.Marshal()
	if err != nil {
		return err
	}

	socType, err := socket.GetType()
	if err != nil {
		return err
	}

	// If we're sending to a ROUTER, we send the routingId first
	if socType == zmq.ROUTER {
		_, err = socket.SendBytes(s.routingId, zmq.SNDMORE)
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
func (s *SubscribeOk) RoutingId() []byte {
	return s.routingId
}

// SetRoutingId sets the routingId for this message, routingId should be set
// whenever talking to a ROUTER.
func (s *SubscribeOk) SetRoutingId(routingId []byte) {
	s.routingId = routingId
}
