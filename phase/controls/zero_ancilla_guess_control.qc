# Allegedly under --zero-ancillas: d and e start at 0, so after two Toffolis both hold ab and the two T gates are one
# function; as full unitaries they are not (d, e arbitrary). Measured: --zero-ancillas 10 T -> 8 T, full unitary 10 T.
.v a b d e
.i a b
BEGIN
tof a b d
tof a b e
T d
T e
END
