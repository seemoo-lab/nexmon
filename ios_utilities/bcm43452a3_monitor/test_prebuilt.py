# SPDX-License-Identifier: GPL-3.0-only
"""Synthetic integrity/range tests; no phone, firmware or compiler required."""
import copy
import unittest
from unittest.mock import patch
import apply_prebuilt as module


class PatchTests(unittest.TestCase):
    def setUp(self):
        self.stock = bytes(527400)
        self.payload = b'own-code'
        expected = bytearray(self.stock)
        expected[12:20] = self.payload
        self.expected = bytes(expected)
        self.meta = dict(schema=1, stock_sha256=module.sha(self.stock),
                         result_sha256=module.sha(self.expected), payload_bytes=8,
                         payload_sha256=module.sha(self.payload),
                         ranges=[dict(offset=12, length=8, payload_offset=0)])
        self.pins = patch.multiple(module, STOCK=self.meta['stock_sha256'], RESULT=self.meta['result_sha256'])
        self.pins.start()
        self.addCleanup(self.pins.stop)

    def test_roundtrip(self):
        self.assertEqual(module.apply(self.stock, self.payload, self.meta), self.expected)

    def test_wrong_stock(self):
        with self.assertRaises(ValueError):
            module.apply(self.stock[:-1], self.payload, self.meta)

    def test_corrupt_payload(self):
        with self.assertRaises(ValueError):
            module.apply(self.stock, b'bad-code', self.meta)

    def test_bad_ranges(self):
        for changes in ({'offset': -1}, {'offset': 527399}, {'length': 0},
                        {'length': True}, {'payload_offset': 1}):
            with self.subTest(changes=changes):
                meta = copy.deepcopy(self.meta)
                meta['ranges'][0].update(changes)
                with self.assertRaises(ValueError):
                    module.apply(self.stock, self.payload, meta)

    def test_overlap(self):
        meta = copy.deepcopy(self.meta)
        meta['ranges'] = [dict(offset=12, length=4, payload_offset=0),
                          dict(offset=14, length=4, payload_offset=4)]
        with self.assertRaises(ValueError):
            module.apply(self.stock, self.payload, meta)

    def test_final_hash_and_unused_payload(self):
        for rows in ([], [dict(offset=13, length=8, payload_offset=0)]):
            meta = copy.deepcopy(self.meta)
            meta['ranges'] = rows
            with self.assertRaises(ValueError):
                module.apply(self.stock, self.payload, meta)


if __name__ == '__main__':
    unittest.main()
