#!/usr/bin/env python3
"""Rebuild final authored proxies, never the discarded convex-decomposition trials.
Run with the isolated numpy/scipy authoring Python documented in README.md.
"""
from pathlib import Path
import subprocess
import sys
root=Path(__file__).resolve().parent
for script in ['repair_spire.py','build_tree_trunks.py']:
    subprocess.run([sys.executable,str(root/script)],check=True)
