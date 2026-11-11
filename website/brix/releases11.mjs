// A stage becomes a download only when its required public assets exist.
export function stageRelease11(releases){
 for(const number of [6,5,4]){
  const tag=`m11${number}-v0.11.${number}`,image=`EngacerOS-M11${number===6?'FINAL':`STAGE${number}`}`;
  const release=releases.find(r=>r.tag_name===tag&&!r.draft&&!r.prerelease);
  if(!release)continue;
  const names=new Set(release.assets.map(a=>a.name));
  if(!names.has(image+'.iso')||!names.has(image+'-VMware.zip')||!names.has(`m11${number===6?'final':`stage${number}`}-build-manifest.json`))continue;
  return {id:`M11${number}`,label:`M11.${number}`,tag,image,guide:`install-m11${number}`};
 }
 return null;
}
