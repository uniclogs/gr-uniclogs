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


if __name__ == "__main__":
    test_table = lookup_table()
    #print(test_table)

    print(test_table["0000"])