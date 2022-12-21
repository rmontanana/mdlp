# mdlp
Discretization algorithm based on the paper by Fayyad &amp; Irani [Multi-Interval Discretization of Continuous-Valued Attributes for Classification Learning](https://www.ijcai.org/Proceedings/93-2/Papers/022.pdf)

The implementation tries to mitigate the problem of different label values with the same value of the variable:

- Sorts the values of the variable using the label values as a tie-breaker
- Once found a valid candidate for the split, it checks if the previous value is the same as actual one, and tries to get previous one, or next if the former is not possible.

The algorithm returns the cut points for the variable.