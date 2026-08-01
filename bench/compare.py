#!/usr/bin/env python3
import sys, os, glob
d=sys.argv[1]
eng={}
for f in sorted(glob.glob(os.path.join(d,'*.txt'))):
    name=os.path.basename(f)[:-4]; eng[name]={'S':{},'T':{}}
    for ln in open(f):
        p=ln.split()
        if len(p)>=3 and p[0]=='SANITY': eng[name]['S'][p[1]]=p[2]
        elif len(p)>=3 and p[0]=='TIME':
            try: eng[name]['T'][p[1]]=float(p[2])
            except: pass
if not eng: print("no engine output"); sys.exit(0)
names=list(eng); ref='amber' if 'amber' in eng else names[0]
# SANITY table
skeys=sorted({k for e in eng.values() for k in e['S']})
print("SANITY (vs %s)"%ref)
print("  %-12s %-22s %s"%("check",ref,"others (PASS if ~equal)"))
def close(a,b):
    try: return abs(float(a)-float(b))<=1e-6*max(1,abs(float(a)))
    except: return a==b
for k in skeys:
    rv=eng[ref]['S'].get(k,'-')
    others=[]
    for n in names:
        if n==ref: continue
        v=eng[n]['S'].get(k)
        if v is None: continue
        others.append("%s=%s[%s]"%(n,v,'PASS' if close(rv,v) else 'FAIL'))
    print("  %-12s %-22s %s"%(k,rv," ".join(others)))
# TIME table
tkeys=sorted({k for e in eng.values() for k in e['T']})
print("\nTIME  ms/op (lower is better)")
hdr="  %-10s"%"op"+"".join("%14s"%n for n in names); print(hdr)
for k in tkeys:
    row="  %-10s"%k
    for n in names:
        v=eng[n]['T'].get(k)
        row+="%14s"%(("%.2f"%v) if v is not None else "-")
    print(row)
