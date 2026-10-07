DEBUG_OUTPUTS = True

SOURCE = "file"                     # "file", "stream", or "test"
FILENAME = 'test_fec_frame'
INVERTED_BITS = True               # IEEE 802.15.4
ENDIANNESS = 'big'                  # 'big' or 'little'

HEADER_SIZE = 16
BITS_FOR_DEFINING_FRAME_SIZE = 11

MAX_PACKET_SIZE = 240               # In bytes
MAX_DEPTH = 10                      # Maximum number of chunks to read through
RANDOM_SIZE = False                 # For synthetic messages. If false uses PACKET_SIZE