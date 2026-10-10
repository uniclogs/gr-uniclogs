DEBUG_OUTPUTS = True

SOURCE = "test"                     # "file", "stream", or "test"
FILENAME = 'test_fec_frame'
INVERTED_BITS = True                # IEEE 802.15.4
ENDIANNESS = 'little'               # 'big' or 'little'

HEADER_SIZE = 16
BITS_FOR_DEFINING_FRAME_SIZE = 11
K = 4                               # Size of register (input + state) THIS DECODER ONLY WORKS FOR k = 4 BY DESIGN!!!! CHANGING IT WILL BREAK EVERYTHING
HEADER_OVERHEAD = HEADER_SIZE * K
NUM_STATES = 2 ** (K - 1)           

MAX_PACKET_SIZE = 240               # In bytes
RANDOM_SIZE = False                 # For synthetic messages. If false uses MAX_PACKET_SIZE
ADD_NOISE = True                    # Intentionally adds some error when encoding the test message
NOISE_AMOUNT = 0.05                 # 0.0 (no noise) to 1.0 (all bits flipped)

BITS_PER_BYTE = 8