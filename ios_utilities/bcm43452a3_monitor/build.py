# SPDX-License-Identifier: GPL-3.0-only
"""Exact-target source-only builder. No phone/network operations or binary inputs bundled."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
from types import SimpleNamespace
from capstone import Cs,CS_ARCH_ARM,CS_MODE_THUMB
from keystone import Ks,KS_ARCH_ARM,KS_MODE_THUMB

HERE=Path(__file__).resolve().parent/'source'
BASE=0x199c00
STOCK_SHA='280bc2d7ad25f2c789ce332c37da9d687693fbda156830803fe1516e8d938244'
OUTPUT_SHA='0245b815b9ff0a5d7e7a3b4f42b491338d2dbeefd8f5bf66cf8a86a83fb00f1f'
FLAGS=['-target','thumb-freestanding-eabi','-mcpu=cortex_r4','-mthumb','-mfloat-abi=soft',
       '-Oz','-ffreestanding','-fno-builtin','-fomit-frame-pointer','-fno-stack-protector',
       '-fno-unwind-tables','-fno-asynchronous-unwind-tables','-ffunction-sections','-fdata-sections',
       '-fno-jump-tables','-Wall','-Wextra','-Werror']
fw=SimpleNamespace(md=Cs(CS_ARCH_ARM,CS_MODE_THUMB))
fw.md.skipdata=True


def relocate(blob, expected_relocations=13, allow_zero_addend_mov16=False, allow_exact_chip_helpers=False, allow_exact_mirror=False):
    header=struct.unpack_from('<16sHHIIIIIHHHHHH',blob)
    assert header[0][:7]==b'\x7fELF\x01\x01\x01' and header[1:3]==(1,40)
    sections=[struct.unpack_from('<10I',blob,header[6]+i*header[11]) for i in range(header[12])]
    string_header=sections[header[13]]
    names=blob[string_header[4]:string_header[4]+string_header[5]]
    name=lambda s:names[s[0]:].split(b'\0')[0].decode()
    wanted=('.text.callback','.text.change','.text.gate')
    addresses={};parts={};cursor=BASE
    for target in wanted:
        candidates=[i for i,s in enumerate(sections) if name(s)==target]
        assert len(candidates)==1
        index=candidates[0];s=sections[index]
        cursor=(cursor+max(s[8],4)-1)&~(max(s[8],4)-1)
        addresses[index]=cursor
        parts[index]=bytearray(blob[s[4]:s[4]+s[5]])
        cursor+=s[5]
    for i,s in enumerate(sections):
        if s[1]==1 and s[2]&2 and s[5]:assert i in addresses,(name(s),s[5])
    symtabs=[s for s in sections if s[1]==2]
    assert len(symtabs)==1
    table=symtabs[0];sh=sections[table[6]];strings=blob[sh[4]:sh[4]+sh[5]]
    symbols=[];resolved={}
    for index in range(table[5]//table[9]):
        n,value,size,info,other,section=struct.unpack_from('<IIIBBH',blob,table[4]+index*table[9])
        label=strings[n:].split(b'\0')[0].decode()
        symbol=dict(name=label,value=value,section=section,info=info)
        if section in addresses:
            symbol['address']=addresses[section]+value
            if label and not label.startswith('$'):resolved[label]=symbol['address']
        if section==0 and allow_exact_chip_helpers and label in {
            'allocate':0x1a719b,'release':0x19ac01,'copy':0x19a691,'chip_dispatch':0x19b595}:
            assert value==0 and info&15 in (0,2),'Unexpected external helper symbol ABI'
            symbol['address']={'allocate':0x1a719b,'release':0x19ac01,
                               'copy':0x19a691,'chip_dispatch':0x19b595}[label]
        if section==0 and allow_exact_mirror and label=='mirror_capture':
            assert value==0 and info&15 in (0,2)
            symbol['address']=0x199e01
        symbols.append(symbol)
    asm=Ks(KS_ARCH_ARM,KS_MODE_THUMB);relocations=[]
    for section in sections:
        if section[1]!=9 or section[7] not in addresses:continue
        index=section[7];code=parts[index]
        for offset in range(0,section[5],8):
            place,info=struct.unpack_from('<II',blob,section[4]+offset)
            kind=info&255;symbol=symbols[info>>8]
            assert 'address' in symbol,'Undefined/unsupported symbol'
            assert place+4<=len(code)
            target=symbol['address']
            if kind==2:
                addend=struct.unpack_from('<I',code,place)[0]
                struct.pack_into('<I',code,place,(target+addend)&0xffffffff)
                assert struct.unpack_from('<I',code,place)[0]==target+addend
            elif kind in (47,48) and allow_zero_addend_mov16:
                # AAELF32 THM_MOVW_ABS_NC / THM_MOVT_ABS, narrow zero-addend case.
                first,second=struct.unpack_from('<HH',code,place)
                immediate=((first&15)<<12)|(((first>>10)&1)<<11)|(((second>>12)&7)<<8)|(second&255)
                assert immediate==0,'Nonzero MOV16 addend is outside this linker contract'
                instruction=list(fw.md.disasm_lite(bytes(code[place:place+4]),place))[0]
                mnemonic='movw' if kind==47 else 'movt'
                assert instruction[2]==mnemonic
                register=instruction[3].split(',')[0]
                value=target&65535 if kind==47 else target>>16
                encoded=bytes(asm.asm(mnemonic+' '+register+', #'+str(value))[0])
                assert len(encoded)==4
                code[place:place+4]=encoded
            else:
                assert kind in (10,30),kind
                original=list(fw.md.disasm_lite(bytes(code[place:place+4]),place))[0]
                mnemonic='bl' if kind==10 else 'b.w'
                assert original[2]==mnemonic
                addend=int(original[3][1:],0)-(place+4)
                # AAELF32 encodes the Thumb PC bias as the original -4 addend.
                assert addend==-4,addend
                destination=target&~1
                encoded=bytes(asm.asm(mnemonic+' #'+hex(destination),addr=addresses[index]+place)[0])
                assert len(encoded)==4
                assert list(fw.md.disasm_lite(encoded,addresses[index]+place))[0][2:]==(mnemonic,'#'+hex(destination))
                code[place:place+4]=encoded
            relocations.append(dict(section=name(sections[index]),offset=place,kind=kind,symbol=symbol['name']))
    output=bytearray(cursor-BASE)
    for i,part in parts.items():output[addresses[i]-BASE:addresses[i]-BASE+len(part)]=part
    assert len(relocations)==expected_relocations and set(resolved)>={'fetch_complete','fetch_gate','change'}
    return bytes(output),resolved,relocations

def assemble(source,address,bindings):
    asm=Ks(KS_ARCH_ARM,KS_MODE_THUMB)
    code=bytearray(asm.asm(source,addr=address)[0])
    for marker,(mnemonic,target) in bindings.items():
        pattern=bytes([marker,0xbe,0xff,0xbe])
        assert code.count(pattern)==1
        offset=code.index(pattern)
        encoded=bytes(asm.asm(mnemonic+' #'+hex(target),addr=address+offset)[0])
        assert len(encoded)==4
        assert list(fw.md.disasm_lite(encoded,address+offset))[0][2:]==(mnemonic,'#'+hex(target))
        code[offset:offset+4]=encoded
    return bytes(code)

def build():
    obj=OBJECT.read_bytes()
    compiled,symbols,relocs=link.relocate(obj,expected_relocations=11,
        allow_zero_addend_mov16=True,allow_exact_chip_helpers=True,allow_exact_mirror=True)
    assert len(compiled)==488 and len(compiled)<=512
    capture=link.assemble((HERE/'eapol_mirror_capture_small.S').read_text(),0x199e00,{})
    assert len(capture)==448
    wrapper=link.assemble('''push.w {r0-r7,r12,lr}
bkpt #1
bkpt #255
cmp r0,#1
pop.w {r0-r7,r12,lr}
beq owned
bkpt #2
bkpt #255
owned:
bkpt #3
bkpt #255
''',0x199fc0,{1:('bl',symbols['fetch_gate']&~1),2:('b.w',0x1f6c8),3:('b.w',0x1b7c5e)})
    assert len(wrapper)==24
    control=link.assemble((HERE/'eapol_mirror_control.S').read_text(),0x1ed100,
                          {1:('b.w',0x1b0d5c),2:('bl',0x2988)})
    init_address=(0x1ed100+len(control)+3)&~3
    initializer=link.assemble('''push {r4,lr}
ldr r4,=0x1eeaf0
mov r0,r4
movs r1,#0
movs r2,#128
bkpt #1
bkpt #255
ldr r0,=0x31585452
str r0,[r4]
movs r0,#3
str r0,[r4,#24]
ldr r0,=0x31574152
str r0,[r4,#64]
movs r0,#1
str r0,[r4,#88]
pop {r4,pc}
.ltorg
''',init_address,{1:('bl',0x2988)})
    assert init_address+len(initializer)<=0x1ed400
    assert not any(link.fw.read(0x199c00,1024)) and not any(link.fw.read(0x1ed100,768))
    image=bytearray(link.fw.data);edits=[]
    def edit(address,data):
        image[address-link.fw.base:address-link.fw.base+len(data)]=data
        edits.append(dict(address=hex(address),bytes=len(data)))
    for address,data in [(0x199c00,compiled),(0x199e00,capture),(0x199fc0,wrapper),
                         (0x1ed100,control),(init_address,initializer)]: edit(address,data)
    for address,op,target in [(0x1b0d58,'b.w',0x1ed100),(0x1b7b2c,'bl',0x199fc0),(0x19b348,'b.w',init_address)]:
        edit(address,link.assemble('bkpt #1\nbkpt #255',address,{1:(op,target)}))
    for address,value in [(0x19af64,0x1eeb70),(0x1f024c,0x199000),(0x1ef3bc,0x199000)]:
        edit(address,struct.pack('<I',value))
    # The exact stock and final image hashes bind this reproduction to the
    # already examined reservations. This builder does not re-audit ROM/MPU.
    allowed={i for row in edits for i in range(int(row['address'],16)-link.fw.base,int(row['address'],16)-link.fw.base+row['bytes'])}
    assert all(i in allowed for i,(a,b) in enumerate(zip(link.fw.data,image)) if a!=b)
    digest=hashlib.sha256(image).hexdigest()
    assert digest==OUTPUT_SHA,'Image differs from the live-validated composition'
    proof=dict(image_sha256=digest,object_sha256=hashlib.sha256(obj).hexdigest(),
               compiled_bytes=len(compiled),capture_bytes=len(capture),control_bytes=len(control),
               initializer_bytes=len(initializer),edits=edits,phone_operations=0)
    return bytes(image),proof


def main():
    global OBJECT,link
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stock',type=Path,required=True,help='Privately obtained exact stock RAM image')
    parser.add_argument('--zig',required=True,help='Zig0.13.0 executable')
    parser.add_argument('--output',type=Path,required=True,help='New private output directory, never commit its binaries')
    args=parser.parse_args()
    stock=args.stock.read_bytes()
    if len(stock)!=527400 or hashlib.sha256(stock).hexdigest()!=STOCK_SHA:
        raise SystemExit('Stock input mismatch; no output generated')
    if args.output.exists():raise SystemExit('Output exists; preserve previous build')
    version=subprocess.run([args.zig,'version'],capture_output=True,text=True,check=True).stdout.strip()
    if version!='0.13.0':raise SystemExit('Exact reproduction requires Zig0.13.0')
    args.output.mkdir(parents=True)
    OBJECT=args.output/'combined-fetch.o'
    compile=subprocess.run([args.zig,'cc',*FLAGS,'-c',str(HERE/'eapol_mirror_fetch_combined.c'),'-o',str(OBJECT)],capture_output=True)
    if compile.returncode:raise SystemExit('Compilation failed: '+compile.stderr.decode(errors='replace'))
    fw.data=stock;fw.base=0x198000
    def read(address,length):
        if not fw.base<=address<=address+length<=fw.base+len(stock):raise ValueError('Unmapped stock read')
        return stock[address-fw.base:address-fw.base+length]
    fw.read=read;link=SimpleNamespace(fw=fw,assemble=assemble,relocate=relocate)
    image,proof=build()
    (args.output/'candidate.trx').write_bytes(image)
    edits=[dict(offset=int(row['address'],16)-fw.base,
                data=image[int(row['address'],16)-fw.base:int(row['address'],16)-fw.base+row['bytes']].hex()) for row in proof['edits']]
    (args.output/'edits.json').write_text(json.dumps(edits,separators=(',',':'))+'\n')
    (args.output/'proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps({k:v for k,v in proof.items() if k!='edits'}))


if __name__=='__main__':main()
