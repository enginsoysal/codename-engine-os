import test from 'node:test';
import assert from 'node:assert/strict';
import {canonicalPublicUrl,articleBranding} from './branding.mjs';

test('old public routes move to the canonical origin without losing the destination',()=>{
 assert.equal(canonicalPublicUrl('https://engineos.brixcanvas.com/blog/downloads?q=usb#hash'),'https://engaceros.brixcanvas.com/blog/downloads?q=usb#hash');
 assert.equal(canonicalPublicUrl('http://engacer.brixcanvas.com/search'),'https://engaceros.brixcanvas.com/search');
});
test('repository links retain immutable commits, filenames, tags and anchors',()=>{
 assert.equal(canonicalPublicUrl('https://github.com/enginsoysal/codename-engine-os/releases/download/m6-v0.6.0/EngineOS-M6.iso'),'https://github.com/enginsoysal/engacer-os/releases/download/m6-v0.6.0/EngineOS-M6.iso');
 assert.equal(canonicalPublicUrl('https://raw.githubusercontent.com/enginsoysal/codename-engine-os/abc/README.md#old'),'https://raw.githubusercontent.com/enginsoysal/engacer-os/abc/README.md#old');
 assert.equal(canonicalPublicUrl('https://github.com/enginsoysal/codename-engine-os-extra'),'https://github.com/enginsoysal/codename-engine-os-extra');
 assert.equal(canonicalPublicUrl('https://example.com/engineos.brixcanvas.com'),'https://example.com/engineos.brixcanvas.com');
});
test('old guide titles use the new brand and clearly identify their historical release',()=>{
 assert.deepEqual(articleBranding('docs/handbook/install-m7.md','Install Engine OS M7 in VMware'),{title:'Install Engacer OS M7 in VMware (historical)',historical:true});
 assert.equal(articleBranding('docs/releases/m92-v0.9.2.md','Engine OS 0.9.2').historical,true);
 assert.equal(articleBranding('evidence/M9.2/README.md','M9.2 proof').historical,true);
 assert.deepEqual(articleBranding('docs/handbook/install-m10.md','Install Engacer OS M10'),{title:'Install Engacer OS M10',historical:false});
 assert.equal(articleBranding('README.md','Engacer OS').historical,false);
});
