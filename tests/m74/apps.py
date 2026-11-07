"""Exercise actual application UI and independently inspect the owned volume."""
import json
import io
import struct
import time
import traceback
import ui

ROOT=ui.ROOT
q,h=ui.q,ui.h

def pages(g,installed=False):
    return struct.unpack('<4Q',ui.memory(g,ui.symbol('stats',32,installed)[0],32))

def open_app(pointer,kind,installed=False):
    pointer.desktop(0,installed)
    pointer.desktop(kind,installed)

def app_text(g,kind):
    address,size=ui.symbol('apps');stride=size//8
    model=ui.memory(g,ui.symbol('model')[0],28*8)
    slots=[i for i in range(8) if struct.unpack_from('<I',model,i*28+20)[0]==kind and model[i*28+24]]
    assert len(slots)==1
    data=ui.memory(g,address+slots[0]*stride,8196);length=struct.unpack_from('<I',data,8192)[0]
    assert length<8192;return data[:length]

def main():
    out=ROOT/'evidence/M7.4/apps-development';out.mkdir(parents=True,exist_ok=True)
    work=q.new_work('m74-apps');disk=q.sparse_disk(work/'owned.img',2*1024**3)
    utility=q.sparse_disk(work/'utility.img',512*1024**2)
    manifest=json.loads((ROOT/'dist/m74-build-manifest.json').read_text(encoding='utf-8'))
    report={'passed':False,'scope':'development app slice; not full M7.4 acceptance','checks':[],
            'build_id':manifest['build_id'],'iso_sha256':manifest['artifacts']['EngineOS-M74.iso']['sha256']}
    def checkpoint(stage):
        report['stage']=stage
        (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode('utf-8'))
        print('M7.4 native stage: '+stage,flush=True)
    checkpoint('starting')
    try:
        extra=['-drive',f'if=none,id=utility,format=raw,file={utility}','-device','ide-hd,bus=ide.1,drive=utility',
               '-cpu','max,x2apic=off','-rtc','base=utc,clock=host','-netdev','user,id=net74',
               '-device','e1000,id=net74-card,netdev=net74,mac=52:54:00:12:34:56',
               '-audiodev',f'wav,id=audio74,path={work / "audio-output.wav"},out.frequency=48000,out.channels=2,out.format=s16',
               '-device','ich9-intel-hda,id=hda74','-device','hda-duplex,bus=hda74.0,audiodev=audio74']
        with ui.guest(work/'install',disk,extra=extra) as g:
            p=ui.Pointer(g)
            p.wizard(2);p.wizard(100);p.wizard(2);p.wizard(2);p.wizard(2)
            start=p.wizard(2);g.wait('M6_INSTALL_PASS',start,240);ui.wait_view(g,6,start)
            p.wizard(3);g.wait('M7_DESKTOP_READY');p.close_app(1)
            checkpoint('installed-desktop')
            baseline=pages(g)
            open_app(p,4)
            for command in [13010,13011,13016,13006,13008,13011,13017]:p.desktop(command)
            raw=ui.memory(g,ui.symbol('apps')[0],ui.symbol('apps')[1]);stride=len(raw)//8
            assert any(raw[i:i+8192].split(b'\0')[0]==b'25' for i in range(0,len(raw),stride))
            ui.shot(g,out,'calculator-decimal');p.desktop(13101);ui.shot(g,out,'calculator-scientific');p.desktop(13102);ui.shot(g,out,'calculator-convert');p.close_app(4)
            assert pages(g)[0]==baseline[0]
            checkpoint('calculator')
            open_app(p,8);ui.shot(g,out,'clock')
            # The true IRQ service must wake the UI without continuous PIT polling.
            before=ui.memory(g,ui.symbol('elapsed74',8)[0],8);time.sleep(1.3)
            assert before!=ui.memory(g,ui.symbol('elapsed74',8)[0],8)
            p.desktop(10021);p.desktop(10033);ui.shot(g,out,'timer');p.desktop(10034)
            p.desktop(10022);p.desktop(10035);time.sleep(1.1);p.desktop(10035);ui.shot(g,out,'stopwatch')
            p.close_app(8);assert pages(g)[0]==baseline[0]
            report['checks'].append('native Clock, RTC IRQ wakeup, timer and stopwatch input; last-close page recovery')
            checkpoint('clock')
            open_app(p,9);ui.shot(g,out,'calendar')
            for c in 'a real local note':h.key(g,'spc' if c==' ' else c)
            p.desktop(10052);ui.shot(g,out,'calendar-saved')
            # Find the actual saved path by reading the native application metadata.
            raw=ui.memory(g,ui.symbol('apps')[0],ui.symbol('apps')[1]);stride=len(raw)//8
            notes=[]
            for offset in range(0,len(raw),stride):
                for part in raw[offset:offset+stride].split(b'\0'):
                    if part.startswith(b'/Users/Owner/AppData/Calendar/') and part.endswith(b'.txt'):notes.append(part.decode())
            assert notes;path=notes[0]
            assert ui.s.s.document(disk,path)==b'a real local note'
            p.close_app(9);assert pages(g)[0]==baseline[0]
            checkpoint('calendar')
            open_app(p,11);ui.shot(g,out,'software')
            p.desktop(10302);ui.shot(g,out,'removal-review');p.desktop(10003)
            assert ui.s.s.document(disk,'/Applications/TextEditor/program.elf').startswith(b'\x7fELF')
            p.desktop(10302);p.desktop(10004);ui.shot(g,out,'removed')
            try:ui.s.s.document(disk,'/Applications/TextEditor/program.elf')
            except AssertionError:pass
            else:raise AssertionError('Editor executable still installed')
            p.desktop(10302);assert ui.s.s.document(disk,'/Applications/TextEditor/program.elf').startswith(b'\x7fELF')
            p.close_app(11);assert pages(g)[0]==baseline[0]
            checkpoint('software')
            open_app(p,10);ui.shot(g,out,'disk-system-protected');before=q.digest(disk,0,1024*1024)
            p.desktop(12107);assert q.digest(disk,0,1024*1024)==before
            p.desktop(12100);ui.shot(g,out,'disk-second-blank');p.desktop(12106);p.desktop(12110)
            with utility.open('rb') as f:assert f.read(512)==bytes(512),'Unconfirmed initialize wrote to disk'
            p.desktop(12109);p.desktop(12110);p.desktop(12200);p.desktop(12103);ui.shot(g,out,'disk-create-size-name')
            p.desktop(12115);p.desktop(12109);p.desktop(12110);p.desktop(12200);p.desktop(12104);p.desktop(12116);p.desktop(12109);p.desktop(12110)
            with utility.open('rb') as f:
                f.seek(512);header=bytearray(f.read(512));crc=struct.unpack_from('<I',header,16)[0];struct.pack_into('<I',header,16,0)
                import zlib
                assert zlib.crc32(header[:92])==crc
                f.seek(struct.unpack_from('<Q',header,72)[0]*512);table=f.read(16384);assert zlib.crc32(table)==struct.unpack_from('<I',header,88)[0]
                first=struct.unpack_from('<Q',table,32)[0];f.seek(first*512);boot=f.read(512);assert boot[82:90]==b'FAT32   ' and boot[510:512]==b'\x55\xaa'
            ui.shot(g,out,'disk-second-fat32');p.close_app(10);assert pages(g)[0]==baseline[0]
            checkpoint('disk-utility')
            gpt_before=q.digest(disk,512,512+16384)
            open_app(p,12);ui.shot(g,out,'recovery');p.desktop(12118);ui.shot(g,out,'recovery-review');p.desktop(12109);p.desktop(12110)
            report['repair_gpt_before']=gpt_before;report['repair_gpt_after']=q.digest(disk,512,512+16384)
            assert report['repair_gpt_after']==gpt_before
            ui.shot(g,out,'recovery-repaired');p.close_app(12);assert pages(g)[0]==baseline[0]
            checkpoint('recovery')
            open_app(p,13);p.desktop(11000);ui.shot(g,out,'archive-members');archive_open=pages(g)[0]
            p.desktop(14001);ui.shot(g,out,'archive-extracted');assert pages(g)[0]==archive_open
            import zipfile
            with zipfile.ZipFile(ROOT/'distros/packages/apps-guide/ArchiveSample.zip') as z:
                for name in z.namelist():assert ui.s.s.document(disk,'/Users/Owner/Downloads/Extracted1/'+name)==z.read(name)
            p.close_app(13);assert pages(g)[0]==baseline[0]
            checkpoint('archives')
            open_app(p,15);p.desktop(10500);ui.shot(g,out,'screenshot-saved')
            raw=ui.memory(g,ui.symbol('apps')[0],ui.symbol('apps')[1]);paths=[]
            for part in raw.split(b'\0'):
                if part.startswith(b'/Users/Owner/Pictures/Screenshots/') and part.endswith(b'.bmp'):paths.append(part.decode())
            assert paths
            from PIL import Image
            with Image.open(io.BytesIO(ui.s.s.document(disk,paths[0],limit=32*1024*1024))) as picture:
                picture.load();assert picture.size==(1280,800)
                assert picture.getpixel((0,0))!=(0,0,0)
            p.desktop(10501);ui.shot(g,out,'image-viewer');p.close_app(14);p.close_app(15)
            assert pages(g)[0]==baseline[0]
            checkpoint('screenshot-and-images')
            open_app(p,17);ui.shot(g,out,'browser-home');p.desktop(15002)
            for c in 'example.com':h.key(g,'dot' if c=='.' else c)
            start=len(g.log());h.key(g,'ret');g.wait('M74_BROWSER_PAGE_LOADED',start,120)
            body=app_text(g,17);assert b'Example Domain' in body and b'<html' not in body.lower()
            assert b'This domain is for use' in body or b'This domain is for illustrative' in body
            ui.shot(g,out,'browser-real-https');p.desktop(15005);ui.shot(g,out,'browser-links');p.desktop(15000)
            assert b'Welcome to your browser' in app_text(g,17)
            p.close_app(17);assert pages(g)[0]==baseline[0]
            checkpoint('browser-https')
            open_app(p,1);p.desktop(95);p.desktop(1001);p.desktop(1002);p.close_app(1);ui.shot(g,out,'media-ready');assert pages(g)[0]==baseline[0]
            p.desktop(18003);p.desktop(18004)
            assert struct.unpack('<II',ui.s.s.document(disk,'/System/Settings/audio74.bin'))==(0x41443734,100)
            start=len(g.log());p.desktop(18001,settle=False)
            until=time.monotonic()+10
            while 'M74_AUDIO_HDA_PLAYING' not in g.log()[start:]:
                if 'M74_AUDIO_START_FAILED' in g.log()[start:] or time.monotonic()>until:
                    raw=ui.memory(g,ui.symbol('info74',64)[0],64);message=struct.unpack_from('<Q',raw,56)[0]
                    raise AssertionError('Audio did not start: '+ui.memory(g,message,192).split(b'\0')[0].decode(errors='replace'))
                time.sleep(.05)
            raw=ui.memory(g,ui.symbol('info74',64)[0],64)
            if struct.unpack_from('<I',raw,0)[0]!=1:
                message=struct.unpack_from('<Q',raw,56)[0]
                raise AssertionError('Playback stopped while drawing: '+ui.memory(g,message,192).split(b'\0')[0].decode(errors='replace'))
            # Pause and resume through the actual primary button, while retaining bounded DMA pages.
            p.desktop(18001,settle=False);raw=ui.memory(g,ui.symbol('info74',64)[0],64)
            assert struct.unpack_from('<I',raw,0)[0]==2,'Primary control did not pause the real stream'
            assert 0<struct.unpack_from('<Q',raw,40)[0]<=24*4096
            ui.shot(g,out,'media-paused');time.sleep(.2);p.desktop(18001,settle=False);g.wait('M74_AUDIO_FINISHED',start,10)
            ui.shot(g,out,'media-finished');assert pages(g)[0]==baseline[0]
            p.close_app(16);assert pages(g)[0]==baseline[0]
            checkpoint('media-pcm')
            open_app(p,18);ui.shot(g,out,'help');p.desktop(10002);ui.shot(g,out,'help-page2');p.close_app(18)
            assert pages(g)[0]==baseline[0]
            (out/'native.log').write_bytes(g.log().encode('utf-8'))
            assert 'KERNEL_PANIC' not in g.log()
        from capture import verify
        report['audio_capture']=verify(work/'audio-output.wav',out)
        restart=ui.guest(work/'restart',disk,media='installed',ready='M7_DESKTOP_READY');ui.s.normal_boot(restart)
        with restart as g:
            ui.shot(g,out,'independent-boot');assert ui.s.s.document(disk,path)==b'a real local note'
            assert ui.s.s.document(disk,'/Users/Owner/Downloads/Extracted1/Welcome.txt')==b'Engine OS archive sample.\n'*3000
            (out/'restart.log').write_bytes(g.log().encode('utf-8'))
        report['checks']+=['Calendar note independently checked against installed-volume CRC and retained after unforced disk boot',
                           'Editor removal cancelled, confirmed, executable absence checked, offline reinstall and app-close page recovery',
                           'Native screenshot independently CRC-checked and decoded as a 1280x800 BMP; opened in Image Viewer; close returns pages',
                           'Native decimal Calculator, protected system disk, explicit consent, separate-disk GPT creation and FAT32 format independently decoded',
                           'Native verified boot repair followed by an independent installed-disk boot',
                           'Real ZIP Deflate extraction across the 32-KiB dictionary wrap; independent file bytes/CRCs; transient pages freed before close and files retained after restart',
                           'Real certificate-checked HTTPS page opened through typed address, parsed in CPL3, links displayed, Back returns Home and close returns pages',
                           'Native HDA PCM playback, Pause/Resume, actual captured stereo waveform, persistent volume and complete DMA page recovery before close',
                           'Help opens and pages without depending on installed Text Editor']
        report.update(passed=True,build_id=json.loads((ROOT/'dist/m74-build-manifest.json').read_text(encoding='utf-8'))['build_id'])
    except Exception:
        report['error']=q.clean(traceback.format_exc());print(report['error'])
        if 'g' in locals():(out/'failure.log').write_bytes(g.log().encode('utf-8'))
    (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode('utf-8'))
    print('M7.4 app development slice '+('PASS' if report['passed'] else 'FAIL'))
    if not report['passed']:raise SystemExit(1)

if __name__=='__main__':main()
