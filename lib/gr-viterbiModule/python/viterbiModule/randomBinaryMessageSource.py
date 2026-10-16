#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2026 Portland State Aerospace Society.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#


import numpy as np
import pmt
from gnuradio import gr

NUM_BYTES = 240

class randomBinaryMessageSource(gr.basic_block):
    """
    Emits a PDU containing random bits each time a
    trigger is received.
    """
    def __init__(self, num_bytes = NUM_BYTES, seed = -1):
        gr.basic_block.__init__(self,
            name="randomBinaryMessageSource",
            in_sig=None,
            out_sig=None)
        self.num_bytes = num_bytes
        self.rng = np.random.default_rng(None if seed < 0 else seed)
        self.message_port_register_in(pmt.intern("trigger"))
        self.message_port_register_out(pmt.intern("out"))
        self.set_msg_handler(pmt.intern("trigger"), self.handle_trigger)


    def handle_trigger(self, msg):
        bits = self.rng.integers(0, 2, size=self.num_bytes * 8, dtype=np.uint8)
        self.message_port_pub(
                pmt.intern("out"),
                pmt.cons(pmt.make_dict(), pmt.init_u8vector(len(bits), bits.tolist())))


