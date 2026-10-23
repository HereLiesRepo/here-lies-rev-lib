# Nonlinear-output control (OPT-PATHVERIFY-NONLINEAR-OUTPUTS-20261022): RevLib record misex1_241, its own gates (a classical
# reversible circuit, X and multi-control Toffolis). Its compilation to 2-control Toffolis is nonlinear_output_control.out.qc.
# pathverify proves them EQUAL only by substituting nonlinear polynomials into output wires; before that it answered UNKNOWN.
.v f6 f5 f4 f3 f2 f1 f0 x7 x6 x5 x4 x3 x2 x1 x0
.i f6 f5 f4 f3 f2 f1 f0 x7 x6 x5 x4 x3 x2 x1 x0
BEGIN
X x0
tof x0 x1 x2 x3 f0
tof f0 f2
tof f2 f4
tof f4 f5
tof f5 f6
X x1
tof x0 x1 x2 f1
tof f1 f4
tof f1 f5
X x0
X x3
tof x0 x1 x2 x3 f0
tof f6 f3
tof f0 f3
tof f3 f4
tof f3 f5
tof f3 f6
X x0
tof f5 x0 x3 x5 f1
tof f6 f5
tof f1 f5
tof f6 f2
tof f5 f6
tof f6 f5
tof f5 f3
tof f4 f3
tof x0 x1 x3 f1
tof f4 f2
tof f1 f2
tof f1 f3
tof f1 f5
tof x0 x1 x3 f6
X x3
X x2
tof x0 x2 x3 f2
tof x0 x2 f1
tof f5 f4
tof f1 f4
tof x0 x2 f5
tof x0 x2 f6
tof x1 x2 x3 f1
tof f5 f2
tof f1 f2
tof f0 f2
tof f5 f4
tof f1 f4
tof x1 x2 x3 f5
tof x1 x2 x3 f6
X x1
X x3
tof x0 x2 x3 f3
tof f3 x4 x1 f2
tof f2 x3 x1 f3
tof f2 x3 x1 f6
X x1
tof x0 x1 x2 x3 x6 f1
tof f5 f2
tof f1 f2
tof f0 f2
tof f1 x0 x1 x2 f5
X x1
X x7
tof x1 x7 f1 f3
tof f6 x3 x1 x7 f3
END
