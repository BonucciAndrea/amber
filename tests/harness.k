/ ============================================================================
/ tests/harness.k  -  shared assertion harness for Amber's test suites.
/ GNU AGPLv3 - see LICENSE and NOTICE.
/ ----------------------------------------------------------------------------
/ Load this FIRST, then the suite body, then call  hreport["NAME"].
/   t  [tag; got; want]        assert  got ~ want
/   tv [tag; "expr"; want]     assert  (. expr) ~ want  -- expr evaluated INSIDE
/                              the trap, so a throwing query cannot abort the file
/   te [tag; "expr"]           assert  expr raises a clean runtime error
/   tk [tag; "expr"]           assert  expr evaluates without error (value ignored)
/ Every assertion is trapped with .[f;args;handler], so one failing (or
/ throwing) case can never abort the suite or hide the cases after it.
/ NOTE: never put a bare "/" on a line of its own in a K source file -- that
/ opens a BLOCK comment that runs to the next line starting with "\", which
/ silently swallows the rest of the file.
/ ============================================================================
R:()
hs:{s:.[{$[`C=@x;x;`k x]};,x;{"<unprintable>"}];$[40<#s;(40#s),"..";s]}
hpush:{[tag;ok;msg]R,:,($tag;$[ok;"ok";msg]);}
t:{[tag;got;want]e:.[{(1b;x~y)};(got;want);{(0b;x)}];
   $[e 0;hpush[tag;e 1;"FAIL got<",(hs got),"> want<",(hs want),">"];hpush[tag;0b;"FAIL raised while comparing"]]}
tv:{[tag;s;want]e:.[{(1b;. x)};,s;{(0b;x)}];
   $[e 0;hpush[tag;(e 1)~want;"FAIL got<",(hs e 1),"> want<",(hs want),"> from: ",s];hpush[tag;0b;"FAIL raised: ",s]]}
te:{[tag;s]e:.[{(1b;. x)};,s;{(0b;x)}];hpush[tag;~e 0;"FAIL expected an error from: ",s]}
tk:{[tag;s]e:.[{(1b;. x)};,s;{(0b;x)}];hpush[tag;e 0;"FAIL unexpected error from: ",s]}
hreport:{[nm]f:R@&{"FAIL"~4#x}'R[;1];
 `0:"";
 `0:"================ ",nm," ================";
 `0:(($#R)," tests run, ",($#f)," failures");
 $[#f;{`0:"  ",(x 0),": ",x 1}'f;`0:"ALL TESTS PASSED"];
 `0:"=========================================================";}
