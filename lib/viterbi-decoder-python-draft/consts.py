DEBUG_OUTPUTS = True

SOURCE = "test"                     # "file", "stream", or "test"
FILENAME = 'raw_fec_frame.mp3'
INVERTED_BITS = True                # Per IEEE 802.15.4

PACKET_SIZE = 240  
HEADER_SIZE = 16
BITS_FOR_DEFINING_FRAME_SIZE = 11

MAX_DEPTH = 10                      # Maximum number of chunks to read through
RANDOM_SIZE = False                 # For synthetic messages. If false uses PACKET_SIZE