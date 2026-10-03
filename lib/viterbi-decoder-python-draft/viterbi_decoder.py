import path
import copy

import input_sources
import consts


# Size of register (input + state)
k = 4

# Doubled for checking which paths to prune at each new time step
num_paths = 2 * (2 ** (k-1))

# Test messages generated from viterbi_encoder.py
# 
# This generates a random message of random length within the limit allowed
# by the header specifications
# received_message = viterbi_encoder.message(k)
# encoded_bits = received_message._data_segment 


# Increments and culls paths
def timestep(lower_paths, u_pair):

    # Take each branch for each path
    upper_paths = copy.deepcopy(lower_paths)
    for p in upper_paths:
        p.update_path(0, u_pair)
    for p in lower_paths:
        p.update_path(1, u_pair)

    # Cull the less efficient branch of each path
    paths = []
    for i in range(len(lower_paths)):
        paths.append(min(upper_paths[i], lower_paths[i]))
    return paths

# Used to get the length of the data segment in the packet.
# This value is itself encoded, but can be decoded because
# the length of the header is known in advance.
def parse_header(encoded_data) -> int:

    if consts.DEBUG_OUTPUTS:
        print(f"\nENCODED HEADER BITS: {encoded_data[:consts.HEADER_SIZE * 2]}")
        

    # Initial only has one path with no history
    paths = [path.path()]

    # Call for each bit in the header
    for i in range(consts.HEADER_SIZE):
        paths = timestep(paths, f"{encoded_data[i * 2]}{encoded_data[(i * 2) + 1]}")

    # Get the size
    size = 0
    for i in range(0, consts.BITS_FOR_DEFINING_FRAME_SIZE):
        if paths[0]._decoded_bits[5 + i] == 1:
            size += (2 ** i) 

    if consts.DEBUG_OUTPUTS:
        print(f"DECODED HEADER BITS: {"".join(str(b) for b in paths[0]._decoded_bits[:5])} {"".join(str(b) for b in paths[0]._decoded_bits[5:])}, PACKET SIZE: {size}\n")
    
    return size

# Decode an entire packet. Calls the header parsing function
# to automatically determine length of data segment.
def decode(encoded_data):   

    message_size = parse_header(encoded_data)
    encoded_data = encoded_data[consts.HEADER_SIZE * 2 : (consts.HEADER_SIZE * 2) + (message_size * 2)]

    # Initial only has one path with no history
    paths = [path.path()]

    # Call for each bit in the message
    for i in range(len(encoded_data) // 2):
        paths = timestep(paths, f"{encoded_data[i * 2]}{encoded_data[(i * 2) + 1]}")

    # Check that the tail was correctly found where expected
    print("DECODED MESSAGE:", str(paths[0]), "\n")
    if "".join(str(b) for b in paths[0]._decoded_bits[-k:]) != "0000":
        print(f"TAIL ERROR: {"".join(str(b) for b in paths[0]._decoded_bits[-k:])} is not 0000\n")


if __name__ == "__main__":

    # Read from recording file and decode message chunkwise
    if consts.SOURCE == "file":
        print("READING FROM FILE\n")
        encoded_bits = input_sources.bitstream_from_file(filename=consts.FILENAME)
        while not encoded_bits.is_empty():
            decode(encoded_bits.next_message())

    # Read from generated file for testing
    elif consts.SOURCE == "test":
        message = input_sources.message()
        print(message.info())
        print("READING FROM SYNTHETIC MESSAGE\n")
        decode(message.encoded_bits())

    # FIXME stream input type not defined yet