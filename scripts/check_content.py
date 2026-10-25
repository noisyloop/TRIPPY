#!/usr/bin/env python3
"""Validates TRIPPY content without building the game.

Checks rooms (maps + .meta), dialogue and duel files, and how they connect:
doors vs transitions, slot counts, ids with no dialogue, bad labels, unknown
moods, locks that can never open, rooms you can't reach from the bedroom.

    python3 scripts/check_content.py          # errors + warnings, exit 1 on errors
    python3 scripts/check_content.py --graph  # also print the room graph

Formats: docs/content-format.md. Stdlib only.
"""
import os
import re
import sys
from collections import deque

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
ASSETS = os.path.join(ROOT, "assets")
START_ROOM = "bedroom"
TILES = set("#.~>NIVM^= ")
DIRS = ("north", "south", "east", "west")
OPPOSITE = {"north": "south", "south": "north", "east": "west", "west": "east"}
META_KEYS = {"music_mood", "transitions", "npcs", "items", "on_enter_flag", "trip",
             "kaleidoscope", "locks", "beam", "void_to", "wander"}
# Flags the engine sets itself (besides beat_<boss>, got_<item>, set:, on_enter_flag).
ENGINE_FLAGS = {"woke_up", "collected_everything", "opt_calm"}

errors, warnings = [], []


def err(where, msg):
    errors.append(f"{where}: {msg}")


def warn(where, msg):
    warnings.append(f"{where}: {msg}")


def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def parse_pairs(value, where, key):
    out = {}
    for tok in value.split():
        if ":" not in tok:
            err(where, f"{key}: '{tok}' should be a:b")
            continue
        a, b = tok.split(":", 1)
        out[a] = b
    return out


# --- rooms -------------------------------------------------------------------

def door_dir(x, y, w, h):
    if y == 0: return "north"
    if y == h - 1: return "south"
    if x == 0: return "west"
    if x == w - 1: return "east"
    return "north" if y < h / 2 else "south"


def load_rooms():
    rooms = {}
    maps = os.path.join(ASSETS, "maps")
    for f in sorted(os.listdir(maps)):
        if not f.endswith(".txt"):
            continue
        name = f[:-4]
        where = f"maps/{f}"
        rows = read(os.path.join(maps, f)).rstrip("\n").split("\n")
        while rows and not rows[0].strip():
            rows.pop(0)
        w = max(len(r) for r in rows) if rows else 0
        if any(len(r) != w for r in rows):
            warn(where, "rows have different widths (short rows are padded with wall)")
        for y, r in enumerate(rows):
            for x, c in enumerate(r):
                if c not in TILES:
                    err(where, f"unknown tile '{c}' at {x},{y}")
        doors = {}
        for y, r in enumerate(rows):
            for x, c in enumerate(r):
                if c == ">":
                    doors.setdefault(door_dir(x, y, w, len(rows)), (x, y))
        meta = {}
        mpath = os.path.join(maps, name + ".meta")
        if os.path.exists(mpath):
            for n, line in enumerate(read(mpath).split("\n"), 1):
                line = line.strip()
                if not line or line[0] in "#;":
                    continue
                if "=" not in line:
                    err(f"maps/{name}.meta:{n}", "expected key = value")
                    continue
                k, v = (s.strip() for s in line.split("=", 1))
                if k not in META_KEYS:
                    err(f"maps/{name}.meta:{n}", f"unknown key '{k}'")
                meta[k] = v
        else:
            warn(where, "no .meta file")
        rooms[name] = {"rows": rows, "doors": doors, "meta": meta,
                       "n_npc": sum(r.count("N") for r in rows),
                       "n_item": sum(r.count("I") for r in rows)}
    return rooms


# --- dialogue / duels ----------------------------------------------------------

def check_dialogue(path, sets, gives):
    where = os.path.relpath(path, ASSETS)
    labels, refs, nodes = {}, [], 0
    alt = None
    texted = False
    for n, raw in enumerate(read(path).split("\n"), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("[") and line.endswith("]"):
            label = line[1:-1].strip()
            if not label or label in labels:
                err(f"{where}:{n}", f"empty or duplicate label [{label}]")
            labels[label] = n
            nodes += 1
            texted = False
            continue
        if nodes == 0:
            m = re.match(r"alt:\s*(\S+)\s*->\s*(\S+)$", line)
            if m:
                alt = (m.group(2), n)
            else:
                err(f"{where}:{n}", "expected 'alt: flag -> label' or [label] before text")
            continue
        if line.startswith("->"):
            refs.append((line[2:].strip(), n))
        elif line.startswith("*"):
            if "->" not in line:
                err(f"{where}:{n}", "choice needs '-> label'")
            else:
                refs.append((line.rsplit("->", 1)[1].strip(), n))
        elif line.startswith("set:"):
            sets.add(line[4:].strip())
        elif line.startswith("give:"):
            gives.add(line[5:].strip())
        else:
            texted = True
            if len(line) > 200:
                warn(f"{where}:{n}", "very long line; the box fits about 3 lines (~180 chars)")
    if nodes == 0:
        err(where, "no [label] nodes")
    for label, n in refs + ([alt] if alt else []):
        if label != "end" and label not in labels:
            err(f"{where}:{n}", f"unknown label '{label}'")


def check_duel(path, moods, rooms, gives):
    where = os.path.relpath(path, ASSETS)
    header, rounds, cur = {}, [], None
    for n, raw in enumerate(read(path).split("\n"), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        m = re.match(r"\[(\w+)\]$", line)
        if m:
            if m.group(1) not in ("talk", "echo", "beat"):
                err(f"{where}:{n}", "round must be [talk], [echo] or [beat]")
            cur = {"kind": m.group(1), "replies": 0, "keys": set(), "line": n}
            rounds.append(cur)
            continue
        if line.startswith("*"):
            parts = [p.strip() for p in line[1:].split("|")]
            if cur is None or cur["kind"] != "talk":
                err(f"{where}:{n}", "replies belong in a [talk] round")
            elif len(parts) != 3 or not re.match(r"^[+-]?\d+$", parts[1]):
                err(f"{where}:{n}", "reply needs 'text | +N | response'")
            else:
                cur["replies"] += 1
            continue
        if ":" not in line:
            err(f"{where}:{n}", "expected key: value")
            continue
        k, v = (s.strip() for s in line.split(":", 1))
        if cur is None:
            header[k] = v
        else:
            cur["keys"].add(k)
            if k in ("length", "gain") and not v.isdigit():
                err(f"{where}:{n}", f"{k} must be a number")
    for k in ("name", "intro", "victory"):
        if not header.get(k):
            err(where, f"missing '{k}:'")
    if header.get("mood") and header["mood"] not in moods:
        err(where, f"unknown mood '{header['mood']}'")
    if header.get("after_room") and header["after_room"] not in rooms:
        err(where, f"after_room '{header['after_room']}' is not a room")
    if header.get("reward"):
        gives.add(header["reward"])
    if not rounds:
        err(where, "no rounds")
    for r in rounds:
        if r["kind"] == "talk" and r["replies"] == 0:
            err(f"{where}:{r['line']}", "[talk] round has no replies")
        if r["kind"] in ("echo", "beat"):
            for k in ("success", "failure"):
                if k not in r["keys"]:
                    warn(f"{where}:{r['line']}", f"[{r['kind']}] round has no '{k}:' line")
    return header


def main():
    show_graph = "--graph" in sys.argv
    synth = read(os.path.join(ROOT, "src", "ChiptuneSynth.cpp"))
    moods = set(re.findall(r'\{"(\w+)",\s*\{', synth))
    inventory = read(os.path.join(ROOT, "src", "Inventory.cpp"))
    named_items = set(re.findall(r'\{"(\w+)",\s*"', inventory))

    rooms = load_rooms()
    ddir, udir = os.path.join(ASSETS, "dialogue"), os.path.join(ASSETS, "duels")
    dialogue_ids = {f[:-4] for f in os.listdir(ddir) if f.endswith(".txt")} if os.path.isdir(ddir) else set()
    duel_ids = {f[:-4] for f in os.listdir(udir) if f.endswith(".txt")} if os.path.isdir(udir) else set()

    sets, gives = set(), set()
    for i in sorted(dialogue_ids):
        check_dialogue(os.path.join(ddir, i + ".txt"), sets, gives)
    duels = {i: check_duel(os.path.join(udir, i + ".txt"), moods, rooms, gives) for i in sorted(duel_ids)}

    used_npcs, floor_items, graph = set(), set(), {}
    flags = set(sets) | ENGINE_FLAGS | {f"beat_{b}" for b in duel_ids}
    for name, r in rooms.items():
        flags.add(r["meta"].get("on_enter_flag", ""))
        for item in parse_pairs(r["meta"].get("items", ""), f"maps/{name}.meta", "items").values():
            flags.add(f"got_{item}")
    flags |= {f"got_{g}" for g in gives}

    for name, r in sorted(rooms.items()):
        where, meta = f"maps/{name}", r["meta"]
        tr = {}
        for d, target in parse_pairs(meta.get("transitions", ""), where, "transitions").items():
            if d not in DIRS:
                err(where, f"transition direction '{d}'")
                continue
            tr[d] = target
            if target not in rooms:
                err(where, f"{d} leads to missing room '{target}'")
            if d not in r["doors"]:
                err(where, f"transition {d} has no door on that edge")
        for d in r["doors"]:
            if d not in tr:
                err(where, f"door on the {d} edge leads nowhere (add transitions = {d}:room)")
        npcs = parse_pairs(meta.get("npcs", ""), where, "npcs")
        items = parse_pairs(meta.get("items", ""), where, "items")
        if len(npcs) != r["n_npc"]:
            err(where, f"{r['n_npc']} N slots but {len(npcs)} npcs in .meta")
        if len(items) != r["n_item"]:
            err(where, f"{r['n_item']} I slots but {len(items)} items in .meta")
        for slot in list(npcs) + list(items):
            if not slot.isdigit():
                err(where, f"slot '{slot}' is not a number")
        for nid in npcs.values():
            used_npcs.add(nid)
            if nid.startswith("boss_"):
                if nid not in duel_ids:
                    warn(where, f"{nid} has no duels/{nid}.txt (it will just talk)")
                if nid not in dialogue_ids:
                    warn(where, f"{nid} has no after-duel dialogue/{nid}.txt")
            elif nid not in dialogue_ids:
                err(where, f"npc '{nid}' has no dialogue/{nid}.txt")
        for wid in meta.get("wander", "").split():
            if wid not in npcs.values():
                err(where, f"wander '{wid}' is not one of this room's npcs")
        for item in items.values():
            floor_items.add(item)
        mood = meta.get("music_mood")
        if mood and mood not in moods:
            err(where, f"unknown music_mood '{mood}' (known: {', '.join(sorted(moods))})")
        for key in ("trip", "kaleidoscope"):
            if key in meta:
                try:
                    if not 0 <= float(meta[key]) <= 1: raise ValueError
                except ValueError:
                    err(where, f"{key} must be 0..1")
        for d, flag in parse_pairs(meta.get("locks", ""), where, "locks").items():
            if d not in tr:
                err(where, f"lock on {d} but no transition that way")
            if flag not in flags:
                err(where, f"lock flag '{flag}' is never set anywhere, so the door never opens")
        for d in meta.get("beam", "").split():
            if d not in tr:
                err(where, f"beam {d} but no transition that way")
        vt = meta.get("void_to")
        if vt and vt not in rooms:
            err(where, f"void_to '{vt}' is not a room")
        if any("V" in row for row in r["rows"]) and not vt:
            warn(where, "has void tiles but no void_to (falls return you to the entrance)")
        warps = sum(row.count("^") for row in r["rows"])
        if warps == 1:
            warn(where, "a single ^ warp does nothing (needs 2+)")
        edges = set(tr.values()) | ({vt} if vt else set())
        for nid in npcs.values():
            if nid in duels and duels[nid].get("after_room"):
                edges.add(duels[nid]["after_room"])
        graph[name] = {"edges": sorted(e for e in edges if e in rooms), "tr": tr}

    if START_ROOM not in rooms:
        err("maps", f"start room '{START_ROOM}' is missing")
    else:
        seen, q = {START_ROOM}, deque([START_ROOM])
        while q:
            for nxt in graph[q.popleft()]["edges"]:
                if nxt not in seen:
                    seen.add(nxt)
                    q.append(nxt)
        for name in sorted(set(rooms) - seen):
            warn(f"maps/{name}", f"unreachable from {START_ROOM}")

    for i in sorted(dialogue_ids - used_npcs):
        warn(f"dialogue/{i}.txt", "not used by any room")
    for i in sorted(duel_ids - used_npcs):
        warn(f"duels/{i}.txt", "not used by any room")
    for item in sorted((floor_items | gives) - named_items):
        warn("items", f"'{item}' has no display name in Inventory::displayName")

    if show_graph:
        print("room graph:")
        for name in sorted(graph):
            arrows = ", ".join(f"{d}->{t}" for d, t in sorted(graph[name]["tr"].items()))
            print(f"  {name}: {arrows}")
    for w in warnings:
        print("warning:", w)
    for e in errors:
        print("ERROR:", e)
    print(f"{len(rooms)} rooms, {len(dialogue_ids)} dialogue files, {len(duel_ids)} duels: "
          f"{len(errors)} errors, {len(warnings)} warnings")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
