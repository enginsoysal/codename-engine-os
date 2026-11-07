"""Check native calculator algorithms against independent decimal/math values."""
import ctypes
from decimal import Decimal
import json
import math
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/build'))
from m1 import pinned_tools

class View(ctypes.Structure):
    _fields_=[('text',ctypes.c_char*8192),('length',ctypes.c_uint32),('cursor',ctypes.c_uint32),
              ('dirty',ctypes.c_uint32),('calc_error',ctypes.c_uint32),('number',ctypes.c_int64),
              ('operand',ctypes.c_int64),('operation',ctypes.c_uint32),('entered',ctypes.c_uint32)]

def main():
    work=ROOT/'build/m74/calculator-host';work.mkdir(parents=True,exist_ok=True);tools=pinned_tools()
    (work/'test.c').write_bytes(b'#define APP7_USER\n#include <engine/desktop/app.h>\n#include "calculator74.inc"\n__declspec(dllexport) void evaluate(app7_view *v,unsigned key){calculate74(v,key);}\n')
    subprocess.run([tools['clang'],'--target=x86_64-pc-windows-msvc','-std=c17','-O2','-ffreestanding','-fno-stack-protector',
                    '-I',str(ROOT/'include'),'-I',str(ROOT/'user'),'-c',str(work/'test.c'),'-o',str(work/'test.obj')],check=True,capture_output=True)
    subprocess.run([tools['lld-link'],'/dll','/noentry','/nodefaultlib','/machine:x64','/out:'+str(work/'test.dll'),str(work/'test.obj')],check=True,capture_output=True)
    dll=ctypes.CDLL(str(work/'test.dll'));dll.evaluate.argtypes=[ctypes.POINTER(View),ctypes.c_uint]
    def evaluate(keys):
        view=View()
        for key in keys:dll.evaluate(ctypes.byref(view),ord(key))
        assert view.length<8192 and view.cursor<=view.length and view.text[view.length:]==b''
        return view
    cases=[('12.5*2=',Decimal('25')),('0.1+0.2=',Decimal('0.3')),('2+3*2=',Decimal('10')),
           ('1/3=',Decimal('0.333333')),('2.25r',Decimal('1.5')),('12q',Decimal('144')),
           ('4i',Decimal('0.25')),('50%',Decimal('0.5')),('0f',Decimal('32')),('32t',Decimal('0')),
           ('1m',Decimal('1.609344')),('1b',Decimal('0.453592')),
           ('90s',Decimal(str(math.sin(math.pi/2)))),('180o',Decimal('-1')),('1l',Decimal('0')),
           ('2l',Decimal(str(math.log(2)))),('0.000001*0.000001=',Decimal('0'))]
    for keys,expected in cases:
        view=evaluate(keys);assert not view.calc_error,keys
        actual=Decimal(view.text.decode());assert abs(actual-expected)<=Decimal('0.00001'),(keys,str(actual),str(expected))
    for keys in ['1/0=','1nr','0l','9223372036854.775807+1=','999999999999999999999999']:
        assert evaluate(keys).calc_error,keys
    for keys in ['999999999999999999999999C2+2=','1/0=C2+2=']:
        view=evaluate(keys);assert not view.calc_error and view.text==b'4'
    out=ROOT/'evidence/M7.4';out.mkdir(exist_ok=True)
    (out/'calculator-host-results.json').write_bytes((json.dumps({'passed':True,'host_only':True,'normal_cases':len(cases),'error_cases':5,'recovery_cases':2},indent=2)+'\n').encode('utf-8'))
    print('Calculator algorithms PASS; native UI acceptance remains separate')

if __name__=='__main__':main()
