import decode_lookup

VERBOSE = False

# Reference for finding u1u0 pairs from input and register
_TABLE = decode_lookup.lookup_table()

class path():
    def __init__(self):
        self._branch_metrics = []
        self._metric_accumulator = 0
        self._decoded_bits = []
        self._shift_register = [0, 0, 0]


    def update_path(self, input_bit, actual_u_pair):
        metric = self.hamming_distance(_TABLE["".join(str(m) for m in [input_bit] + self._shift_register)], actual_u_pair)
        self._branch_metrics.append(metric)
        self._metric_accumulator += metric
        self._decoded_bits.append(input_bit)
        self._shift_register = [input_bit, self._shift_register[0], self._shift_register[1]]

    def hamming_distance(self, a, b):
        d = 0
        for i in range(len(a)):
            d += int(a[i] != b[i])
        return d

    def __gt__(self, other):
        return self._metric_accumulator > other._metric_accumulator
    def __lt__(self, other):
        return self._metric_accumulator < other._metric_accumulator

    def __str__(self):
        if not VERBOSE:
            return f"{"".join(str(b) for b in self._decoded_bits)}"
        return f"Decoded Message: {"".join(str(b) for b in self._decoded_bits)}\nBranch Metrics: {self._branch_metrics}\nMetric Sum: {self._metric_accumulator}\n"


if __name__ == "__main__":
    p = path()
    print(p.hamming_distance("0011", "1010"))