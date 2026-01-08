#!/usr/bin/env python3
"""
Strip print-like function calls from preprocessed C to avoid Frama-C WP
invalid-range errors from variadic format strings.
"""

import sys

TARGETS = {
    "print",
    "iprint",
    "pprint",
    "panic",
    "error",
    "exhausted",
    "snprint",
    "vsnprint",
    "sprint",
    "fmtprint",
    "genrandom",
    "memset",
    "memmove",
}
DECL_KEYWORDS = {
    "extern",
    "static",
    "const",
    "volatile",
    "signed",
    "unsigned",
    "void",
    "int",
    "long",
    "short",
    "char",
    "uchar",
    "u8int",
    "u16int",
    "u32int",
    "u64int",
    "s8int",
    "s16int",
    "s32int",
    "s64int",
    "size_t",
    "usize",
    "ssize",
    "uintptr",
    "intptr",
    "struct",
    "enum",
    "union",
    "typedef",
}


def is_ident_char(ch: str) -> bool:
    return ch.isalnum() or ch == "_"

def prev_ident(data: str, idx: int) -> str:
    j = idx - 1
    while j >= 0 and data[j].isspace():
        j -= 1
    if j < 0:
        return ""
    if not is_ident_char(data[j]):
        return ""
    end = j + 1
    while j >= 0 and is_ident_char(data[j]):
        j -= 1
    return data[j + 1:end]


def main() -> int:
    data = sys.stdin.read()
    out = []
    i = 0
    depth = 0
    state = "code"

    while i < len(data):
        c = data[i]
        if state == "code":
            if c == "/" and i + 1 < len(data) and data[i + 1] == "/":
                out.append(c)
                out.append(data[i + 1])
                i += 2
                state = "line_comment"
                continue
            if c == "/" and i + 1 < len(data) and data[i + 1] == "*":
                out.append(c)
                out.append(data[i + 1])
                i += 2
                state = "block_comment"
                continue
            if c == '"':
                out.append(c)
                i += 1
                state = "string"
                continue
            if c == "'":
                out.append(c)
                i += 1
                state = "char"
                continue
            if c == "{":
                depth += 1
                out.append(c)
                i += 1
                continue
            if c == "}":
                depth = max(depth - 1, 0)
                out.append(c)
                i += 1
                continue

            if depth > 0 and (c.isalpha() or c == "_"):
                j = i
                while j < len(data) and is_ident_char(data[j]):
                    j += 1
                ident = data[i:j]

                if ident in TARGETS:
                    if prev_ident(data, i) in DECL_KEYWORDS:
                        out.append(ident)
                        i = j
                        continue
                    k = j
                    while k < len(data) and data[k].isspace():
                        k += 1
                    if k < len(data) and data[k] == "(":
                        level = 0
                        m = k
                        inner_state = "code"
                        while m < len(data):
                            ch = data[m]
                            if inner_state == "code":
                                if ch == "(":
                                    level += 1
                                elif ch == ")":
                                    level -= 1
                                    if level == 0:
                                        m += 1
                                        break
                                elif ch == '"':
                                    inner_state = "string"
                                elif ch == "'":
                                    inner_state = "char"
                                elif ch == "/" and m + 1 < len(data):
                                    nxt = data[m + 1]
                                    if nxt == "/":
                                        inner_state = "line_comment"
                                        m += 1
                                    elif nxt == "*":
                                        inner_state = "block_comment"
                                        m += 1
                            elif inner_state == "string":
                                if ch == "\\":
                                    m += 1
                                elif ch == '"':
                                    inner_state = "code"
                            elif inner_state == "char":
                                if ch == "\\":
                                    m += 1
                                elif ch == "'":
                                    inner_state = "code"
                            elif inner_state == "line_comment":
                                if ch == "\n":
                                    inner_state = "code"
                            elif inner_state == "block_comment":
                                if ch == "*" and m + 1 < len(data) and data[m + 1] == "/":
                                    inner_state = "code"
                                    m += 1
                            m += 1

                        out.append("0")
                        i = m
                        continue

                out.append(ident)
                i = j
                continue

            out.append(c)
            i += 1
            continue

        if state == "string":
            out.append(c)
            if c == "\\" and i + 1 < len(data):
                out.append(data[i + 1])
                i += 2
                continue
            if c == '"':
                state = "code"
            i += 1
            continue

        if state == "char":
            out.append(c)
            if c == "\\" and i + 1 < len(data):
                out.append(data[i + 1])
                i += 2
                continue
            if c == "'":
                state = "code"
            i += 1
            continue

        if state == "line_comment":
            out.append(c)
            i += 1
            if c == "\n":
                state = "code"
            continue

        if state == "block_comment":
            out.append(c)
            i += 1
            if c == "*" and i < len(data) and data[i] == "/":
                out.append(data[i])
                i += 1
                state = "code"
            continue

    sys.stdout.write("".join(out))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
