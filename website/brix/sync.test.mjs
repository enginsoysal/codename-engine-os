import test from 'node:test';
import assert from 'node:assert/strict';
import {assertSite,planSync} from './sync.mjs';
const post={slug:'docs-handbook-index',title:'Handbook',status:'published',bodyFormat:'brix',blocks:[{id:'a',type:'text',props:{text:'Guide'}}],meta:[{key:'engine_source_path',value:'docs/handbook/index.md'}]};
test('reject a key for any other site before writing',()=>assert.throws(()=>assertSite({slug:'another-site',title:'Codename: Engine OS'}),/different website/));
test('never overwrite a matching slug owned by the author rather than the publisher',()=>assert.throws(()=>planSync([post],[{slug:post.slug,title:'Owner content',meta:[]}]),/Unmanaged slug collision/));
test('preserve unrelated author articles',()=>assert.deepEqual(planSync([post],[post,{slug:'owner-news',status:'published',meta:[]}]),[]));
test('archive removed generated sources as drafts rather than delete them',()=>assert.deepEqual(planSync([],[post]).map(x=>x.kind),['archive']));
test('recover only the exact, empty publisher-created draft',()=>{
 const old={slug:post.slug,status:'draft',author:'Engine OS project',excerpt:'Engine OS generated draft: docs/handbook/index.md',blocks:[]};
 assert.equal(planSync([post],[old])[0].kind,'update');
 assert.throws(()=>planSync([post],[{...old,blocks:[{type:'text'}]}]),/Unmanaged slug collision/);
});
test('a mixed plan with a later collision is rejected before any operations are returned',()=>assert.throws(()=>planSync([post,{...post,slug:'second'}],[{slug:'second'}]),/Unmanaged slug collision/));
