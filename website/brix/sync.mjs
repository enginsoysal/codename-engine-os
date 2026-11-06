// Publish generated native articles through the existing, site-scoped BrixCanvas API.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {build} from './build.mjs';

const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const apiBase='https://brixcanvas.com/api/v1';
const slug='codename-engine-os-ltvck';
const fields=['title','status','excerpt','bodyFormat','blocks','category','tags','author','date','featured','meta'];
const canonical=value=>JSON.stringify(value,(_key,v)=>v&&typeof v==='object'&&!Array.isArray(v)?Object.fromEntries(Object.entries(v).sort(([a],[b])=>a.localeCompare(b))):v);
const select=p=>Object.fromEntries(fields.map(key=>[key,p[key]]));
const source=p=>p.meta?.find(m=>m.key==='engine_source_path')?.value;
const pending=p=>`Engine OS generated draft: ${source(p)}`;

export function assertSite(settings){
 assert.equal(settings?.slug,slug,'The API key belongs to a different website; nothing was changed.');
 assert.equal(settings?.title,'Codename: Engine OS','Unexpected website identity; nothing was changed.');
}

export function planSync(desired,current){
 const bySlug=new Map(),operations=[],managed=new Set(desired.map(source));
 for(const p of current){assert(!bySlug.has(p.slug),'Duplicate existing article slug');bySlug.set(p.slug,p);}
 const wanted=new Set();
 for(const p of desired){
   assert(/^[a-z0-9]+(?:-[a-z0-9]+)*$/.test(p.slug),'Invalid generated article slug');
   assert(source(p),'Generated article has no source marker');
   assert(!wanted.has(p.slug),'Duplicate generated article slug');wanted.add(p.slug);
   const old=bySlug.get(p.slug);
   if(!old){operations.push({kind:'create',post:p});continue;}
   const recovery=old.status==='draft'&&old.excerpt===pending(p)&&old.author==='Engine OS project'&&!old.content&&!old.blocks?.length;
   assert(source(old)===source(p)||recovery,`Unmanaged slug collision at ${p.slug}; nothing was changed.`);
   if(canonical(select(old))!==canonical(select(p)))operations.push({kind:'update',post:p});
 }
 for(const p of current){
   const marker=source(p);
   if(typeof marker==='string'&&marker.endsWith('.md')&&!managed.has(marker)&&p.status==='published')operations.push({kind:'archive',post:p});
 }
 return operations;
}

async function publish(){
 const payload=build();
 const desired=payload.page.blog.posts;
 assert.equal(payload.site_id,'b66d0e83-7987-4856-b702-e8d93234b6cd');
 if(process.argv.includes('--dry-run')){
   const plan=planSync(desired,[]);
   console.log(JSON.stringify({dry_run:true,articles:desired.length,planned:plan.length,source_commit:payload.source_commit}));return;
 }
 const key=process.env.BRIXCANVAS_ENGINEOS_KEY;
 assert(/^bx_live_[A-Za-z0-9_-]+$/.test(key||''),'Configure the site-scoped BrixCanvas publishing secret first.');
 let lastRequest=0;
 async function api(route,method='GET',body){
   const pause=Math.max(0,1050-(Date.now()-lastRequest));if(pause)await new Promise(resolve=>setTimeout(resolve,pause));
   lastRequest=Date.now();
   const response=await fetch(apiBase+route,{method,redirect:'error',signal:AbortSignal.timeout(30000),headers:{Authorization:`Bearer ${key}`,...(body?{'Content-Type':'application/json'}:{})},...(body?{body:JSON.stringify(body)}:{})});
   if(!response.ok)throw Error(`BrixCanvas ${method} ${route.split('?')[0]} returned ${response.status}; publication stopped.`);
   return response.json();
 }
 async function inventory(){
   const posts=[];for(let offset=0;;offset+=100){const r=await api(`/posts?limit=100&offset=${offset}`);assert(Array.isArray(r.data),'Invalid article inventory');posts.push(...r.data);if(posts.length>=r.total||r.data.length<100)break;assert(offset<20000,'Article inventory exceeds the bounded publisher');}return posts;
 }
 assertSite((await api('/settings')).data);
 const initial=await inventory();
 const operations=planSync(desired,initial); // Validate all collisions before the first write.
 const result={created:0,updated:0,archived:0};
 for(const operation of operations){
   const p=operation.post,route=`/posts/${p.slug}`;
   if(operation.kind==='create'){
     await api('/posts','POST',{title:p.title,slug:p.slug,status:'draft',excerpt:pending(p),author:'Engine OS project',content:''});
     await api(route,'PATCH',{...select(p),content:''});result.created++;
   }else if(operation.kind==='update'){
     await api(route,'PATCH',{...select(p),content:''});result.updated++;
   }else{
     await api(route,'PATCH',{status:'draft',excerpt:'Archived: the canonical source document was removed. Existing article content is retained.'});result.archived++;
   }
 }
 const final=await inventory(),bySlug=new Map(final.map(p=>[p.slug,p]));
 for(const p of desired){assert.equal(canonical(select(bySlug.get(p.slug))),canonical(select(p)),`Verification failed for ${p.slug}`);}
 for(const operation of operations.filter(o=>o.kind==='archive'))assert.equal(bySlug.get(operation.post.slug)?.status,'draft');
 const report={passed:true,checked_at:new Date().toISOString(),website:'https://engineos.brixcanvas.com',source_commit:payload.source_commit,articles:desired.length,...result,unrelated_articles_preserved:initial.filter(p=>!source(p)).length};
 const out=path.join(root,'build/brix/sync-results.json');fs.writeFileSync(out,JSON.stringify(report,null,2)+'\n');
 console.log(JSON.stringify(report));
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url))publish().catch(error=>{console.error(error.message);process.exitCode=1;});
