import sqlite3
import argparse
from pathlib import Path
import string


TYPE = ["sprt", "elo"]
STATUS = ["queued", "running", "cancelled", "error", "error", "error", "error", "error", "inconclusive", "H0 accepted", "H1 accepted", "done"]
ADJUDICATE = ["none", "draw", "resign", "both"]

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("--old", type=str, help="old sqlite3 db file", required=True)
    parser.add_argument("--new", type=str, help="new sqlite3 db file", required=True)
    parser.add_argument("--patch-dir", type=str, help="directory containing patches", required=True)
    parser.add_argument("--nnue-dir", type=str, help="directory containing NNUEs", required=True)

    args, _ = parser.parse_known_args()

    old = sqlite3.connect(args.old)
    new = sqlite3.connect(args.new)

    oldcur = old.cursor()

    oldcur.execute("""
        SELECT
            id,
            type,
            status,
            tc,
            alpha,
            beta,
            elo0,
            elo1,
            eloe,
            adjudicate,
            queuetime,
            starttime,
            donetime,
            elo,
            pm,
            llr,
            t0,
            t1,
            t2,
            p0,
            p1,
            p2,
            p3,
            p4,
            branch,
            commithash,
            simd
        FROM test;
    """)
    for row in oldcur.fetchall():
        id, type, status, tc, alpha, beta, elo0, elo1, eloe, adjudicate, queuetime, starttime, donetime, elo, pm, llr, t0, t1, t2, p0, p1, p2, p3, p4, branch, commit, simd = row
        if status == 8:
            print("inconclusive???")
            continue

        type = TYPE[type]
        status = STATUS[status]
        adjudicate = ADJUDICATE[adjudicate]

        if commit == "HEAD":
            commit = branch

        if type == "sprt":
            eloe = None
        elif type == "elo":
            alpha = None
            beta = None
            elo0 = None
            elo1 = None

        if None in [t0, t1, t2, p0, p1, p2, p3, p4] or (t0 == 0 and t1 == 0 and t2 == 0):
            t0 = t1 = t2 = p0 = p1 = p2 = p3 = p4 = 0
            elo = pm = llr = None
            print("Skipping cancelled test which didn't run...")
            continue
        elif elo is None:
            print("Elo is none but test ran, skipping...")
            continue

        if t0 >= 10 and t1 == 0 and t2 == 0:
            print("Test with trinomial %d-%d-%d is probably a debugging test, skipping..." % (t0, t1, t2))
            continue

        if not queuetime:
            print("No queuetime for id %d, skipping..." % id)
            continue

        if not donetime:
            donetime = queuetime
        if not starttime:
            starttime = donetime

        if not simd:
            simd = "none"

        if not isinstance(commit, str) or not commit or not all(c in (string.ascii_letters + string.digits + "-_") for c in commit):
            print("bad commit '%s', skipping..." % commit)
            continue

        if not isinstance(simd, str) or not simd or not simd.isalnum():
            print("bad simd '%s', skipping..." % simd)
            continue

        patch_contents = ""
        try:
            path = Path(args.nnue_dir) / (str(id) + ".nnue")
            if path.is_file() and path.stat().st_size > 0:
                print("Legacy NNUE patch")
                patch_contents = "Legacy NNUE patch\n"
        except:
            print("No NNUE for id %d" % id)
            pass

        try:
            with open(Path(args.patch_dir) / str(id), "r") as f:
                patch_contents += f.read()
        except:
            print("No patch for id %d, skipping..." % id)
            continue

        newcur = new.cursor()
        newcur.execute("""
            INSERT INTO tests (
                description,
                legacy,
                type,
                status,
                tc,
                alpha,
                beta,
                elo0,
                elo1,
                eloe,
                adjudicate,
                queuetime,
                starttime,
                donetime,
                elo,
                pm,
                llr,
                t0,
                t1,
                t2,
                p0,
                p1,
                p2,
                p3,
                p4,
                commithash,
                simd,
                patch
            )
            VALUES (
                'No description',
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?
            )
            ON CONFLICT(type, queuetime, patch, commithash) DO NOTHING;
        """, (
            True,
            type,
            status,
            tc,
            alpha,
            beta,
            elo0,
            elo1,
            eloe,
            adjudicate,
            queuetime,
            starttime,
            donetime,
            elo,
            pm,
            llr,
            t0,
            t1,
            t2,
            p0,
            p1,
            p2,
            p3,
            p4,
            commit,
            simd,
            patch_contents.encode("utf-8")
        ))
        new.commit()


    old.close()
    new.close()

if __name__ == "__main__":
    main()
