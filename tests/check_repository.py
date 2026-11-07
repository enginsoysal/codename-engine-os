"""Keep current English documentation, source layout and local commands consistent."""
import ast
import json
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
def main(milestone="M2"):
    output=ROOT/'evidence'/milestone;output.mkdir(parents=True,exist_ok=True)
    (output/'repository-results.json').write_text('{"passed": false, "state": "checking"}\n',encoding='utf-8')
    docs=[];scripts=[]
    for p in ROOT.rglob('*'):
        if not p.is_file():continue
        parts=p.relative_to(ROOT).parts
        if parts[0] in ('.git','build','dist') or 'local' in parts or '__pycache__' in parts:continue
        if p.suffix=='.md':docs.append(p)
        if p.suffix=='.py':scripts.append(p)
    forbidden=re.compile(r'\b(mijlpalen|mijlpaal|afgerond|waarom|geheugen|bestanden|gebruiker|vastgelegd|getoetst|bewijs|werkmap|gekozen|afhankelijkheden|ondersteuning|toekomstige)\b',re.I)
    for p in docs:
        text=p.read_text(encoding='utf-8')
        if '\u00e2\u20ac' in text or '\u00c3\u00af' in text:raise RuntimeError('Broken UTF-8 text in '+str(p.relative_to(ROOT)))
        if forbidden.search(text):raise RuntimeError('Untranslated documentation in '+str(p.relative_to(ROOT)))
        for link in re.findall(r'\]\(([^)]+)\)',text):
            if re.match(r'^[a-z]+://|^#',link):continue
            if not (p.parent/link.split('#')[0]).exists():raise RuntimeError('Broken link in '+str(p.relative_to(ROOT))+': '+link)
        for command in re.findall(r'python\s+((?:tools|tests)/[\w/.-]+\.py)',text):
            if not (ROOT/command).is_file():raise RuntimeError('Stale command in '+str(p.relative_to(ROOT))+': '+command)
    for p in scripts:ast.parse(p.read_text(encoding='utf-8'),filename=str(p.relative_to(ROOT)))
    allowed={'README.md','AGENTS.md','CONTRIBUTING.md','LICENSE','.gitattributes','.gitignore'}
    extras=[p.name for p in ROOT.iterdir() if p.is_file() and p.name not in allowed]
    if extras:raise RuntimeError('Unexpected root files: '+str(extras))
    for old in ('ARCHITECTURE.md','DECISIONS.md','STATUS.md','MILESTONES.md','WINDOWS-COMPATIBILITY.md','M1.md'):
        if (ROOT/old).exists():raise RuntimeError('Old root documentation remains: '+old)
    inventory=json.loads((ROOT/'docs/roadmap/windows-targets.json').read_text(encoding='utf-8'))
    assert {p['id'] for p in inventory['generation_profiles']}=={'msdos','win1x','win2x','win3x','nt3x','win95','win98-me','nt4-2000','xp','vista-7','8-81','10','11'} and inventory['applications']==[]
    checks=[{'name':'English current documentation','passed':True,'documents':len(docs)},
            {'name':'local links and Python commands exist','passed':True},
            {'name':'all current Python sources parse','passed':True,'scripts':len(scripts)},
            {'name':'clean root and milestone/source directory boundaries','passed':True},
            {'name':'Windows generation scope retained without false qualification','passed':True}]
    (output/'repository-results.json').write_text(json.dumps({'passed':True,'checks':checks},indent=2)+'\n',encoding='utf-8')
    print(f'Repository PASS: {len(docs)} English documents, {len(scripts)} Python files, links/commands/layout/inventory')
if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser();parser.add_argument('--milestone',choices=('M2','M3','M4','M4.5','M5','M5.1','M6','M7','M7.1'),default='M2')
    main(parser.parse_args().milestone)
