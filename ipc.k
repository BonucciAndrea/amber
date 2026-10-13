/ ============================================================================
/ ipc.k  -  IPC and a tick architecture for Amber.  Loaded after hdb.k.
/   hopen/hclose                     q's IPC: handles to q (or Amber) processes, and logs
/   u.*                              an in-process tickerplant (pub / sub)
/ IPC is q's wire protocol (src/qipc.c), so Amber and q processes talk to each other.
/ The u.* tickerplant below is in-process and needs no network.
/ ============================================================================

/ ---- q's IPC (2.9.0, src/qipc.c) --------------------------------------------
/ hopen 5000, hopen`:host:5000, hopen`:host:5000:user:pass: a handle to a q process (or Amber); h"expr" and h(f;a;b)
/ wait for the answer, neg[h] x sends and goes on, h[] waits for the next message (a deferred answer); hopen`:path
/ opens a log to append to (h enlist msg), as q's tickerplant does. \p 5000 or -p 5000 listens, with .z.pg .z.ps
/ .z.po .z.pc and .z.w (sys.k); \t 100 calls .z.ts every 100 ms; -11!`:log replays a log
hopen:{`qho x}                               / a handle: a port, `:host:port[:user:pass], or a log file
hclose:{`qhc x}                              / close it

/ ---- in-process tickerplant : publish/subscribe over named tables --------
u.tabs:(0#`)!()                              / table name -> accumulated table
u.subs:(0#`)!()                              / table name -> list of subscriber callbacks
ucat:{[t;r]cs:cols t;+cs!{[t;r;c]((+t)c),(+r)c}[t;r]'cs}                  / row-append two like tables
u.def:{[nm;t]u.tabs::u.tabs,(1#nm,())!,t;u.subs::u.subs,(1#nm,())!,();}   / define a stream
u.sub:{[nm;cb]u.subs::@[u.subs;nm;:;(u.subs nm),cb];nm}                   / subscribe a callback cb[nm;data]
u.pub:{[nm;data]u.tabs::@[u.tabs;nm;:;ucat[u.tabs nm;data]];{[nm;data;cb]cb . (nm;data)}[nm;data]'u.subs nm;data}   / publish rows
u.get:{u.tabs x}                             / current accumulated table for a stream
u.end:{[nm]u.tabs nm}                        / end-of-day snapshot (returns the table)
