#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Manage an isolated, original ETQW 1.5 protocol reference server in OrbStack."""
import argparse
import hashlib
import json
import shutil
import socket
import stat
import subprocess
import time
import urllib.request
import zipfile
from pathlib import Path

from probe_retail_server import HEADER, parse_info

URL = "https://cdn.splashdamage.com/downloads/games/etqw/ETQW-server-1.5-full.x86.run"
SHA256 = "7dc4cf69b3355063b4f8767e23b19d8224ff9b4f556ad906056063ddfe581ac0"
IMAGE = "debian:bookworm-slim@sha256:99c0b25c46d0214d5a2091092e7908907dcaecd6a962cde0ac700acee222b6f3"
NAME = "etqw-retail-local"
LABEL = "local.etqw.role"
ROLE = "retail-test"


def docker(*args, check=True):
    return subprocess.run(["docker", "--context", "orbstack", *args], check=check,
                          text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def container():
    result = docker("container", "inspect", NAME, check=False)
    if result.returncode:
        if "No such object" in result.stderr or "No such container" in result.stderr:
            return None
        raise RuntimeError(result.stderr.strip())
    info = json.loads(result.stdout)[0]
    if info["Config"].get("Labels", {}).get(LABEL) != ROLE:
        raise RuntimeError(f"Refusing to manage unrelated container {NAME}")
    return info


def prepare(home):
    archive = home / "downloads" / URL.rsplit("/", 1)[1]
    archive.parent.mkdir(parents=True, exist_ok=True)
    if not archive.exists():
        print("Downloading official dedicated-server distribution...", flush=True)
        partial = archive.with_suffix(".partial")
        with urllib.request.urlopen(URL, timeout=30) as response, partial.open("wb") as output:
            shutil.copyfileobj(response, output)
        partial.replace(archive)
    with archive.open("rb") as source:
        if hashlib.file_digest(source, "sha256").hexdigest() != SHA256:
            raise RuntimeError("Dedicated-server archive SHA256 mismatch")
    retail = home / "retail"
    if not (retail / ".archive-sha256").exists():
        print("Extracting server files without executing the installer...", flush=True)
        with zipfile.ZipFile(archive) as source:
            for item in source.infolist():
                if not item.filename.startswith("data/") or item.is_dir():
                    continue
                relative = Path(item.filename.removeprefix("data/"))
                if relative.is_absolute() or ".." in relative.parts or stat.S_ISLNK(item.external_attr >> 16):
                    raise RuntimeError("Unsafe archive member")
                target = retail / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                with source.open(item) as content, target.open("wb") as output:
                    shutil.copyfileobj(content, output)
                target.chmod(0o755 if target.name == "etqwded.x86" else 0o644)
        (retail / ".archive-sha256").write_text(SHA256 + "\n")
    if (retail / ".archive-sha256").read_text().strip() != SHA256:
        raise RuntimeError("Extracted server version does not match the pinned archive")
    (home / "profile" / "base").mkdir(parents=True, exist_ok=True)


def endpoint(info):
    network = info["NetworkSettings"]["Networks"].get(NAME)
    if not network or not network["IPAddress"]:
        raise RuntimeError("Local reference server has no internal address")
    return network["IPAddress"], 27733


def status(info):
    if not info:
        return {"running": False, "status": "not_created"}
    result = {"running": info["State"]["Running"], "status": info["State"]["Status"]}
    if not result["running"]:
        result["exit_code"] = info["State"]["ExitCode"]
        return result
    host, port = endpoint(info)
    result["endpoint"] = f"{host}:{port}"
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.settimeout(1)
        sock.connect((host, port))
        nonce = b"ETQWLAB1"
        sock.send(HEADER + b"getInfo\0" + nonce)
        try:
            reply = parse_info(sock.recv(32769), nonce)
        except (OSError, ValueError):
            result["ready"] = False
        else:
            result.update(ready=True, protocol=reply["protocol"],
                          map=reply["values"].get("si_map"), pure=reply["values"].get("si_pure"),
                          players=len(reply["players"]))
    return result


def stop():
    info = container()
    if info:
        if info["State"]["Running"]:
            docker("stop", "--time", "10", NAME)
        docker("rm", NAME)
    print("Local reference server stopped; files and profile retained.")


def start(home, strict_sync):
    prepare(home)
    info = container()
    if info:
        mounts = {m["Destination"]: m.get("Source") for m in info["Mounts"]}
        if mounts.get("/opt/etqw") != str(home / "retail"):
            raise RuntimeError("Existing lab uses another data directory; stop it explicitly first")
        args = info["Config"].get("Cmd", [])
        sync_index = args.index("net_checkSync") if "net_checkSync" in args else -1
        if info["State"]["Running"] and (sync_index < 0 or args[sync_index + 1] != str(int(strict_sync))):
            raise RuntimeError("Existing lab uses another sync mode; stop it explicitly first")
        if not info["State"]["Running"]:
            docker("rm", NAME)
            info = None
    if not info:
        network = docker("network", "inspect", NAME, check=False)
        if network.returncode:
            docker("network", "create", "--internal", "--label", f"{LABEL}={ROLE}", NAME)
        else:
            n = json.loads(network.stdout)[0]
            if not n["Internal"] or n.get("Labels", {}).get(LABEL) != ROLE:
                raise RuntimeError("Refusing unrelated or externally routed Docker network")
        docker("pull", "--platform", "linux/386", IMAGE)
        # net_checkSync=0 is an explicit lab setting for developing the transport.
        # Use --strict-sync to detect declaration incompatibility, as retail does.
        command = ["run", "--detach", "--interactive", "--tty", "--name", NAME,
                   "--platform", "linux/386", "--network", NAME, "--cap-drop", "ALL",
                   "--security-opt", "no-new-privileges", "--user", "1000:1000", "--read-only",
                   "--tmpfs", "/tmp:rw,noexec,nosuid,size=64m", "--label", f"{LABEL}={ROLE}",
                   "--mount", f"type=bind,source={home / 'retail'},target=/opt/etqw,readonly",
                   "--mount", f"type=bind,source={home / 'profile'},target=/profile",
                   "--workdir", "/opt/etqw", "--env", "LD_LIBRARY_PATH=/opt/etqw", IMAGE,
                   "./etqwded.x86", "+set", "fs_basepath", "/opt/etqw", "+set", "fs_savepath", "/profile"]
        settings = dict(net_ip="0.0.0.0", net_port="27733", net_serverDedicated="1", net_LANServer="1",
                        si_name="ETQW Metal protocol lab", si_pure="0",
                        net_allowCheats=str(int(not strict_sync)), net_checkSync=str(int(strict_sync)),
                        net_serverPunkbusterEnabled="0", si_rules="sdGameRulesObjective", bot_minClients="0",
                        si_minPlayers="1", si_maxPlayers="4", si_teamForceBalance="0", logFile="2")
        for name, value in settings.items():
            command.extend(["+set", name, value])
        docker(*command, "+spawnServer", "valley")
    print("Waiting for the original server to load Valley (32-bit emulation)...", flush=True)
    deadline = time.monotonic() + 120
    while time.monotonic() < deadline:
        current = status(container())
        if current.get("ready"):
            print(json.dumps(current, indent=2))
            return
        if not current["running"]:
            raise RuntimeError("Server exited: " + docker("logs", "--tail", "25", NAME).stdout)
        time.sleep(1)
    raise RuntimeError("Server is still starting; inspect its logs and run status")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("prepare", "start", "stop", "status", "logs"))
    parser.add_argument("--home", type=Path, default=Path(__file__).resolve().parents[3] / "build-macos-server")
    parser.add_argument("--strict-sync", action="store_true", help="Keep the retail declaration-checksum check enabled")
    args = parser.parse_args()
    try:
        if args.action == "prepare":
            prepare(args.home.resolve())
        elif args.action == "start":
            start(args.home.resolve(), args.strict_sync)
        elif args.action == "stop":
            stop()
        elif args.action == "status":
            print(json.dumps(status(container()), indent=2))
        elif args.action == "logs":
            if container():
                print(docker("logs", "--tail", "80", NAME).stdout, end="")
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as exc:
        parser.exit(1, f"Local server: {getattr(exc, 'stderr', None) or exc}\n")


if __name__ == "__main__":
    main()
