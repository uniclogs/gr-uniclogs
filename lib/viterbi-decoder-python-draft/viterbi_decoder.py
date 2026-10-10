import copy

import path
import input_sources
import consts

# Increments and culls paths
def timestep(paths, u_pair):

    new_paths = [None] * consts.NUM_STATES  

    for p in range(consts.NUM_STATES):
        input_bit = (p >> 2) & 1  
        candidates = []

        # Check if state can be reached from existing tracked paths
        for m3 in (0, 1):
            previous_state = ((p & (consts.K - 1)) << 1) | m3

            # No further work needed if previous state can not 
            # exist under these conditions
            if paths[previous_state] is None:
                continue

            # If previous state is possible, add to list for
            # metric examination
            copy_path = copy.deepcopy(paths[previous_state])
            copy_path.update_path(input_bit, u_pair)
            candidates.append(copy_path)

        # Add the candidate with the lowest branch metric to the
        # list of maintained paths; cull others
        if candidates:
            new_paths[p] = min(candidates)

    return new_paths

# Used to get the length of the data segment in the packet.
# This value is itself encoded, but can be decoded because
# the length of the header is known in advance.
# FIXME if errors exist in the header and the packet size is way off, it is currently not detected!!
def parse_header(encoded_data) -> int:

    if consts.DEBUG_OUTPUTS:
        print(f"\nENCODED HEADER BITS: {encoded_data[:consts.HEADER_SIZE * 2]}")
        
    # Initial only has one path with no history
    paths = [path.path()] + [None] * (consts.NUM_STATES - 1)

    # Call for each bit in the header PLUS an overhead of several buffer lengths
    for i in range(consts.HEADER_SIZE + consts.HEADER_OVERHEAD):
        paths = timestep(paths, f"{encoded_data[i * 2]}{encoded_data[(i * 2) + 1]}")

    # Get the size, looking through only the bits used in the actual header and ignoring the overhead
    size = 0
    min_path = min(p for p in paths if p)
    for i in range(0, consts.BITS_FOR_DEFINING_FRAME_SIZE):
        if min_path._decoded_bits[(consts.HEADER_SIZE - consts.BITS_FOR_DEFINING_FRAME_SIZE) + i] == 1:
            if consts.ENDIANNESS == 'big':
                size += (2 ** (consts.BITS_FOR_DEFINING_FRAME_SIZE - 1 - i)) 
            else:
                size += (2 ** i)

    # Packet size is in bytes
    size *= consts.BITS_PER_BYTE

    if consts.DEBUG_OUTPUTS:
        print(f"DECODED HEADER BITS: {"".join(str(b) for b in min_path._decoded_bits[:5])} {"".join(str(b) for b in min_path._decoded_bits[5:consts.HEADER_SIZE])}, PACKET SIZE: {size // 8} bytes ({size} bits)\n")
    
    return size

# Decode an entire packet. Calls the header parsing function
# to automatically determine length of data segment.
def decode(encoded_data):   

    message_size = parse_header(encoded_data)

    # Initial only has one path with no history
    paths = [path.path()] + [None] * (consts.NUM_STATES - 1)

    # Call for each bit in the message
    for i in range(consts.HEADER_SIZE + (message_size // 1) + (consts.K - 1)):
        paths = timestep(paths, f"{encoded_data[i * 2]}{encoded_data[(i * 2) + 1]}")

    # Display the decoded message (FIXME remove later once return value is used more effectively)
    print("DECODED MESSAGE:", str(paths[0])[consts.HEADER_SIZE:-(consts.K - 1)], "\n")

    # Trim off the header
    return str(paths[0])[consts.HEADER_SIZE:-(consts.K - 1)]


if __name__ == "__main__":

    # Read from recording file and decode message chunkwise
    if consts.SOURCE == "file":
        print("READING FROM FILE\n")
        message = input_sources.file(filename=consts.FILENAME)
        decoded_message = decode(message.encoded_bits())

    # Read from generated file for testing
    elif consts.SOURCE == "test":
        message = input_sources.message()
        print(message.info())
        print("READING FROM SYNTHETIC MESSAGE\n")
        decoded_message = decode(message.encoded_bits())

        # Error checking
        d_ham = 0
        for i in range(min(len(decoded_message), len(message._initial_message))):
            d_ham += int(message._initial_message[i] != decoded_message[i])
        print("NUM ERRORS:", d_ham, "\nERROR RATE:", f"{(d_ham/max(min(len(decoded_message), len(message._initial_message)), 1) * 100):.2f}%")

        # Compare to noise generated, if used
        if consts.ADD_NOISE:
            print("ERRORS INJECTED ARTIFICIALLY:", message._injected_errors)


    # FIXME stream input type not defined yet