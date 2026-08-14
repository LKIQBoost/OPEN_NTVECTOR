#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
convert_encoding.py - 批量将 GBK(GB2312) 源码文件转换为 UTF-8(带 BOM)。

为什么带 BOM：
  项目用 MSVC 编译且 CMake 未开启 /utf-8，MSVC 默认按系统 ANSI 代码页(中文系统=GBK)
  读取源码。UTF-8 文件加 BOM 后 MSVC/GCC/Clang 都会自动按 UTF-8 解析，
  这样源码里的中文字符串字面量在运行时才不会变成乱码。

用法：
  python tools/convert_encoding.py                 # 转换默认范围
  python tools/convert_encoding.py --dry-run       # 只报告，不写文件
  python tools/convert_encoding.py --no-bom        # 输出无 BOM 的 UTF-8(需配合 /utf-8)
  python tools/convert_encoding.py --root xxx      # 追加自定义扫描根目录(可多次)

检测逻辑（整文件读取，避免截断多字节字符）：
  1. 有 UTF-8/UTF-16 BOM          -> 已处理，跳过
  2. 严格 UTF-8 可解码            -> 已是 UTF-8(或纯 ASCII)，跳过
  3. 严格 GB18030 可解码          -> GBK 系，转换
  4. 其它                          -> 无法识别，跳过并报告
"""
import argparse
import os
import sys

TEXT_EXT = (".h", ".hpp", ".c", ".cc", ".cpp", ".md", ".txt", ".rc", ".json", ".cmake", ".py")

# 默认扫描根目录（第三方的 include/ 与 third_party/ 不转换）
DEFAULT_ROOTS = ["application", "docs"]
DEFAULT_FILES = ["CMakeLists.txt", "CMakeSettings.json", ".gitignore"]


def detect_encoding(path):
    """返回 ('utf8-bom'|'utf16'|'utf8'|'gbk'|'other', raw_bytes)"""
    with open(path, "rb") as f:
        raw = f.read()
    if raw.startswith(b"\xef\xbb\xbf"):
        return "utf8-bom", raw
    if raw.startswith(b"\xff\xfe") or raw.startswith(b"\xfe\xff"):
        return "utf16", raw
    try:
        raw.decode("utf-8")
        return "utf8", raw
    except UnicodeDecodeError:
        pass
    try:
        raw.decode("gb18030")
        return "gbk", raw
    except UnicodeDecodeError:
        return "other", raw


def collect_files(roots, extra_files):
    files = []
    for root in roots:
        if not os.path.isdir(root):
            print(f"  [skip] 目录不存在: {root}")
            continue
        for dp, dn, fn in os.walk(root):
            parts = dp.replace("\\", "/").split("/")
            if "include" in parts or "third_party" in parts or "build" in parts or ".git" in parts:
                continue
            for name in fn:
                if name.lower().endswith(TEXT_EXT):
                    files.append(os.path.join(dp, name))
    for f in extra_files:
        if os.path.isfile(f):
            files.append(f)
    return sorted(set(files))


def main():
    ap = argparse.ArgumentParser(description="批量转换 GBK 源码到 UTF-8")
    ap.add_argument("--dry-run", action="store_true", help="只报告不写文件")
    ap.add_argument("--no-bom", action="store_true", help="输出无 BOM 的 UTF-8（需编译加 /utf-8）")
    ap.add_argument("--root", action="append", default=[], help="追加扫描目录")
    args = ap.parse_args()

    roots = list(DEFAULT_ROOTS) + args.root
    files = collect_files(roots, DEFAULT_FILES)

    converted, skipped_utf8, skipped_other = [], [], []
    for path in files:
        enc, raw = detect_encoding(path)
        if enc == "gbk":
            text = raw.decode("gb18030")
            if args.dry_run:
                converted.append(path)
                continue
            if args.no_bom:
                out = text.encode("utf-8")
            else:
                out = b"\xef\xbb\xbf" + text.encode("utf-8")
            with open(path, "wb") as f:
                f.write(out)
            converted.append(path)
        elif enc in ("utf8", "utf8-bom", "utf16"):
            skipped_utf8.append(path)
        else:
            skipped_other.append(path)

    print(f"== 结果 ==")
    print(f"已转换 GBK -> UTF-8{'（dry-run，未写入）' if args.dry_run else ''}: {len(converted)}")
    for p in converted:
        print(f"  [convert] {p}")
    print(f"已跳过（UTF-8/UTF-16/ASCII）: {len(skipped_utf8)}")
    print(f"无法识别编码，跳过: {len(skipped_other)}")
    for p in skipped_other:
        print(f"  [unknown] {p}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
