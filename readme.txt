README

=== The Receiver ===
This peer scans standard input for a message:
GET [chunks-file] [where-to-save-file]
Upon receiving this message, the peer opens up the chunks file and sends out a
WHOHAS request.
Subsequently, it should receive IHAVE responses from other peers. Upon receiving
an IHAVE response, the peer sends GET requests accordingly to download from
multiple peers.
Upon receiving DATA packets, the peer sends ACK responses. Once a full chunk has
been received, the peer looks to send GET requests to get other chunks. If there
is not a response from a peer after 5 seconds, the peer attempts again to
contact that peer. After successive timeouts, the peer tries another peer.
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