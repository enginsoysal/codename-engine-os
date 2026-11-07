// Native Brix composition matching the owner's preferred local documentation design.
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
let sequence=0;
const site='https://engineos.brixcanvas.com';
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
 const header=b('menubrix',{brand:'E_  ENGINE OS',brandHref:'/',items:[{label:'Documentation',href:'/blog/documentation'},{label:'Downloads',href:'/blog/downloads'},{label:'GitHub ↗',href:'https://github.com/enginsoysal/codename-engine-os'},{label:'Search',href:'/search'}],mode:'inline',width:'full',shape:'square',size:'l',bg:dark,textColor:'#d5e1e8',hoverColor:'#35ceba',blur:false,border:false,shadow:false});
 header.style={bg:{color:dark}};
 const status=layout([[text('● Independent kernel. Open source.',11,'#50616c',{py:6})],[text('CODENAME · BUILT FROM SCRATCH',10,'#50616c',{align:'right',letterSpacing:'1px',py:6})]],{bg:{kind:'color',color:'#eaf0f0'},borderBottom:1,borderColor:'#dbe3e6'});
 const terminal=layout([[
   text('PROJECT SNAPSHOT / X86-64',10,'#90a6b8',{letterSpacing:'1.5px',py:10}),
   heading('CODENAME: ENGINE OS',18,'#e4edf1',{py:12}),
   text('Original kernel · BIOS + UEFI boot',13,'#35ceba'),
   text('M7 installed desktop · Custom distributions',12,'#90a6b8'),
   b('code',{code:'Welcome → Your disk → Components\nYour style → Review → Install → Finish\nDesktop · Files · Editor · HTTPS'}),
 ]],{bg:{kind:'color',color:dark},border:1,borderColor:'#35ceba',radius:4,padY:18,minH:320,valign:'center'});
 const hero=layout([[
   text('THE ENGINE OS KNOWLEDGE BASE',11,teal,{letterSpacing:'1.6px',weight:600,py:12}),
   heading('From first boot.',60,ink,{level:1,py:0,letterSpacing:'-1.8px'}),
   heading('To what comes next.',60,teal,{py:0,letterSpacing:'-1.8px'}),
   text('An independent operating system, built from scratch. Find the setup guides, explore the architecture, and download the latest bootable release.',18,muted,{py:20}),
   layout([[button('Start with the handbook →',`${site}/blog/docs-handbook-index`,{size:'sm'})],[button('Get the ISO ↓',`${site}/blog/downloads`,{variant:'outline',bg:ink,size:'sm'})]]),
 ],[terminal]],{padY:52,valign:'center'});
 hero.props.columns[0].width=55;hero.props.columns[1].width=45;
 hero.props.columns[0].children.at(-1).props.columns[0].width=60;
 hero.props.columns[0].children.at(-1).props.columns[1].width=40;
 const cards=layout([
   card('01 / FIRST BOOT','Run it in VMware','Download the ISO, choose the VM settings, and know what to expect on screen.','Open the setup guide ↗','docs-handbook-install-m7'),
   card('02 / CUSTOMIZE','Build your own distro.','Choose your own name, accent, wallpaper, dark mode and optional Calculator with a validated distribution profile.','Open the distro guide ↗','docs-handbook-distributions'),
   card('03 / RECOVERY','Repair and update','Use the graphical recovery choices to repair boot, update or roll back compatible system slots.','Open the recovery guide ↗','docs-handbook-recovery-m7'),
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
   hero,heading('A good place to start',27,ink,{py:24}),cards,
   b('alert',{variant:'warning',title:'Before you install M7',text:'Use a new SATA disk of at least 2 GiB and disable Secure Boot. Follow the graphical wizard and review the selected disk before Install. Setup creates boot/system partitions and uses the remaining aligned capacity for native data. After Finish, power off, disconnect the ISO and restart. The installed disk enters the M7 desktop. Use e1000e NAT for downloads; DOS/Windows applications remain M8.'}),bottom,
 ]],{bg:{kind:'color',color:paper},padY:12});
 const footer=b('socialcircles',{copyright:'Codename: Engine OS / MIT licensed · Documentation & releases on GitHub',bg:paper,circleBg:teal,items:[{label:'github',href:'https://github.com/enginsoysal/codename-engine-os'},{label:'website',href:`${site}/blog/documentation`}]});
 return [header,status,main,footer];
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 const out=process.argv[2]||'build/brix/design';fs.mkdirSync(out,{recursive:true});
 nativeDesign().forEach((section,index)=>fs.writeFileSync(path.join(out,`${index+1}.json`),JSON.stringify({brixcanvas:1,section})));
 console.log('Prepared four native Brix clipboard payloads for the complete homepage.');
}
