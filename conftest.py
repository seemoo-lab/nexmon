"""pytest collection policy for this repository.

The scripts that drive the radio are hardware checks, not unit tests. Several of
them transmit or retune the interface, so letting pytest import them during
collection would put frames on air and change the channel of a live capture just
because someone ran the test suite. They are named hwcheck_*.py / benchmark_*.py
and are excluded here as well, so that a future file cannot reintroduce the
hazard by being named test_*.py.

What IS collected: the genuine unit tests under tests/. They compile the C under
test into a temporary shared object and exercise it through ctypes with a mocked
transport - no kernel module, no radio, no network. Those are the only tests in
the tree and they should run.

Run the suite with:  python3 -m pytest tests/ -v
or, without pytest:  python3 -m unittest discover -s tests -v
"""

collect_ignore_glob = [
    "hwcheck_*.py",
    "benchmark_*.py",
    "nexbench.py",
]
