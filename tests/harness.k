/ ============================================================================
/ tests/harness.k  -  shared assertion harness for Amber's test suites.
/ GNU AGPLv3 - see LICENSE and NOTICE.
/ ----------------------------------------------------------------------------
/ Assertions
/   t  [tag; got; want]     assert  got ~ want          (got evaluated by caller)
/   tv [tag; "expr"; want]  assert  (. expr) ~ want     (expr evaluated INSIDE the trap)
/   te [tag; "expr"]        assert  expr raises a clean runtime error
/   tk [tag; "expr"]        assert  expr evaluates without error (value ignored)
/   hexpect[n]              assert exactly n assertions have been recorded
/   hreport["NAME"]         print the summary, restore diagnostics, set FAILN
/ Every assertion is trapped with .[f;args;handler], so one failing (or throwing)
/ case can never abort the suite or hide the cases after it.
/ ----------------------------------------------------------------------------
/ QUIET BY DESIGN.  Amber renders its Rust-style diagnostic at error-CREATION
/ time, so a suite full of deliberate must-raise cases used to splash hundreds
/ of red error blocks across the terminal for errors it caught and handled
/ perfectly -- the summary was correct but the run looked catastrophic.  Loading
/ this file turns that report off with `diag 0 (a runtime switch added in 1.9)
/ and hreport turns it back on.  The compact caret text is NOT suppressed: it is
/ still handed to the trap handler, so a genuinely unexpected error is reported
/ in full on the FAIL line for that case.
/ ----------------------------------------------------------------------------
/ THREE K GOTCHAS THIS FILE DELIBERATELY AVOIDS - each silently broke an earlier
/ revision of these suites:
/   1. A bare "/" alone on a line opens a BLOCK comment that runs to the next
/      line starting with "\", swallowing the rest of the file with no error.
/      Every comment line here therefore starts with "/ " or "/x", never "/".
/   2. "f [a;b]" WITH A SPACE is not a call - K reads "[a;b]" as a bracketed
/      statement block and quietly builds a projection f[b;] that is then
/      thrown away.  No error, no output: the assertion simply never runs.  An
/      earlier tests/test_qsql.k lost 42 of its 93 cases to exactly this and
/      still printed "ALL TESTS PASSED".  Always write "f[a;b]", and call
/      hexpect[n] so a silently skipped block is caught rather than celebrated.
/   3. Definitions are ONE PER LINE, no lambda split across lines, because the
/      suites load their modules line-by-line (the same way repl.k does) so they
/      can run from any working directory.
/ ============================================================================
HDIAG:`diag 0
R:()
hs:{s:.[{$[`C=@x;x;`k x]};,x;{"<unprintable>"}];$[40<#s;(40#s),"..";s]}
he:{s:.[{$[`C=@x;x;$x]};,x;{"?"}];l:"\n"\s;$[#l;*l;""]}
hpush:{[tag;ok;msg]R,:,($tag;$[ok;"ok";msg]);}
t:{[tag;got;want]e:.[{(1b;x~y)};(got;want);{(0b;x)}];$[e 0;hpush[tag;e 1;"FAIL got<",(hs got),"> want<",(hs want),">"];hpush[tag;0b;"FAIL raised while comparing: ",he e 1]]}
tv:{[tag;s;want]e:.[{(1b;. x)};,s;{(0b;x)}];$[e 0;hpush[tag;(e 1)~want;"FAIL got<",(hs e 1),"> want<",(hs want),">  <-  ",s];hpush[tag;0b;"FAIL raised: ",(he e 1),"  <-  ",s]]}
te:{[tag;s]e:.[{(1b;. x)};,s;{(0b;x)}];hpush[tag;~e 0;"FAIL expected an error  <-  ",s]}
tk:{[tag;s]e:.[{(1b;. x)};,s;{(0b;x)}];$[e 0;hpush[tag;1b;""];hpush[tag;0b;"FAIL unexpected error: ",(he e 1),"  <-  ",s]]}
FAILN:0
hexpect:{[n]hpush[`assertionCount;n=#R;"FAIL only ",($#R)," of ",($n)," assertions ran - a block was silently skipped (see the \"f [a;b]\" note in tests/harness.k)"];}
hreport:{[nm]`diag HDIAG;f:R@&{"FAIL"~4#x}'R[;1];FAILN::#f;`0:"";`0:"================ ",nm," ================";`0:(($#R)," tests run, ",($#f)," failures");$[#f;{`0:"  ",(x 0),": ",x 1}'f;`0:"ALL TESTS PASSED"];`0:"=========================================================";}
