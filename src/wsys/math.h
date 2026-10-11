/* minimal freestanding math.h -- sin/cos/log/exp are left as undefined
 * externs on purpose: wasm-ld will turn them into WASM imports, resolved
 * from JS (env.sin/env.cos/env.log/env.exp, and tan asin acos atan, in amber.js), matching exactly
 * what amber.js already provides. */
#ifndef WSYS_MATH_H
#define WSYS_MATH_H
double sin(double);
double cos(double);
double log(double);
double exp(double);
double tan(double);   /* Amber 2.7.3: q's tan asin acos atan, env.tan.. in amber.js too */
double asin(double);
double acos(double);
double atan(double);
double round(double);   /* in wasmlibc.c */
#endif
