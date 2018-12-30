#!/bin/python2 
import argparse
import os

from pwn import *
from internalblue import core

context.arch = 'thumb'

parser = argparse.ArgumentParser(description='Patch aligner')
parser.add_argument('target', type=str, help='Where the patch should be applied.')
parser.add_argument('patchFile', type=str, help='Patch which needs to be aligned')
parser.add_argument('patchOutFile', type=str, help='Name of the patch files to be written. They \
                    will be prefixed with _one / _two')


def write_patch(patch, filename):
    with open(filename, 'wb') as file:
        file.write(patch)
    print("Wrote patch %#x to file %s" % (u32(patch), filename))


def read_patch(filename):
    patch = ''
    with open(filename, 'rb') as file:
        file.seek(0, os.SEEK_END)
        fsize = file.tell()
        file.seek(0, 0)
        patch = file.read(fsize)
    if len(patch) != 4:
        print("Error: Patch must be 4 bytes long but is %d bytes long! %s" %
              (len(patch), bytearray(patch)))
        exit(-1)
    return patch


if __name__ == '__main__':
    args = parser.parse_args()
    target = int(args.target, 16)
    patch_file = args.patchFile
    out_file = args.patchOutFile
    internalblue = core.InternalBlue()
    internalblue.connect()
    alignment = target % 4
    if alignment == 0:
        print("patchRom: Error! patch is already aligned!")
        sys.exit(-1)
    print("patchRom: Address 0x%x is not 4-byte aligned!" % target)
    patch = read_patch(patch_file)
    print("patchRom: applying patch 0x%x in two rounds" % u32(patch))
    # read original content
    orig = internalblue.readMem(target - alignment, 8)
    # patch the difference of the 4 bytes we want to patch within the original 8 bytes
    patch_one = orig[:alignment] + patch[:4-alignment]
    patch_two = patch[4-alignment:] + orig[alignment+4:]
    write_patch(patch_one, out_file + '_one')
    write_patch(patch_two, out_file + '_two')


