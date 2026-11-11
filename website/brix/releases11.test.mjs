import test from 'node:test';import assert from 'node:assert/strict';
import {stageRelease11} from './releases11.mjs';
const release=(n,extra={})=>({tag_name:`m11${n}-v0.11.${n}`,draft:false,prerelease:false,assets:[{name:`EngacerOS-M11STAGE${n}.iso`},{name:`EngacerOS-M11STAGE${n}-VMware.zip`},{name:`m11stage${n}-build-manifest.json`}],...extra});
test('stage downloads require all matching public assets and prefer the newest stage',()=>{
 assert.equal(stageRelease11([release(4)]).label,'M11.4');
 assert.equal(stageRelease11([release(4),release(5)]).tag,'m115-v0.11.5');
 assert.equal(stageRelease11([release(4),release(5,{draft:true})]).id,'M114');
 assert.equal(stageRelease11([release(4,{prerelease:true})]),null);
 assert.equal(stageRelease11([release(4,{assets:[]})]),null);
});
