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