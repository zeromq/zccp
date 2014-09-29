package msg

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"

	zmq "github.com/pebbe/zmq4"
)

// Client subscribes to some set of messages
type Subscribe struct {
	routingId  []byte
	Expression string
	Headers    map[string]string
}

// New creates new Subscribe message.
func NewSubscribe() *Subscribe {
	subscribe := &Subscribe{}
	subscribe.Headers = make(map[string]string)
	return subscribe
}

// String returns print friendly name.
func (s *Subscribe) String() string {
	str := "ZCCP_MSG_SUBSCRIBE:\n"
	str += fmt.Sprintf("    Expression = %v\n", s.Expression)
	str += fmt.Sprintf("    Headers = %v\n", s.Headers)
	return str
}

// Marshal serializes the message.
func (s *Subscribe) Marshal() ([]byte, error) {
	// Calculate size of serialized data
	bufferSize := 2 + 1 // Signature and message ID

	// Expression is a string with 1-byte length
	bufferSize++ // Size is one byte
	bufferSize += len(s.Expression)

	// Headers is a hash table
	bufferSize += 4 // Size is 4 bytes
	for key, val := range s.Headers {
		bufferSize += 1 + len(key)
		bufferSize += 4 + len(val)
	}

	// Now serialize the message
	tmpBuf := make([]byte, bufferSize)
	tmpBuf = tmpBuf[:0]
	buffer := bytes.NewBuffer(tmpBuf)
	binary.Write(buffer, binary.BigEndian, Signature)
	binary.Write(buffer, binary.BigEndian, SubscribeId)

	// Expression
	putString(buffer, s.Expression)

	// Headers
	binary.Write(buffer, binary.BigEndian, uint32(len(s.Headers)))
	for key, val := range s.Headers {
		putString(buffer, key)
		putLongString(buffer, val)
	}

	return buffer.Bytes(), nil
}

// Unmarshals the message.
func (s *Subscribe) Unmarshal(frames ...[]byte) error {
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
	if id != SubscribeId {
		return errors.New("malformed Subscribe message")
	}
	// Expression
	s.Expression = getString(buffer)
	// Headers
	var headersSize uint32
	binary.Read(buffer, binary.BigEndian, &headersSize)
	for ; headersSize != 0; headersSize-- {
		key := getString(buffer)
		val := getLongString(buffer)
		s.Headers[key] = val
	}

	return nil
}

// Sends marshaled data through 0mq socket.
func (s *Subscribe) Send(socket *zmq.Socket) (err error) {
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
func (s *Subscribe) RoutingId() []byte {
	return s.routingId
}

// SetRoutingId sets the routingId for this message, routingId should be set
// whenever talking to a ROUTER.
func (s *Subscribe) SetRoutingId(routingId []byte) {
	s.routingId = routingId
}
