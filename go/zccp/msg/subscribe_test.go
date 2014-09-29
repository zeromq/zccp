package msg

import (
	"testing"

	zmq "github.com/pebbe/zmq4"
)

// Yay! Test function.
func TestSubscribe(t *testing.T) {

	// Create pair of sockets we can send through

	// Output socket
	output, err := zmq.NewSocket(zmq.DEALER)
	if err != nil {
		t.Fatal(err)
	}
	defer output.Close()

	routingId := "Shout"
	output.SetIdentity(routingId)
	err = output.Bind("inproc://selftest-subscribe")
	if err != nil {
		t.Fatal(err)
	}
	defer output.Unbind("inproc://selftest-subscribe")

	// Input socket
	input, err := zmq.NewSocket(zmq.ROUTER)
	if err != nil {
		t.Fatal(err)
	}
	defer input.Close()

	err = input.Connect("inproc://selftest-subscribe")
	if err != nil {
		t.Fatal(err)
	}
	defer input.Disconnect("inproc://selftest-subscribe")

	// Create a Subscribe message and send it through the wire
	subscribe := NewSubscribe()

	subscribe.Expression = "Life is short but Now lasts for ever"

	subscribe.Headers = map[string]string{"Name": "Brutus", "Age": "43"}

	err = subscribe.Send(output)
	if err != nil {
		t.Fatal(err)
	}
	transit, err := Recv(input)
	if err != nil {
		t.Fatal(err)
	}

	tr := transit.(*Subscribe)

	if tr.Expression != "Life is short but Now lasts for ever" {
		t.Fatalf("expected %s, got %s", "Life is short but Now lasts for ever", tr.Expression)
	}

	for key, val := range map[string]string{"Name": "Brutus", "Age": "43"} {
		if tr.Headers[key] != val {
			t.Fatalf("expected %s, got %s", val, tr.Headers[key])
		}
	}

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
