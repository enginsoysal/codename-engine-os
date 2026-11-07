"""Cross-check actual calendar conversion against Python's independent calendar."""
import ctypes as c
from datetime import date,timedelta
import json
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/build'))
from m1 import pinned_tools


class Time(c.Structure):
    _fields_=[(n,c.c_uint32) for n in ['year','month','day','hour','minute','second','days','seconds']]


def main():
    work=ROOT/'build/m7/clock-test';work.mkdir(parents=True,exist_ok=True);tools=pinned_tools()
    def run(args):
        result=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True)
        if result.returncode:raise RuntimeError(result.stderr.replace(str(ROOT),'<project-root>'))
    flags=['-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-I',ROOT/'include']
    run([tools['clang'],'--target=x86_64-unknown-none-elf',*flags,'-c',ROOT/'kernel/network/clock.c','-o',work/'rtc.o'])
    run([tools['clang'],'--target=x86_64-pc-windows-msvc',*flags,'-c',ROOT/'kernel/network/calendar.c','-o',work/'calendar.obj'])
    run([tools['lld-link'],'/dll','/noentry','/nodefaultlib','/export:clock7_calendar',work/'calendar.obj','/out:'+str(work/'calendar.dll')])
    lib=c.CDLL(str(work/'calendar.dll'));f=lib.clock7_calendar;f.argtypes=[*[c.c_uint]*6,c.POINTER(Time)]
    current=date(2020,1,1);count=0
    while current.year<2100:
        out=Time();assert not f(current.year,current.month,current.day,23,59,59,c.byref(out))
        assert (out.days,out.seconds)==(current.toordinal()+365,86399)
        current+=timedelta(days=1);count+=1
    for values in [(2019,12,31,0,0,0),(2100,1,1,0,0,0),(2021,2,29,0,0,0),(2024,2,30,0,0,0),(2026,0,1,0,0,0),(2026,13,1,0,0,0),(2026,1,0,0,0,0),(2026,1,1,24,0,0),(2026,1,1,0,60,0),(2026,1,1,0,0,60)]:
        out=Time();c.memset(c.byref(out),0xa5,c.sizeof(out));before=bytes(out)
        assert f(*values,c.byref(out)) and bytes(out)==before
    (ROOT/'evidence/M7/clock-results.json').write_text(json.dumps({'passed':True,'calendar_days':count,'scope':'actual calendar C conversion and RTC freestanding compilation only; no native RTC qualification','m7_complete':False},indent=2)+'\n',encoding='utf-8')
    print('Calendar PASS: '+str(count)+' dates and invalid input refusal; native RTC pending')


if __name__=='__main__':main()
