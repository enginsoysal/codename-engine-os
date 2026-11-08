// Convert canonical Git documentation into ordinary editable BrixCanvas articles.
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import {marked} from '../public/vendor/marked.esm.js';

const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const git=(...args)=>execFileSync('git',args,{cwd:root,encoding:'utf8',maxBuffer:8*1024*1024});
export const slugFor=p=>'docs-'+p.replace(/\.md$/i,'').replace(/^docs\//,'').toLowerCase().replace(/[^a-z0-9]+/g,'-').replace(/^-|-$/g,'');
const GH='https://github.com/enginsoysal/codename-engine-os';
const SITE='https://engineos.brixcanvas.com';
const escape=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const categoryFor=p=>p.startsWith('docs/')?p.split('/')[1]:p.startsWith('evidence/')?'evidence':p.startsWith('tools/')?'developer':p.startsWith('website/')?'website':'project';
const categoryNames={handbook:'User handbook',architecture:'Architecture',milestones:'Milestones',releases:'Release guides',roadmap:'Roadmap & status',evidence:'Test evidence',developer:'Developer guides',website:'Website maintenance',project:'Project guides',downloads:'Downloads'};

export function build(output){
 const ref=git('rev-parse','HEAD').trim();
 const paths=git('ls-tree','-r','--name-only',ref).trim().split('\n').filter(p=>p.endsWith('.md'));
 const pathsSet=new Set(paths),slugs=new Set(),posts=[];
 const titleFor=(p,text)=>text.match(/^#\s+(.+)$/m)?.[1]||p.split('/').pop();
 const link=(href,source,image=false)=>{
   if(/^https?:\/\//i.test(href)||/^mailto:/i.test(href))return href;
   if(href.startsWith('//')||/^[a-z][a-z0-9+.-]*:/i.test(href))return '#';
   const u=new URL(href,`https://source.local/${source}`),p=decodeURIComponent(u.pathname.slice(1));
   if(image)return `https://raw.githubusercontent.com/enginsoysal/codename-engine-os/${ref}/${p}`;
   // Native heading Brix do not supply GitHub heading anchors; preserve exact anchor navigation at the source.
   return pathsSet.has(p)&&!u.hash?`${SITE}/blog/${slugFor(p)}`:`${GH}/blob/${ref}/${p}${u.hash}`;
 };
 for(const p of paths){
   const text=git('show',`${ref}:${p}`),title=titleFor(p,text),slug=slugFor(p);
   if(slugs.has(slug))throw Error(`Documentation slug collision: ${slug}`);slugs.add(slug);
   const tokens=marked.lexer(text);
   marked.walkTokens(tokens,t=>{if(t.type==='link'||t.type==='image')t.href=link(t.href,p,t.type==='image')});
   const blocks=[];
   const add=(type,props)=>blocks.push({id:`${slug}-${blocks.length+1}`,type,props});
   add('richtext',{html:`<p><strong>Canonical source:</strong> <a href="${GH}/blob/${ref}/${p}">${escape(p)}</a> · commit ${ref.slice(0,7)} · <a href="${SITE}/blog/documentation">All guides</a></p>`,size:14});
   for(const token of tokens){
     if(token.type==='space'||token.type==='def')continue;
     if(token.type==='heading'){
       if(token.depth===1&&blocks.length===1)continue;
       add('heading',{text:token.text.replace(/[*`]/g,''),level:Math.max(2,token.depth),size:token.depth<=2?30:23,color:'#152434',weight:700,padX:0,py:14});
     }else if(token.type==='code')add('code',{code:token.text,language:token.lang||'text'});
     else if(token.type==='hr')add('divider',{});
     else if(token.type==='paragraph'&&token.tokens?.length===1&&token.tokens[0].type==='image'){
       const t=token.tokens[0];add('image',{url:t.href,alt:t.text,naturalSize:true,radius:6});
     }else add('richtext',{html:marked.parser([token]),size:17});
   }
   const post={id:slug,slug,title,status:'published',bodyFormat:'brix',blocks,excerpt:`${categoryNames[categoryFor(p)]} — ${title}. Synced from the canonical Engine OS repository.`,category:categoryFor(p),tags:['Engine OS',categoryNames[categoryFor(p)]],author:'Engine OS project',date:new Date().toISOString().slice(0,10),featured:p==='docs/handbook/index.md',meta:[{key:'engine_source_path',value:p},{key:'engine_source_commit',value:ref}]};
   if(Buffer.byteLength(JSON.stringify(post))>240*1024)throw Error(`Article exceeds the bounded API payload: ${p}`);
   posts.push(post);
 }
 const releases=JSON.parse(execFileSync('gh',['api','repos/enginsoysal/codename-engine-os/releases?per_page=100'],{cwd:root,encoding:'utf8',maxBuffer:4*1024*1024})).filter(r=>!r.draft);
 const downloads=[];const add=(type,props)=>downloads.push({id:`engine-downloads-${downloads.length+1}`,type,props});
 const current=releases.some(r=>r.tag_name==='m75-v0.7.5')?'M75':releases.some(r=>r.tag_name==='m74-v0.7.4')?'M74':releases.some(r=>r.tag_name==='m73-v0.7.3')?'M73':releases.some(r=>r.tag_name==='m72-v0.7.2')?'M72':releases.some(r=>r.tag_name==='m71-v0.7.1')?'M71':releases.some(r=>r.tag_name==='m7-v0.7.0')?'M7':'M6';
 const currentLabel=current==='M75'?'M7.5':current==='M74'?'M7.4':current==='M73'?'M7.3':current==='M72'?'M7.2':current==='M71'?'M7.1':current;
 const tag=current==='M75'?'m75-v0.7.5':current==='M74'?'m74-v0.7.4':current==='M73'?'m73-v0.7.3':current==='M72'?'m72-v0.7.2':current==='M71'?'m71-v0.7.1':current==='M7'?'m7-v0.7.0':'m6-v0.6.0';
 const guide=current==='M75'?'install-m75':current==='M74'?'install-m74':current==='M73'?'install-m73':current==='M72'?'install-m72':current==='M71'?'install-m71':current==='M7'?'install-m7':'install-m6';
 add('text',{text:`Download original Engine OS release files directly from GitHub. Check the complete SHA256 value before booting. ${currentLabel} provides the current graphical installer${current!=='M6'?' and installed desktop':''}; use a separate test VM, a SATA test target and disable Secure Boot for public unsigned media. Earlier releases remain historical.`,size:17,color:'#152434'});
 if(releases.some(r=>r.tag_name===tag))add('richtext',{html:`<p><strong>Start here: <a href="${GH}/releases/download/${tag}/EngineOS-${current}.iso">Download the standard Engine OS ${currentLabel} ISO</a></strong></p><p>Use the standard ISO for ordinary VMware testing; NIGHT/MINIMAL/U1 images are separate distribution or update examples. Download the accompanying third-party notices. <a href="${SITE}/blog/docs-handbook-${guide}">Follow the installation tutorial</a>.</p>`,size:17});
   if(['M72','M73','M74','M75'].includes(current))add('richtext',{html:`<p><strong><a href="${GH}/releases/download/${tag}/EngineOS-${current}-VMware.zip">Easiest start: download the prepared VMware ZIP</a></strong>. Extract it all, open EngineOS.vmx, follow Next and Install, then choose ${current==='M75'?'Restart to Engine OS':'Open desktop'}. A new blank growable SATA disk is included; networking is unnecessary.</p>`,size:17});
add('richtext',{html:`<p><a href="${GH}/releases">Official release history</a> · <a href="${SITE}/blog/docs-releases-${tag.replaceAll('.','-')}">VMware setup and verification</a> · <a href="${SITE}/blog/docs-handbook-${guide}">Missing disk?</a></p>`,size:17});
 let stable=false;
 for(const r of releases){
   const label=r.prerelease?'Historical preview':stable?'Earlier release':'Latest stable';if(!r.prerelease)stable=true;
   add('heading',{text:`${r.name||r.tag_name} — ${label}`,level:2,size:27,color:'#152434'});
   add('richtext',{html:`<p>Published ${escape(r.published_at?.slice(0,10)||'unknown')} · <a href="${escape(r.html_url)}">Official release notes</a></p>`,size:15});
   const assets=[...r.assets].sort((a,b)=>{const rank=n=>/^EngineOS-(?:M6(?:U1|MINIMAL)|M7NIGHT|M71NIGHT|M74MINIMAL)\.iso$/i.test(n)?4:/\.iso$/i.test(n)?0:/\.img$/i.test(n)?1:/sha256/i.test(n)?2:3;return rank(a.name)-rank(b.name)});
   for(const a of assets)add('richtext',{html:`<p><a href="${escape(a.browser_download_url)}"><strong>Download ${escape(a.name)}</strong></a> — ${(a.size/1048576).toFixed(2)} MiB</p>${a.digest?`<pre><code>${escape(a.digest)}</code></pre>`:''}`,size:16});
 }
 posts.push({id:'engine-downloads',slug:'downloads',title:'Downloads and SHA256 verification',status:'published',bodyFormat:'brix',blocks:downloads,category:'downloads',tags:['Downloads','Releases'],author:'Engine OS project',date:new Date().toISOString().slice(0,10),featured:true,meta:[{key:'engine_source_path',value:'@releases'},{key:'engine_source_commit',value:ref}]});
 const index=[];const push=(type,props)=>index.push({id:`engine-docindex-${index.length+1}`,type,props});
 push('text',{text:'English manuals, architecture, release guides and evidence from the canonical Engine OS GitHub repository. Each guide is an ordinary editable BrixCanvas article. Downloads are original GitHub release assets.',size:17,color:'#152434'});
 push('richtext',{html:`<p><a href="${SITE}/blog/docs-handbook-index">Start with the handbook</a> · <a href="${SITE}/blog/downloads">Downloads &amp; checksums</a> · <a href="${SITE}/blog/docs-roadmap-status">Current capabilities and limitations</a></p>`,size:17});
 for(const [key,name] of Object.entries(categoryNames)){
   const matching=posts.filter(p=>p.category===key);if(!matching.length)continue;
   push('heading',{text:name,level:2,size:27,color:'#152434'});
   push('richtext',{html:'<ul>'+matching.map(p=>`<li><a href="${SITE}/blog/${p.slug}">${escape(p.title)}</a></li>`).join('')+'</ul>',size:17});
 }
 posts.push({id:'engine-documentation',slug:'documentation',title:'Documentation — all guides',status:'published',bodyFormat:'brix',blocks:index,category:'handbook',tags:['Handbook','Index'],author:'Engine OS project',date:new Date().toISOString().slice(0,10),featured:true,meta:[{key:'engine_source_path',value:'@index'},{key:'engine_source_commit',value:ref}]});
 const result={version:1,source_commit:ref,generated:new Date().toISOString(),site_id:'b66d0e83-7987-4856-b702-e8d93234b6cd',site_slug:'codename-engine-os-ltvck',page:{blog:{posts,categories:Object.entries(categoryNames).map(([handle,title])=>({handle,title}))}}};
 const out=output||path.join(root,'build/brix/site-content.json');fs.mkdirSync(path.dirname(out),{recursive:true});fs.writeFileSync(out,JSON.stringify(result));
 console.log(JSON.stringify({documents:paths.length,articles:posts.length,releases:releases.length,blocks:posts.reduce((n,p)=>n+p.blocks.length,0),source_commit:ref,output:path.relative(root,out).replaceAll('\\','/')}));
 return result;
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url))build(process.argv[2]);
