README

=== The Receiver ===
This peer scans standard input for a message:
GET [chunks-file] [where-to-save-file]
Upon receiving this message, the peer opens up the chunks file and sends out a
WHOHAS request. It also creates a linked list with a node for every chunk in
that we wish to get.
Subsequently, it should receive IHAVE responses from other peers. Upon receiving
an IHAVE response, the we add that peer to the list for every chunk it says it
has. We constantly scan in our select loop (which timesout every 0.1 seconds) to
see if we can be downloading a chunk that we are not.
Upon receiving DATA packets, the peer sends ACK responses. Once a full chunk has
been received, the peer looks to send GET requests to get other chunks. If a
peer times out 5 times, we label it bad, until either a fixed amount of time is
up or we hear from it again. We never GET from bad peers.
Once all the chunks have been received, the peer put all the chunks together and
prints out:
GOT [chunks-file]

=== The Sender ===
Upon receiving a WHOHAS request, the peer checks for the chunks and returns an
IHAVE response.
Upon receiving a GET request, the peer looks up the data from the master file
and creates packets for that data.
DATA requests are sent according to congestion control rules. After 3 succesive
ACKs, the next expected DATA packet is sent. After 5 dropped packets or
timeouts, the sender quits.
Once all ACK reponses have been received, the sender is done!
