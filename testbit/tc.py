#!/usr/bin/env python3

# Check if string contains only "0123456789."
def valid_float_chars(s: str) -> bool:
    return all(c in "0123456789." for c in s)

# Check if string contains only "0123456789"
def valid_int_chars(s: str) -> bool:
    return all(c in "0123456789" for c in s)

def validatetc(tc: str) -> bool:
    if "/" in tc:
        if "+" in tc:
            moves, tc = tc.split("/", 1)
            maintime, increment = tc.split("+", 1)
            try:
                return (
                    valid_int_chars(moves) and
                    valid_float_chars(maintime) and
                    valid_float_chars(increment) and
                    int(moves) > 0 and
                    float(maintime) > 0 and
                    float(increment) > 0
                )
            except:
                False
        else:
            moves, maintime = tc.split("/", 1)
            try:
                return (
                    valid_int_chars(moves) and
                    valid_float_chars(maintime) and
                    int(moves) > 0 and
                    float(maintime) > 0
                )
            except:
                False
    else:
        if "+" in tc:
            maintime, increment = tc.split("+", 1)
            try:
                return (
                    valid_float_chars(maintime) and
                    valid_float_chars(increment) and
                    float(maintime) > 0 and
                    float(increment) > 0
                )
            except:
                False
        else:
            maintime = tc
            try:
                return (
                    valid_float_chars(maintime) and
                    float(maintime) > 0
                )
            except:
                False
