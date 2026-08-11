/ differential: all widths, all edge sizes, signed + wrapping data
r:()
f:{[v] r,:(*/v;&/v;|/v;+/v)}
sizes:0 1 2 3 4 5 7 8 15 16 17 63 64 65 1000 100003
{[n] f n#0 1 -1 2 -2 127 -128; f n#0N; f 3+!n; f n#-9223372036854775806 9223372036854775806 5; f n#255 -255 65535 -65535; f n#2147483647 -2147483648; f n#0.5 -1.25 3.0 }'sizes
`0:$r
