package msg

import (
	"testing"

	zmq "github.com/pebbe/zmq4"
)

// Yay! Test function.
func TestPublish(t *testing.T) {

	// Create pair of sockets we can send through

	// Output socket
	output, err := zmq.NewSocket(zmq.DEALER)
	if err != nil {
		t.Fatal(err)
	}
	defer output.Close()

	routingId := "Shout"
	output.SetIdentity(routingId)
	err = output.Bind("inproc://selftest-publish")
	if err != nil {
		t.Fatal(err)
	}
	defer output.Unbind("inproc://selftest-publish")

	// Input socket
	input, err := zmq.NewSocket(zmq.ROUTER)
	if err != nil {
		t.Fatal(err)
	}
	defer input.Close()

	err = input.Connect("inproc://selftest-publish")
	if err != nil {
		t.Fatal(err)
	}
	defer input.Disconnect("inproc://selftest-publish")

	// Create a Publish message and send it through the wire
	publish := NewPublish()

	publish.Address = "Life is short but Now lasts for ever"

	publish.Headers = map[string]string{"Name": "Brutus", "Age": "43"}

	publish.Content = []byte("Captcha Diem")

	err = publish.Send(output)
	if err != nil {
		t.Fatal(err)
	}
	transit, err := Recv(input)
	if err != nil {
		t.Fatal(err)
	}

	tr := transit.(*Publish)

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
