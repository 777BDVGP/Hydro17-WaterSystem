# sync_from_private.py - copy the firmware source from the private project
# (../Hydro17_WaterSystem) into this public copy (Hydro17/), WITHOUT the
# private WiFi file, then check that no private WiFi name, password or
# personal name slipped in.
#
#   python sync_from_private.py
#
# Copied: src/ (all except wifi_secrets.h), tools/export_bin.py,
#         partitions_ota.csv.  platformio.ini is NOT copied: the public one has
#         no personal upload addresses.
# In the public copy, DEBUG is forced to 0 in main.cpp (production build: no
# Serial output, generic default IP) - the private project keeps its own value
# for the test box.
# The check reads the private src/wifi_secrets.h (only on this PC) and refuses
# when any string from it, or a word listed in the private project's
# private_words.txt (personal names; never published), occurs in the copy.
import os, re, shutil, sys

HERE    = os.path.dirname(os.path.abspath(__file__))
PRIVATE = os.path.join(os.path.dirname(HERE), "Hydro17_WaterSystem")
PUBLIC  = os.path.join(HERE, "Hydro17")
SKIP    = {"wifi_secrets.h"}

def copy_tree(src, dst):
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    os.makedirs(dst)
    for name in sorted(os.listdir(src)):
        if name in SKIP:
            print("  skipped (private): %s" % name)
            continue
        s = os.path.join(src, name)
        if os.path.isfile(s):
            shutil.copyfile(s, os.path.join(dst, name))

if not os.path.isdir(PRIVATE):
    sys.exit("Private project not found: %s" % PRIVATE)

print("Copying %s -> %s" % (PRIVATE, PUBLIC))
copy_tree(os.path.join(PRIVATE, "src"), os.path.join(PUBLIC, "src"))
os.makedirs(os.path.join(PUBLIC, "tools"), exist_ok=True)
shutil.copyfile(os.path.join(PRIVATE, "tools", "export_bin.py"), os.path.join(PUBLIC, "tools", "export_bin.py"))
shutil.copyfile(os.path.join(PRIVATE, "partitions_ota.csv"), os.path.join(PUBLIC, "partitions_ota.csv"))

# -- Public build = production build: DEBUG 0 ---------------------------------
main_cpp = os.path.join(PUBLIC, "src", "main.cpp")
text = open(main_cpp, encoding="utf-8").read()
text, n = re.subn(r"(#define\s+DEBUG\s+)\d", r"\g<1>0", text, count=1)
if n != 1:
    sys.exit("STOP - could not find '#define DEBUG' in main.cpp")
open(main_cpp, "w", encoding="utf-8", newline="").write(text)
print("  DEBUG set to 0 in the public main.cpp")

# -- Secret check --------------------------------------------------------------
secrets = []
words_file = os.path.join(PRIVATE, "private_words.txt")
if os.path.exists(words_file):
    for line in open(words_file, encoding="utf-8"):
        line = line.strip()
        if line and not line.startswith("#"):
            secrets.append(line)
sec_file = os.path.join(PRIVATE, "src", "wifi_secrets.h")
if os.path.exists(sec_file):
    t = open(sec_file, encoding="utf-8", errors="replace").read()
    t = re.sub(r"//[^\n]*", "", t)                           # ignore comments
    for s in re.findall(r'"([^"]{4,})"', t):                 # SSIDs and passwords
        secrets.append(s)
bad = []
for root, _, files in os.walk(PUBLIC):
    if ".pio" in root or "firmware_out" in root:
        continue
    for f in files:
        p = os.path.join(root, f)
        data = open(p, encoding="utf-8", errors="replace").read()
        for s in secrets:
            if s.lower() in data.lower():
                bad.append("%s (%s...)" % (os.path.relpath(p, HERE), s[:2]))
if os.path.exists(os.path.join(PUBLIC, "src", "wifi_secrets.h")):
    bad.append("Hydro17/src/wifi_secrets.h")
if bad:
    print("STOP - private strings found in: %s" % ", ".join(sorted(set(bad))))
    sys.exit(1)
print("OK - %d private strings checked, none in the public copy" % len(secrets))
