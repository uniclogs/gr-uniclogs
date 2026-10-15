#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2026 AidanAkenson.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import numpy as np
import pmt
from gnuradio import gr

class lookup_table():
    def __init__(self):

        self._table = dict()

        for m1 in range(0, 2):
            for m2 in range(0, 2):
                for m3 in range(0, 2):
                    for m0 in range(0, 2):

                        self._table[f"{m0}{m1}{m2}{m3}"] = f"{int(m0 ^ m2 ^ m3)}{int(m0 ^ m1 ^ m2 ^ m3)}"

    def __str__(self):
        table_str = ""
        for i in self._table.keys():
            table_str += f"==============\n {i[0]} | {i[1:]} | {self._table[i]}\n"
        return table_str + "==============\n"

    def __getitem__(self, key):
        return self._table[key]

class traceback_table():
    def __init__(self):
        self._rows = []

    def append(self, row):
        self._rows.append(row)

    def __len__(self):
        return len(self._rows)

    def trace(self, end_state):
        bits = [0] * len(self._rows)
        state = end_state
        # Traverse rows backwards
        for t in range(len(self._rows) -1, -1, -1): 
            # The input bit is the top bit of state, recovered by shifting to discard other bits
            bits[t] = state >> 2                 
            # The new state is the surviving predecessor.
            # b and c bits of current state, are tested as the 
            # two high bits of the predecessor.
            state = ((state & 3) << 1) | self._rows[t][state]
        return bits

INVERTED_BITS = True                # IEEE 802.15.4
ENDIANNESS = 'little'               # 'big' or 'little'

HEADER_SIZE = 16
BITS_FOR_DEFINING_FRAME_SIZE = 11
K = 4                               # Size of register (input + state) THIS DECODER ONLY WORKS FOR k = 4 BY DESIGN!!!! CHANGING IT WILL BREAK EVERYTHING
HEADER_OVERHEAD = HEADER_SIZE * K
NUM_STATES = 2 ** (K - 1)           

MAX_PACKET_SIZE = 240               # In bytes

BITS_PER_BYTE = 8
_TABLE = lookup_table()
_OUT = []
for s in range(NUM_STATES):
    row = []
    for a in (0, 1):
        m1, m2, m3 = (s >> 2) & 1, (s >> 1) & 1, s & 1
        row.append(int(_TABLE[f"{a}{m1}{m2}{m3}"], 2))
    _OUT.append(row)

INF = float("inf")
POPCOUNT = [0, 1, 1, 2]              # Used for hamming distance calculations

# Returns state with the lowest metric sum
def best_state(metrics):
    return min(range(NUM_STATES), key=lambda s: metrics[s])
# Gets all metrics for the next timestep in the lattice
def timestep(metrics, u_pair):

    new_metrics = [INF] * NUM_STATES 
    row = [0] * NUM_STATES 

    for s in range(NUM_STATES):
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
    metrics = [INF] * NUM_STATES
    metrics[0] = 0
    table = traceback_table()

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

    if len(encoded_data) // 2 < HEADER_SIZE + HEADER_OVERHEAD:  
        return -1
            
    # Iterate over each bit in the header PLUS a lookahead for a more reliable result
    metrics, table = solve(encoded_data, HEADER_SIZE + HEADER_OVERHEAD)

    # Record the bits of the best path, but only as many bits as are defined to be in the header
    bits = table.trace(best_state(metrics))[:HEADER_SIZE]
        
    # Use the decoded header bits to find the size of the packet
    field = "".join(map(str, bits[HEADER_SIZE - BITS_FOR_DEFINING_FRAME_SIZE:]))
    size = int(field[::-1] if ENDIANNESS == "little" else field, 2) * BITS_PER_BYTE

    # Sanity checking and debug outputs
    if str("".join(str(b) for b in bits[:5])) != "00000":
        return -1
    if size > MAX_PACKET_SIZE * BITS_PER_BYTE: 
        return -1
    return size
# Decode an entire packet using the length derived from the
# decoded header and return the message portion.
def decode(encoded_data):   

    message_size = parse_header(encoded_data)

    # If sanity check on message header was failed, indicate that this message 
    # is likely invalid and only decode the max packet size
    if message_size == -1:
        message_size = min((len(encoded_data) // 2 - HEADER_SIZE), (MAX_PACKET_SIZE * BITS_PER_BYTE))

    # Entire size will be sum of header size, message size, and tail size
    steps = HEADER_SIZE + message_size + (K - 1)
    if steps > len(encoded_data) // 2:
        steps = len(encoded_data) // 2
        truncated = True
    else:
        truncated = False

    # Get the metrics and state table, from which associated decoded bits are gathered
    metrics, table = solve(encoded_data, steps)
    bits = table.trace(best_state(metrics) if truncated else 0)
        
    # Trim off the header and tail unless truncated, in which case tail is not trimmed
    if not truncated:
        return "".join(map(str, bits[HEADER_SIZE:-(K - 1)]))
    else:
        return "".join(map(str, bits[HEADER_SIZE:]))


class viterbi(gr.basic_block):
    """
    GNU Radio module for viterbi decoder with k = 4 

    Takes in an encoded message with coding efficiency 0.5
    and returns a decoded message
    """
    def __init__(self):
        gr.basic_block.__init__(self,
            name="viterbi",
            in_sig=None,
            out_sig=None)
        self.message_port_register_in(pmt.intern("in"))
        self.message_port_register_out(pmt.intern("out"))
        self.set_msg_handler(pmt.intern("in"), self.handle_pdu)

    def handle_pdu(self, msg):
        meta = pmt.car(msg)
        data = pmt.to_python(pmt.cdr(msg))
        bits = "".join("0" if b else "1" for b in data) if INVERTED_BITS else "".join("1" if b else "0" for b in data)
        decoded_bits = decode(bits)
        out = [int(c) for c in decoded_bits]
        self.message_port_pub(pmt.intern("out"), pmt.cons(meta, pmt.init_u8vector(len(out), out)))