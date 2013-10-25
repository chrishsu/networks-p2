README

As of Checkpoint 1:
This peer scans standard input for a message:
GET [chunks-file] [where-to-save-file]
Upon receiving this message, the peer opens up the chunks-file and sends out a
WHOHAS request on its UDP port. Subsequently, it should receive IHAVE responses
from other peers.

Upon receiving a WHOHAS request, the peer checks for the chunks and returns an
IHAVE response on its UDP port.