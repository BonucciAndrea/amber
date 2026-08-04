/* minimal freestanding math.h -- sin/cos/log/exp are left as undefined
 * externs on purpose: wasm-ld will turn them into WASM imports, resolved
 * from JS (env.sin/env.cos/env.log/env.exp in amber.js), matching exactly
 * what amber.js already provides. */
#ifndef WSYS_MATH_H
#define WSYS_MATH_H
double sin(double);
double cos(double);
double log(double);
double exp(double);
#endif
