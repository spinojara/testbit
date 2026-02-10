#!/usr/bin/env python3

import requests
import json
import sys
import argparse
import getpass
import string

from . import tc

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("patch", type=str, help="patch path")
    parser.add_argument("--type", type=str, help="sprt or elo", default="sprt")
    parser.add_argument("--alpha", type=float, help="alpha", default=0.025)
    parser.add_argument("--beta", type=float, help="beta", default=0.025)
    parser.add_argument("--elo0", type=float, help="elo0", default=0.0)
    parser.add_argument("--elo1", type=float, help="elo1", default=2.0)
    parser.add_argument("--eloe", type=float, help="elo error", default=5.0)
    parser.add_argument("--commit", type=str, help="commit", default="master")
    parser.add_argument("--simd", type=str, help="simd", default="avx2")
    parser.add_argument("--adjudicate", type=str, help="adjudication", default="both")
    parser.add_argument("--tc", type=str, help="time control", default="40/10+0.1")
    parser.add_argument("--host", type=str, help="host", default="localhost")
    parser.add_argument("--port", type=int, help="port", default=2718)
    parser.add_argument("--description", type=str, help="description", default="")
    parser.add_argument("--cancel", action="store_true", default=False)

    args, _ = parser.parse_known_args()
    if args.alpha <= 0.0 or args.beta <= 0.0 or args.alpha + args.beta >= 0.5:
        print("bad alpha or beta")
        return 1
    if args.elo0 >= args.elo1:
        print("bad elo0 or elo1")
        return 1
    if args.type not in ["sprt", "elo"]:
        print("type must be either sprt or elo")
        return 1
    if not isinstance(args.commit, str) or not args.commit or not all(c in (string.ascii_letters + string.digits + "-_") for c in args.commit):
        print("commit can't be empty")
        return 1
    if not args.simd or not args.simd.isalnum():
        print("simd must contain only alpha numeric characters")
        return 1
    if args.adjudicate not in ["none", "draw", "resign", "both"]:
        print("adjudicate must be either none, draw, resign or both")
        return 1
    if not tc.validatetc(args.tc):
        print("bad tc")
        return 1

    while not args.description.strip():
        args.description = input("Description: ")

    try:
        patch = open(args.patch, "r")
    except FileNotFoundError:
        print("failed to open '%s'" % args.patch)
        return 1

    verify = not args.host in ["localhost", "127.0.0.1"]

    if not args.host.startswith("https://"):
        args.host = "https://" + args.host

    password = getpass.getpass("Enter passphrase: ")

    try:
        response = requests.post(
            url="%s:%d/test" % (args.host, args.port),
            data={"data": json.dumps({
                "type": args.type,
                "alpha": args.alpha,
                "beta": args.beta,
                "elo0": args.elo0,
                "elo1": args.elo1,
                "eloe": args.eloe,
                "commit": args.commit,
                "simd": args.simd,
                "adjudicate": args.adjudicate,
                "tc": args.tc,
                "description": args.description
            })},
            files={"patch": patch},
            auth=("", password),
            verify=verify
        )
    except requests.exceptions.ConnectionError:
        print("Connection to %s:%d refused" % (args.host, args.port))
        return 1
    finally:
        patch.close()

    if response.status_code == 200:
        return 0
    print("%d %s" % (response.status_code, response.reason))
    try:
        message = response.json()
        if message.get("message"):
            print("reason: %s" % message.get("message"))
    except json.JSONDecodeError:
        pass
    return 1

if __name__ == "__main__":
    sys.exit(main())
