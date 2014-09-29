package msg

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"

	zmq "github.com/pebbe/zmq4"
)

// Client replies to a PING
type PingOk struct {
	routingId []byte
}

// New creates new PingOk message.
func NewPingOk() *PingOk {
	pingok := &PingOk{}
	return pingok
}

// String returns print friendly name.
func (p *PingOk) String() string {
	str := "ZCCP_MSG_PINGOK:\n"
	return str
}

// Marshal serializes the message.
func (p *PingOk) Marshal() ([]byte, error) {
	// Calculate size of serialized data
	bufferSize := 2 + 1 // Signature and message ID

	// Now serialize the message
	tmpBuf := make([]byte, bufferSize)
	tmpBuf = tmpBuf[:0]
	buffer := bytes.NewBuffer(tmpBuf)
	binary.Write(buffer, binary.BigEndian, Signature)
	binary.Write(buffer, binary.BigEndian, PingOkId)

	return buffer.Bytes(), nil
}

// Unmarshals the message.
func (p *PingOk) Unmarshal(frames ...[]byte) error {
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
	if id != PingOkId {
		return errors.New("malformed PingOk message")
	}

	return nil
}

// Sends marshaled data through 0mq socket.
func (p *PingOk) Send(socket *zmq.Socket) (err error) {
	frame, err := p.Marshal()
	if err != nil {
		return err
	}

	socType, err := socket.GetType()
	if err != nil {
		return err
	}

	// If we're sending to a ROUTER, we send the routingId first
	if socType == zmq.ROUTER {
		_, err = socket.SendBytes(p.routingId, zmq.SNDMORE)
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
func (p *PingOk) RoutingId() []byte {
	return p.routingId
}

// SetRoutingId sets the routingId for this message, routingId should be set
// whenever talking to a ROUTER.
func (p *PingOk) SetRoutingId(routingId []byte) {
	p.routingId = routingId
}
