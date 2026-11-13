#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run a matching native ETQW server and client on loopback; close both together."""
import argparse
import os
from pathlib import Path
import secrets
import signal
import socket
import subprocess
import time

from probe_retail_server import HEADER, envelope

PROTOCOL = (14 << 16) | 42


def query(port):
    nonce = secrets.token_bytes(4)
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.settimeout(.5)
        sock.connect(("127.0.0.1", port))
        sock.send(HEADER + b"getInfo\0" + nonce)
        kind, reply = envelope(sock.recv(32768))
    if kind != "infoResponse" or reply.take(4) != nonce:
        raise ValueError("Unexpected native server response")
    version = reply.number("I")
    if version != PROTOCOL:
        raise ValueError(f"Expected protocol 14.42, received {version >> 16}.{version & 65535}")
    values = {}
    for _ in range(256):
        key = reply.string()
        if not key:
            if reply.string():
                raise ValueError("Unexpected removed server-info key")
            return values
        if key in values:
            raise ValueError("Duplicate server-info key")
        values[key] = reply.string()
    raise ValueError("Excessive server-info keys")


def stop(process):
    if process is not None and process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=8)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def settings(values):
    return [arg for name, value in values.items() for arg in ("+set", name, str(value))]


def run(args):
    # Never reuse or terminate an unrelated process already bound to this port.
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as reservation:
        reservation.bind(("127.0.0.1", args.port))
    home = args.home.resolve()
    home.mkdir(parents=True, exist_ok=True)
    launcher = Path(__file__).resolve().parents[1] / "launch.sh"
    environment = dict(os.environ, ETQW_BUILD_DIR=str(args.build.resolve()))
    shared = dict(net_ip="127.0.0.1", net_LANServer=1, net_allowCheats=0,
                  logFile=2, net_verbose=0, harm_r_printShaderSource=0)
    server_settings = dict(shared, net_port=args.port, net_serverDedicated=1,
                           si_pure=0, si_name="ETQW Metal Local", si_rules="sdGameRulesObjective",
                           si_minPlayers=1, si_maxPlayers=16, bot_minClients=args.bots, s_noSound=1)
    server = client = None
    with (home / "server.log").open("w") as server_log, (home / "client.log").open("w") as client_log:
        try:
            server = subprocess.Popen(["bash", str(launcher), str(args.data.resolve()),
                                       *settings(server_settings), "+spawnServer", "valley"],
                                      env=dict(environment, ETQW_PROFILE=str(home / "server")),
                                      stdin=subprocess.DEVNULL, stdout=server_log, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 120
            while True:
                if server.poll() is not None:
                    raise RuntimeError(f"Server exited; see {home / 'server.log'}")
                try:
                    info = query(args.port)
                    if info.get("si_map") == "maps/valley":
                        break
                except (OSError, ValueError):
                    pass
                if time.monotonic() >= deadline:
                    raise RuntimeError(f"Server did not load Valley; see {home / 'server.log'}")
                time.sleep(.25)
            print(f"Native server ready: 127.0.0.1:{args.port}, protocol 14.42, Valley", flush=True)
            print(f"Logs: {home}", flush=True)
            if args.server_only:
                return server.wait()
            client_environment = dict(environment, ETQW_PROFILE=str(home / "client"))
            if args.client_executable:
                client_environment["ETQW_ENGINE"] = str(args.client_executable.resolve())
            client_settings = dict(shared, net_serverDedicated=0, ui_name=args.name,
                                   r_renderWidth=3840, r_renderHeight=2160,
                                   harm_gui_fontRasterScale=4, image_anisotropy=16)
            client = subprocess.Popen(["bash", str(launcher), str(args.data.resolve()),
                                       *settings(client_settings), "+connect", f"127.0.0.1:{args.port}"],
                                      env=client_environment, stdin=subprocess.DEVNULL,
                                      stdout=client_log, stderr=subprocess.STDOUT)
            while client.poll() is None:
                if server.poll() is not None:
                    raise RuntimeError(f"Server stopped; see {home / 'server.log'}")
                time.sleep(.5)
            return client.returncode
        finally:
            stop(client)
            stop(server)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data", type=Path, required=True, help="Your ETQW 1.5 directory, containing base/")
    parser.add_argument("--build", type=Path, default=Path(__file__).resolve().parents[3] / "build-macos")
    parser.add_argument("--home", type=Path, default=Path.home() / "Library/Application Support/idtech4amm/etqw-local")
    parser.add_argument("--client-executable", type=Path, help="Optional native app executable for macOS window ownership")
    parser.add_argument("--server-only", action="store_true")
    parser.add_argument("--port", type=int, default=27735)
    parser.add_argument("--bots", type=int, default=8, help="Minimum population, including human players")
    parser.add_argument("--name", default="Player")
    args = parser.parse_args()
    if not 1024 <= args.port <= 65535 or not 0 <= args.bots <= 15:
        parser.error("Port must be 1024..65535; population must be 0..15")
    def interrupted(signum, frame):
        raise KeyboardInterrupt
    signal.signal(signal.SIGTERM, interrupted)
    try:
        raise SystemExit(run(args))
    except KeyboardInterrupt:
        parser.exit(0, "Local match stopped.\n")
    except (OSError, RuntimeError) as exc:
        parser.exit(1, f"Local match: {exc}\n")


if __name__ == "__main__":
    main()
