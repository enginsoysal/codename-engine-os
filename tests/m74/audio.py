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
    out=ROOT/'evidence/M7.4/audio-development';out.mkdir(parents=True,exist_ok=True)
    work=q.new_work('m74-audio');disk=q.sparse_disk(work/'owned.img',2*1024**3)
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
            open_app(p,13);p.desktop(11000);p.desktop(14001);p.close_app(13);assert pages(g)[0]==baseline[0]
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
            open_app(p,1);p.desktop(95);p.desktop(1001);p.desktop(1002);p.close_app(1);start=len(g.log());p.desktop(18001,settle=False);g.wait('M74_AUDIO_FINISHED',start,10)
            p.close_app(16);assert pages(g)[0]==baseline[0]
            (out/'native.log').write_bytes(g.log().encode('utf-8'))
        from capture import verify
        import zipfile
        expected=work/'expected.wav'
        with zipfile.ZipFile(ROOT/'distros/packages/apps-guide/ArchiveSample.zip') as z:expected.write_bytes(z.read('ChimeLong.wav'))
        report['audio_capture']=verify(work/'audio-output.wav',out,expected)
        report.update(passed=True,build_id=json.loads((ROOT/'dist/m74-build-manifest.json').read_text(encoding='utf-8'))['build_id'])
    except Exception:
        report['error']=q.clean(traceback.format_exc());print(report['error'])
        if 'g' in locals():(out/'failure.log').write_bytes(g.log().encode('utf-8'))
    (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode('utf-8'))
    print('M7.4 app development slice '+('PASS' if report['passed'] else 'FAIL'))
    if not report['passed']:raise SystemExit(1)

if __name__=='__main__':main()
