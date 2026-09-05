#!/usr/bin/env python3
"""
geob_tool.py -- add / inspect / verify a C2PA-style GEOB ID3 frame in an MP3.

Purpose (SquareDesk issue #1710): C2PA provenance manifests ride inside an
ID3v2 GEOB (General Encapsulated Object) frame.  We need to prove that
SquareDesk's tag writer (TagLib) does not blow that frame away when it edits
ordinary tags such as TITLE / ARTIST / or the LOOPSTART / LOOPLENGTH
user-text frames.

Workflow:

    # 1. stamp a synthetic C2PA manifest into the file, and remember it
    python3 geob_tool.py --add "AAA 100 - Hello Test.mp3"

    # 2. ...now edit ID3 tags with SquareDesk (LOOPSTART, LOOPLENGTH, ...)

    # 3. confirm the GEOB frame survived byte-for-byte
    python3 geob_tool.py --check "AAA 100 - Hello Test.mp3"

    # anytime: show every ID3 frame in the file
    python3 geob_tool.py --dump "AAA 100 - Hello Test.mp3"

--add writes a sidecar reference file next to the MP3
("<name>.mp3.geobref.json") holding a copy of the frame it wrote.  --check
compares what is in the MP3 now against that reference and reports exactly
which part changed.

No third-party modules required (no mutagen / no taglib) -- that is
deliberate, so the checker is independent of the library under test.

ID3v2.2, v2.2 with "GEO", v2.3 and v2.4 tags are all understood on read,
including whole-tag unsynchronisation, extended headers, per-frame
compression/encryption/grouping flags and data-length indicators.  On write
the tag is emitted as ID3v2.3 or ID3v2.4 (default: keep the file's existing
version if it is already 2.3/2.4, otherwise upgrade 2.2 -> 2.4).
"""

import argparse
import base64
import hashlib
import json
import os
import shutil
import struct
import sys

# ---------------------------------------------------------------------------
# C2PA specifics
# ---------------------------------------------------------------------------

# Per the C2PA spec, an MP3 manifest store lives in an ID3v2 GEOB frame whose
# MIME type is "application/x-c2pa-manifest-store" and whose content
# description is "c2pa".  The payload is a JUMBF box structure.
C2PA_MIME = "application/x-c2pa-manifest-store"
C2PA_DESCRIPTION = "c2pa"
C2PA_FILENAME = ""

TEXT_ENCODING_LATIN1 = 0x00

# ---------------------------------------------------------------------------
# small helpers
# ---------------------------------------------------------------------------


def syncsafe_encode(n):
    if n < 0 or n > 0x0FFFFFFF:
        raise ValueError("value %d does not fit in a syncsafe integer" % n)
    return bytes(((n >> 21) & 0x7F, (n >> 14) & 0x7F, (n >> 7) & 0x7F, n & 0x7F))


def syncsafe_decode(b):
    v = 0
    for byte in b:
        v = (v << 7) | (byte & 0x7F)
    return v


def deunsynchronise(data):
    """Undo ID3 unsynchronisation: 0xFF 0x00 -> 0xFF."""
    return data.replace(b"\xff\x00", b"\xff")


def needs_unsynchronisation(data):
    for i in range(len(data) - 1):
        if data[i] == 0xFF and (data[i + 1] == 0x00 or data[i + 1] >= 0xE0):
            return True
    return bool(data) and data[-1] == 0xFF


def split_terminated(data, offset, encoding):
    """Return (text, next_offset) for a null-terminated string at offset."""
    if encoding in (0x01, 0x02):  # UTF-16 (with / without BOM): 2-byte terminator
        i = offset
        while i + 1 < len(data):
            if data[i] == 0 and data[i + 1] == 0:
                raw = data[offset:i]
                return raw.decode("utf-16" if encoding == 0x01 else "utf-16-be",
                                  "replace"), i + 2
            i += 2
        return data[offset:].decode("utf-16", "replace"), len(data)

    codec = "utf-8" if encoding == 0x03 else "latin-1"
    i = data.find(b"\x00", offset)
    if i < 0:
        return data[offset:].decode(codec, "replace"), len(data)
    return data[offset:i].decode(codec, "replace"), i + 1


# ---------------------------------------------------------------------------
# ID3v2 tag reading
# ---------------------------------------------------------------------------


class Frame(object):
    def __init__(self, frame_id, data, flags=0):
        self.id = frame_id          # normalised 4-char id ("GEOB", "TIT2", ...)
        self.raw_id = frame_id      # id as it appeared in the file
        self.data = data            # frame payload, already de-unsynchronised
        self.flags = flags

    def __repr__(self):
        return "<Frame %s %d bytes>" % (self.id, len(self.data))


# ID3v2.2 (3-char) -> ID3v2.3/2.4 (4-char) frame id upgrades we care about.
V22_TO_V23 = {
    "BUF": "RBUF", "CNT": "PCNT", "COM": "COMM", "CRA": "AENC", "ETC": "ETCO",
    "EQU": "EQUA", "GEO": "GEOB", "IPL": "IPLS", "LNK": "LINK", "MCI": "MCDI",
    "MLL": "MLLT", "PIC": "APIC", "POP": "POPM", "REV": "RVRB", "RVA": "RVAD",
    "SLT": "SYLT", "STC": "SYTC", "TAL": "TALB", "TBP": "TBPM", "TCM": "TCOM",
    "TCO": "TCON", "TCR": "TCOP", "TDA": "TDAT", "TDY": "TDLY", "TEN": "TENC",
    "TFT": "TFLT", "TIM": "TIME", "TKE": "TKEY", "TLA": "TLAN", "TLE": "TLEN",
    "TMT": "TMED", "TOA": "TOPE", "TOF": "TOFN", "TOL": "TOLY", "TOR": "TORY",
    "TOT": "TOAL", "TP1": "TPE1", "TP2": "TPE2", "TP3": "TPE3", "TP4": "TPE4",
    "TPA": "TPOS", "TPB": "TPUB", "TRC": "TSRC", "TRD": "TRDA", "TRK": "TRCK",
    "TSI": "TSIZ", "TSS": "TSSE", "TT1": "TIT1", "TT2": "TIT2", "TT3": "TIT3",
    "TXT": "TEXT", "TXX": "TXXX", "TYE": "TYER", "UFI": "UFID", "ULT": "USLT",
    "WAF": "WOAF", "WAR": "WOAR", "WAS": "WOAS", "WCM": "WCOM", "WCP": "WCOP",
    "WPB": "WPUB", "WXX": "WXXX",
}


class Id3Tag(object):
    def __init__(self, major=4, revision=0, frames=None):
        self.major = major
        self.revision = revision
        self.frames = frames if frames is not None else []
        self.present = True

    def find(self, frame_id):
        return [f for f in self.frames if f.id == frame_id]


def _plausible_frame_at(body, pos):
    """True if pos is end-of-tag, padding, or the start of a sane v2.3/2.4 frame."""
    if pos == len(body):
        return True
    if pos > len(body) - 10:
        return False
    fid = body[pos:pos + 4]
    if fid == b"\x00\x00\x00\x00":
        return True
    return all(0x30 <= b <= 0x5A for b in fid)


def read_id3_tag(path):
    """Return (Id3Tag, audio_offset).  Id3Tag.present is False when absent."""
    with open(path, "rb") as fh:
        header = fh.read(10)
        if len(header) < 10 or header[:3] != b"ID3":
            tag = Id3Tag(major=4)
            tag.present = False
            return tag, 0
        major, revision, flags = header[3], header[4], header[5]
        size = syncsafe_decode(header[6:10])
        body = fh.read(size)

    audio_offset = 10 + size
    tag = Id3Tag(major=major, revision=revision)

    unsync_whole_tag = bool(flags & 0x80) and major < 4
    if unsync_whole_tag:
        body = deunsynchronise(body)

    pos = 0
    # extended header
    if flags & 0x40:
        if major == 3:
            ext_size = struct.unpack(">I", body[0:4])[0]
            pos = 4 + ext_size
        elif major == 4:
            ext_size = syncsafe_decode(body[0:4])
            pos = ext_size  # v2.4 ext size includes its own 4 bytes

    id_len = 3 if major == 2 else 4
    hdr_len = 6 if major == 2 else 10

    while pos + hdr_len <= len(body):
        raw_id = body[pos:pos + id_len]
        if raw_id[0:1] in (b"\x00", b""):
            break  # padding
        if major == 2:
            fsize = int.from_bytes(body[pos + 3:pos + 6], "big")
            fflags = 0
        else:
            raw_size = body[pos + 4:pos + 8]
            if major == 4:
                fsize = syncsafe_decode(raw_size)
                # Some encoders wrongly write v2.4 frame sizes as plain
                # big-endian.  Trust syncsafe unless it does not land on a
                # plausible next frame while the flat reading does.
                flat = int.from_bytes(raw_size, "big")
                if flat != fsize and not _plausible_frame_at(body, pos + 10 + fsize) \
                        and _plausible_frame_at(body, pos + 10 + flat):
                    fsize = flat
            else:
                fsize = int.from_bytes(raw_size, "big")
            fflags = int.from_bytes(body[pos + 8:pos + 10], "big")

        if fsize <= 0 or pos + hdr_len + fsize > len(body):
            break

        payload = body[pos + hdr_len:pos + hdr_len + fsize]
        pos += hdr_len + fsize

        # per-frame flags
        if major == 4:
            if fflags & 0x0002:  # frame-level unsynchronisation
                payload = deunsynchronise(payload)
            if fflags & 0x0040:  # grouping identity
                payload = payload[1:]
            if fflags & 0x0004:  # compressed
                pass  # left alone; we never touch such frames
            if fflags & 0x0008:  # encrypted
                payload = payload[1:]
            if fflags & 0x0001:  # data length indicator
                payload = payload[4:]
        elif major == 3:
            if fflags & 0x0080:  # compressed -> 4-byte decompressed size
                payload = payload[4:]
            if fflags & 0x0040:  # encrypted
                payload = payload[1:]
            if fflags & 0x0020:  # grouping
                payload = payload[1:]

        fid = raw_id.decode("latin-1", "replace")
        norm = V22_TO_V23.get(fid, fid) if major == 2 else fid
        frame = Frame(norm, payload, fflags)
        frame.raw_id = fid
        tag.frames.append(frame)

    return tag, audio_offset


# ---------------------------------------------------------------------------
# ID3v2 tag writing
# ---------------------------------------------------------------------------


def build_id3_tag(frames, major=4, padding=2048):
    if major not in (3, 4):
        raise ValueError("can only write ID3v2.3 or ID3v2.4")
    body = bytearray()
    for f in frames:
        fid = f.id
        if len(fid) == 3:
            fid = V22_TO_V23.get(fid, fid)
        if len(fid) != 4:
            raise ValueError("bad frame id %r" % fid)
        size = len(f.data)
        size_bytes = syncsafe_encode(size) if major == 4 \
            else struct.pack(">I", size)
        body += fid.encode("latin-1") + size_bytes + b"\x00\x00" + f.data
    body += b"\x00" * padding

    flags = 0x00
    return b"ID3" + bytes((major, 0, flags)) + syncsafe_encode(len(body)) + bytes(body)


# ---------------------------------------------------------------------------
# GEOB frame encode / decode
# ---------------------------------------------------------------------------


def encode_geob(mime, filename, description, data):
    out = bytearray()
    out.append(TEXT_ENCODING_LATIN1)
    out += mime.encode("latin-1") + b"\x00"
    out += filename.encode("latin-1") + b"\x00"
    out += description.encode("latin-1") + b"\x00"
    out += data
    return bytes(out)


def decode_geob(payload):
    if not payload:
        return None
    encoding = payload[0]
    mime, pos = split_terminated(payload, 1, TEXT_ENCODING_LATIN1)  # MIME is always latin-1
    filename, pos = split_terminated(payload, pos, encoding)
    description, pos = split_terminated(payload, pos, encoding)
    return {
        "encoding": encoding,
        "mime": mime,
        "filename": filename,
        "description": description,
        "data": payload[pos:],
    }


# ---------------------------------------------------------------------------
# synthetic C2PA / JUMBF payload
# ---------------------------------------------------------------------------


def jumbf_box(box_type, payload):
    return struct.pack(">I", 8 + len(payload)) + box_type + payload


def make_c2pa_payload(source_path):
    """Build a structurally-valid (but unsigned) JUMBF manifest store.

    Real c2pa-cli output is a signed COSE structure; we only need realistic,
    non-trivial binary content -- including 0xFF bytes and embedded nulls,
    which is exactly what shakes out unsynchronisation and string-termination
    bugs in a tag writer.
    """
    title = os.path.basename(source_path)
    manifest = {
        "claim_generator": "geob_tool.py/1.0 (SquareDesk issue #1710 test fixture)",
        "title": title,
        "format": "audio/mpeg",
        "assertions": [
            {"label": "c2pa.actions",
             "data": {"actions": [{"action": "c2pa.created",
                                   "digitalSourceType":
                                   "http://cv.iptc.org/newscodes/digitalsourcetype/digitalCapture"}]}},
            {"label": "cawg.training-mining",
             "data": {"entries": {"cawg.ai_generative_training": {"use": "notAllowed"},
                                  "cawg.ai_inference": {"use": "notAllowed"}}}},
        ],
    }
    claim = json.dumps(manifest, separators=(",", ":")).encode("utf-8")

    # jumd description box: 16-byte UUID + toggles + label
    def jumd(uuid_bytes, label):
        return jumbf_box(b"jumd", uuid_bytes + b"\x03" + label.encode("utf-8") + b"\x00")

    json_uuid = bytes.fromhex("6a736f6e0011001000800000aa00389b71")[:16]
    c2pa_uuid = bytes.fromhex("63327061 0011 0010 8000 00aa00389b71".replace(" ", ""))[:16]

    claim_super = jumbf_box(b"jumb",
                            jumd(json_uuid, "c2pa.claim") + jumbf_box(b"json", claim))
    # A fake signature blob, full of high bytes so unsynchronisation matters.
    sig = hashlib.sha256(claim).digest() * 8
    sig = bytes((b | 0xE0) if i % 7 == 0 else b for i, b in enumerate(sig))
    sig = sig.replace(b"\x00", b"\xff\x00")
    sig_super = jumbf_box(b"jumb",
                          jumd(json_uuid, "c2pa.signature") + jumbf_box(b"uuid", sig))

    manifest_super = jumbf_box(b"jumb",
                               jumd(c2pa_uuid, "urn:c2pa:squaredesk-test-manifest")
                               + claim_super + sig_super)
    return jumbf_box(b"jumb", jumd(c2pa_uuid, "c2pa") + manifest_super)


# ---------------------------------------------------------------------------
# sidecar reference file
# ---------------------------------------------------------------------------


def ref_path_for(mp3_path):
    return mp3_path + ".geobref.json"


def frame_fingerprint(geob, raw_payload):
    return {
        "encoding": geob["encoding"],
        "mime": geob["mime"],
        "filename": geob["filename"],
        "description": geob["description"],
        "data_length": len(geob["data"]),
        "data_sha256": hashlib.sha256(geob["data"]).hexdigest(),
        "payload_sha256": hashlib.sha256(raw_payload).hexdigest(),
        "payload_b64": base64.b64encode(raw_payload).decode("ascii"),
    }


# ---------------------------------------------------------------------------
# commands
# ---------------------------------------------------------------------------


def find_c2pa_geob(tag):
    """Return (Frame, decoded) for the C2PA GEOB frame, or (None, None)."""
    fallback = None
    for f in tag.find("GEOB"):
        g = decode_geob(f.data)
        if g is None:
            continue
        if g["mime"] == C2PA_MIME or g["description"] == C2PA_DESCRIPTION:
            return f, g
        if fallback is None:
            fallback = (f, g)
    return fallback if fallback else (None, None)


def cmd_add(args):
    path = args.mp3
    tag, audio_offset = read_id3_tag(path)

    if not tag.present:
        print("No ID3v2 tag found -- creating one.")
    else:
        print("Found ID3v2.%d.%d tag, %d frame(s), audio starts at offset %d."
              % (tag.major, tag.revision, len(tag.frames), audio_offset))

    existing, _ = find_c2pa_geob(tag)
    if existing is not None and not args.force:
        print("ERROR: a C2PA GEOB frame is already present. Use --force to replace it.",
              file=sys.stderr)
        return 1

    out_major = args.id3_version
    if out_major is None:
        out_major = tag.major if tag.major in (3, 4) else 4
    if tag.present and tag.major == 2:
        print("Upgrading ID3v2.2 tag to ID3v2.%d (v2.2 has no room for large frames)."
              % out_major)

    payload = make_c2pa_payload(path)
    geob_payload = encode_geob(C2PA_MIME, C2PA_FILENAME, C2PA_DESCRIPTION, payload)

    frames = [f for f in tag.frames
              if not (f.id == "GEOB" and existing is not None and f is existing)]
    frames.append(Frame("GEOB", geob_payload))

    new_tag = build_id3_tag(frames, major=out_major, padding=args.padding)

    if args.backup:
        backup = path + ".bak"
        shutil.copy2(path, backup)
        print("Backup written to %s" % backup)

    with open(path, "rb") as fh:
        fh.seek(audio_offset)
        audio = fh.read()
    tmp = path + ".geobtmp"
    with open(tmp, "wb") as fh:
        fh.write(new_tag)
        fh.write(audio)
    os.replace(tmp, path)

    ref = {
        "mp3": os.path.basename(path),
        "written_id3_version": "2.%d" % out_major,
        "geob": frame_fingerprint(decode_geob(geob_payload), geob_payload),
    }
    with open(ref_path_for(path), "w") as fh:
        json.dump(ref, fh, indent=2)

    print("Wrote GEOB frame: mime=%s description=%r %d bytes of JUMBF data"
          % (C2PA_MIME, C2PA_DESCRIPTION, len(payload)))
    print("  frame payload sha256 = %s" % ref["geob"]["payload_sha256"])
    print("Reference saved to %s" % ref_path_for(path))
    print("\nNow edit the tags with SquareDesk, then run:")
    print('  python3 %s --check "%s"' % (os.path.basename(__file__), path))
    return 0


def cmd_check(args):
    path = args.mp3
    ref_path = ref_path_for(path)
    if not os.path.exists(ref_path):
        print("ERROR: no reference file %s -- run --add first." % ref_path,
              file=sys.stderr)
        return 2
    with open(ref_path) as fh:
        ref = json.load(fh)["geob"]

    tag, audio_offset = read_id3_tag(path)
    if not tag.present:
        print("FAIL: the ID3v2 tag is gone entirely.")
        return 1

    print("File now has an ID3v2.%d.%d tag with %d frame(s): %s"
          % (tag.major, tag.revision, len(tag.frames),
             ", ".join(f.id for f in tag.frames)))

    frame, geob = find_c2pa_geob(tag)
    if frame is None:
        print("\nFAIL: the C2PA GEOB frame is GONE. The tag writer destroyed it.")
        return 1

    now = frame_fingerprint(geob, frame.data)

    problems = []
    for key, label in (("mime", "MIME type"),
                       ("filename", "filename"),
                       ("description", "content description"),
                       ("encoding", "text encoding byte"),
                       ("data_length", "payload length"),
                       ("data_sha256", "payload SHA-256")):
        if now[key] != ref[key]:
            problems.append("  %-20s expected %r, got %r"
                            % (label + ":", ref[key], now[key]))

    if not problems:
        print("\nPASS: the C2PA GEOB frame is byte-for-byte intact.")
        print("  mime        = %s" % now["mime"])
        print("  description = %r" % now["description"])
        print("  data        = %d bytes, sha256 %s"
              % (now["data_length"], now["data_sha256"]))
        return 0

    print("\nFAIL: the GEOB frame survived but was altered:")
    for p in problems:
        print(p)

    old = base64.b64decode(ref["payload_b64"])
    new = frame.data
    n = min(len(old), len(new))
    for i in range(n):
        if old[i] != new[i]:
            lo = max(0, i - 8)
            print("\n  first difference at byte %d of the frame payload:" % i)
            print("    was: %s" % old[lo:i + 8].hex(" "))
            print("    now: %s" % new[lo:i + 8].hex(" "))
            break
    else:
        if len(old) != len(new):
            print("\n  payload was truncated/extended: %d -> %d bytes"
                  % (len(old), len(new)))
    return 1


def cmd_dump(args):
    path = args.mp3
    tag, audio_offset = read_id3_tag(path)
    if not tag.present:
        print("No ID3v2 tag.")
        return 0
    print("ID3v2.%d.%d  tag size %d bytes  audio starts at %d"
          % (tag.major, tag.revision, audio_offset - 10, audio_offset))
    for f in tag.frames:
        label = f.id if f.raw_id == f.id else "%s (%s)" % (f.id, f.raw_id)
        if f.id == "GEOB":
            g = decode_geob(f.data)
            print("  %-12s %7d bytes  mime=%r filename=%r description=%r data=%d bytes"
                  % (label, len(f.data), g["mime"], g["filename"],
                     g["description"], len(g["data"])))
            print("               %s sha256=%s"
                  % (" " * 7, hashlib.sha256(g["data"]).hexdigest()))
        elif f.id.startswith("T") or f.id in ("COMM", "USLT"):
            enc = f.data[0] if f.data else 0
            codec = {0: "latin-1", 1: "utf-16", 2: "utf-16-be", 3: "utf-8"}.get(enc, "latin-1")
            text = f.data[1:].decode(codec, "replace").replace("\x00", " | ")
            print("  %-12s %7d bytes  %r" % (label, len(f.data), text[:120]))
        else:
            print("  %-12s %7d bytes" % (label, len(f.data)))
    return 0


def main():
    ap = argparse.ArgumentParser(
        description="Add / inspect / verify a C2PA-style GEOB ID3 frame in an MP3.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__)
    mode = ap.add_mutually_exclusive_group(required=True)
    mode.add_argument("--add", action="store_true",
                      help="write a C2PA GEOB frame and save a reference sidecar")
    mode.add_argument("--check", action="store_true",
                      help="verify the GEOB frame still matches the sidecar reference")
    mode.add_argument("--dump", action="store_true",
                      help="list every ID3 frame in the file")
    ap.add_argument("mp3", help="path to the MP3 file")
    ap.add_argument("--force", action="store_true",
                    help="--add: replace an existing C2PA GEOB frame")
    ap.add_argument("--no-backup", dest="backup", action="store_false",
                    help="--add: do not write a .bak copy first")
    ap.add_argument("--id3-version", type=int, choices=(3, 4), default=None,
                    help="--add: ID3v2 minor version to write (default: keep 2.3/2.4, "
                         "upgrade 2.2 to 2.4)")
    ap.add_argument("--padding", type=int, default=2048,
                    help="--add: bytes of padding to leave after the frames "
                         "(default 2048)")
    args = ap.parse_args()

    if not os.path.exists(args.mp3):
        print("ERROR: no such file: %s" % args.mp3, file=sys.stderr)
        return 2

    if args.add:
        return cmd_add(args)
    if args.check:
        return cmd_check(args)
    return cmd_dump(args)


if __name__ == "__main__":
    sys.exit(main())
