[![Build](https://github.com/rmontanana/mdlp/actions/workflows/build.yml/badge.svg)](https://github.com/rmontanana/mdlp/actions/workflows/build.yml)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=rmontanana_mdlp&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=rmontanana_mdlp)
[![Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=rmontanana_mdlp&metric=reliability_rating)](https://sonarcloud.io/summary/new_code?id=rmontanana_mdlp)
[![Coverage Badge](https://img.shields.io/badge/Coverage-96,1%25-green)](html/index.html)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/rmontanana/mdlp)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.14245443.svg)](https://doi.org/10.5281/zenodo.14245443)

# <img src="logo.png" alt="logo" width="50"/> mdlp

Discretization algorithm based on the paper by Fayyad &amp; Irani [Multi-Interval Discretization of Continuous-Valued Attributes for Classification Learning](https://www.ijcai.org/Proceedings/93-2/Papers/022.pdf)

The implementation tries to mitigate the problem of different label values with the same value of the variable:

- Sorts the values of the variable using the label values as a tie-breaker
- Once found a valid candidate for the split, it checks if the previous value is the same as actual one, and tries to get previous one, or next if the former is not possible.

Other features:

- Intervals with the same value of the variable are not taken into account for cutpoints.
- Intervals have to have more than two examples to be evaluated (mdlp).

- The algorithm returns the cut points for the variable.

- The transform method uses the cut points returning its index in the following way:

        cut[i - 1] <= x < cut[i]

    using the [std::upper_bound](https://en.cppreference.com/w/cpp/algorithm/upper_bound) method

- K-Bins discretization is also implemented, and "quantile" and "uniform" strategies are available.

## Sample

To run the sample, just execute the following commands:

```bash
make build
build_release/sample/sample -f iris -m 2
build_release/sample/sample -h
```

## Test

To run the tests and see coverage (llvm with lcov and genhtml have to be installed), execute the following commands:

```bash
make test
```
