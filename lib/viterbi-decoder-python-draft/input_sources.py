import random
from bitstring import BitArray
import soundfile as sf

import consts

# Synthetic
class message():

    def __init__(self, k: int = 4):
        self._k = k

        if consts.RANDOM_SIZE:
            self._frame_len = random.randrange(consts.MAX_PACKET_SIZE) * 8  # Since packet size is in bytes
        else:
            self._frame_len = consts.MAX_PACKET_SIZE * 8  # Since packet size is in bytes
        if consts.ENDIANNESS == 'little':
            self._phr = "00000" + (format(self._frame_len, '011b')[::-1])
        else:
            self._phr = "00000" + (format(self._frame_len, '011b'))
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
        return f"Frame Length: {self._frame_len // 8} bytes ({self._frame_len} bits)\n\nPHR: {self._phr}\n\nInitial Message: {self._initial_message}\n\nTail: {self._tail}\n\nComplete data segment of packet: {self._data_segment}\n\nEncoded Message: {self._encoded_message}\n\n"

# Real
class file():
    def __init__(self, filename: str = "", k = 4):
        with open(filename, mode="rb") as file:
            self._bits = BitArray(file.read()).bin

            # IEEE 802.15.4 inverts bits
            if consts.INVERTED_BITS:
                self._bits = ''.join(['1' if b == '0' else '0' for b in self._bits])

            if consts.DEBUG_OUTPUTS:
                print("RAW BITS:", self._bits, "\n\n")

    # Returns the encoded header and message
    def encoded_bits(self):
        return self._bits

if __name__ == "__main__":

    if not consts.SOURCE == "test":
        test_message = message()
        print(test_message)

    elif consts.SOURCE == "file":
        test_message = file(filename=consts.FILENAME)
        print(test_message)

    # FIXME stream input type not defined yet