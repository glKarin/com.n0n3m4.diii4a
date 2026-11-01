# Retail ETQW network interoperability investigation

Status: **diagnostic handshake implemented; retail multiplayer is not implemented**.
This work is on `codex/etqw-retail-network`, separate from the Metal port PR.
The engine's `connect` command still uses the reconstructed Doom 3 transport.

## Reproduce the admission check

```sh
python3 doom3/macos/tools/probe_retail_server.py HOST:PORT
python3 doom3/macos/tools/probe_retail_server.py HOST:PORT \
  --handshake --base /path/to/ETQW/base --check-native-module
python3 doom3/macos/tests/test_retail_protocol.py -v
```

The first command requests server information. The second sends at most four
datagrams: query, challenge, connection request and native-module validation.
It does not join gameplay, change the running game, download files, read keys,
send an authentication ticket or impersonate a retail game-code package.
Each exchange has a three-second timeout. JSON reports always distinguish
diagnostics from a player connection; exit status zero means the probe ran.

`--check-native-module` accurately reports game-code package checksum zero:
the port loads an external ARM64 dylib and has no approved retail code PK4.
The probe sends verified local data-package checksums only when all requested
packages are present. It never copies the expected game-code hash into its reply.
PK4 identities follow the engine's existing MD4-of-CRC-directory algorithm.
This is the legacy package identity, not an independent integrity/security audit.

The wire parser rejects truncation, wrong query nonces, payload length mismatches,
unknown versions, excessive strings/checksum lists, duplicate player slots and
trailing bytes. Unit tests include byte-level fixtures, every prefix truncation,
MD4 reference vectors and a complete four-packet exchange with a loopback peer.

## Observed admission result, 2026-11-01

Target: `178.162.169.12:27733`, NEW EU Vanilla / Makron Horde.

- Server advertises ETQW 1.5, protocol **10.21**, `si_pure=1`, no password and
  PunkBuster disabled.
- Its challenge does not require a service-authentication session.
- The correctly framed connection request receives `pureServer`.
- All nine requested data-package identities match the local retail 1.5 files.
- The requested macOS game-code package is `a02f1c18`; no local PK4 has that hash.
- Reporting the actual native external-module state receives a `print` response,
  opcode zero, with **`Invalid game code pak.\n`**.
- No `connectResponse`, map initialization or gameplay snapshot was received.

| Required data package | Checksum |
| --- | --- |
| pak008.pk4 | 71a93b80 |
| pak007.pk4 | 074a1a2f |
| pak006.pk4 | 9edf1b7d |
| pak005.pk4 | 5ccc7213 |
| pak004.pk4 | 7e49f838 |
| pak003.pk4 | 99dfcabb |
| pak002.pk4 | 8dbe7353 |
| pak001.pk4 | 010e16e6 |
| pak000.pk4 | 442eb08b |

This is a concrete admission blocker for the current build on that server.
Resolving the code-package check alone would **not** establish multiplayer
compatibility: the transport and game-state work below is still required.
Testing it needs a controlled server whose policy explicitly accepts the native
module, or another supported arrangement with the server operator. Receiving a
matching retail code archive would not make its x86 module executable on ARM64.

## Wire formats established so far

Integers are little-endian. Connectionless messages start with `ff ff` followed
by a zero-terminated command name. These layouts are specific to retail 1.5;
they must not replace the reconstructed engine's local bot/server protocol.

- `getInfo`: 8-byte query nonce. `infoResponse`: echoed nonce, uint32 protocol,
  uint32 payload size, delta dictionary, player list, OS mask and server status.
  Player records include a slot byte, uint16 ping, name, clan position, clan name
  and bot flag; byte 32 terminates the list.
- `challenge`: two uint16 client channel IDs and a one-bit repeater flag.
  `challengeResponse`: uint32 challenge, two uint16 server channel IDs, two
  uint32 words of session ID, game-base string and game string.
- `connect`: uint32 protocol, int16 OS ID, uint32 declaration checksum,
  uint32 server challenge, two uint16 client channel IDs, uint32 rate, one-bit
  authentication-ticket-present flag, byte-aligned password and repeater bit.
  The diagnostic uses a zero declaration checksum because it has no loaded
  declaration table; acceptance of this initial request does not validate it.
- `pureServer`: zero-terminated list of uint32 data-package checksums, followed
  by a uint32 game-code-package checksum.
- `pureClient`: uint32 challenge, uint16 first client channel ID, the locally
  validated checksum list and actual game-code-package checksum.

The server-query format is documented in the
[Splash Damage forum protocol description](https://forums.splashdamage.com/t/quake-wars-master-server-o/138305).
Handshake field order was inspected in the user's local Mac 1.5 update executable
and confirmed by bounded exchanges with the target server. No executable,
disassembly, keys, player identities or commercial assets are included here.

## Work still required before a playable retail connection

1. Integrate an explicit retail connection state machine into the engine,
   including admission errors and supported authentication/mod transitions.
2. Implement the two retail message channels and validate fragmentation,
   reliability, compression, game initialization and reconnection against a
   controlled retail server. The existing channel is Doom 3-derived.
3. Reconcile retail snapshot and user-command framing with the game SDK. The
   current adapter passes the same message as both snapshot and user-command
   streams; this has not been validated against the retail transport.
4. Establish an accepted native game-module/data-package policy with the test
   server. Keep verification honest and exclude incompatible local overrides.
5. Test map loading, spawning, movement, combat, map changes and disconnects
   before attempting a public gameplay session or claiming compatibility.
