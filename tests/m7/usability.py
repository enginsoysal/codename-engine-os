"""Exercise named files, confirmation/cancel, nested folders and optional terminal.

These are automated native user tasks, not a human usability study.
"""
import ctypes as c
import importlib.util
import json
from pathlib import Path
import time
import traceback
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('m7resources',Path(__file__).with_name('resources.py'))
r=importlib.util.module_from_spec(spec);spec.loader.exec_module(r)
s,i,h,q=r.s,r.i,r.h,r.q


class View(c.Structure):
    _fields_=[('text',c.c_char*8192),*[(n,c.c_uint32) for n in ['length','cursor','dirty','calc_error']],('number',c.c_int64),('operand',c.c_int64),('operation',c.c_uint32),('entered',c.c_uint32)]
class App(c.Structure):
    _fields_=[('view',View),('path',c.c_char*256),('notice',c.c_char*96),*[(n,c.c_uint32) for n in ['closing','offset','stopped','existed','source_crc']],('pending',c.c_char*256),('name_cursor',c.c_uint32),('name_selected',c.c_uint32)]


def app(guest,kind):
    slot,bounds=i.window(guest,kind);base,size=i.symbol('apps');assert size//8==c.sizeof(App)
    return App.from_buffer_copy(i.memory(guest,base+slot*c.sizeof(App),c.sizeof(App))),bounds


def type_text(guest,text):
    model=i.model(guest);slot=model.focused;assert slot<8;kind=model.windows[slot].app
    current,_=app(guest,kind);base,size=i.symbol('apps')
    if kind==7 or current.closing==3:
        offset=App.pending.offset;expected=b'' if current.name_selected else current.pending
    else:offset=0;expected=current.view.text
    address=base+slot*(size//8)+offset
    mapping={' ':'spc','/':'slash','.':'dot','-':'minus','_':'minus',':':'semicolon'}
    for char in text:
        keys=([{'type':'qcode','data':'shift'}] if char.isupper() or char in '_:' else [])+[{'type':'qcode','data':mapping.get(char,char.lower())}]
        guest.qmp('send-key',{'keys':keys,'hold-time':60});time.sleep(.08);expected+=char.encode()
        deadline=time.monotonic()+2
        while time.monotonic()<deadline:
            actual=i.memory(guest,address,len(expected)+1).split(b'\0')[0]
            if actual==expected:break
            time.sleep(.02)
        assert actual==expected,('Native typed prefix mismatch',actual,expected)


def delete_row(pointer,bounds,index):pointer.click(bounds.x+bounds.width-56,bounds.y+48+70+index*30+15)
def open_row(pointer,bounds,index):pointer.click(bounds.x+70,bounds.y+48+70+index*30+15)


def main():
    work=q.new_work('m7-usability');out=ROOT/'evidence/M7/usability';out.mkdir(parents=True,exist_ok=True)
    report={'passed':False,'m7_complete':False,'human_usability_study':False,'checks':[]}
    try:
        disk=q.sparse_disk(work/'owned.img',2*1024**3)
        with q.Guest(work/'install',disk,image_name='EngineOS-M7') as guest:
            h.next(guest);h.next(guest);h.next(guest,2);h.next(guest,3);h.next(guest,5);start=len(guest.log());h.next(guest,1);guest.wait('M6_INSTALL_PASS',start,240)
        original_setup=s.document(disk,'/System/Settings/setup.bin')
        with q.Guest(work/'desktop',disk,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY') as guest:
            pointer=s.Pointer(guest);pointer.click(55,210);current,editor=app(guest,2)
            type_text(guest,'my project');pointer.click(editor.x+400,editor.y+63)
            assert app(guest,2)[0].closing==3
            type_text(guest,'project.txt');assert app(guest,2)[0].pending==b'project.txt'
            h.key(guest,'ret');guest.wait('M7_DOCUMENT_SAVE_AS');current,_=app(guest,2)
            assert current.path==b'/Users/Owner/Documents/project.txt' and not current.view.dirty
            type_text(guest,' update');h.key(guest,'esc');assert app(guest,2)[0].closing==1
            h.key(guest,'ret');assert app(guest,2)[0].closing==0 and app(guest,2)[0].view.dirty
            i.control(guest,'s');guest.wait('M7_DOCUMENT_SAVED')
            pointer.click(editor.x+400,editor.y+63);h.key(guest,'ret')
            current,_=app(guest,2);assert current.closing==3 and b'already exists' in current.notice
            h.key(guest,'esc');assert app(guest,2)[0].closing==0
            h.shot(guest,out,'chosen-filename-and-save')
            i.focus(guest,pointer,1);_,files=app(guest,1);delete_row(pointer,files,1)
            assert b'Close the editor' in app(guest,1)[0].notice
            i.focus(guest,pointer,2);h.key(guest,'esc');i.focus(guest,pointer,1)
            _,files=app(guest,1);pointer.click(files.x+265,files.y+63)
            open_row(pointer,files,2);assert app(guest,1)[0].path.endswith(b'/Folder1')
            pointer.click(files.x+135,files.y+63);current,_=app(guest,2)
            assert current.path==b'/Users/Owner/Documents/Folder1/Note1.txt'
            type_text(guest,'nested');i.control(guest,'s');h.key(guest,'esc')
            i.focus(guest,pointer,1);pointer.click(files.x+44,files.y+63);assert app(guest,1)[0].path==b'/Users/Owner/Documents'
            r.start_menu(guest,7);assert app(guest,7)[0].view.text.endswith(b'engine> ')
            type_text(guest,'help');h.key(guest,'ret');guest.wait('menu | help | volume')
            type_text(guest,'write /Users/Owner/Documents/from-terminal.txt terminal data');h.key(guest,'ret');guest.wait('Write status: 0')
            type_text(guest,'rm /System/Settings/setup.bin');h.key(guest,'ret');guest.wait('Remove status: 7')
            type_text(guest,'cat /Users/Owner/Documents/project.txt');start=len(guest.log());h.key(guest,'ret');guest.wait('my project update',start)
            current,_=app(guest,7);assert b'my project update' in current.view.text and b'Remove status: 7' in current.view.text
            h.shot(guest,out,'advanced-terminal-window');type_text(guest,'exit');h.key(guest,'ret')
            assert not any(w.live and w.app==7 for w in i.model(guest).windows)
            i.focus(guest,pointer,1);_,files=app(guest,1);delete_row(pointer,files,1)
            assert app(guest,1)[0].closing==2;pointer.click(files.x+95,files.y+165)
            assert not app(guest,1)[0].closing;open_row(pointer,files,1)
            assert app(guest,2)[0].view.text==b'my project update';h.key(guest,'esc')
            i.focus(guest,pointer,1);delete_row(pointer,files,1);pointer.click(files.x+260,files.y+165);guest.wait('M7_FILE_DELETED')
            delete_row(pointer,files,1);pointer.click(files.x+260,files.y+165)
            assert b'not empty' in app(guest,1)[0].notice
            delete_row(pointer,files,0);pointer.click(files.x+260,files.y+165)
            h.shot(guest,out,'confirmed-delete-and-protected-folder')
            for _ in range(7):r.start_menu(guest,1)
            assert i.model(guest).count==8
            r.start_menu(guest,2);assert i.model(guest).count==8
            assert b'Eight windows' in i.memory(guest,i.symbol('shell_notice')[0],96)
            h.shot(guest,out,'visible-window-limit')
            h.key(guest,'esc');focused=i.model(guest);bounds=focused.windows[focused.focused].bounds
            pointer.click(bounds.x+bounds.width-30,bounds.y+18);assert i.model(guest).count==7
            r.start_menu(guest,2);assert i.model(guest).count==8 and not app(guest,2)[0].stopped
            (out/'serial.log').write_bytes(guest.log().encode())
        assert s.document(disk,'/Users/Owner/Documents/Folder1/Note1.txt')==b'nested'
        assert s.document(disk,'/Users/Owner/Documents/from-terminal.txt')==b'terminal data'
        assert s.document(disk,'/System/Settings/setup.bin')==original_setup
        for path in ['/Users/Owner/Documents/project.txt','/Users/Owner/Documents/Welcome.txt']:
            try:s.document(disk,path)
            except AssertionError:pass
            else:raise AssertionError('Confirmed deleted file still exists: '+path)
        with q.Guest(work/'restart',disk,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY') as guest:
            h.shot(guest,out,'named-and-nested-files-after-restart');(out/'restart.log').write_bytes(guest.log().encode())
        assert s.document(disk,'/Users/Owner/Documents/Folder1/Note1.txt')==b'nested'
        meta=json.loads((ROOT/'dist/m7-build-manifest.json').read_text(encoding='utf-8'))
        report.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M7.iso']['sha256'])
        report['checks']=['Save as chooses a real persistent filename and refuses an existing name','dirty close defaults to keeping text','GUI deletion refuses an open document and asks with safe cancel','new folders/text use the currently displayed folder','advanced terminal is a native window with real bounded file commands and protected core removal','confirmed deletion removes only the selected file and refuses nonempty folders','eight-window quota gives a visible explanation, then closing one permits another app','named/nested/terminal files and exact core bytes survive detached restart']
    except Exception:
        report['error']=q.clean(traceback.format_exc());print(report['error'])
        if 'guest' in locals():(out/'last-run.log').write_bytes(guest.log().encode())
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('M7 native everyday tasks '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
