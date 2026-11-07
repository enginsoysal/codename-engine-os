"""Exercise separately loaded CPL3 apps, faults and executable removal in the native desktop."""
import ctypes as c
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct
import time
import traceback

ROOT=Path(__file__).resolve().parents[2]
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path)
    value=importlib.util.module_from_spec(spec);spec.loader.exec_module(value);return value
s=module('m7session',Path(__file__).with_name('session.py'))
w=module('m7windows',Path(__file__).with_name('windows.py'))
h,q=s.h,s.q


def symbol(name):
    mapping=(ROOT/'dist/EngineOS-M7-installed-kernel.map').read_text(encoding='utf-8')
    result=re.search(r'^\s*([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)\s+\d+\s+'+name+r'$',mapping,re.M)
    assert result,name
    return int(result[1],16),int(result[2],16)


def memory(guest,address,size):
    count=(size+7)//8
    raw=guest.qmp('human-monitor-command',{'command-line':f'xp /{count}gx 0x{address:x}'})
    values=[]
    for line in raw.splitlines():
        if ':' in line:values.extend(int(value,16) for value in re.findall(r'0x[0-9a-fA-F]+',line.split(':',1)[1]))
    assert len(values)==count
    return b''.join(struct.pack('<Q',value) for value in values)[:size]


def model(guest):
    return w.Model.from_buffer_copy(memory(guest,symbol('model')[0],c.sizeof(w.Model)))


def window(guest,kind):
    state=model(guest)
    for index in reversed(state.order[:state.count]):
        if state.windows[index].app==kind:return index,state.windows[index].bounds
    raise AssertionError('Expected native window missing: '+str(kind)+'; count='+str(state.count)+'; order='+str(list(state.order))+'; windows='+str([(v.app,v.live,v.minimized) for v in state.windows]))


def focus(guest,pointer,kind):
    slot,_=window(guest,kind);state=model(guest)
    index=list(state.order[:state.count]).index(slot)
    pointer.click(130+index*100+42,770)


def calc(guest,pointer):
    _,bounds=window(guest,4)
    for key in '6*7=':
        index='789/456*123-C0=+'.index(key)
        pointer.click(bounds.x+22+(index%4)*62+26,bounds.y+55+72+(index//4)*42+15)
    slot,_=window(guest,4);address,size=symbol('apps');stride=size//8
    assert struct.unpack('<q',memory(guest,address+slot*stride+8192+16,8))[0]==42


def control(guest,key):
    guest.qmp('send-key',{'keys':[{'type':'qcode','data':'ctrl'},{'type':'qcode','data':key}],'hold-time':30})
    time.sleep(.15)


def main():
    work=q.new_work('m7-isolation');out=ROOT/'evidence/M7/isolation';out.mkdir(parents=True,exist_ok=True)
    checks=[];result={'passed':False,'m7_complete':False,'checks':checks}
    try:
        disk=q.sparse_disk(work/'owned.img',2*1024**3)
        with q.Guest(work/'install',disk,image_name='EngineOS-M7') as guest:
            h.next(guest);h.next(guest);h.next(guest,2);h.next(guest,3);h.next(guest,5)
            start=len(guest.log());h.next(guest,1);guest.wait('M6_INSTALL_PASS',start,240)
        with q.Guest(work/'desktop',disk,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY') as guest:
            pointer=s.Pointer(guest)
            pointer.click(55,375);guest.wait('M7_APP_RING3_CREATED kind=0x0000000000000004')
            calc(guest,pointer)
            _,bounds=window(guest,4)
            pointer.click(bounds.x+22+290+70,bounds.y+55+72+15)
            guest.wait('M7_APP_CONTAINED kind=0x0000000000000004')
            h.shot(guest,out,'calculator-crash-contained')
            pointer.click(55,210)
            for character in 'survives crash':h.key(guest,'spc' if character==' ' else character)
            control(guest,'s');guest.wait('M7_DOCUMENT_SAVED')
            # The actual model exposes separate slots; the native runtime owns distinct CR3s.
            slot,_=window(guest,2);address,size=symbol('instances');stride=size//8
            cr3=struct.unpack('<Q',memory(guest,address+slot*stride+96,8))[0]
            assert cr3 and 'M7_APP_FAULT vector=0x0000000000000006' in guest.log()
            checks.append({'name':'separate Calculator ELF executes arithmetic in CPL3, actual UD2 is contained, private space is reaped and a separate Editor still saves useful text','passed':True})
            # Native launcher > System details > actual negative app protocol/MMU probes.
            h.key(guest,'f10');h.key(guest,'tab',6);h.key(guest,'ret')
            _,bounds=window(guest,6)
            pointer.click(bounds.x+20+100,bounds.y+290+15)
            guest.wait('M7_APP_ISOLATION_PASS')
            assert 'M7_APP_ISOLATION_FAILED' not in guest.log()
            assert 'status=0x0000000000000008' in guest.log()
            assert guest.log().count('M7_APP_FAULT vector=0x000000000000000e')==2
            checks.append({'name':'UD2, CPU-bound loop timeout, kernel/code writes and malformed output are contained; allocations return to baseline; required Editor removal is denied','passed':True,'fault_cases':5})
            pointer.click(55,294)
            _,bounds=window(guest,3)
            pointer.click(bounds.x+20+70,bounds.y+55+205+15)
            guest.wait('M7_CALCULATOR_REMOVED')
            pointer.click(bounds.x+20+100+40,bounds.y+55+60+15)
            pointer.click(bounds.x+20+60,bounds.y+55+130+15)
            pointer.click(bounds.x+20+75,bounds.y+55+260+15)
            guest.wait('M7_PREFERENCES_SAVED')
            focus(guest,pointer,2);control(guest,'c')
            pointer.click(55,210);control(guest,'v');start=len(guest.log());control(guest,'s');guest.wait('M7_DOCUMENT_SAVED',start)
            h.shot(guest,out,'editor-after-calculator-removal')
            (out/'desktop.log').write_bytes(guest.log().encode())
            time.sleep(.2)
            assert guest.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==2
        assert s.document(disk,'/Users/Owner/Documents/Note1.txt')==b'survives crash'
        assert s.document(disk,'/Users/Owner/Documents/Note2.txt')==b'survives crash'
        try:s.document(disk,'/Applications/Calculator/program.elf');raise RuntimeError('Calculator executable still installed')
        except AssertionError as error:assert 'Expected persistent file not found' in str(error)
        assert s.document(disk,'/Applications/TextEditor/program.elf')[:4]==b'\x7fELF'
        checks.append({'name':'Calculator executable removed while Editor/shared services remain usable; clipboard copies real document into a second saved file; independent volume CRC/bytes inspection','passed':True})
        with q.Guest(work/'restart',disk,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY') as guest:
            pointer=s.Pointer(guest)
            pointer.click(230,250);guest.wait('M7_APP_RING3_CREATED kind=0x0000000000000002')
            h.shot(guest,out,'persistent-customization-after-restart')
            pointer.click(55,294)
            _,bounds=window(guest,3)
            pointer.click(bounds.x+20+70,bounds.y+55+205+15)
            guest.wait('M7_CALCULATOR_INSTALLED')
            pointer.click(55,375);calc(guest,pointer)
            h.shot(guest,out,'calculator-restored-from-cache')
            (out/'restart.log').write_bytes(guest.log().encode())
        assert s.document(disk,'/Users/Owner/Documents/Note1.txt')==b'survives crash'
        assert s.document(disk,'/Applications/Calculator/program.elf')[:4]==b'\x7fELF'
        prefs=s.document(disk,'/System/Settings/desktop7.bin')
        assert struct.unpack_from('<III',prefs,8)==(0x9caeff,1,0)
        checks.append({'name':'removed app and actual theme/accent persist across detached boot, existing document reopens, then optional app reinstalls from cache without changing user files','passed':True})
        manifest=json.loads((ROOT/'dist/m7-build-manifest.json').read_text(encoding='utf-8'))
        for kind,app in [('TextEditor','editor'),('Calculator','calculator')]:
            actual=s.document(disk,'/Applications/'+kind+'/program.elf')
            assert hashlib.sha256(actual).hexdigest()==manifest['artifacts']['EngineOS-M7-'+app+'.elf']['sha256']
        result.update(passed=True,build_id=manifest['build_id'],iso_sha256=manifest['artifacts']['EngineOS-M7.iso']['sha256'],
                      scope='native desktop/app/package slice; networking and full M7 qualification remain pending')
    except Exception:
        result['error']=q.clean(traceback.format_exc());print(result['error'])
        if 'guest' in locals():(out/'last-run.log').write_bytes(guest.log().encode())
    (out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode())
    print('M7 native app isolation '+('PASS' if result['passed'] else 'FAIL'),flush=True)
    if not result['passed']:raise SystemExit(1)


if __name__=='__main__':main()
