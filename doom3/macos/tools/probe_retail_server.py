#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Diagnose ETQW 1.5 interoperability without claiming to be a connected player.

No credentials, automatic downloads, pure-check bypass or gameplay packets.
The optional handshake stops at the first admission response. Each stage sends
one datagram, and accepts responses only from the selected UDP endpoint.
"""
import argparse
import ctypes
import hashlib
import json
import secrets
import socket
import struct
import sys
import time
import zipfile
from pathlib import Path

PROTOCOL = (10 << 16) | 21
HEADER = b"\xff\xff"
MAX_PACKET = 32768


class ProtocolError(ValueError):
    pass


class Reader:
    def __init__(self, data):
        self.data, self.pos = data, 0

    def take(self, count):
        if count < 0 or self.pos + count > len(self.data):
            raise ProtocolError("Truncated packet")
        result = self.data[self.pos:self.pos + count]
        self.pos += count
        return result

    def number(self, fmt):
        return struct.unpack("<" + fmt, self.take(struct.calcsize("<" + fmt)))[0]

    def string(self, limit=1024):
        end = self.data.find(b"\0", self.pos, self.pos + limit + 1)
        if end < 0:
            raise ProtocolError("Missing terminator or oversized string")
        return self.take(end - self.pos + 1)[:-1].decode("latin1")

    def finish(self):
        if self.pos != len(self.data):
            raise ProtocolError("Unexpected trailing bytes")


def envelope(data):
    if len(data) > MAX_PACKET or not data.startswith(HEADER):
        raise ProtocolError("Invalid connectionless packet")
    reader = Reader(data[2:])
    return reader.string(32), reader


def parse_info(data, nonce):
    kind, r = envelope(data)
    if kind != "infoResponse" or r.take(8) != nonce:
        raise ProtocolError("Unexpected response or query nonce")
    version = r.number("I")
    size = r.number("I")
    if size != len(r.data) - r.pos:
        raise ProtocolError("Incorrect info payload length")
    if version != PROTOCOL:
        raise ProtocolError(f"Unsupported retail protocol {version >> 16}.{version & 65535}")
    values = {}
    for _ in range(257):
        key = r.string()
        if not key:
            break
        if key in values or len(values) == 256:
            raise ProtocolError("Duplicate or excessive info keys")
        values[key] = r.string()
    # The delta dictionary has a second, empty list of removed keys.
    if r.string():
        raise ProtocolError("Unexpected removed dictionary key")
    players = []
    seen = set()
    while True:
        slot = r.number("B")
        if slot == 32:
            break
        if slot > 31 or slot in seen:
            raise ProtocolError("Invalid or duplicate player slot")
        seen.add(slot)
        ping = r.number("H")
        name = r.string()
        clan_position = r.number("B")
        clan = r.string()
        bot = r.number("B")
        if clan_position > 1 or bot > 1:
            raise ProtocolError("Invalid player flags")
        players.append(dict(slot=slot, ping=ping, name=name, clan=clan, bot=bool(bot)))
    os_mask = r.number("I")
    ranked = r.number("B")
    time_left = r.number("I")
    game_state = r.number("B")
    server_type = r.number("B")
    if ranked > 1 or server_type > 1:
        raise ProtocolError("Invalid server flags")
    if server_type:
        r.number("I")  # TV viewers
        r.number("I")  # TV capacity
    else:
        r.number("B")  # interested clients
    r.finish()
    return dict(protocol="10.21", values=values, players=players, os_mask=os_mask,
                ranked=bool(ranked), time_left_ms=time_left, game_state=game_state)


def parse_challenge(data):
    kind, r = envelope(data)
    if kind != "challengeResponse":
        raise ProtocolError("Expected challengeResponse")
    result = dict(challenge=r.number("I"), channel_ids=(r.number("H"), r.number("H")),
                  session_id=(r.number("I"), r.number("I")),
                  game_base=r.string(), game=r.string())
    r.finish()
    return result


def parse_pure(data):
    kind, r = envelope(data)
    if kind != "pureServer":
        raise ProtocolError("Expected pureServer")
    checksums = []
    for _ in range(128):
        value = r.number("I")
        if not value:
            game_code = r.number("I")
            r.finish()
            return checksums, game_code
        checksums.append(value)
    raise ProtocolError("Too many pure checksums")


def connect_packet(challenge, client_ids, declaration_checksum, os_id=1):
    if any(challenge["session_id"]):
        raise ProtocolError("Server requires an authentication session")
    if challenge["game"] or challenge["game_base"]:
        raise ProtocolError("Server requires a mod; no automatic switch performed")
    # Retail format: two channel IDs, then rate, auth-ticket-present bit (false),
    # byte-aligned empty password and repeater bit (false). macOS OS ID is 1.
    return (HEADER + b"connect\0" + struct.pack("<I H I I H H I", PROTOCOL, os_id,
            declaration_checksum, challenge["challenge"], *client_ids, 16000) + b"\0\0\0")


def md4(data):
    try:
        return hashlib.new("md4", data).digest()
    except ValueError:
        if sys.platform != "darwin":
            raise RuntimeError("PK4 audit needs MD4 support or macOS CommonCrypto")
        crypto = ctypes.CDLL("/usr/lib/system/libcommonCrypto.dylib")
        crypto.CC_MD4.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_void_p]
        crypto.CC_MD4.restype = ctypes.c_void_p
        result = ctypes.create_string_buffer(16)
        if not crypto.CC_MD4(data, len(data), result):
            raise RuntimeError("CommonCrypto MD4 failed")
        return result.raw


def pak_inventory(base):
    inventory = {}
    for path in sorted(base.glob("*.pk4")):
        with zipfile.ZipFile(path) as archive:
            # Same archive identity as idFileSystemLocal::LoadZipFile: MD4 of
            # nonempty entries' CRC32 values, in central-directory order.
            crcs = b"".join(struct.pack("<I", item.CRC) for item in archive.infolist() if item.file_size)
        a, b, c, d = struct.unpack("<IIII", md4(crcs))
        inventory.setdefault(a ^ b ^ c ^ d, []).append(path.name)
    return inventory


def reply_description(data):
    kind, r = envelope(data)
    result = {"response": kind}
    if kind == "print":
        result["code"] = r.number("I")
        # Only the observed plain-text miscellaneous opcode is decoded here.
        if result["code"] == 0:
            result["message"] = r.string()
            r.finish()
    return result


def probe(args):
    report = {"endpoint": args.endpoint, "connected": False}
    host, port = args.endpoint.rsplit(":", 1)
    port = int(port)
    if not 1 <= port <= 65535:
        raise ValueError("Invalid port")
    inventory = pak_inventory(args.base) if args.base else {}
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.settimeout(args.timeout)
        sock.connect((host, port))

        def exchange(packet):
            sock.send(packet)
            return sock.recv(MAX_PACKET + 1)

        nonce = secrets.token_bytes(8)
        start = time.monotonic()
        info = parse_info(exchange(HEADER + b"getInfo\0" + nonce), nonce)
        report.update(protocol=info["protocol"], query_ms=round((time.monotonic() - start) * 1000, 1),
                      server=info["values"], players=len(info["players"]),
                      bots=sum(p["bot"] for p in info["players"]))
        if not args.handshake:
            return report
        ids = (secrets.randbelow(32767), secrets.randbelow(32767))
        challenge = parse_challenge(exchange(HEADER + b"challenge\0" + struct.pack("<HHB", *ids, 0)))
        report["authentication_required"] = any(challenge["session_id"])
        if report["authentication_required"]:
            report["stopped_at"] = "authentication_required"
            return report
        # This diagnostic has no loaded declaration table; zero is explicitly
        # a probe value, not a claim of game-data/transport compatibility.
        data = exchange(connect_packet(challenge, ids, declaration_checksum=0))
        report["admission"] = reply_description(data)
        if report["admission"]["response"] != "pureServer":
            report["stopped_at"] = "admission_response"
            return report
        assets, code = parse_pure(data)
        report["required_assets"] = [{"checksum": f"{v:08x}", "local_paks": inventory.get(v, [])} for v in assets]
        report["required_code_pak"] = {"checksum": f"{code:08x}", "local_paks": inventory.get(code, [])}
        report["stopped_at"] = "pure_validation_required"
        if args.check_native_module:
            if not assets or any(v not in inventory for v in assets):
                report["stopped_at"] = "required_assets_missing"
                return report
            # This port loads an external ARM64 dylib, with no approved retail
            # game-code PK4. Report zero honestly, never echo the expected hash.
            packet = (HEADER + b"pureClient\0" + struct.pack("<IH", challenge["challenge"], ids[0]) +
                      struct.pack("<" + "I" * (len(assets) + 2), *assets, 0, 0))
            report["native_module_result"] = reply_description(exchange(packet))
            report["stopped_at"] = "native_module_validation"
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("endpoint", help="IPv4 address or hostname:port")
    parser.add_argument("--handshake", action="store_true", help="Negotiate up to admission; never enter gameplay")
    parser.add_argument("--base", type=Path, help="Local base directory for PK4 checksum matching")
    parser.add_argument("--check-native-module", action="store_true", help="Report native external-module checksum zero to the server")
    parser.add_argument("--timeout", type=float, default=3)
    parser.add_argument("--output", type=Path, help="Write a JSON diagnostic report")
    args = parser.parse_args()
    if not 0 < args.timeout <= 10:
        parser.error("timeout must be between 0 and 10 seconds")
    if args.base and not args.base.is_dir():
        parser.error("base directory does not exist")
    if args.check_native_module and not (args.handshake and args.base):
        parser.error("--check-native-module requires --handshake and --base")
    try:
        report = probe(args)
    except (OSError, ValueError, RuntimeError, zipfile.BadZipFile) as exc:
        print(f"Probe failed: {exc}", file=sys.stderr)
        return 1
    output = json.dumps(report, indent=2, ensure_ascii=True) + "\n"
    if args.output:
        args.output.write_text(output)
    print(output, end="")
    # Successful diagnostics are not a successful player connection.
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
