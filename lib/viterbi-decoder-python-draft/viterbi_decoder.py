import input_sources
import consts
import decode_lookup
import traceback_table

_TABLE = decode_lookup.lookup_table()
_OUT = []
for s in range(consts.NUM_STATES):
    row = []
    for a in (0, 1):
        m1, m2, m3 = (s >> 2) & 1, (s >> 1) & 1, s & 1
        row.append(int(_TABLE[f"{a}{m1}{m2}{m3}"], 2))
    _OUT.append(row)

INF = float("inf")
# Used for hamming distance calculations
POPCOUNT = [0, 1, 1, 2]

# Returns state with the lowest metric sum
def best_state(metrics):
    return min(range(consts.NUM_STATES), key=lambda s: metrics[s])
# Gets all metrics for the next timestep in the lattice
def timestep(metrics, u_pair):

    new_metrics = [INF] * consts.NUM_STATES 
    row = [0] * consts.NUM_STATES 

    for s in range(consts.NUM_STATES):
        input_bit = (s >> 2) & 1  

        # Check if state can be reached from existing tracked paths
        for m3 in (0, 1):
            previous_state = ((s & 3) << 1) | m3

            # No further work needed if previous state can not 
            # exist under these conditions
            if metrics[previous_state] == INF:
                continue

            # Record decision bit if one of the possible parents beats the other candidate
            candidate = metrics[previous_state] + POPCOUNT[_OUT[previous_state][input_bit] ^ u_pair]
            if candidate < new_metrics[s]:
                new_metrics[s] = candidate
                row[s] = m3

    return new_metrics, row
# Returns all non-culled metrics and, for each step and state, which predecessor 
# survived and the final metric sum for each state
def solve(encoded_data, steps):
    # Initialize metrics and table
    metrics = [INF] * consts.NUM_STATES
    metrics[0] = 0
    table = traceback_table.traceback_table()

    # Iterate over all timesteps
    for i in range(steps):
        metrics, row = timestep(metrics, int(encoded_data[2 * i: 2* i + 2], 2))
        table.append(row)

    # Return completed lattice
    return metrics, table

# Used to get the length of the data segment in the packet.
# This value is itself encoded, but can be decoded because
# the length of the header is known in advance.
def parse_header(encoded_data) -> int:

    if consts.DEBUG_OUTPUTS:
        print(f"\nENCODED HEADER BITS: {encoded_data[:consts.HEADER_SIZE * 2]}")
        
    # Iterate over each bit in the header PLUS a lookahead for a more reliable result
    metrics, table = solve(encoded_data, consts.HEADER_SIZE + consts.HEADER_OVERHEAD)

    # Record the bits of the best path, but only as many bits as are defined to be in the header
    bits = table.trace(best_state(metrics))[:consts.HEADER_SIZE]
    
    # Use the decoded header bits to find the size of the packet
    field = "".join(map(str, bits[consts.HEADER_SIZE - consts.BITS_FOR_DEFINING_FRAME_SIZE:]))
    size = int(field[::-1] if consts.ENDIANNESS == "little" else field, 2) * consts.BITS_PER_BYTE

    # Sanity checking and debug outputs
    if str("".join(str(b) for b in bits[:5])) != "00000":
        print("DECODED RESERVED BITS ARE NOT 00000:", "".join(str(b) for b in bits[:5]), ", ",end="")
        return -1
    if size > consts.MAX_PACKET_SIZE * consts.BITS_PER_BYTE:
        print("DECODED PACKET SIZE IS LARGER THAN MAXIMUM:", size, "bits, ", end="")
        return -1
    if consts.DEBUG_OUTPUTS:
        print(f"DECODED HEADER BITS: {"".join(str(b) for b in bits[:5])} {"".join(str(b) for b in bits[5:consts.HEADER_SIZE])}, PACKET SIZE: {size // 8} bytes ({size} bits)\n")
    
    return size
# Decode an entire packet using the length derived from the
# decoded header and return the message portion.
def decode(encoded_data):   

    message_size = parse_header(encoded_data)

    # If sanity check on message header was failed, indicate that this message 
    # is likely invalid and only decode the max packet size
    if message_size == -1:
        print("THE FOLLOWING MESSAGE IS LIKELY INVALID!!\n")
        message_size = consts.MAX_PACKET_SIZE * consts.BITS_PER_BYTE

    # Entire size will be sum of header size, message size, and tail size
    steps = consts.HEADER_SIZE + message_size + (consts.K - 1)

    # Get the metrics and state table, from which associated decoded bits are gathered
    metrics, table = solve(encoded_data, steps)
    bits = table.trace(0)
    
    # Display the decoded message (FIXME remove later once return value is used more effectively)
    print("DECODED MESSAGE:", "".join(map(str, bits[consts.HEADER_SIZE:-(consts.K - 1)])), "\n")

    # Trim off the header and tail
    return "".join(map(str, bits[consts.HEADER_SIZE:-(consts.K - 1)])), metrics[0]


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
        decoded_message, metric_sum = decode(message.encoded_bits())

        # Error checking
        d_ham = 0
        if abs(len(decoded_message) - len(message._initial_message)) != 0:
            print("MESSAGES OF UNEVEN LENGTH! DECODED MESSAGE LENGTH: ", len(decoded_message), ", ORIGINAL MESSAGE LENGTH: ", len(message._initial_message), sep="")
        for i in range(min(len(decoded_message), len(message._initial_message))):
            d_ham += int(message._initial_message[i] != decoded_message[i])
        print("NUM ERRORS:", d_ham, "\nERROR RATE:", f"{(d_ham/max(min(len(decoded_message), len(message._initial_message)), 1) * 100):.2f}%")

        # Compare to noise generated, if used
        if consts.ADD_NOISE:
            print("\nERRORS INJECTED ARTIFICIALLY: ", message._injected_errors, "\nNOISE LEVEL: ", consts.NOISE_AMOUNT * 100, "%", sep="")
            if metric_sum > message._injected_errors:
                print("DECODE FAILURE: METRIC SUM EXCEEDS THE AMOUNT OF ACTUAL ERRORS INDUCED")
            


    # FIXME stream input type not defined yet