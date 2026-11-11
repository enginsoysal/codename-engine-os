// Native Brix composition matching the owner's preferred local documentation design.
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const progress=JSON.parse(fs.readFileSync(new URL('../../docs/milestones/m11-progress.json',import.meta.url),'utf8'));
let sequence=0;
const site='https://engaceros.brixcanvas.com';
const ink='#152434',muted='#5a6976',teal='#087c71',dark='#101e2c',paper='#f7f9f9';
const b=(type,props)=>({id:`engine-home-${++sequence}`,type,props});
const text=(value,size=16,color=muted,more={})=>b('text',{text:value,size,color,padX:0,py:10,lineHeight:1.65,...more});
const heading=(value,size=26,color=ink,more={})=>b('heading',{text:value,size,color,level:2,weight:600,padX:0,py:12,fluid:true,...more});
const button=(label,href,more={})=>b('button',{label,href,bg:teal,color:'#ffffff',hoverBg:'#065f56',radius:'none',size:'sm',align:'left',padX:0,py:12,...more});
const layout=(columns,more={})=>b('layout',{columns:columns.map(children=>({children})),contentWidth:1280,padY:0,...more});
const card=(n,title,description,label,slug)=>[
 text(n,11,teal,{letterSpacing:'1.2px',weight:600}),heading(title,21,ink,{level:3}),text(description,14),
 button(label,`${site}/blog/${slug}`,{bg:'#ffffff',color:teal,hoverBg:'#edf5f2',size:'sm'}),
];
export function nativeDesign(){
 sequence=0;
 const header=b('menubrix',{brand:'E_  ENGACER OS',brandHref:'/',items:[{label:'Documentation',href:'/blog/documentation'},{label:'Downloads',href:'/blog/downloads'},{label:'GitHub ↗',href:'https://github.com/enginsoysal/engacer-os'},{label:'Search',href:'/search'}],mode:'inline',width:'full',shape:'square',size:'l',bg:dark,textColor:'#d5e1e8',hoverColor:'#35ceba',blur:false,border:false,shadow:false});
 header.style={bg:{color:dark}};
 const status=layout([[text('● Independent kernel. Open source.',11,'#50616c',{py:6})],[text('INDEPENDENT · BUILT FROM SCRATCH',10,'#50616c',{align:'right',letterSpacing:'1px',py:6})]],{bg:{kind:'color',color:'#eaf0f0'},borderBottom:1,borderColor:'#dbe3e6'});
 const terminal=layout([[
   text('PROJECT SNAPSHOT / X86-64',10,'#90a6b8',{letterSpacing:'1.5px',py:10}),
   heading('ENGACER OS',18,'#e4edf1',{py:12}),
   text('Original kernel · BIOS + UEFI boot',13,'#35ceba'),
   text('Tested download: '+progress.qualified_download+' · Devices · USB files',12,'#90a6b8'),
   text(progress.current_stage+' · '+progress.stages.find(s=>s.id===progress.current_stage).title,13,'#35ceba'),
   text('Completed stages: '+progress.stages.filter(s=>s.status==='complete').map(s=>s.id).join(', '),12,'#90a6b8'),
   b('code',{code:'Next → Review → Install → Restart\nIDE / SATA / supported SCSI / NVMe\nDevices · USB file copies · Safe removal'}),
 ]],{bg:{kind:'color',color:dark},border:1,borderColor:'#35ceba',radius:4,padY:18,minH:320,valign:'center'});
 const hero=layout([[
   text('THE ENGACER OS KNOWLEDGE BASE',11,teal,{letterSpacing:'1.6px',weight:600,py:12}),
   heading('The M11 desktop',52,ink,{level:1,py:0,letterSpacing:'-1.8px'}),
   heading('is taking shape.',52,teal,{py:0,letterSpacing:'-1.8px'}),
   text(progress.current_stage+' completed: '+progress.stages.find(s=>s.id===progress.current_stage).title+'. Follow the tested work stage by stage. The final M11 ISO follows M11.6 qualification; the current tested download is '+progress.qualified_download+'.',18,muted,{py:20}),
   layout([[button('See M11 progress →',`${site}/blog/docs-milestones-m11`,{size:'sm'})],[button('Get the ISO ↓',`${site}/blog/downloads`,{variant:'outline',bg:ink,size:'sm'})]]),
 ],[terminal]],{padY:52,valign:'center'});
 hero.props.columns[0].width=55;hero.props.columns[1].width=45;
 hero.props.columns[0].children.at(-1).props.columns[0].width=60;
 hero.props.columns[0].children.at(-1).props.columns[1].width=40;
 const cards=layout([
   card('01 / CURRENT DEVELOPMENT','Start, made yours.','Local search, Pinned, All apps, editable categories, folder shortcuts and a separate taskbar window group. Real restart and scoped virtual shutdown include safety reviews.','Read the Start guide ↗','docs-handbook-start-m11'),
   card('02 / TESTED DOWNLOAD','Install in VMware','The current qualified download is '+progress.qualified_download+'. Use its prepared VMware ZIP and matching installation guide while the M11 stages are completed.','Open the setup guide ↗','docs-handbook-install-m10'),
   card('03 / MAKE IT YOURS','Build your own distro.','Choose your accent, wallpaper, window style and default content with a validated distribution profile.','Open the distro guide ↗','docs-handbook-distributions'),
 ],{bg:{kind:'color',color:'#ffffff'},border:1,borderColor:'#dbe3e6',divided:true,ruleColor:'#dbe3e6',radius:4,padY:24});
 cards.props.columns.forEach(column=>{column.width=100/3;});
 const bottom=layout([[
   text('BUILT IN THE OPEN',11,teal,{letterSpacing:'1.5px'}),heading('Understand the decisions behind the OS.',19),
   text('Explore the kernel contracts, memory model, filesystem layout and compatibility roadmap. Every milestone links implementation claims to evidence.',14),
   button('Explore the architecture →',`${site}/blog/docs-architecture-overview`,{bg:paper,color:teal,hoverBg:'#edf5f2'}),
 ],[
   text('ONE SOURCE OF TRUTH',11,teal,{letterSpacing:'1.5px'}),heading('Documentation that follows the code.',19),
   text('Guides identify their canonical GitHub source commit. Official releases supply original download files and complete SHA256 checksums.',14),
   button('View the milestone plan →',`${site}/blog/docs-roadmap-milestones`,{bg:paper,color:teal,hoverBg:'#edf5f2'}),
 ]],{padY:32,borderBottom:1,borderColor:'#dbe3e6'});
 bottom.props.columns.forEach(column=>{column.width=50;});
 const main=layout([[
   hero,heading('From the real M11.3 VM',27,ink,{py:18}),b('image',{url:'https://raw.githubusercontent.com/enginsoysal/engacer-os/main/evidence/M11/3/native/start-pinned.png',alt:'Native Engacer OS M11.3 Start: local search, catalogue and real app/folder pins. Actual VM capture, not a design concept.',naturalSize:true,radius:8}),heading('A good place to start',27,ink,{py:24}),cards,
   b('alert',{variant:'info',title:'Development progress and tested downloads',text:progress.stages.filter(s=>s.status==='complete').map(s=>s.id).join(', ')+' are completed stages, with native test evidence and English guides. The remaining stages follow in order before final M11 VMware media. '+progress.qualified_download+' is the existing qualified download. Qualification covers named virtual profiles; physical hardware, 3D and general Windows compatibility remain unqualified.'}),bottom,
 ]],{bg:{kind:'color',color:paper},padY:12});
 const footer=b('socialcircles',{copyright:'Engacer OS / MIT licensed · Documentation & releases on GitHub',bg:paper,circleBg:teal,items:[{label:'github',href:'https://github.com/enginsoysal/engacer-os'},{label:'website',href:`${site}/blog/documentation`}]});
 return [header,status,main,footer];
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 const out=process.argv[2]||'build/brix/design';fs.mkdirSync(out,{recursive:true});
 nativeDesign().forEach((section,index)=>fs.writeFileSync(path.join(out,`${index+1}.json`),JSON.stringify({brixcanvas:1,section})));
 console.log('Prepared four native Brix clipboard payloads for the complete homepage.');
}
