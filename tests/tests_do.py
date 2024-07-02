from sklearn.preprocessing import KBinsDiscretizer

with open("datasets/tests.txt") as f:
    data = f.readlines()

data = [x.strip() for x in data if x[0] != "#"]

for i in range(0, len(data), 3):
    print("Experiment:", data[i])
    from_, to_, step_, n_bins_, strategy_ = data[i].split(",")
    strategy = "quantile" if strategy_.strip() == "Q" else "uniform"
    disc = KBinsDiscretizer(
        n_bins=int(n_bins_),
        encode="ordinal",
        strategy=strategy,
    )
    X = [[float(x)] for x in range(int(from_), int(to_), int(step_))]
    # result = disc.fit_transform(X)
    disc.fit(X)
    result = disc.transform(X)
    result = [int(x) for x in result.flatten()]
    expected = [int(x) for x in data[i + 1].split(",")]
    assert len(result) == len(expected)
    for j in range(len(result)):
        if result[j] != expected[j]:
            print("Error at", j, "Expected=", expected[j], "Result=", result[j])
    expected_cuts = disc.bin_edges_[0]
    computed_cuts = [float(x) for x in data[i + 2].split(",")]
    assert len(expected_cuts) == len(computed_cuts)
    for j in range(len(expected_cuts)):
        if round(expected_cuts[j], 5) != computed_cuts[j]:
            print(
                "Error at",
                j,
                "Expected=",
                expected_cuts[j],
                "Result=",
                computed_cuts[j],
            )
