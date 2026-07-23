/ ============================================================================
/ ipc.k  -  IPC and a tick architecture for Amber.  Loaded after hdb.k.
/   hopen/hclose/hsend/hrecv/hsync   raw-socket messaging (needs a peer)
/   u.*                              an in-process tickerplant (pub / sub)
/ NOTE on IPC: Amber's core speaks raw sockets (text), not the kdb+ binary
/ wire protocol, so hsync exchanges plain Amber expressions/results as text.
/ The tickerplant below is fully in-process and needs no network.
/ ============================================================================

/ ---- raw-socket IPC -------------------------------------------------------
hopen:{<`$x}                                 / open  "host:port" | ":port"  -> handle
hclose:{>x}                                  / close a handle
hsend:{[h;m]h 1:m;h}                         / async: write text m to handle h
hrecv:{`c$1:x}                               / read text from handle x
hsync:{[h;q]h 1:q;`c$1:h}                    / sync: send query q, read the reply text

/ ---- in-process tickerplant : publish/subscribe over named tables --------
u.tabs:(0#`)!()                              / table name -> accumulated table
u.subs:(0#`)!()                              / table name -> list of subscriber callbacks
ucat:{[t;r]cs:cols t;+cs!{[t;r;c]((+t)c),(+r)c}[t;r]'cs}                  / row-append two like tables
u.def:{[nm;t]u.tabs::u.tabs,(1#nm,())!,t;u.subs::u.subs,(1#nm,())!,();}   / define a stream
u.sub:{[nm;cb]u.subs::@[u.subs;nm;:;(u.subs nm),cb];nm}                   / subscribe a callback cb[nm;data]
u.pub:{[nm;data]u.tabs::@[u.tabs;nm;:;ucat[u.tabs nm;data]];{[nm;data;cb]cb . (nm;data)}[nm;data]'u.subs nm;data}   / publish rows
u.get:{u.tabs x}                             / current accumulated table for a stream
u.end:{[nm]u.tabs nm}                        / end-of-day snapshot (returns the table)
