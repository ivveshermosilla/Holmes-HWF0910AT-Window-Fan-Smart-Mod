const fs=require("node:fs");
const vm=require("node:vm");
const assert=require("node:assert/strict");
const path=require("node:path");
const html=fs.readFileSync(path.join(__dirname,"../firmware/Holmes_HWF0910AT_Smart_Mod/data/index.html"),"utf8");
const script=html.match(/<script>([\s\S]*?)<\/script>/)[1];
new vm.Script(script);
function extract(name){
  const start=script.indexOf("  function "+name+"(");
  const end=script.indexOf("\n  function ",start+1);
  assert(start>=0&&end>start);
  return script.slice(start,end);
}
let offline=false,uptime=1000;
const posts=[];
const context=vm.createContext({
  Date,Promise,
  apiGet:async()=>{if(offline) throw new Error("offline");return {uptimeMs:uptime,time:{synced:true}}},
  apiPost:async(route,body)=>{posts.push({route,body});return {ok:true}},
  applyStatus:()=>{},$:()=>({textContent:""}),tr:x=>x,setDot:()=>{}
});
vm.runInContext("let refreshBusy=false,clockConnectionObserved=false,clockDeviceUptime=0;\n"+extract("refreshStatus")+extract("syncTime"),context);
(async()=>{
  await context.refreshStatus();
  assert.equal(posts.length,1);
  assert.equal(posts[0].body.reason,"app-reconnect");
  await context.refreshStatus();
  assert.equal(posts.length,1);
  offline=true;
  await context.refreshStatus();
  offline=false;uptime=2000;
  await context.refreshStatus();
  assert.equal(posts.length,2);
  uptime=100;
  await context.refreshStatus();
  assert.equal(posts.length,3);
  await context.syncTime();
  assert.equal(posts.at(-1).body.reason,"manual");
  assert(posts.every(p=>p.route==="/api/time"));
  console.log("PWA clock: initial connection, outage recovery, ESP reboot, manual sync and time-only requests passed");
})().catch(err=>{console.error(err);process.exitCode=1});
