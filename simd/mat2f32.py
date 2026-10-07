#!/usr/bin/env python3
"""mat2f32.py FILE.mat [TÊN-BIẾN] — đọc MATLAB v5 (không cần scipy), liệt kê biến; nếu có TÊN-BIẾN thì ghi ra
<TÊN-BIẾN>.f32 (float32, little-endian). Hỗ trợ miMATRIX double/single, nén zlib (miCOMPRESSED)."""
import struct, sys, zlib

TYPES = {1: ("b", 1), 2: ("B", 1), 3: ("h", 2), 4: ("H", 2), 5: ("i", 4), 6: ("I", 4), 7: ("f", 4), 9: ("d", 8),
         12: ("q", 8), 13: ("Q", 8)}

def elements(buf, off=0, end=None):
    end = len(buf) if end is None else end
    while off + 8 <= end:
        t, n = struct.unpack_from("<II", buf, off)
        if t >> 16:                               # phần tử nhỏ: kiểu ở 16 bit thấp, cỡ ở 16 bit cao, dữ liệu 4 byte
            yield t & 0xFFFF, buf[off + 4: off + 4 + (t >> 16)]
            off += 8
        else:
            yield t, buf[off + 8: off + 8 + n]
            off += 8 + ((n + 7) & ~7)

def matrix(body):
    parts = list(elements(body))
    dims = struct.unpack("<%di" % (len(parts[1][1]) // 4), parts[1][1])
    name = parts[2][1].decode("latin1")
    t, data = parts[3]
    fmt, sz = TYPES[t]
    vals = struct.unpack("<%d%s" % (len(data) // sz, fmt), data)
    return name, dims, vals

def variables(path):
    buf = open(path, "rb").read()
    for t, data in elements(buf, 128):
        if t == 15:
            data = zlib.decompress(data)
            for t2, d2 in elements(data):
                if t2 == 14:
                    yield matrix(d2)
        elif t == 14:
            yield matrix(data)

if __name__ == "__main__":
    want = sys.argv[2] if len(sys.argv) > 2 else None
    for name, dims, vals in variables(sys.argv[1]):
        print(f"  {name:16s} {dims}")
        if name == want:
            with open(name + ".f32", "wb") as f:
                f.write(struct.pack("<%df" % len(vals), *vals))
            print(f"  → {name}.f32 ({len(vals)} float32)")
