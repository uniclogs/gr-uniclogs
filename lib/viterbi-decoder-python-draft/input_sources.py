import random
from bitstring import BitArray
import soundfile as sf

import consts

# Synthetic
class message():

    def __init__(self, k: int = 4):
        self._k = k

        if consts.RANDOM_SIZE:
            self._frame_len = random.randrange(consts.PACKET_SIZE)
        else:
            self._frame_len = consts.PACKET_SIZE
        self._phr = "00000" + (format(self._frame_len, '011b')[::-1])
        self._tail = "0" * self._k

        # Initial message with half the length of the frame length to leave room for encoding
        self._initial_message = ""
        for i in range(self._frame_len // 2):
                self._initial_message += f"{random.randrange(2)}"

        self._data_segment = self._phr + self._initial_message + self._tail

        def encode(unencoded_message: str = ""):
            m1, m2, m3 = 0, 0, 0
            encoded_message = ""
            for i in range(len(unencoded_message)):
                m0 = int(unencoded_message[i])
                u1 = int(m0 ^ m2 ^ m3)
                u0 = int(m0 ^ m1 ^ m2 ^ m3)

                encoded_message += f"{u1}{u0}"

                m3, m2, m1 = m2, m1, m0

            return encoded_message

        self._encoded_message = encode(self._phr + self._initial_message + self._tail)

    # Returns the encoded header and message  
    def encoded_bits(self): 
        return f"{self._encoded_message}"
    # Returns more information
    def info(self):
        return f"Frame Length: {self._frame_len}\n\nPHR: {self._phr}\n\nInitial Message: {self._initial_message}\n\nTail: {self._tail}\n\nComplete data segment of packet: {self._data_segment}\n\nEncoded Message: {self._encoded_message}\n\n"

# Real
class bitstream_from_file():
    def __init__(self, filename: str = "", k = 4):
        with open(filename, mode="rb") as file:
            self._bits = BitArray(file.read()).bin

            # IEEE 802.15.4 inverts bits
            if consts.INVERTED_BITS:
                self._bits = ''.join(['1' if b == '0' else '0' for b in self._bits])

            if consts.DEBUG_OUTPUTS:
                print("RAW BITS:", self._bits, "\n\n")

        self._k = k
        self._offset = 0

    def next_message(self):
        # FIXME implement a methodology for waiting until the next header to grab a
        # packet. For now this isn't needed, but will be quite soon.

        # Isolate next message
        # Packet size is doubled due to the encoding method doubling the size of the packet
        message = self._bits[self._offset:self._offset + (consts.PACKET_SIZE * 2)]

        # Handle cases where the file ended unexpectedly
        if len(message) != (consts.PACKET_SIZE * 2):
            print("INSUFFICIENT BITS FOR COMPLETE PACKET")

            # Indicate that file is empty
            self._offset = len(self._bits)
            return message

        # Increment offset
        self._offset += consts.PACKET_SIZE * 2

        return message

    def __getitem__(self, key: int = -1):
        return self._message[key]

    def is_empty(self):
        return self._offset >= len(self._bits)

if __name__ == "__main__":

    if not consts.SOURCE == "test":
        test_message = message()
        print(test_message)

    elif consts.SOURCE == "file":
        test_message = bitstream_from_file(filename="rx_fec_recording_3")
        for i in range(len(test_message)):
            print(test_message[i])

    # FIXME stream input type not defined yet