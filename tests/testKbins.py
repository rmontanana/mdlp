from scipy.io.arff import loadarff
from sklearn.preprocessing import KBinsDiscretizer


def test(clf, X, expected, title):
    X = [[x] for x in X]
    clf.fit(X)
    computed = [int(x[0]) for x in clf.transform(X)]
    print(f"{title}")
    print(f"{computed=}")
    print(f"{expected=}")
    assert computed == expected
    print("-" * 80)


# Test Uniform Strategy
clf3u = KBinsDiscretizer(
    n_bins=3, encode="ordinal", strategy="uniform", subsample=200_000
)
clf3q = KBinsDiscretizer(
    n_bins=3, encode="ordinal", strategy="quantile", subsample=200_000
)
clf4u = KBinsDiscretizer(
    n_bins=4, encode="ordinal", strategy="uniform", subsample=200_000
)
clf4q = KBinsDiscretizer(
    n_bins=4, encode="ordinal", strategy="quantile", subsample=200_000
)
#
X = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0]
labels = [0, 0, 0, 1, 1, 1, 2, 2, 2]
test(clf3u, X, labels, title="Easy3BinsUniform")
test(clf3q, X, labels, title="Easy3BinsQuantile")
#
X = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0]
labels = [0, 0, 0, 1, 1, 1, 2, 2, 2, 2]
# En C++ se obtiene el mismo resultado en ambos, no como aquí
labels2 = [0, 0, 0, 1, 1, 1, 1, 2, 2, 2]
test(clf3u, X, labels, title="X10BinsUniform")
test(clf3q, X, labels2, title="X10BinsQuantile")
#
X = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0]
labels = [0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2]
# En C++ se obtiene el mismo resultado en ambos, no como aquí
# labels2 = [0, 0, 0, 1, 1, 1, 1, 2, 2, 2]
test(clf3u, X, labels, title="X11BinsUniform")
test(clf3q, X, labels, title="X11BinsQuantile")
#
X = [1.0, 1.0, 1.0, 1.0, 1.0, 1.0]
labels = [0, 0, 0, 0, 0, 0]
test(clf3u, X, labels, title="ConstantUniform")
test(clf3q, X, labels, title="ConstantQuantile")
#
X = [3.0, 1.0, 1.0, 3.0, 1.0, 1.0, 3.0, 1.0, 1.0]
labels = [2, 0, 0, 2, 0, 0, 2, 0, 0]
labels2 = [1, 0, 0, 1, 0, 0, 1, 0, 0]  # igual que en C++
test(clf3u, X, labels, title="EasyRepeatedUniform")
test(clf3q, X, labels2, title="EasyRepeatedQuantile")
#
X = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0]
labels = [0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3]
test(clf4u, X, labels, title="Easy4BinsUniform")
test(clf4q, X, labels, title="Easy4BinsQuantile")
#
X = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0]
labels = [0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3]
test(clf4u, X, labels, title="X13BinsUniform")
test(clf4q, X, labels, title="X13BinsQuantile")
#
X = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0]
labels = [0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3]
test(clf4u, X, labels, title="X14BinsUniform")
test(clf4q, X, labels, title="X14BinsQuantile")
#
X1 = [15.0, 8.0, 12.0, 14.0, 6.0, 1.0, 13.0, 11.0, 10.0, 9.0, 7.0, 4.0, 3.0, 5.0, 2.0]
X2 = [15.0, 13.0, 12.0, 14.0, 6.0, 1.0, 8.0, 11.0, 10.0, 9.0, 7.0, 4.0, 3.0, 5.0, 2.0]
labels1 = [3, 2, 3, 3, 1, 0, 3, 2, 2, 2, 1, 0, 0, 1, 0]
labels2 = [3, 3, 3, 3, 1, 0, 2, 2, 2, 2, 1, 0, 0, 1, 0]
test(clf4u, X1, labels1, title="X15BinsUniform")
test(clf4q, X2, labels2, title="X15BinsQuantile")
#
X = [0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 3.0, 3.0, 3.0, 4.0]
labels = [0, 1, 1, 1, 2, 2, 3, 3, 3, 3]
test(clf4u, X, labels, title="RepeatedValuesUniform")
test(clf4q, X, labels, title="RepeatedValuesQuantile")

print(f"Uniform   {clf4u.bin_edges_=}")
print(f"Quaintile {clf4q.bin_edges_=}")
print("-" * 80)
#
data, meta = loadarff("tests/datasets/iris.arff")

labelsu = [
    0,
    0,
    0,
    0,
    0,
    1,
    0,
    0,
    0,
    0,
    1,
    0,
    0,
    0,
    1,
    1,
    1,
    0,
    1,
    0,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    0,
    1,
    0,
    0,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    0,
    3,
    2,
    2,
    1,
    2,
    1,
    2,
    0,
    2,
    0,
    0,
    1,
    1,
    1,
    1,
    2,
    1,
    1,
    2,
    1,
    1,
    1,
    2,
    1,
    2,
    2,
    2,
    2,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    2,
    2,
    1,
    1,
    1,
    1,
    1,
    0,
    1,
    1,
    1,
    2,
    0,
    1,
    2,
    1,
    3,
    2,
    2,
    3,
    0,
    3,
    2,
    3,
    2,
    2,
    2,
    1,
    1,
    2,
    2,
    3,
    3,
    1,
    2,
    1,
    3,
    2,
    2,
    3,
    2,
    1,
    2,
    3,
    3,
    3,
    2,
    2,
    1,
    3,
    2,
    2,
    1,
    2,
    2,
    2,
    1,
    2,
    2,
    2,
    2,
    2,
    2,
    1,
]
labelsq = [
    1,
    0,
    0,
    0,
    0,
    1,
    0,
    0,
    0,
    0,
    1,
    0,
    0,
    0,
    2,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    1,
    0,
    0,
    0,
    1,
    1,
    0,
    0,
    1,
    1,
    1,
    0,
    0,
    1,
    0,
    0,
    1,
    0,
    0,
    0,
    0,
    1,
    0,
    1,
    0,
    1,
    0,
    3,
    3,
    3,
    1,
    3,
    1,
    2,
    0,
    3,
    1,
    0,
    2,
    2,
    2,
    1,
    3,
    1,
    2,
    2,
    1,
    2,
    2,
    2,
    2,
    3,
    3,
    3,
    3,
    2,
    1,
    1,
    1,
    2,
    2,
    1,
    2,
    3,
    2,
    1,
    1,
    1,
    2,
    2,
    0,
    1,
    1,
    1,
    2,
    1,
    1,
    2,
    2,
    3,
    2,
    3,
    3,
    0,
    3,
    3,
    3,
    3,
    3,
    3,
    1,
    2,
    3,
    3,
    3,
    3,
    2,
    3,
    1,
    3,
    2,
    3,
    3,
    2,
    2,
    3,
    3,
    3,
    3,
    3,
    2,
    2,
    3,
    2,
    3,
    2,
    3,
    3,
    3,
    2,
    3,
    3,
    3,
    2,
    3,
    2,
    2,
]
# test(clf4u, data["sepallength"], labelsu, title="IrisUniform")
# test(clf4q, data["sepallength"], labelsq, title="IrisQuantile")
sepallength = [[x] for x in data["sepallength"]]
clf4u.fit(sepallength)
clf4q.fit(sepallength)
computedu = clf4u.transform(sepallength)
computedq = clf4q.transform(sepallength)
wrongu = 0
wrongq = 0
for i in range(len(labelsu)):
    if labelsu[i] != computedu[i]:
        wrongu += 1
    if labelsq[i] != computedq[i]:
        wrongq += 1
print(f"Iris sepallength diff. between BinDisc & sklearn::KBins Uniform  ={wrongu:3d}")
print(f"Iris sepallength diff. between BinDisc & sklearn::KBins Quantile ={wrongq:3d}")
