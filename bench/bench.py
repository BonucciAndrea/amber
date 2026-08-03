#!/usr/bin/env python3
# Prints "SANITY name value" and "TIME name ms" for each available Python engine.
import time, sys
N=1_000_000
def timeit(f,reps):
    f(); t0=time.perf_counter()
    for _ in range(reps): r=f()
    return (time.perf_counter()-t0)/reps*1000

def run_numpy_pandas():
    import numpy as np, pandas as pd
    i=np.arange(N,dtype=np.int64)
    px=(np.int64(2654435761)*i % 100000)/1000.0
    sym=(i%10).astype(np.int64); v=(np.int64(2654435761)*i)%100000
    print("SANITY sum",float(px.sum()))
    print("SANITY count_gt50",int((px>50).sum()))
    print("SANITY median",float(np.sort(px)[N//2]))
    print("SANITY rollchk",float(pd.Series(px).rolling(100,min_periods=1).mean().sum()))
    print("SANITY distinct",int(np.unique(v).size))
    print("TIME sum",round(timeit(lambda:float(px.sum()),30),4))
    print("TIME filter",round(timeit(lambda:int((px>50).sum()),30),4))
    df=pd.DataFrame({'sym':sym,'px':px})
    print("TIME groupby",round(timeit(lambda:df.groupby('sym')['px'].sum(),10),4))
    print("TIME rolling",round(timeit(lambda:pd.Series(px).rolling(100,min_periods=1).mean().sum(),10),4))
    print("TIME sort",round(timeit(lambda:np.sort(px),10),4))
    print("TIME distinct",round(timeit(lambda:np.unique(v),10),4))
    M=50000
    ti=np.arange(M,dtype=np.int64)
    tr=pd.DataFrame({'sym':ti%10,'time':np.sort(np.int64(2654435761)*ti%1000000),'px':(np.int64(40503)*ti%10000)/100}).sort_values('time')
    qu=pd.DataFrame({'sym':ti%10,'time':np.sort(np.int64(6700417)*ti%1000000),'bid':(np.int64(2246822519)*ti%10000)/100}).sort_values('time')
    print("TIME asof",round(timeit(lambda:pd.merge_asof(tr,qu,on='time',by='sym'),10),4))

def run_duckdb():
    import duckdb, numpy as np
    i=np.arange(N,dtype=np.int64); px=(np.int64(2654435761)*i%100000)/1000.0; sym=i%10
    con=duckdb.connect(); con.register('t', __import__('pandas').DataFrame({'sym':sym,'px':px}))
    print("TIME groupby",round(timeit(lambda:con.execute("select sym,sum(px) from t group by sym").fetchall(),10),4))
    print("TIME filter",round(timeit(lambda:con.execute("select count(*) from t where px>50").fetchall(),30),4))
    print("TIME distinct",round(timeit(lambda:con.execute("select count(distinct cast(px*1000 as bigint)) from t").fetchall(),10),4))

def run_polars():
    import polars as pl, numpy as np
    i=np.arange(N,dtype=np.int64); px=(np.int64(2654435761)*i%100000)/1000.0; sym=i%10
    df=pl.DataFrame({'sym':sym,'px':px})
    print("TIME groupby",round(timeit(lambda:df.group_by('sym').agg(pl.col('px').sum()),10),4))
    print("TIME sort",round(timeit(lambda:df.select(pl.col('px').sort()),10),4))
    print("TIME distinct",round(timeit(lambda:df.select(pl.col('px').n_unique()),10),4))

engines={'numpy_pandas':run_numpy_pandas,'duckdb':run_duckdb,'polars':run_polars}
want=sys.argv[1] if len(sys.argv)>1 else 'numpy_pandas'
try:
    engines[want]()
except ImportError as e:
    print(f"SKIP {want} ({e})",file=sys.stderr); sys.exit(2)
