package msg

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"

	zmq "github.com/pebbe/zmq4"
)

// Request some action
type Request struct {
	routingId []byte
	Method    string
	Content   []byte
}

// New creates new Request message.
func NewRequest() *Request {
	request := &Request{}
	return request
}

// String returns print friendly name.
func (r *Request) String() string {
	str := "ZCCP_MSG_REQUEST:\n"
	str += fmt.Sprintf("    Method = %v\n", r.Method)
	str += fmt.Sprintf("    Content = %v\n", r.Content)
	return str
}

// Marshal serializes the message.
func (r *Request) Marshal() ([]byte, error) {
	// Calculate size of serialized data
	bufferSize := 2 + 1 // Signature and message ID

	// Method is a string with 1-byte length
	bufferSize++ // Size is one byte
	bufferSize += len(r.Method)

	// Content is a block of []byte with one byte length
	bufferSize += 1 + len(r.Content)

	// Now serialize the message
	tmpBuf := make([]byte, bufferSize)
	tmpBuf = tmpBuf[:0]
	buffer := bytes.NewBuffer(tmpBuf)
	binary.Write(buffer, binary.BigEndian, Signature)
	binary.Write(buffer, binary.BigEndian, RequestId)

	// Method
	putString(buffer, r.Method)

	// Content
	putBytes(buffer, r.Content)

	return buffer.Bytes(), nil
}

// Unmarshals the message.
func (r *Request) Unmarshal(frames ...[]byte) error {
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
	if id != RequestId {
		return errors.New("malformed Request message")
	}
	// Method
	r.Method = getString(buffer)
	// Content
	r.Content = getBytes(buffer)

	return nil
}

// Sends marshaled data through 0mq socket.
func (r *Request) Send(socket *zmq.Socket) (err error) {
	frame, err := r.Marshal()
	if err != nil {
		return err
	}

	socType, err := socket.GetType()
	if err != nil {
		return err
	}

	// If we're sending to a ROUTER, we send the routingId first
	if socType == zmq.ROUTER {
		_, err = socket.SendBytes(r.routingId, zmq.SNDMORE)
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
func (r *Request) RoutingId() []byte {
	return r.routingId
}

// SetRoutingId sets the routingId for this message, routingId should be set
// whenever talking to a ROUTER.
func (r *Request) SetRoutingId(routingId []byte) {
	r.routingId = routingId
}
