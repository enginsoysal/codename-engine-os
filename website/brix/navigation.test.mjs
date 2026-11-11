import test from 'node:test';
import assert from 'node:assert/strict';
import {documentationGroups} from './navigation.mjs';
const post=(path,title,category='handbook')=>({slug:path,title,category,meta:[{key:'engine_source_path',value:path}]});
test('current install and M11 user tasks precede architecture and history',()=>{
 const posts=[post('docs/handbook/install-m6.md','M6 installation (historical)'),post('docs/architecture/input-m11.md','Input','architecture'),post('docs/handbook/files-m11.md','Files'),post('docs/handbook/install-m10.md','Install M10'),post('docs/handbook/index.md','Handbook'),post('docs/milestones/m11.md','M11','milestones')];
 const groups=documentationGroups(posts);
 assert.equal(groups[0].posts[0].title,'Handbook');assert.equal(groups[0].posts[1].title,'Install M10');
 assert.equal(groups[1].posts[0].title,'M11');assert.equal(groups[1].posts[1].title,'Files');
 assert.equal(groups.at(-1).posts.at(-1).title,'M6 installation (historical)');
 assert.equal(new Set(groups.flatMap(g=>g.posts.map(p=>p.slug))).size,posts.length);
});
test('numeric names sort naturally and every uncategorized article stays discoverable once',()=>{
 const posts=[post('b','Guide 10'),post('a','Guide 2'),post('c','Other','custom')];
 const groups=documentationGroups(posts);assert.deepEqual(groups[0].posts.map(p=>p.title),['Guide 2','Guide 10']);
 assert.deepEqual(groups.flatMap(g=>g.posts.map(p=>p.slug)).sort(),['a','b','c']);
});
