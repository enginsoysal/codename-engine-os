// Website presentation changes must not rewrite accepted release source or binaries.
export const siteOrigin='https://engaceros.brixcanvas.com';
const oldSites=new Set(['engineos.brixcanvas.com','engacer.brixcanvas.com','codenameenginos.brixcanvas.com']);

export function canonicalPublicUrl(href){
 if(!/^https?:\/\//i.test(href))return href;
 const u=new URL(href);
 if(oldSites.has(u.hostname)){
  const target=new URL(siteOrigin);u.protocol=target.protocol;u.host=target.host;
 }
 if(['github.com','raw.githubusercontent.com'].includes(u.hostname))
  u.pathname=u.pathname.replace(/^\/enginsoysal\/codename-engine-os(?=\/|$)/,'/enginsoysal/engacer-os');
 return u.href;
}

export function articleBranding(source,originalTitle){
 const title=originalTitle.replace(/\b(?:Codename:\s*)?Engine OS\b/gi,'Engacer OS');
 const match=source.match(/\/(?:[^/]*-)?m(\d+)(?:[^/]*)\.md$/i);
 const oldMilestones=new Set([0,1,2,3,4,5,6,7,8,9,45,51,71,72,73,74,75,91,92]);
 const historical=(match&&oldMilestones.has(Number(match[1])))||/^evidence\/M(?!10(?:\/|$))/i.test(source)||title!==originalTitle||['docs/handbook/console.md','docs/handbook/vmware-storage.md','docs/handbook/m5-console.md','docs/handbook/recovery.md'].includes(source);
 return {title:title+(historical?' (historical)':''),historical:Boolean(historical)};
}
