package msg

import (
	"testing"

	zmq "github.com/pebbe/zmq4"
)

// Yay! Test function.
func TestDeliver(t *testing.T) {

	// Create pair of sockets we can send through

	// Output socket
	output, err := zmq.NewSocket(zmq.DEALER)
	if err != nil {
		t.Fatal(err)
	}
	defer output.Close()

	routingId := "Shout"
	output.SetIdentity(routingId)
	err = output.Bind("inproc://selftest-deliver")
	if err != nil {
		t.Fatal(err)
	}
	defer output.Unbind("inproc://selftest-deliver")

	// Input socket
	input, err := zmq.NewSocket(zmq.ROUTER)
	if err != nil {
		t.Fatal(err)
	}
	defer input.Close()

	err = input.Connect("inproc://selftest-deliver")
	if err != nil {
		t.Fatal(err)
	}
	defer input.Disconnect("inproc://selftest-deliver")

	// Create a Deliver message and send it through the wire
	deliver := NewDeliver()

	deliver.Sender = "Life is short but Now lasts for ever"

	deliver.Address = "Life is short but Now lasts for ever"

	deliver.Headers = map[string]string{"Name": "Brutus", "Age": "43"}

	deliver.Content = []byte("Captcha Diem")

	err = deliver.Send(output)
	if err != nil {
		t.Fatal(err)
	}
	transit, err := Recv(input)
	if err != nil {
		t.Fatal(err)
	}

	tr := transit.(*Deliver)

	if tr.Sender != "Life is short but Now lasts for ever" {
		t.Fatalf("expected %s, got %s", "Life is short but Now lasts for ever", tr.Sender)
	}

	if tr.Address != "Life is short but Now lasts for ever" {
		t.Fatalf("expected %s, got %s", "Life is short but Now lasts for ever", tr.Address)
	}

	for key, val := range map[string]string{"Name": "Brutus", "Age": "43"} {
		if tr.Headers[key] != val {
			t.Fatalf("expected %s, got %s", val, tr.Headers[key])
		}
	}

	if string(tr.Content) != "Captcha Diem" {
		t.Fatalf("expected %s, got %s", "Captcha Diem", tr.Content)
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
