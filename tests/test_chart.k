/ ============================================================================
/ tests/test_chart.k  -  terminal charting: temporal axes, tick labels, shape.
/ GNU AGPLv3 - see LICENSE and NOTICE.
/ ----------------------------------------------------------------------------
/     ./amber tests/test_chart.k          (exits non-zero on any failure)
/ ----------------------------------------------------------------------------
/ amber 2.2 gave the chart kernel an axis UNIT (src/i.c, AXN..AXS) so an axis
/ whose values are a time reads as a time instead of as the integer a temporal
/ column is stored as.  Amber's temporal types keep numeric storage inside a
/ column, so a VECTOR cannot carry its own unit -- it is either stated by the
/ caller (`xunit`/`yunit`) or inferred, and the inference is deliberately narrow.
/ These cases pin down: the unit codes, what `auto does and does NOT claim,
/ every label format, that the tick count adapts to the label width so labels
/ never overlap, and that none of it moves the plot area.
/ `plt is exercised directly (it RETURNS the rendered text); `chart` prints.
/ ============================================================================
DIR:{$[#p:"/"/-1_"/"\x;p;"."]}`argv 1
LOAD:{{. x^(`c$13)}'0:x;}
LOAD DIR,"/../amber.k"
LOAD DIR,"/../std.k"
LOAD DIR,"/../qsql.k"
LOAD DIR,"/../temporal.k"
LOAD DIR,"/../sys.k"
LOAD DIR,"/harness.k"

/ ---- helpers ---------------------------------------------------------------
/ render: y, x and the unit codes -> the chart as one char vector.
/ The spec is the canonical 10 slots; opt slot 6 is the x unit, 7 the y unit.
render:{[ys;xs;xu;yu;w;h]`plt (,`f$ys;$[0=#xs;();,`f$xs];(w;h;0;1;0;0;xu;yu);0n 0n 0n 0n;"";"";"";();0#0;0#0)}
has:{[s;sub]0<#ss[s;sub]}
/ the row of x tick labels is the last non-empty line of the rendering
lastrow:{[s]l:"\n"\s;l@|/&{0<#x}'l}
/ every FRAMED line has the same DISPLAY width (vlen, not #: a braille glyph is
/ three bytes, so byte length differs line to line while the frame is square)
widths:{[s]?vlen'(1_-2_"\n"\s)}
/ the lengths of the non-space runs in a label row.  Two labels that touched
/ would merge into one run twice as long as any label this unit can produce,
/ so bounding the longest run IS the no-overlap check.
runs:{[r]m:~r=" ";b:&differ m;$[0=#b;0#0;(#'b _ m)@&m b]}

SIN:100+10*{sin x%25}@!200
/ a full session in milliseconds of day, 09:30:00 .. 16:00:00
MST:hms[9;30;0]+_(hms[16;0;0]-hms[9;30;0])*(!200)%199
/ the same shape in seconds of day
SEC:_MST%1000
/ days since 2000.01.01, from 2024.01.01 (day 8766), weekly
DAY:8766+7*!60
/ nanoseconds since 2000.01.01 on 2026.09.18, one session
NSD:(86400000000000*9757)+(1000000*hms[9;30;0])+_23400000000000.0*(!200)%199
/ the same, spanning nine days
NSW:(86400000000000*9757)+_(9.0*86400000000000)*(!200)%199

/ ---- 1. the unit table and the codes it maps to -----------------------------
t["unit num";      chunit`num;        0]
t["unit time";     chunit`time;       1]
t["unit ms";       chunit`ms;         1]
t["unit date";     chunit`date;       2]
t["unit stamp";    chunit`stamp;      3]
t["unit ns";       chunit`ns;         3]
t["unit sec";      chunit`sec;        4]

/ ---- 2. chauto claims a unit ONLY where it cannot be mistaken ---------------
t["auto ms session";   chauto`f$MST;      1]
t["auto ns";           chauto`f$NSD;      3]
t["auto ns wide";      chauto`f$NSW;      3]
t["auto plain index";  chauto`f$!200;     0]   / a 0-based index is NOT a clock
t["auto small counts"; chauto 1.0 2 3;    0]
t["auto fractional";   chauto 0.5+`f$MST; 0]   / not whole -> not a clock
t["auto days";         chauto`f$DAY;      0]   / ambiguous with counts: stays 0
t["auto empty";        chauto 0#0.0;      0]
/ a session that starts before 01:00 is ambiguous with an index, so auto
/ declines it -- that is exactly what xunit:`time is for
t["auto near midnight";chauto`f$_(hms[2;0;0])*(!200)%199; 0]

/ ---- 3. chuk: explicit unit wins, `auto defers to chauto --------------------
t["chuk explicit";     chuk[`time;,`f$!200];  1]
t["chuk auto defers";  chuk[`auto;,`f$!200];  0]
t["chuk auto finds ms";chuk[`auto;,`f$MST];   1]
t["chuk unknown name"; chuk[`nonsense;,`f$MST];0]  / unknown -> plain number
t["chuk empty";        chuk[`time;()];        0]

/ ---- 4. every label format actually appears in the rendering ---------------
tv["ms label 10:00";   "has[render[SIN;MST;1;0;76;10];\"10:00:00\"]";  1b]
tv["ms label 16:00";   "has[render[SIN;MST;1;0;76;10];\"16:00:00\"]";  1b]
tv["ms not raw int";   "has[render[SIN;MST;1;0;76;10];\"34200000\"]";  0b]
tv["sec label";        "has[render[SIN;SEC;4;0;76;10];\"10:00:00\"]";  1b]
tv["date label";       "has[render[SIN@!60;DAY;2;0;76;10];\"2024.\"]"; 1b]
tv["date full";        "has[render[SIN@!60;DAY;2;0;76;10];\"2024.03.02\"]"; 1b]
tv["ns intraday clock";"has[render[SIN;NSD;3;0;76;10];\"12:00:00\"]";  1b]
tv["ns wide is dated"; "has[render[SIN;NSW;3;0;76;10];\"09.\"]";       1b]
/ A numeric axis prints numbers, and a clock label is the only thing on these
/ charts that contains a colon -- so "is there a colon in the tick row" is the
/ robust test in both directions.  (The exact numbers on a numeric axis are the
/ 1-2-5 ladder's, not the data's minimum, so asserting "34200000" would be
/ asserting the wrong thing.)
tv["numeric has no clock"; "has[lastrow render[SIN;MST;0;0;76;10];\":\"]"; 0b]
tv["numeric has digits";   "0<#runs lastrow render[SIN;MST;0;0;76;10]";    1b]

/ ---- 5. a time on the Y axis -----------------------------------------------
/ H=10 gives three y ticks over a 6.5-hour span, so the step lands on 4 hours
/ and the labels are 12:00:00 / 16:00:00.
tv["y unit time";      "has[render[MST;();0;1;76;10];\"12:00:00\"]";   1b]
tv["y numeric no clock";"has[render[MST;();0;0;76;10];\":\"]";         0b]

/ ---- 6. labels never overlap ----------------------------------------------
/ No run of non-space in the tick row may be longer than one label of that
/ unit: two labels that touched would merge into a single double-length run.
tv["ms no overlap";    "8<|/runs lastrow render[SIN;MST;1;0;76;10]";   0b]
tv["sec no overlap";   "8<|/runs lastrow render[SIN;SEC;4;0;76;10]";   0b]
tv["date no overlap";  "10<|/runs lastrow render[SIN@!60;DAY;2;0;76;10]"; 0b]
tv["ns no overlap";    "19<|/runs lastrow render[SIN;NSD;3;0;76;10]";  0b]
tv["ns wide no overlap";"19<|/runs lastrow render[SIN;NSW;3;0;76;10]"; 0b]
/ ... and at least one label is actually drawn, i.e. the packer did not solve
/ the overlap problem by dropping everything
tv["ms labels present";  "0<#runs lastrow render[SIN;MST;1;0;76;10]";  1b]
tv["ns labels present";  "0<#runs lastrow render[SIN;NSW;3;0;76;10]";  1b]
/ the widest labels on the narrowest charts must still render, not corrupt
tk["narrow ns chart";    "render[SIN;NSD;3;0;20;4]"]
tk["narrow date chart";  "render[SIN@!60;DAY;2;0;20;4]"]
tk["tiny chart";         "render[SIN;MST;1;0;8;2]"]

/ ---- 7. the plot SHAPE does not move with the unit --------------------------
/ Same series, same size, any unit: same row count and the frame stays square.
/ The gutter absorbs the label width; the plot area does not.
tv["rows unit-invariant";  "(#\"\\n\"\\render[SIN;MST;1;0;76;10])=#\"\\n\"\\render[SIN;MST;0;0;76;10]"; 1b]
tv["frame square ms";      "1=#widths render[SIN;MST;1;0;76;10]"; 1b]
tv["frame square numeric"; "1=#widths render[SIN;MST;0;0;76;10]"; 1b]
tv["frame square date";    "1=#widths render[SIN@!60;DAY;2;0;76;10]"; 1b]

/ ---- 8. an out-of-range unit code degrades to plain numbers, never crashes --
tv["unit code 99";  "has[lastrow render[SIN;MST;99;0;76;10];\":\"]"; 0b]
tv["unit code -1";  "has[lastrow render[SIN;MST;-1;0;76;10];\":\"]"; 0b]

/ ---- 9. candlesticks: the 4-slot spec still works, 6-slot adds the axis -----
OP:181.0 182 183 182.5
HI:183.0 184 184.5 183
LO:180.0 181 182 181.5
CL:182.0 183 182.5 182
tk["cdl 4-slot";  "`cdl (OP;HI;LO;CL)"]
tk["cdl 6-slot";  "`cdl (OP;HI;LO;CL;`f$hms[9;30;0]+0 600000 1200000 1800000;1)"]
tv["cdl time axis";  "has[`cdl (OP;HI;LO;CL;`f$hms[9;30;0]+0 600000 1200000 1800000;1);\"09:30:00\"]"; 1b]
tv["cdl no axis when unit 0"; "has[`cdl (OP;HI;LO;CL;`f$hms[9;30;0]+0 600000 1200000 1800000;0);\"09:30:00\"]"; 0b]
tv["cdl empty times ok"; "0<#`cdl (OP;HI;LO;CL;();0)"; 1b]
/ the price gutter is a fixed number of decimals, not a truncated exact double
tv["cdl price not truncated"; "has[`cdl (OP;HI;LO;CL);\"182.3342\"]"; 0b]
te["cdl wrong arity";  "`cdl (OP;HI;LO)"]
te["cdl 5 slots";      "`cdl (OP;HI;LO;CL;OP)"]

/ ---- 10. the shorthand verbs -----------------------------------------------
tk["tplot";  "tplot (MST;SIN)"]
tk["dplot";  "dplot (DAY;SIN@!60)"]
tk["pplot";  "pplot (NSD;SIN)"]
tk["chart xunit";  "chart `x`y`w`h`xunit!(MST;SIN;60;6;`time)"]
tk["chart yunit";  "chart `y`w`h`yunit!(MST;60;6;`time)"]

hexpect[62]
hreport"AMBER CHART / TEMPORAL AXIS"
$[FAILN;`exit 1;]
