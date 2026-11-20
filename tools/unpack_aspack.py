#!/usr/bin/env python3
"""Pure-Python 3.4+ (stdlib only) ASPack 2.12 PE32 unpacker and PE rebuilder.

Implements the ASPack 2.12 section decompressor (canonical Huffman + LZ77 with
3-slot LRU distance cache + E8/E9 branch filter reversal) so original KAWAI
Lianliankan 2 binaries (D4s.exe v1.02 / lianliankan2.ExE v1.00-cn) can be
unpacked on any OS with zero third-party packages.

Usage:
  python unpack_aspack.py <packed.exe> <out_dump.bin> [--pe <out_unpacked.exe>]
"""
from __future__ import print_function
import os
import struct
import sys

_LEN_BASE = (
    0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28,
    32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224,
)

_LEN_EXTRA = (
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5,
)

_DIST_EXTRA = (
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14,
    15, 15, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 18, 18, 18, 18, 18, 18, 18, 18,
)

_STUB_SIGNATURE = b'\x60\xe8\x03\x00\x00\x00\xe9\xeb\x04\x5d\x45\x55\xc3'


class _BitReader(object):
    """MSB-first 32-bit buffered bitstream reader matching ASPack 2.12."""

    def __init__(self, buf, offset):
        self.buf = buf
        self.pos = offset
        self.bits_avail = 32
        self.bitbuf = 0
        self.blen = len(buf)

    def _fill(self):
        while self.bits_avail >= 8:
            b = self.buf[self.pos] if self.pos < self.blen else 0
            self.pos += 1
            self.bitbuf = ((self.bitbuf << 8) & 0xFFFFFFFF) | b
            self.bits_avail -= 8

    def read_bits(self, n):
        if n == 0:
            return 0
        self._fill()
        shift = 8 - self.bits_avail
        if shift >= 0:
            val = (self.bitbuf >> shift) & 0xFFFFFFFF
        else:
            val = (self.bitbuf << (-shift)) & 0xFFFFFFFF
        val = (val & 0xFFFFFF) >> (24 - n)
        self.bits_avail += n
        return val

    def peek_24_msb_aligned(self):
        self._fill()
        shift = 8 - self.bits_avail
        if shift >= 0:
            val = (self.bitbuf >> shift) & 0xFFFFFFFF
        else:
            val = (self.bitbuf << (-shift)) & 0xFFFFFFFF
        return val & 0xFFFE00


class _HuffTable(object):
    """Canonical Huffman table matching ASPack 2.12 (0xAFFD8 / 0xB0154)."""

    def __init__(self, num_syms):
        self.num_syms = num_syms
        self.min_code = [0] * 17
        self.delta = [0] * 16
        self.syms = [0] * num_syms
        self.fast_lut = [0] * 256

    def build(self, lengths):
        counts = [0] * 16
        for i in range(self.num_syms):
            counts[lengths[i]] += 1

        self.min_code = [0] * 17
        self.delta = [0] * 16
        self.fast_lut = [0] * 256

        code = 0
        prev_fast = 0
        shift = 23
        running = 0
        cursors = [0] * 16
        for bitlen in range(1, 16):
            self.min_code[bitlen] = code
            self.delta[bitlen] = running
            cursors[bitlen] = running
            running += counts[bitlen]
            code += counts[bitlen] << shift
            if code > 0x1000000:
                raise ValueError('Invalid canonical Huffman code space')
            if shift >= 16:
                top = code >> 16
                if top > prev_fast:
                    self.fast_lut[prev_fast:top] = [bitlen] * (top - prev_fast)
                prev_fast = top
            shift -= 1
        self.min_code[16] = code
        if code != 0x1000000:
            raise ValueError('Incomplete Huffman tree: 0x%x' % code)

        for sym in range(self.num_syms):
            bl = lengths[sym]
            if bl != 0:
                pos = cursors[bl]
                self.syms[pos] = sym
                cursors[bl] = pos + 1

    def decode(self, br):
        val = br.peek_24_msb_aligned()
        mc = self.min_code
        if val < mc[9]:
            bitlen = self.fast_lut[val >> 16]
        elif val < mc[11]:
            bitlen = 9 if val < mc[10] else 10
        elif val < mc[12]:
            bitlen = 11
        elif val < mc[13]:
            bitlen = 12
        elif val < mc[14]:
            bitlen = 13
        elif val < mc[15]:
            bitlen = 14
        else:
            bitlen = 15
        br.bits_avail += bitlen
        idx = self.delta[bitlen] + ((val - mc[bitlen]) >> (24 - bitlen))
        return self.syms[idx]


def _build_dist_base():
    base = []
    acc = 0
    for eb in _DIST_EXTRA:
        base.append(acc)
        acc += 1 << eb
    return tuple(base)


_DIST_BASE = _build_dist_base()


def _decompress_section(src_buf, src_off, dst_len):
    """Decompress one ASPack 2.12 packed block of known output size."""
    br = _BitReader(src_buf, src_off)
    main_tree = _HuffTable(721)
    len_tree = _HuffTable(28)
    dist_tree = _HuffTable(8)
    codelen_tree = _HuffTable(19)

    prev_lengths = [0] * 757
    use_dist_tree = [False]
    lru = [0, 0, 0]

    def read_trees():
        flag = br.read_bits(1)
        if flag == 0:
            for i in range(757):
                prev_lengths[i] = 0

        cl_lengths = [br.read_bits(4) for _ in range(19)]
        codelen_tree.build(cl_lengths)

        lengths = [0] * 757
        i = 0
        while i < 757:
            sym = codelen_tree.decode(br)
            if sym < 16:
                lengths[i] = (prev_lengths[i] + sym) & 0xF
                i += 1
            elif sym == 16:
                rep = br.read_bits(2) + 3
                val = lengths[i - 1]
                while rep > 0 and i < 757:
                    lengths[i] = val
                    i += 1
                    rep -= 1
            else:
                if sym == 17:
                    rep = br.read_bits(3) + 3
                else:
                    rep = br.read_bits(7) + 11
                while rep > 0 and i < 757:
                    lengths[i] = 0
                    i += 1
                    rep -= 1

        main_tree.build(lengths[:721])
        len_tree.build(lengths[721:749])
        dist_tree.build(lengths[749:757])
        use_dist_tree[0] = any(x != 3 for x in lengths[749:757])
        prev_lengths[:] = lengths

    read_trees()
    out = bytearray(dst_len)
    out_pos = 0
    while out_pos < dst_len:
        sym = main_tree.decode(br)
        if sym < 256:
            out[out_pos] = sym
            out_pos += 1
        elif sym < 720:
            v = sym - 256
            dist_slot = v >> 3
            len_low = v & 7
            match_len = len_low + 2
            if len_low == 7:
                lsym = len_tree.decode(br)
                match_len += _LEN_BASE[lsym] + br.read_bits(_LEN_EXTRA[lsym])

            dbase = _DIST_BASE[dist_slot]
            dextra = _DIST_EXTRA[dist_slot]
            if use_dist_tree[0] and dextra >= 3:
                high = br.read_bits(dextra - 3)
                low = dist_tree.decode(br)
                dist_code = dbase + (high << 3) + low
            else:
                dist_code = dbase + br.read_bits(dextra)

            if dist_code < 3:
                actual_dist = lru[dist_code]
                if dist_code != 0:
                    lru[dist_code] = lru[0]
                    lru[0] = actual_dist
            else:
                actual_dist = dist_code - 3
                lru[2] = lru[1]
                lru[1] = lru[0]
                lru[0] = actual_dist

            src_idx = out_pos - (actual_dist + 1)
            for _ in range(match_len):
                out[out_pos] = out[src_idx]
                out_pos += 1
                src_idx += 1
        else:
            read_trees()

    return out


def _unfilter_e8_e9(buf):
    """Reverse ASPack's E8/E9 relative CALL/JMP conversion on .text."""
    limit = len(buf) - 6
    i = 0
    while i < limit:
        b = buf[i]
        if b == 0xE8 or b == 0xE9:
            val = struct.unpack_from('<I', buf, i + 1)[0]
            struct.pack_into('<I', buf, i + 1, (val - i) & 0xFFFFFFFF)
            i += 5
        else:
            i += 1


def parse_pe_headers(raw):
    """Return basic PE32 header info from raw bytes."""
    if raw[:2] != b'MZ':
        raise ValueError('Not an MZ/PE file')
    e_lfanew = struct.unpack_from('<I', raw, 0x3C)[0]
    if raw[e_lfanew:e_lfanew + 4] != b'PE\x00\x00':
        raise ValueError('Invalid PE signature')
    nsec = struct.unpack_from('<H', raw, e_lfanew + 6)[0]
    opt_size = struct.unpack_from('<H', raw, e_lfanew + 20)[0]
    oh = e_lfanew + 24
    ep_rva = struct.unpack_from('<I', raw, oh + 16)[0]
    image_base = struct.unpack_from('<I', raw, oh + 28)[0]
    size_of_image = struct.unpack_from('<I', raw, oh + 56)[0]
    size_of_headers = struct.unpack_from('<I', raw, oh + 60)[0]
    sec_off = oh + opt_size
    sections = []
    for i in range(nsec):
        off = sec_off + i * 40
        name = raw[off:off + 8].rstrip(b'\x00').decode('latin-1', 'replace')
        vsz, va, rsz, rptr = struct.unpack_from('<IIII', raw, off + 8)
        chars = struct.unpack_from('<I', raw, off + 36)[0]
        sections.append({
            'name': name,
            'vsize': vsz,
            'va': va,
            'raw_size': rsz,
            'raw_ptr': rptr,
            'chars': chars,
        })
    return {
        'e_lfanew': e_lfanew,
        'oh': oh,
        'opt_size': opt_size,
        'ep_rva': ep_rva,
        'image_base': image_base,
        'size_of_image': size_of_image,
        'size_of_headers': size_of_headers,
        'sections': sections,
    }


def _find_section_by_rva(sections, rva):
    for s in sections:
        span = max(s['vsize'], s['raw_size'])
        if s['va'] <= rva < s['va'] + span:
            return s
    return None


def unpack_pe_bytes(raw):
    """Unpack an ASPack 2.12 PE32 binary in memory.

    Returns (image_bytearray, image_base, oep_rva, info_dict).
    """
    info = parse_pe_headers(raw)
    sections = info['sections']
    ep_rva = info['ep_rva']
    ep_sec = _find_section_by_rva(sections, ep_rva)
    if ep_sec is None:
        raise ValueError('EP RVA 0x%x not in any section' % ep_rva)

    ep_off = ep_sec['raw_ptr'] + (ep_rva - ep_sec['va'])
    if raw[ep_off:ep_off + len(_STUB_SIGNATURE)] != _STUB_SIGNATURE:
        raise ValueError('Unsupported packer stub at EP RVA 0x%x (expected ASPack 2.12)' % ep_rva)

    # Locate stub base in file (one byte before pusha if EP is stub+1, else find stub+0)
    stub_rva = ep_sec['va']
    stub_file = ep_sec['raw_ptr']
    # Extract original EP RVA from stub (`mov eax, imm32; push eax; add eax, ...` at +0x39A)
    oep_pos = stub_file + 0x39A
    if raw[oep_pos] != 0xB8:
        # Fallback search near end of stub loop
        idx = raw.find(b'\x61\x75\x08', stub_file, stub_file + 0x500)
        if idx < 0:
            raise ValueError('Could not locate OEP instruction in ASPack stub')
        # walk back for `b8 <imm32> 50`
        oep_rva = None
        for scan in range(idx - 16, idx):
            if raw[scan] == 0xB8 and raw[scan + 5] == 0x50:
                oep_rva = struct.unpack_from('<I', raw, scan + 1)[0]
                break
        if oep_rva is None:
            raise ValueError('Could not read OEP RVA from ASPack stub')
    else:
        oep_rva = struct.unpack_from('<I', raw, oep_pos + 1)[0]

    # Map headers and raw sections into virtual image buffer
    img_end = max(s['va'] + max(s['vsize'], s['raw_size']) for s in sections)
    img_size = max(info['size_of_image'], (img_end + 0xFFF) & ~0xFFF)
    image = bytearray(img_size)
    hdr_copy = min(info['size_of_headers'], len(raw), len(image))
    image[:hdr_copy] = raw[:hdr_copy]
    for s in sections:
        if s['raw_size'] and s['raw_ptr']:
            chunk = raw[s['raw_ptr']:s['raw_ptr'] + s['raw_size']]
            image[s['va']:s['va'] + len(chunk)] = chunk

    # Parse ASPack section descriptor table at stub + 0x57C: (rva, unpacked_size)
    tbl_off = stub_file + 0x57C
    idx = 0
    unpacked_chunks = []
    while True:
        sec_rva, dst_len = struct.unpack_from('<II', raw, tbl_off + idx * 8)
        if sec_rva == 0:
            break
        if sec_rva < stub_rva:
            dec = _decompress_section(image, sec_rva, dst_len)
            if idx == 0:
                _unfilter_e8_e9(dec)
            image[sec_rva:sec_rva + dst_len] = dec
            unpacked_chunks.append((sec_rva, dst_len))
        idx += 1

    info['oep_rva'] = oep_rva
    info['unpacked_chunks'] = unpacked_chunks
    return image, info['image_base'], oep_rva, info


def rebuild_pe_bytes(raw):
    """Unpack and emit a clean 4-section PE32 executable for static analysis."""
    image, _, oep_rva, info = unpack_pe_bytes(raw)
    sections = info['sections']

    # Keep pre-packer original sections (.text, .data, .idata, .rsrc)
    kept = []
    for s in sections:
        if s['name'].startswith('.aspack') or s['name'].startswith('.adata') or s['name'].startswith('.reloc'):
            continue
        vsz = max(s['vsize'], 0x1000)
        # Preserve original unaligned/page-rounded bounds when known
        kept.append((s['name'], s['va'], vsz))

    if not kept:
        raise ValueError('No sections retained in rebuilt PE')

    # Determine virtual size of last kept section cleanly from original Resource DataDir if .rsrc
    oh = info['oh']
    ddir = oh + 96
    res_rva, res_sz = struct.unpack_from('<II', image, ddir + 2 * 8)
    imp_rva, imp_sz = struct.unpack_from('<II', image, ddir + 1 * 8)

    last_name, last_va, last_vsz = kept[-1]
    if last_name == '.rsrc' and res_rva == last_va:
        # Round up end of actual .rsrc to 0x1000 page boundary
        rsrc_end = max(last_va + last_vsz, res_rva + ((res_sz + 0xFFF) & ~0xFFF))
        new_img_size = (rsrc_end + 0xFFF) & ~0xFFF
        kept[-1] = (last_name, last_va, new_img_size - last_va)
    else:
        new_img_size = ((last_va + last_vsz + 0xFFF) & ~0xFFF)

    # Restore clean single-DLL IAT/Import table if original descriptor in .idata was kept
    e_lfanew = info['e_lfanew']
    sec_off = oh + info['opt_size']
    new_secs = bytearray()
    for name, va, vs in kept:
        raw_sz = (vs + 0xFFF) & ~0xFFF
        chars = 0xC0000040
        if name == '.text':
            chars = 0x60000020
        elif name == '.rsrc':
            chars = 0x40000040
        new_secs += (
            name.encode('latin-1')[:8].ljust(8, b'\x00')
            + struct.pack('<IIIIIIHHI', vs, va, raw_sz, va, 0, 0, 0, 0, chars)
        )

    struct.pack_into('<H', image, e_lfanew + 6, len(kept))
    struct.pack_into('<I', image, oh + 16, oep_rva)
    struct.pack_into('<I', image, oh + 56, new_img_size)
    image[ddir:ddir + 128] = bytearray(128)

    # Check if an intact MSVBVM50.DLL import descriptor exists at .idata (0xB9000)
    idata_s = next((s for s in kept if s[0] == '.idata'), None)
    if idata_s is not None:
        id_rva = idata_s[1]
        ilt_rva, _, _, name_rva, iat_rva = struct.unpack_from('<IIIII', image, id_rva)
        if ilt_rva and iat_rva:
            n_imp = 0
            while struct.unpack_from('<I', image, ilt_rva + n_imp * 4)[0]:
                n_imp += 1
            for i in range(n_imp):
                v = struct.unpack_from('<I', image, ilt_rva + i * 4)[0]
                struct.pack_into('<I', image, iat_rva + i * 4, v)
            struct.pack_into('<II', image, ddir + 1 * 8, id_rva, 40)
            struct.pack_into('<II', image, ddir + 12 * 8, iat_rva, n_imp * 4)
        elif imp_rva and imp_sz:
            struct.pack_into('<II', image, ddir + 1 * 8, imp_rva, imp_sz)

    if res_rva:
        # Calculate actual end of data entries inside .rsrc
        rsrc_vsize = kept[-1][2] if kept[-1][0] == '.rsrc' else res_sz
        struct.pack_into('<II', image, ddir + 2 * 8, res_rva, rsrc_vsize)

    old_slots = len(sections)
    wipe_len = max(old_slots, len(kept)) * 40
    image[sec_off:sec_off + wipe_len] = bytearray(wipe_len)
    image[sec_off:sec_off + len(new_secs)] = new_secs
    if len(image) > 0x1000:
        image[max(sec_off + len(new_secs), 0x400):0x1000] = bytearray(
            0x1000 - max(sec_off + len(new_secs), 0x400)
        )

    return bytes(image[:new_img_size])


def main(argv=None):
    if argv is None:
        argv = sys.argv[1:]
    if len(argv) < 2 or argv[0] in ('-h', '--help'):
        print('Usage: python unpack_aspack.py <packed.exe> <out_dump.bin> [--pe <out_unpacked.exe>]')
        return 1 if len(argv) < 2 else 0

    in_exe = argv[0]
    out_dump = argv[1]
    out_pe = None
    i = 2
    while i < len(argv):
        if argv[i] == '--pe' and i + 1 < len(argv):
            out_pe = argv[i + 1]
            i += 2
        else:
            print('Unknown argument: %s' % argv[i], file=sys.stderr)
            return 1

    with open(in_exe, 'rb') as f:
        raw = f.read()

    image, base, oep_rva, info = unpack_pe_bytes(raw)
    out_dir = os.path.dirname(os.path.abspath(out_dump))
    if out_dir and not os.path.isdir(out_dir):
        os.makedirs(out_dir)
    with open(out_dump, 'wb') as f:
        f.write(image)
    print('[unpack] %s -> %s (%d bytes, OEP=0x%08X, sections=%s)' % (
        in_exe, out_dump, len(image), base + oep_rva,
        [(hex(r), hex(sz)) for r, sz in info['unpacked_chunks']]
    ))

    if out_pe:
        pe_bytes = rebuild_pe_bytes(raw)
        pe_dir = os.path.dirname(os.path.abspath(out_pe))
        if pe_dir and not os.path.isdir(pe_dir):
            os.makedirs(pe_dir)
        with open(out_pe, 'wb') as f:
            f.write(pe_bytes)
        print('[unpack] rebuilt PE -> %s (%d bytes)' % (out_pe, len(pe_bytes)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
