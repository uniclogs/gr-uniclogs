#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2026 Portland State Aerospace Society.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#


import random
import pmt
from gnuradio import gr

K = 4
ENDIANNESS = 'little'
INVERTED_BITS = True

class viterbiEncoder(gr.basic_block):
    """
    docstring for block viterbiEncoder
    """
    def __init__(self, add_noise=False, noise_amt=0.05):
        gr.basic_block.__init__(self,
            name="viterbiEncoder",
            in_sig=None,
            out_sig=None)
        self._add_noise = add_noise
        self._noise_amt = noise_amt
        self.message_port_register_in(pmt.intern("in"))
        self.message_port_register_out(pmt.intern("out"))
        self.set_msg_handler(pmt.intern("in"), self.handle_msg)

    def handle_msg(self, msg):
        meta = pmt.car(msg)
        data = pmt.to_python(pmt.cdr(msg))

        # Return payloads with incomplete bytes or over max packet size
        if len(data) % 8 != 0 or len(data) // 8 > 240:
            return
        
        frame_len = len(data) // 8
        phr = ("00000" + (format(frame_len, '011b')[::-1])) if ENDIANNESS == 'little' else ("00000" + (format(frame_len, '011b')))
        tail = "0" * (K - 1)
        payload = "".join("1" if b else "0" for b in data)
        frame = phr + payload + tail

        m1, m2, m3 = 0, 0, 0
        encoded_bits = []
        for b in frame:
            m0 = int(b)
            u1 = int(m0 ^ m2 ^ m3)
            u0 = int(m0 ^ m1 ^ m2 ^ m3)

            encoded_bits += [u1, u0]
            m3, m2, m1 = m2, m1, m0

        if self._add_noise:
            encoded_bits = [(b ^ 1) if random.random() < self._noise_amt else b for b in encoded_bits]

        if INVERTED_BITS:
            encoded_bits = [b ^ 1 for b in encoded_bits]

        self.message_port_pub(pmt.intern("out"), pmt.cons(meta, pmt.init_u8vector(len(encoded_bits), encoded_bits)))
