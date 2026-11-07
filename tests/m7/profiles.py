"""Validate actual desktop profile schema and generated implemented defaults."""
import copy
import json
from pathlib import Path
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/build'))
from distro import load,prepare


def main():
    standard=load(ROOT/'distros/engineos-desktop.json',desktop=True)
    night=load(ROOT/'distros/engineos-night.json',desktop=True)
    assert standard['desktop']['calculator'] and not night['desktop']['calculator']
    assert night['desktop']['dark'] and night['accent']=='#9caeff'
    with tempfile.TemporaryDirectory(dir=ROOT/'distros',prefix='m7-profile-test-') as temporary:
        work=Path(temporary)
        prepare(work,night);header=(work/'distro.h').read_text(encoding='utf-8')
        assert '#define DISTRO_DESKTOP_DARK 1u' in header
        assert '#define DISTRO_DESKTOP_CALCULATOR 0u' in header
        assert '#define DISTRO_DESKTOP_WALLPAPER 0x241d35u' in header
        assert '#define DISTRO_NAME "Engine OS Night"' in header
        base=json.loads((ROOT/'distros/engineos-night.json').read_text(encoding='utf-8'))
        cases=[]
        for field,value in [('dark',1),('calculator','false'),('wallpaper','#fffffff'),('tagline','x'*49),('hooks',['run'])]:
            p=copy.deepcopy(base);p['desktop'][field]=value;cases.append(p)
        p=copy.deepcopy(base);p['desktop'].pop('calculator');cases.append(p)
        p=copy.deepcopy(base);p['core_removal']=True;cases.append(p)
        for p in cases:
            filename=work/'invalid.json';filename.write_text(json.dumps(p),encoding='utf-8')
            try:load(filename,desktop=True)
            except ValueError:pass
            else:raise AssertionError('Unsupported or unsafe profile accepted')
        try:load(ROOT/'distros/engineos-night.json')
        except ValueError:pass
        else:raise AssertionError('Desktop profile silently accepted by a historical data-only build')
    report={'passed':True,'m7_complete':False,'scope':'actual profile validation/generated defaults; custom native ISO still requires separate qualification','negative_cases':len(cases)+1}
    (ROOT/'evidence/M7/profile-results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('Desktop profile validation PASS; native custom-distro proof pending')


if __name__=='__main__':main()
