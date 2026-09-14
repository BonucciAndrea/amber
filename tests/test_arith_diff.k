/ tests/test_arith_diff.k -- differential oracle for the arithmetic layer.
/ Prints the results (and types) of many arithmetic expressions over every
/ integer width, floats, ranges, atoms, nulls and overflow cases. Run it under
/ two binaries and diff the outputs: they must be byte-identical.
/   ./amber tests/test_arith_diff.k > new.txt ; /path/to/old/amber tests/test_arith_diff.k > old.txt ; diff old.txt new.txt
\l amber.k
N:1000
L1:(N?2000000)-1000000            / longs, signed
L2:(N?2000000)-1000000
I1:1000000!H:1048573!262147*!N     / ints (tI) 0..1048572
I2:900001!H
H1:1000!H                          / shorts
H2:997!H
G1:100!H                           / bytes
G2:97!H
F1:0.0+L1
F2:0.0+L2
R1:!N                              / range
R2:7+!N
B1:H1>500
B2:H2>300
S:0N 0N,3 4 5
sh:{"  ",($@x)," ",$x}
p:{`0:sh x}
/ ---- subtraction: every width pairing, atom/vector, ranges, floats, nulls --
p L1-L2; p H1-H2; p G1-G2; p I1-I2; p F1-F2
p H1-G2; p G1-H2; p I1-H2; p L1-I2; p F1-L2; p L1-F2; p H1-F2
p L1-5; p 5-L1; p H1-5; p 5-H1; p G1-5; p 5-G1; p I1-5; p 5-I1
p F1-2.5; p 2.5-F1; p H1-2.5; p 2.5-H1
p R1-3; p 3-R1; p R2-R1; p R1-L2
p B1-B2; p B1-1; p 1-B1; p H1-B2
p S-1; p 1-S; p S-S; p S-0N; p 0N-S
p 5-3; p 5-3.5; p 5.5-3; p 0N-3; p 3-0N; p 0W-1; p -0W-1
p (-128+!5)-(1+!5)                 / narrow overflow: bytes below -128
p (-32768+!5)-(1+!5)
p (-2147483648+!5)-(1+!5)
p (127-!5)-(-1-!5)
p (100#-100)-100#100               / byte subtraction crossing the width
p (1000#-30000)-1000#30000
p "abc"-"a"; p "abc"-1
p (1 2 3;4 5 6)-(1 1 1;2 2 2)
p (1 2 3;4 5)-1
p (`a`b!1 2)-1; p (`a`b!1 2)-`a`b!3 4
/ ---- multiplication: overflow re-run at every width, atom/vector -----------
p G1*G2; p H1*H2; p I1*I2; p L1*L2; p F1*F2
p G1*3; p 3*G1; p H1*300; p 300*H1; p I1*3000; p 3000*I1; p L1*7; p 7*L1
p (100#100)*100#100; p (100#120)*100#120; p (100#3000)*100#3000; p (100#300000)*100#300000
p (100#3037000500)*100#3037000500            / 64-bit wrap
p 1000000*I1; p -1000000*I1; p 0*L1; p -1*G1; p -1*H1
p S*2; p 2*S; p S*S; p 0N*0
p F1*2.5; p 2.5*F1; p H1*2.5
/ ---- mod: general modulus, power of two, negative modulus, every width ------
p 7!L1; p 7!I1; p 7!H1; p 7!G1; p 1000!L1; p 1000!I1; p 1000!H1; p 65536!L1; p 8!L1; p 8!H1
p 3!S; p 7!0N; p -7!L1; p -7!H1; p 1!L1; p 2147483647!L1; p 4294967311!L1
p 7!(-10+!20); p 7!(-10+!20); p 3!-5 -4 -3 -2 -1 0 1 2 3 4 5
p 1000!H
/ ---- addition regression (the checked kernels) ------------------------------
p G1+G2; p H1+H2; p I1+I2; p L1+L2; p (100#127)+100#1; p (100#32767)+100#1; p (100#2147483647)+100#1
p 5+G1; p 300+H1; p 100000+I1; p S+1; p R1+5; p B1+B2
/ ---- reductions and scans over narrow types --------------------------------
p +/G1; p +/H1; p +/I1; p +/L1; p +/F1; p |/F1; p &/F1; p |/H1; p &/H1
p +/B1; p +\G1; p +\H1; p -/H1; p */H1; p |/0n 1 2; p &/1 0n 2; p |/0#0.0
p +/0.0 -0.0; p |/-0.0 0.0; p &/-0.0 0.0
p +/1000#-0.0
