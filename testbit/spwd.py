#!/usr/bin/env python3

import ctypes
import ctypes.util
import threading

libc = ctypes.CDLL(ctypes.util.find_library("c"))
libcrypt = ctypes.CDLL(ctypes.util.find_library("crypt"))

class struct_spwd(ctypes.Structure):
    _fields_ = [
        ('sp_namp', ctypes.c_char_p),
        ('sp_pwdp', ctypes.c_char_p),
        ('sp_lstchg', ctypes.c_long),
        ('sp_min', ctypes.c_long),
        ('sp_max', ctypes.c_long),
        ('sp_warn', ctypes.c_long),
        ('sp_inact', ctypes.c_long),
        ('sp_expire', ctypes.c_long),
        ('sp_flag', ctypes.c_ulong),
    ]

libc.getspnam.argtypes = [ctypes.c_char_p]
libc.getspnam.restype = ctypes.POINTER(struct_spwd)

libcrypt.crypt.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
libcrypt.crypt.restype = ctypes.c_char_p

passwordlock = threading.Lock()

def authenticate(password: str):
    with passwordlock:
        spwd = libc.getspnam(b"testbit")
        if not spwd or not spwd.contents.sp_pwdp.decode():
            return False, "no testbit user password"
        hashed = libcrypt.crypt(password.encode(), spwd.contents.sp_pwdp)
        if hashed.decode() == spwd.contents.sp_pwdp.decode():
            return True, ""
        return False, "wrong password"
