package msg

import (
	"testing"

	zmq "github.com/pebbe/zmq4"
)

// Yay! Test function.
func TestGoodbye(t *testing.T) {

	// Create pair of sockets we can send through

	// Output socket
	output, err := zmq.NewSocket(zmq.DEALER)
	if err != nil {
		t.Fatal(err)
	}
	defer output.Close()

	routingId := "Shout"
	output.SetIdentity(routingId)
	err = output.Bind("inproc://selftest-goodbye")
	if err != nil {
		t.Fatal(err)
	}
	defer output.Unbind("inproc://selftest-goodbye")

	// Input socket
	input, err := zmq.NewSocket(zmq.ROUTER)
	if err != nil {
		t.Fatal(err)
	}
	defer input.Close()

	err = input.Connect("inproc://selftest-goodbye")
	if err != nil {
		t.Fatal(err)
	}
	defer input.Disconnect("inproc://selftest-goodbye")

	// Create a Goodbye message and send it through the wire
	goodbye := NewGoodbye()

	err = goodbye.Send(output)
	if err != nil {
		t.Fatal(err)
	}
	transit, err := Recv(input)
	if err != nil {
		t.Fatal(err)
	}

	tr := transit.(*Goodbye)

	err = tr.Send(input)
	if err != nil {
		t.Fatal(err)
	}

	transit, err = Recv(output)
	if err != nil {
		t.Fatal(err)
	}

	if routingId != string(tr.RoutingId()) {
		t.Fatalf("expected %s, got %s", routingId, string(tr.RoutingId()))
	}
}
