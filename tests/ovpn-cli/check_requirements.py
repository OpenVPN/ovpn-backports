#!/usr/bin/env python3

import sys
from pathlib import Path

import pkg_resources

REQ_FILE = Path("requirements.txt")


requirements = pkg_resources.parse_requirements(REQ_FILE.open())
for requirement in requirements:
    requirement = str(requirement)
    try:
        pkg_resources.require(requirement)
    except pkg_resources.DistributionNotFound:
        sys.exit(1)
