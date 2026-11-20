#!/usr/bin/env python3
"""Pure Python 3.4+ stdlib unified diff patch applier (binary/CRLF-safe).

Designed so Windows XP users without git or GNU patch.exe can apply and reverse
patches cleanly while preserving raw Big5/GBK bytes and CRLF line endings.

Usage:
    python apply_patch.py [--check] [-R] [-d ROOT] PATCH [PATCH ...]
"""

import argparse
import os
import re
import sys

HUNK_RE = re.compile(br"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@")


def split_lines_preserve_eol(data):
    """Return list of (body_bytes, eol_bytes) preserving exact line endings."""
    out = []
    i = 0
    n = len(data)
    while i < n:
        nl = data.find(b"\n", i)
        if nl == -1:
            line = data[i:]
            out.append((line, b""))
            break
        if nl > i and data[nl - 1 : nl] == b"\r":
            out.append((data[i : nl - 1], b"\r\n"))
        else:
            out.append((data[i:nl], b"\n"))
        i = nl + 1
    return out


def dominant_eol(lines, path):
    """Determine preferred line ending for inserted lines in a file."""
    crlf = sum(1 for _, eol in lines if eol == b"\r\n")
    lf = sum(1 for _, eol in lines if eol == b"\n")
    if crlf > lf:
        return b"\r\n"
    if lf > crlf:
        return b"\n"
    ext = os.path.splitext(path)[1].lower()
    if ext in (".frm", ".bas", ".cls", ".vbp", ".vbw", ".bat", ".cmd"):
        return b"\r\n"
    return b"\n"


def strip_prefix(raw_path, p_strip):
    """Strip p_strip leading path components from a diff path."""
    text = raw_path.decode("utf-8", "surrogateescape").strip()
    if text.startswith('"') and text.endswith('"'):
        text = text[1:-1]
    # Remove optional timestamp after tab
    if "\t" in text:
        text = text.split("\t", 1)[0]
    if text == "/dev/null":
        return None
    parts = text.replace("\\", "/").split("/")
    if p_strip > 0 and len(parts) > p_strip:
        parts = parts[p_strip:]
    return "/".join(parts)


def parse_patch(patch_bytes, p_strip=1):
    """Parse a unified diff into a list of file patch dicts."""
    raw_lines = split_lines_preserve_eol(patch_bytes)
    files = []
    cur_file = None
    i = 0
    total = len(raw_lines)

    while i < total:
        body, _ = raw_lines[i]
        if body.startswith(b"diff --git "):
            cur_file = {"old_path": None, "new_path": None, "hunks": []}
            files.append(cur_file)
            i += 1
            continue

        if body.startswith(b"--- ") and i + 1 < total and raw_lines[i + 1][0].startswith(b"+++ "):
            old_p = strip_prefix(body[4:], p_strip)
            new_p = strip_prefix(raw_lines[i + 1][0][4:], p_strip)
            if cur_file is None or (cur_file["old_path"] is not None or cur_file["hunks"]):
                cur_file = {"old_path": old_p, "new_path": new_p, "hunks": []}
                files.append(cur_file)
            else:
                cur_file["old_path"] = old_p
                cur_file["new_path"] = new_p
            i += 2
            continue

        m = HUNK_RE.match(body)
        if m:
            if cur_file is None:
                raise ValueError("Found hunk header before file header at line %d" % (i + 1))
            old_start = int(m.group(1))
            old_cnt = int(m.group(2)) if m.group(2) is not None else 1
            new_start = int(m.group(3))
            new_cnt = int(m.group(4)) if m.group(4) is not None else 1
            i += 1

            ops = []
            old_seen = 0
            new_seen = 0
            old_no_eol = False
            new_no_eol = False
            last_tag = None

            while i < total and (old_seen < old_cnt or new_seen < new_cnt):
                hline, _ = raw_lines[i]
                if hline.startswith(b"\\ "):
                    if last_tag in (b"-", b" "):
                        old_no_eol = True
                    if last_tag in (b"+", b" "):
                        new_no_eol = True
                    i += 1
                    continue
                if hline == b"":
                    # Treat blank line inside hunk body as empty context line
                    tag = b" "
                    content = b""
                else:
                    tag = hline[:1]
                    content = hline[1:]
                if tag not in (b" ", b"-", b"+"):
                    raise ValueError("Unexpected hunk line %r at patch line %d" % (hline, i + 1))
                ops.append((tag, content))
                last_tag = tag
                if tag in (b" ", b"-"):
                    old_seen += 1
                if tag in (b" ", b"+"):
                    new_seen += 1
                i += 1

            # Consume trailing "\ No newline at end of file" immediately after last op
            while i < total and raw_lines[i][0].startswith(b"\\ "):
                if last_tag in (b"-", b" "):
                    old_no_eol = True
                if last_tag in (b"+", b" "):
                    new_no_eol = True
                i += 1

            if old_seen != old_cnt or new_seen != new_cnt:
                raise ValueError(
                    "Truncated hunk for %s: expected -%d,+%d got -%d,+%d"
                    % (cur_file["new_path"] or cur_file["old_path"], old_cnt, new_cnt, old_seen, new_seen)
                )

            cur_file["hunks"].append({
                "old_start": old_start,
                "old_cnt": old_cnt,
                "new_start": new_start,
                "new_cnt": new_cnt,
                "ops": ops,
                "old_no_eol": old_no_eol,
                "new_no_eol": new_no_eol,
            })
            continue

        i += 1

    return files


def reverse_hunk(hunk):
    """Return a reversed copy of a hunk dict."""
    rev_ops = []
    for tag, content in hunk["ops"]:
        if tag == b"-":
            rev_ops.append((b"+", content))
        elif tag == b"+":
            rev_ops.append((b"-", content))
        else:
            rev_ops.append((b" ", content))
    return {
        "old_start": hunk["new_start"],
        "old_cnt": hunk["new_cnt"],
        "new_start": hunk["old_start"],
        "new_cnt": hunk["old_cnt"],
        "ops": rev_ops,
        "old_no_eol": hunk["new_no_eol"],
        "new_no_eol": hunk["old_no_eol"],
    }


def hunk_matches_at(lines, pos, expected_old, check_no_eol, target_ends_with_eof_no_eol):
    """Check whether expected_old bodies match lines[pos:pos+len(expected_old)]."""
    n = len(expected_old)
    if pos < 0 or pos + n > len(lines):
        return False
    for k in range(n):
        if lines[pos + k][0] != expected_old[k]:
            return False
    if check_no_eol and pos + n != len(lines):
        return False
    if check_no_eol and not target_ends_with_eof_no_eol and n > 0:
        return False
    return True


def find_hunk_pos(lines, expected_old, hint_pos, min_pos, check_no_eol):
    """Locate hunk position starting from hint_pos, scanning outward cleanly."""
    if not expected_old:
        if hint_pos <= 0:
            return 0
        return min(hint_pos, len(lines))

    eof_no_eol = bool(lines and lines[-1][1] == b"")
    max_pos = len(lines) - len(expected_old)
    if max_pos < min_pos:
        return -1

    clamped_hint = max(min_pos, min(hint_pos, max_pos))
    if hunk_matches_at(lines, clamped_hint, expected_old, check_no_eol, eof_no_eol):
        return clamped_hint

    max_delta = max(clamped_hint - min_pos, max_pos - clamped_hint)
    for d in range(1, max_delta + 1):
        for cand in (clamped_hint - d, clamped_hint + d):
            if min_pos <= cand <= max_pos:
                if hunk_matches_at(lines, cand, expected_old, check_no_eol, eof_no_eol):
                    return cand
    return -1


def apply_file_patch(orig_bytes, rel_path, hunks, reverse=False):
    """Apply all hunks for one file and return the resulting bytes."""
    lines = split_lines_preserve_eol(orig_bytes)
    eol = dominant_eol(lines, rel_path)
    cursor = 0
    drift = 0

    ordered_hunks = hunks if not reverse else [reverse_hunk(h) for h in hunks]
    for idx, h in enumerate(ordered_hunks, 1):
        expected_old = [c for t, c in h["ops"] if t in (b" ", b"-")]
        hint = (h["old_start"] - 1 + drift) if h["old_start"] > 0 else 0
        pos = find_hunk_pos(lines, expected_old, hint, cursor, h["old_no_eol"])
        if pos < 0:
            raise ValueError(
                "Hunk #%d failed for %s at line %d"
                % (idx, rel_path, h["old_start"])
            )

        old_slice = lines[pos : pos + len(expected_old)]
        old_iter = iter(old_slice)
        new_slice = []
        for tag, content in h["ops"]:
            if tag == b" ":
                _, orig_eol = next(old_iter)
                new_slice.append((content, orig_eol if orig_eol else eol))
            elif tag == b"-":
                next(old_iter)
            elif tag == b"+":
                new_slice.append((content, eol))

        if new_slice:
            if h["new_no_eol"]:
                last_body, _ = new_slice[-1]
                new_slice[-1] = (last_body, b"")
            elif pos + len(expected_old) < len(lines) and new_slice[-1][1] == b"":
                last_body, _ = new_slice[-1]
                new_slice[-1] = (last_body, eol)

        lines[pos : pos + len(expected_old)] = new_slice
        cursor = pos + len(new_slice)
        drift += len(new_slice) - len(expected_old)

    # Ensure non-EOF lines never lack EOL
    for k in range(len(lines) - 1):
        if lines[k][1] == b"":
            lines[k] = (lines[k][0], eol)

    return b"".join(body + leol for body, leol in lines)


def apply_patch_files(root_dir, patch_paths, reverse=False, check_only=False, p_strip=1):
    """Apply all given patch files transactionally across virtual buffer."""
    cache = {}  # rel_path -> bytes or None (deleted)

    def read_virtual(rel_path):
        if rel_path in cache:
            return cache[rel_path]
        full = os.path.join(root_dir, rel_path.replace("/", os.sep))
        if not os.path.exists(full):
            return None
        with open(full, "rb") as f:
            data = f.read()
        cache[rel_path] = data
        return data

    applied_count = 0
    ordered_patches = list(reversed(patch_paths)) if reverse else patch_paths
    for pp in ordered_patches:
        with open(pp, "rb") as f:
            patch_bytes = f.read()
        file_patches = parse_patch(patch_bytes, p_strip=p_strip)
        if not file_patches:
            raise ValueError("No hunks found in patch: %s" % pp)

        order = file_patches if not reverse else list(file_patches)
        for fp in order:
            if not fp["hunks"]:
                continue
            rel = (fp["new_path"] or fp["old_path"]) if not reverse else (fp["old_path"] or fp["new_path"])
            if rel is None:
                raise ValueError("Patch %s missing valid target path" % pp)

            is_create = (fp["old_path"] is None) if not reverse else (fp["new_path"] is None)
            is_delete = (fp["new_path"] is None) if not reverse else (fp["old_path"] is None)

            if is_create:
                if read_virtual(rel) is not None and not fp["hunks"]:
                    raise ValueError("Target already exists for creation: %s" % rel)
                orig = read_virtual(rel) or b""
                updated = apply_file_patch(orig, rel, fp["hunks"], reverse=reverse)
                cache[rel] = updated
            elif is_delete:
                orig = read_virtual(rel)
                if orig is None:
                    raise ValueError("Missing target file for deletion: %s" % rel)
                updated = apply_file_patch(orig, rel, fp["hunks"], reverse=reverse)
                if updated != b"":
                    raise ValueError("Deletion patch did not empty %s" % rel)
                cache[rel] = None
            else:
                orig = read_virtual(rel)
                if orig is None:
                    raise ValueError("Missing target file: %s" % rel)
                updated = apply_file_patch(orig, rel, fp["hunks"], reverse=reverse)
                cache[rel] = updated
            applied_count += 1

    if not check_only:
        for rel, content in sorted(cache.items()):
            full = os.path.join(root_dir, rel.replace("/", os.sep))
            if content is None:
                if os.path.exists(full):
                    os.remove(full)
            else:
                parent = os.path.dirname(full)
                if parent and not os.path.isdir(parent):
                    os.makedirs(parent)
                with open(full, "wb") as f:
                    f.write(content)

    return applied_count, sorted(cache.keys())


def main():
    parser = argparse.ArgumentParser(description="Apply unified diffs cleanly (Python 3.4+ stdlib)")
    parser.add_argument("patches", nargs="+", help="Patch file(s) to apply in order")
    parser.add_argument("-d", "--directory", default=".", help="Repository root directory (default: .)")
    parser.add_argument("-p", "--strip", type=int, default=1, help="Strip N leading path components (default: 1)")
    parser.add_argument("-R", "--reverse", action="store_true", help="Reverse-apply patch(es)")
    parser.add_argument("--check", action="store_true", help="Dry run without modifying any files")
    args = parser.parse_args()

    try:
        _, touched = apply_patch_files(
            args.directory,
            args.patches,
            reverse=args.reverse,
            check_only=args.check,
            p_strip=args.strip,
        )
    except (OSError, ValueError) as exc:
        sys.stderr.write("ERROR: %s\n" % exc)
        return 1

    action = "Checked" if args.check else ("Reversed" if args.reverse else "Applied")
    sys.stdout.write("%s %d patch file(s) (%d file(s) touched).\n" % (action, len(args.patches), len(touched)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
