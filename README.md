[![Build](https://github.com/rmontanana/mdlp/actions/workflows/build.yml/badge.svg)](https://github.com/rmontanana/mdlp/actions/workflows/build.yml)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=rmontanana_mdlp&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=rmontanana_mdlp)
[![Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=rmontanana_mdlp&metric=reliability_rating)](https://sonarcloud.io/summary/new_code?id=rmontanana_mdlp)

# mdlp

Discretization algorithm based on the paper by Fayyad &amp; Irani [Multi-Interval Discretization of Continuous-Valued Attributes for Classification Learning](https://www.ijcai.org/Proceedings/93-2/Papers/022.pdf)

The implementation tries to mitigate the problem of different label values with the same value of the variable:

- Sorts the values of the variable using the label values as a tie-breaker
- Once found a valid candidate for the split, it checks if the previous value is the same as actual one, and tries to get previous one, or next if the former is not possible.

Other features:

- Intervals with the same value of the variable are not taken into account for cutpoints.
- Intervals have to have more than two examples to be evaluated.

The algorithm returns the cut points for the variable.

## Sample

To run the sample, just execute the following commands:

```bash
cd sample
cmake -B build
cd build
make
./sample -f iris -m 2
./sample -h
```

## Test

To run the tests and see coverage (llvm & gcovr have to be installed), execute the following commands:

```bash
cd tests
./test
```
