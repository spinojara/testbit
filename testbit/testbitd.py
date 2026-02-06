#!/usr/bin/env python3

from aiohttp import web
import threading, sqlite3, json
from typing import Dict
from pathlib import Path
import tempfile
import time
import docker
import sys
from docker.errors import BuildError
import math
import base64
import ssl
import argparse

from . import tc
from . import elo
from . import spwd

dbcond = threading.Condition()
con = sqlite3.connect("temp.sqlite", check_same_thread=False)

Dockerfile = """
FROM alpine:3.23.3

RUN apk add --no-cache make clang lld compiler-rt llvm git curl

RUN curl -L -o fastchess.tar.gz https://github.com/Disservin/fastchess/archive/refs/tags/v1.8.0-alpha.tar.gz && \
    mkdir fastchess && \
    tar -xf fastchess.tar.gz -C fastchess --strip-components=1 && \
    make -C fastchess CXX=clang++ && \
    make -C fastchess install && \
    rm -rf fastchess fastchess.tar.gz

ARG COMMIT

COPY patch patch

RUN git clone https://github.com/spinojara/bitbit.git && \
    git -C bitbit checkout $COMMIT && \
    make -C bitbit clean && make -C bitbit SIMD=avx2 bitbit-pgo && \
    mv bitbit/etc/book/testbit-50cp5d6m100k.epd book.epd && \
    mv bitbit/bitbit bitbit-old && \
    git -C bitbit apply ../patch && \
    make -C bitbit clean && make -C bitbit SIMD=avx2 bitbit-pgo && \
    mv bitbit/bitbit bitbit-new && \
    rm -rf bitbit patch nnue
"""

def get_next_image_to_build():
    cursor = con.cursor()
    cursor.execute("""
        SELECT id, commithash, simd, patch
        FROM tests
        WHERE status = "building"
        ORDER BY queuetime ASC
        LIMIT 1;
    """)
    return cursor.fetchone()

def build_docker_images():
    while True:
        with dbcond:
            while not (row := get_next_image_to_build()):
                print("waiting")
                dbcond.wait()
                print("woke up")
        print(f"got row: {row}")
        id, commit, simd, patch = row


        tempdir = Path(tempfile.mkdtemp(prefix="testbit-docker-"))
        print(tempdir)
        patch_path = tempdir / "patch"
        dockerfile = tempdir / "Dockerfile"
        print(patch_path)
        with open(patch_path, "wb") as f:
            f.write(patch)
        with open(dockerfile, "w") as f:
            f.write(Dockerfile)

        try:
            client = docker.from_env()
            print("building docker image")
            image, build_logs = client.images.build(
                path=str(tempdir),
                dockerfile=str(dockerfile),
                buildargs={"COMMIT": commit},
                tag="testbit:%d" % id,
            )
        except BuildError as e:
            errorlog = ""
            for log in e.build_log:
                errorlog += log.get("stream", "")

            with dbcond:
                cursor = con.cursor()
                cursor.execute("""
                    UPDATE tests
                    SET status = "error",
                        errorlog = ?,
                        starttime = CASE
                            WHEN starttime IS NULL THEN unixepoch()
                            ELSE starttime
                        END
                        donetime = unixepoch()
                    WHERE id = ? AND status = "building";
                """, (errorlog.encode("utf-8"), id))
                con.commit()
            print(errorlog)
        except Exception as e:
            errorlog = str(e)
            print(errorlog)
            with dbcond:
                cursor = con.cursor()
                cursor.execute("""
                    UPDATE tests
                    SET status = "error",
                        errorlog = ?,
                        starttime = CASE
                            WHEN starttime IS NULL THEN unixepoch()
                            ELSE starttime
                        END
                        donetime = unixepoch()
                    WHERE id = ? AND status = "building";
                """, (errorlog.encode("utf-8"), id))
                con.commit()
        finally:
            dockerfile.unlink()
            patch_path.unlink()
            tempdir.rmdir()

        with dbcond:
            cursor = con.cursor()
            cursor.execute("""
                UPDATE tests
                SET status = "queued"
                WHERE id = ? AND status = "building";
            """, (id, ))
            con.commit()


def create_table():
    cursor = con.cursor()
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS tests (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            type       TEXT NOT NULL,
            status     TEXT DEFAULT "building",
            tc         TEXT NOT NULL,
            alpha      REAL,
            beta       REAL,
            elo0       REAL,
            elo1       REAL,
            eloe       REAL,
            adjudicate TEXT NOT NULL,
            queuetime  INTEGER,
            starttime  INTEGER,
            donetime   INTEGER,
            elo        REAL,
            pm         REAL,
            llr        REAL,
            t0         INTEGER DEFAULT 0,
            t1         INTEGER DEFAULT 0,
            t2         INTEGER DEFAULT 0,
            p0         INTEGER DEFAULT 0,
            p1         INTEGER DEFAULT 0,
            p2         INTEGER DEFAULT 0,
            p3         INTEGER DEFAULT 0,
            p4         INTEGER DEFAULT 0,
            commithash TEXT NOT NULL,
            simd       TEXT NOT NULL,
            patch      BLOB NOT NULL,
            errorlog   BLOB
        );
    """)
    con.commit()

async def test_new(request):
    try:
        reader = await request.multipart()
        patch_contents = None
        data = None
        async for part in reader:
            if part.name == "patch":
                patch_contents = await part.read()
            elif part.name == "data":
                data = await part.text()
                data = json.loads(data)
    except json.JSONDecodeError:
        return web.json_response({"message": "invalid json"}, status=400)
    except Exception:
        return web.json_response({"message": "no json"}, status=400)

    if not isinstance(data, Dict):
        return web.json_response({"message": "no data"}, status=400)
    if not patch_contents:
        return web.json_response({"message": "no patch"}, status=400)

    if data.get("type") not in ["elo", "sprt"]:
        return web.json_response({"message": "bad type"}, status=400)
    if not isinstance(data.get("tc"), str) or not tc.validatetc(data.get("tc")):
        return web.json_response({"message": "bad tc"}, status=400)
    if data.get("type") == "sprt":
        print(type(data.get("alpha")))
        if not isinstance(data.get("alpha"), float) or data.get("alpha") <= 0.0:
            return web.json_response({"message": "bad alpha"}, status=400)
        if not isinstance(data.get("beta"), float) or data.get("beta") <= 0.0:
            return web.json_response({"message": "bad beta"}, status=400)
        if not isinstance(data.get("elo0"), float):
            return web.json_response({"message": "bad elo0"}, status=400)
        if not isinstance(data.get("elo1"), float):
            return web.json_response({"message": "bad elo1"}, status=400)
        if data.get("elo0") >= data.get("elo1"):
            return web.json_response({"message": "need elo0 < elo1"}, status=400)
        if data.get("alpha") + data.get("beta") >= 0.5:
            return web.json_response({"message": "need alpha + beta < 0.5"}, status=400)
    elif data.get("type") == "elo":
        if not isinstance(data.get("eloe"), float) or data.get("eloe") <= 0.0:
            return web.json_response({"message": "bad eloe"}, status=400)

    if not data.get("commit"):
        return web.json_response({"message": "no commit"}, status=400)
    if data.get("adjudicate") not in ["none", "draw", "resign", "both"]:
        return web.json_response({"message": "bad adjudicate"}, status=400)

    if data.get("simd") not in ["none", "avx2"]:
        return web.json_response({"message": "bad simd"}, status=400)

    with dbcond:
        cursor = con.cursor()
        cursor.execute("""
            INSERT INTO tests (
                type,
                tc,
                alpha,
                beta,
                elo0,
                elo1,
                eloe,
                adjudicate,
                queuetime,
                commithash,
                simd,
                patch
            )
            VALUES (
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                unixepoch(),
                ?,
                ?,
                ?
            );
        """, (
                data.get("type"),
                data.get("tc"),
                data.get("alpha"),
                data.get("beta"),
                data.get("elo0"),
                data.get("elo1"),
                data.get("eloe"),
                data.get("adjudicate"),
                data.get("commit"),
                data.get("simd"),
                patch_contents
             ))
        con.commit()
        dbcond.notify()

    return web.json_response({"message": "ok"})

async def test_data(request):
    id = request.match_info.get("id")
    if not id:
        return web.json_response({"message": "bad id"}, status=400)

    try:
        data = await request.json()
    except json.JSONDecodeError:
        return web.json_response({"message": "invalid json"}, status=400)
    except Exception:
        return web.json_response({"message": "no json"}, status=400)

    if not isinstance(data.get("wins"), int) or data.get("wins") < 0:
        return web.json_response({"message": "bad wins"}, status=400)
    if not isinstance(data.get("losses"), int) or data.get("losses") < 0:
        return web.json_response({"message": "bad losses"}, status=400)
    if not isinstance(data.get("draws"), int) or data.get("draws") < 0:
        return web.json_response({"message": "bad draws"}, status=400)
    if data.get("wins") + data.get("draws") + data.get("losses") != 2:
        return web.json_response({"message": "need losses + draws + wins = 2"}, status=400)
    with dbcond:
        cursor = con.cursor()
        cursor.execute("""
            SELECT type, alpha, beta, t0, t1, t2, p0, p1, p2, p3, p4
            FROM tests
            WHERE id = ? AND (
                status = "running"
                OR (status = "building" AND starttime IS NOT NULL)
            );
        """, (id, ))
        row = cursor.fetchone()

        if not row:
            # It could be that this id was already set to done.
            return web.json_response({"message": "ok"}, status=400)

        type, alpha, beta, t0, t1, t2, p0, p1, p2, p3, p4 = row
        p = [p0, p1, p2, p3, p4]
        t0 += data.get("losses")
        t1 += data.get("draws")
        t2 += data.get("wins")
        p[data.get("draws") + 2 * data.get("wins")] += 1

        elo, pm = elo.calculate_elo(p)
        if type == "sprt":
            llr = elo.calculate_llr(p)
            A = math.log(beta / (1.0 - alpha))
            B = math.log((1.0 - beta) / alpha)
            if llr < A:
                status = "H0 accepted"
            elif llr > B:
                status = "H1 accepted"

            cursor.execute("""
                UPDATE tests
                SET t0 = ?,
                    t1 = ?,
                    t2 = ?,
                    p0 = ?,
                    p1 = ?,
                    p2 = ?,
                    p3 = ?,
                    p4 = ?,
                    status = ?,
                    llr = ?,
                    elo = ?,
                    pm = ?,
                    donetime = CASE
                        WHEN ? IN ("H0 accepted", "H1 accepted")
                            THEN unixepoch()
                        ELSE NULL
                    END,
                WHERE id = ?
            """, (t0, t1, t2, p[0], p[1], p[2], p[3], p[4], status, llr, elo, pm, status, id))
            con.commit()
        else:
            if pm < eloe:
                status = "done"
            cursor.execute("""
                UPDATE tests
                SET t0 = ?,
                    t1 = ?,
                    t2 = ?,
                    p0 = ?,
                    p1 = ?,
                    p2 = ?,
                    p3 = ?,
                    p4 = ?,
                    status = ?,
                    elo = ?,
                    pm = ?,
                    donetime = CASE
                        WHEN ? = "done")
                            THEN unixepoch()
                        ELSE NULL
                    END,
                WHERE id = ?
            """, (t0, t1, t2, p[0], p[1], p[2], p[3], p[4], status, elo, pm, status, id))
            con.commit()

    return web.json_response({"message": "ok"})

async def test_cancel(request):
    id = request.match_info.get("id")
    if not id:
        return web.json_response({"message": "bad id"}, status=400)
    with dbcond:
        cursor = con.cursor()
        cursor.execute("""
            UPDATE tests
            SET status = "cancelled",
                startime = CASE
                    WHEN starttime IS NULL THEN unixepoch()
                    ELSE starttime
                END,
                donetime = unixepoch()
            WHERE status IN ("running", "building", "queued")
                AND id = ?;
        """, (id, ))
        con.commit()
    return web.json_response({"message": "ok"})

async def test_error(request):
    id = request.match_info.get("id")
    if not id:
        return web.json_response({"message": "bad id"}, status=400)

    try:
        data = await request.json()
    except json.JSONDecodeError:
        return web.json_response({"message": "invalid json"}, status=400)
    except Exception:
        return web.json_response({"message": "no json"}, status=400)

    errorlog = data.get("errorlog")
    if not isinstance(errorlog, str):
        return web.json_response({"message": "bad errorlog"}, status=400)

    with dbcond:
        cursor = con.cursor()
        cursor.execute("""
            UPDATE tests
            SET status = "error", errorlog = ?
            WHERE status IN ("running", "building") AND id = ?;
        """, (errorlog.encode("utf-8"), id))
        con.commit()
    return web.json_response({"message": "ok"})

async def test_docker(request):
    id = request.match_info.get("id")
    if not id:
        return web.json_response({"message": "bad id"}, status=400)

    with dbcond:
        cursor = con.cursor()
        cursor.execute("""
            UPDATE tests
            SET status = "building"
            WHERE status = "running" AND id = ?;
        """, (errorlog.encode("utf-8"), id))
        con.commit()
        dbcond.notify()
    return web.json_response({"message": "ok"})

async def get_task(request):
    with dbcond:
        cursor = con.cursor()
        cursor.execute("""
            UPDATE tests
                SET starttime = CASE
                    WHEN status = "queued" THEN unixepoch()
                    ELSE starttime
                END
            WHERE id = (
                SELECT id FROM tests
                WHERE status IN ("running", "queued")
                ORDER BY queuetime ASC
                LIMIT 1
            )
            RETURNING id, tc, adjudicate;
        """)
        row = cursor.fetchone()
        con.commit()

    if not row:
        return web.json_response({"id": None, "tc": None, "adjudicate": None})

    id, tc, adjudicate = row
    return web.json_response({"id": id, "tc": tc, "adjudicate": adjudicate})

async def test_fetch(request):
    return web.json_response({"message": "ok"})

@web.middleware
async def enforce_https(request, handler):
    if request.scheme != "https" and False:
        return web.json_response({"message": "use https"})
    return await handler(request)

@web.middleware
async def authenticate(request, handler):
    public_endpoints = [
        ("/test", "GET"),
    ]

    if (request.path, request.method) in public_endpoints:
        return await handler(request)

    auth_header = request.headers.get("Authorization")
    if not auth_header:
        return web.json_response({"message": "no authorization header"}, status=401)

    try:
        scheme, credentials = auth_header.split(" ")
        if scheme.lower() != "basic":
            return web.json_response({"message": "need basic authorization"}, status=401)

        decoded = base64.b64decode(credentials).decode("utf-8")
        _, password = decoded.split(":", 1)

        status, message = spwd.authenticate(password)
        if not status:
            return web.json_response({"message": message}, status=401)

    except Exception as e:
        return web.json_response({"message": "an error occured"}, status=401)

    return await handler(request)


def create_app():
    app = web.Application(middlewares=[enforce_https, authenticate])

    app.router.add_post("/test", test_new)
    app.router.add_put("/test/{id}", test_data)
    app.router.add_put("/test/cancel/{id}", test_cancel)
    app.router.add_put("/test/error/{id}", test_error)
    app.router.add_put("/test/docker/{id}", test_docker)
    app.router.add_get("/test/task", get_task)
    app.router.add_get("/test", test_fetch)

    return app

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, help="port", default=2718)
    parser.add_argument("--cert-chain", type=str, help="SSL certificate chain", default="")
    parser.add_argument("--cert-key", type=str, help="SSL certificate key", default="")

    args, unknown = parser.parse_known_args()

    """
    ctx = ssl.create_default_context(purpose=ssl.Purpose.CLIENT_AUTH)
    try:
        ctx.load_cert_chain(args.cert_chain, args.cert_key)
    except FileNotFoundError:
        print("failed to find cert or key file")
        return 1
    """

    create_table()

    thread = threading.Thread(target=build_docker_images, daemon=True)
    thread.start()

    app = create_app()

    web.run_app(app, host="0.0.0.0", port=args.port)

    thread.join()
    con.close()
    return 0

if __name__ == "__main__":
    sys.exit(main())
