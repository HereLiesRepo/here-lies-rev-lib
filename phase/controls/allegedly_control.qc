# Allegedly (Confirmed) positive control: T on d^e before and after two Toffolis that each add ab to one of d, e.
# The two T gates sit on different parities (inputs, then the Toffolis' output variables) that are the same function;
# phase folding cannot merge them, Allegedly guesses the merge and the judge proves it: 10 T -> 8 T (S replaces T T).
.v a b d e
BEGIN
tof d e
T e
tof d e
tof a b d
tof a b e
tof d e
T e
tof d e
END
