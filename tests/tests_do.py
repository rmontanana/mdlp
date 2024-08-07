# ***************************************************************
# SPDX-FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT
# ***************************************************************

import json
from sklearn.preprocessing import KBinsDiscretizer

with open("datasets/tests.txt") as f:
    data = f.readlines()

data = [x.strip() for x in data if x[0] != "#"]

errors = False
for i in range(0, len(data), 4):
    experiment_type = data[i]
    print("Experiment:", data[i + 1])
    if experiment_type == "RANGE":
        range_data = data[i + 1]
        from_, to_, step_, n_bins_, strategy_ = range_data.split(",")
        X = [[float(x)] for x in range(int(from_), int(to_), int(step_))]
    else:
        strategy_ = data[i + 1][0]
        n_bins_ = data[i + 1][1]
        vector = data[i + 1][2:]
        X = [[float(x)] for x in json.loads(vector)]

    strategy = "quantile" if strategy_.strip() == "Q" else "uniform"
    disc = KBinsDiscretizer(
        n_bins=int(n_bins_),
        encode="ordinal",
        strategy=strategy,
    )
    expected_data = data[i + 2]
    cuts_data = data[i + 3]
    disc.fit(X)
    #
    # Normalize the cutpoints to remove numerical errors such as 33.0000000001
    # instead of 33
    #
    for j in range(len(disc.bin_edges_[0])):
        disc.bin_edges_[0][j] = round(disc.bin_edges_[0][j], 5)
    result = disc.transform(X)
    result = [int(x) for x in result.flatten()]
    expected = [int(x) for x in expected_data.split(",")]
    #
    # Check the Results
    #
    assert len(result) == len(expected)
    for j in range(len(result)):
        if result[j] != expected[j]:
            print("* Error at", j, "Expected=", expected[j], "Result=", result[j])
            errors = True
    expected_cuts = disc.bin_edges_[0]
    computed_cuts = [float(x) for x in cuts_data.split(",")]
    assert len(expected_cuts) == len(computed_cuts)
    for j in range(len(expected_cuts)):
        if round(expected_cuts[j], 5) != computed_cuts[j]:
            print(
                "* Error at",
                j,
                "Expected=",
                expected_cuts[j],
                "Result=",
                computed_cuts[j],
            )
            errors = True
if errors:
    raise Exception("There were errors!")
print("*** All tests run succesfully! ***")
