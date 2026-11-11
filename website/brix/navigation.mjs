// Reader-first order: current tasks precede internals and historical releases.
const source=p=>p.meta?.find(m=>m.key==='engine_source_path')?.value||'';
const compare=new Intl.Collator('en',{numeric:true,sensitivity:'base'}).compare;
export function documentationGroups(posts){
 const remaining=new Map(posts.map(p=>[p.slug,p]));const groups=[];
 const add=(title,description,items)=>{const unique=items.filter(p=>p&&remaining.delete(p.slug));if(unique.length)groups.push({title,description,posts:unique});};
 const find=path=>posts.find(p=>source(p)===path);
 add('1. Start here','Install the tested release, understand what works and find downloads.',[
  find('docs/handbook/index.md'),find('docs/handbook/install-m10.md'),find('docs/roadmap/status.md'),posts.find(p=>p.slug==='downloads'),find('docs/releases/m10-v0.10.0.md')]);
 add('2. M11 desktop and installation refresh','Current development, in stage order. The final M11 download follows qualification.',[
  find('docs/milestones/m11.md'),find('docs/handbook/desktop-install-m11.md'),find('docs/handbook/files-m11.md'),find('docs/architecture/desktop-m11.md'),find('docs/architecture/input-m11.md'),find('docs/architecture/files-m11.md')]);
 const sorted=()=>[...remaining.values()].sort((a,b)=>compare(a.title,b.title));
 const historical=p=>/\(historical\)$/.test(p.title)||source(p).startsWith('docs/roadmap/history/')||p.category==='releases';
 add('3. Everyday use and customization','Current manuals for files, devices, distributions and ordinary tasks.',sorted().filter(p=>p.category==='handbook'&&!historical(p)));
 add('4. Architecture and development','Technical reference, source builds and design decisions.',sorted().filter(p=>!['handbook','roadmap','milestones','evidence','website','downloads','releases'].includes(p.category)&&!historical(p)));
 add('5. Roadmap and verification','Current plans and test evidence; qualification is always scoped.',sorted().filter(p=>['roadmap','milestones','evidence'].includes(p.category)&&!historical(p)));
 add('6. Website maintenance','How the English website follows canonical GitHub documentation.',sorted().filter(p=>p.category==='website'));
 add('7. Historical releases and earlier milestones','Archive material. Use the current installation guide above for the tested download.',sorted());
 return groups;
}
