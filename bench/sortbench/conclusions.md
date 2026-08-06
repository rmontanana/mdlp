## Why this study exists

The cross-platform benchmark kept showing the same three operations slower on
Linux than on macOS, on **identical data** (verified by dataset checksum) built
from **identical sources** (verified by source hash). That is not a difference the
library can explain about itself, and it was not small: `BinDisc::fit (quantile)`
ran about 3× slower, `CPPFImdlp::transform` 2.3-2.6× slower.

For a while it did not matter much. `CPPFImdlp::fit` was quadratic and dwarfed
everything by four orders of magnitude, so the AMD machines' raw throughput won by
a third overall and the penalty stayed buried in operations that cost microseconds.

Phase 7 changed that. Removing the quadratic term made `fit` roughly 500× faster
and left it **dominated by `std::stable_sort`** — measured here at **37%** of `fit` at
n = 100 000 (6.01 ms of 16.27 ms on the M4 Max). With the masking gone, the toolchain penalty surfaced in `fit` itself:
Linux went from **0.67× (faster)** than the M4 Max to **1.10× (slower)**. An
operation that had been noise became the main remaining cost.

That mattered practically. This library is the discretization stage for research on
discrete Bayesian classifiers, and that research runs on Linux. A 3× penalty on
`PKIDisc` — which always selects the QUANTILE strategy and therefore always
sorts — deserved an explanation rather than a shrug.

**The project cannot answer the question itself.** libtorch arrives from conan
prebuilt against libstdc++, and libstdc++ and libc++ have incompatible ABIs for
`std::string` and `std::vector`, so mdlp cannot be built against libc++ without
rebuilding libtorch from source. Compiler and standard library are two variables
and the build system cannot separate them.

Hence a standalone program with **zero dependencies**, which one machine can
compile several ways.

## How it was measured

Four replicas of the library's hot loops, on the same portable dataset generator
the library benchmark uses, with fixed repetition counts and the median as the
statistic:

| Shape | Replicates |
|---|---|
| `sort<float>` | `BinDisc::fit_quantile` — sorts a by-value copy |
| `stable_sort<index>` | `CPPFImdlp::sortIndices` — indirect comparator, bounds checks included |
| `transform(fnptr)` | `Discretizer::transform` as written, bound function held in a pointer |
| `transform(direct)` | the same with the branch hoisted so the call can inline |

**Limitation, stated plainly:** these are replicas, not the library. A result here
is a strong hypothesis about a cause, not a measurement of mdlp. Confirming one
inside the library would need the ABI-compatible build that is precisely what is
unavailable.

## What was found

**1. GCC's default target, not the compiler and not its version.** Building with
`-march=native` brings GCC to within 0.1-0.9% of clang on three of four shapes. The
gap is GCC's generic x86-64 instruction scheduling, which suits Zen 4 — the
7950X3D, where no gap ever appeared — and does not suit Zen 5. For scale, a
GCC 15 → 16 upgrade moved the same measurement by 0.4%; `-march=native` moves it by
31%.

**2. libstdc++'s `std::sort` is slower, independently and unresolved.** After
`-march=native` has done its work, libc++ still sorts floats **2.9× faster**. The
two effects do not overlap. The same library also produces a superlinear jump when
n grows from 1 000 to 10 000 — 59-122× under libstdc++ against 13× under libc++ on
every machine tested. This is what makes `BinDisc::fit (quantile)`, and therefore
`PKIDisc`, about 3× slower on Linux. It has no cheap remedy: libtorch's ABI rules
out libc++.

**3. `transform`'s gap is not the toolchain.** The *fastest* Linux build lands at
1.064 ms at n = 100 000 against macOS's 0.223 ms — **4.8×** — and the slowest at
1.834 ms. A gap of that size survives
holding compiler family, compiler version, standard library and tuning constant, so
it is microarchitectural. This diagnostic **ruled the toolchain out** rather than
confirming it, which is a useful result even though it is a negative one. The
related function-pointer hypothesis is dead too: `transform(fnptr)` and
`transform(direct)` measure within noise of each other on every build on every
machine.

## How the answer was reached, including the wrong turns

Four explanations were proposed and three were wrong. They are recorded because
the reasoning errors are worth not repeating.

| Hypothesis | Outcome |
|---|---|
| libstdc++'s sort explains everything | **Partly right.** It explains `sort<float>` and nothing else |
| The allocator behind the by-value copy | **Wrong.** Eliminating the copy with the Phase 5 move overload changed nothing (0.1-2.1%) |
| GCC 15 is worse than GCC 16 | **Wrong.** Upgrading moved the number 0.4% |
| GCC's default target is wrong for this CPU | **Right.** `-march=native` closes the gap |

The GCC-version error is the instructive one. Two machines differed in verdicts,
and they differed in **both** hardware and compiler release. Clang measuring within
4-13% across them looked like a hardware control, so the GCC gap was attributed to
the release. But the data only supported the weaker claim — *GCC behaves
differently on these two machines* — with the version as one candidate among
several. It was presented as a conclusion when it was a hypothesis with an
insufficient control.

What produced the real answer was holding one variable fixed at a time: repeating
runs on one machine to establish that the effect was reproducible rather than
noise, then upgrading GCC to eliminate the version, then adding a `-march=native`
build to test tuning directly.

## What to do about it

**Build with `-march=native` on Zen 5.** It recovers 30-35% across every shape, needs
no toolchain change and no ABI compatibility work. The binary stops being portable
to other CPUs, which is acceptable on a fixed research machine and not acceptable
for a distributed package.

**For `PKIDisc`-heavy work, know the cost.** The remaining ~2.9× on `std::sort` is a
libstdc++ property with no cheap fix. If it ever becomes the bottleneck, the answer
is to replace that one `std::sort` call rather than to change toolchain — a
replacement would help every platform. It is a far cheaper code path than
`CPPFImdlp::fit`, so it is not where optimisation should start.

**Nothing to do about `transform`.** The toolchain is ruled out and the remaining
cause is beyond what this diagnostic can reach.

## Still open

- **`transform`'s 5× cross-platform gap** has no explanation.
- **The 7950X3D has no `gcc_native` run**, because that build was added after it was
  measured. Whether `-march=native` also helps on Zen 4 — where GCC's generic
  scheduling already performs well — is untested.
- **One run per build per machine**, apart from the Strix Halo which was measured
  four times. Single cells should be treated with suspicion; the patterns above hold
  across sizes and across machines.
