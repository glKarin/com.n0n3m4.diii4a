#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import importlib.util
import socket
import struct
import tempfile
import threading
import unittest
import zipfile
from pathlib import Path
from types import SimpleNamespace

spec = importlib.util.spec_from_file_location("retail", Path(__file__).resolve().parents[1] / "tools/probe_retail_server.py")
retail = importlib.util.module_from_spec(spec)
spec.loader.exec_module(retail)
NONCE = bytes.fromhex("0123456789abcdef")


def info_packet(nonce=NONCE):
    body = (b"si_name\0Test server\0si_pure\x001\0\0\0" +
            b"\x03\x2a\x00Test player\0\x00TAG\0\x00\x20" +
            bytes.fromhex("07000000 00 10270000 02 00 00"))
    return b"\xff\xffinfoResponse\0" + nonce + struct.pack("<II", 0x000a0015, len(body)) + body


CHALLENGE = b"\xff\xffchallengeResponse\0" + bytes.fromhex("44332211 3412 7856 00000000 00000000 00 00")
PURE = b"\xff\xffpureServer\0" + bytes.fromhex("78563412 21436587 00000000 11223344")
CONNECT = (b"\xff\xffconnectResponse\0" + bytes.fromhex("652f 1c27 00000000 01 e7040000 c7a10000 00") +
           b"si_map\0maps/valley.entities\0\0\0")


class RetailProtocolTests(unittest.TestCase):
    def test_complete_info(self):
        result = retail.parse_info(info_packet(), NONCE)
        self.assertEqual(result["protocol"], "10.21")
        self.assertEqual(result["values"]["si_name"], "Test server")
        self.assertEqual(result["players"], [dict(slot=3, ping=42, name="Test player", clan="TAG", bot=False)])
        self.assertEqual(result["time_left_ms"], 10000)
        self.assertEqual(result["os_mask"], 7)

    def test_wrong_nonce_version_and_payload_length(self):
        with self.assertRaises(retail.ProtocolError):
            retail.parse_info(info_packet(), b"different")
        for offset, replacement in ((23, struct.pack("<I", 0x000e0029)), (27, struct.pack("<I", 100000))):
            data = bytearray(info_packet())
            data[offset:offset + 4] = replacement
            with self.assertRaises(retail.ProtocolError):
                retail.parse_info(bytes(data), NONCE)

    def test_all_truncations_and_trailing_bytes(self):
        for data, parse in ((info_packet(), lambda p: retail.parse_info(p, NONCE)),
                            (CHALLENGE, retail.parse_challenge), (PURE, retail.parse_pure),
                            (CONNECT, retail.parse_connect_response)):
            for size in range(len(data)):
                with self.subTest(packet=data[2:12], size=size), self.assertRaises(retail.ProtocolError):
                    parse(data[:size])
            with self.assertRaises(retail.ProtocolError):
                parse(data + b"\0")

    def test_challenge_and_connect_layout(self):
        challenge = retail.parse_challenge(CHALLENGE)
        self.assertEqual(challenge["channel_ids"], (0x1234, 0x5678))
        packet = retail.connect_packet(challenge, (0x4321, 0x7654), 0xaabbccdd)
        self.assertEqual(packet, b"\xff\xffconnect\0" + bytes.fromhex(
            "15000a00 0100 ddccbbaa 44332211 2143 5476 803e0000 00 00 00"))
        challenge["session_id"] = (1, 0)
        with self.assertRaises(retail.ProtocolError):
            retail.connect_packet(challenge, (1, 2), 0)
        challenge["session_id"] = (0, 0)
        challenge["game"] = "uninstalled_mod"
        with self.assertRaises(retail.ProtocolError):
            retail.connect_packet(challenge, (1, 2), 0)

    def test_invalid_player_records_and_legacy_challenge(self):
        for old, new in ((b"\x03\x2a\x00Test player", b"\x21\x2a\x00Test player"),
                         (b"Test player\0\x00TAG", b"Test player\0\x02TAG")):
            with self.assertRaises(retail.ProtocolError):
                retail.parse_info(info_packet().replace(old, new), NONCE)
        with self.assertRaises(retail.ProtocolError):
            retail.parse_challenge(b"\xff\xffchallengeResponse\0" + bytes.fromhex("44332211 3412 00 00"))

    def test_pure_and_miscellaneous_reply(self):
        self.assertEqual(retail.parse_pure(PURE), ([0x12345678, 0x87654321], 0x44332211))
        too_many = b"\xff\xffpureServer\0" + b"\x01\0\0\0" * 128 + b"\0" * 8
        with self.assertRaises(retail.ProtocolError):
            retail.parse_pure(too_many)
        self.assertEqual(retail.reply_description(b"\xff\xffprint\0" + b"\0" * 4 + b"Invalid game code pak.\n\0"),
                         dict(response="print", code=0, message="Invalid game code pak.\n"))

    def test_local_connect_response_and_sync_failure(self):
        result = retail.reply_description(CONNECT)
        self.assertEqual(result["channel_ids"], (0x2f65, 0x271c))
        self.assertEqual(result["client_num"], 0)
        self.assertEqual(result["game_init_id"], 1)
        self.assertEqual(result["game_frame"], 1255)
        self.assertEqual(result["game_time"], 41415)
        self.assertFalse(result["pure_wait"])
        self.assertEqual(result["server"]["si_map"], "maps/valley.entities")
        self.assertEqual(retail.reply_description(b"\xff\xffconnectFailed\0\x07\0\0\0"),
                         dict(response="connectFailed", code=7, reason="game/errors/servernosync"))
        for offset in (22, 35):
            bad = bytearray(CONNECT)
            bad[offset] = 255
            with self.assertRaises(retail.ProtocolError):
                retail.parse_connect_response(bytes(bad))

    def test_md4_reference_vectors(self):
        self.assertEqual(retail.md4(b"").hex(), "31d6cfe0d16ae931b73c59d7e0c089c0")
        self.assertEqual(retail.md4(b"abc").hex(), "a448017aaf21d8525fc10ae87aa6729d")

    def test_bounded_udp_handshake_reports_actual_native_module(self):
        with tempfile.TemporaryDirectory() as temp, socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server:
            base = Path(temp)
            with zipfile.ZipFile(base / "pak000.pk4", "w") as z:
                z.writestr("example.txt", "Test data")
            checksum = next(iter(retail.pak_inventory(base)))
            server.bind(("127.0.0.1", 0))
            server.settimeout(2)
            failures = []
            received = []

            def peer():
                try:
                    packet, addr = server.recvfrom(1024)
                    received.append(packet)
                    self.assertEqual(packet[:10], b"\xff\xffgetInfo\0")
                    server.sendto(info_packet(packet[10:]), addr)
                    packet, addr = server.recvfrom(1024)
                    received.append(packet)
                    self.assertEqual(packet[:12], b"\xff\xffchallenge\0")
                    self.assertEqual(len(packet), 17)
                    ids = struct.unpack("<HH", packet[12:16])
                    server.sendto(CHALLENGE, addr)
                    packet, addr = server.recvfrom(1024)
                    received.append(packet)
                    self.assertEqual(packet, retail.connect_packet(retail.parse_challenge(CHALLENGE), ids, 0))
                    server.sendto(b"\xff\xffpureServer\0" + struct.pack("<III", checksum, 0, 0xa02f1c18), addr)
                    packet, addr = server.recvfrom(1024)
                    received.append(packet)
                    self.assertEqual(packet, b"\xff\xffpureClient\0" + struct.pack("<IHIII", 0x11223344, ids[0], checksum, 0, 0))
                    server.sendto(b"\xff\xffprint\0" + b"\0" * 4 + b"Invalid game code pak.\n\0", addr)
                except BaseException as exc:
                    failures.append(exc)

            thread = threading.Thread(target=peer)
            thread.start()
            try:
                report = retail.probe(SimpleNamespace(endpoint=f"127.0.0.1:{server.getsockname()[1]}",
                                     base=base, timeout=2, handshake=True, check_native_module=True))
            finally:
                thread.join(3)
            if failures:
                raise failures[0]
            self.assertFalse(thread.is_alive())
            self.assertEqual(len(received), 4)
            self.assertFalse(report["connected"])
            self.assertEqual(report["stopped_at"], "native_module_validation")
            self.assertEqual(report["native_module_result"]["message"], "Invalid game code pak.\n")


if __name__ == "__main__":
    unittest.main()
