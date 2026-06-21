#pragma once
#include <Arduino.h>

// Captive-portal setup wizard, served from PROGMEM in AP mode.
static const char FL_SETUP_HTML[] PROGMEM = R"=====(<!DOCTYPE html>
<html lang="en"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="light dark">
<title>FireLabs S31 setup</title>
<style>
:root{color-scheme:light dark;--red:#FA2800;--orange:#FFA01E;--grad:linear-gradient(135deg,#FA2800,#FF7A12 55%,#FFA01E);
--ink:#1B1613;--muted:#90847b;--card:#fff;--line:#efe4d9;--ls:#e4d6c8;--page:#FBF6F1;--inp:#fffdfb;--good:#28a86a;--goodbg:#e7f6ee;--addr:#1b1613;--addrfg:#ffd9a8}
@media(prefers-color-scheme:dark){:root{--ink:#f3ebe3;--muted:#9c8e82;--card:#1d1916;--line:#2b2521;--ls:#39312b;--page:#141110;--inp:#171310;--good:#36d484;--goodbg:rgba(54,212,132,.14);--addr:#0e0c0a}}
*{box-sizing:border-box}html,body{overflow-x:hidden}body{margin:0;overflow-wrap:anywhere;background:var(--page);color:var(--ink);font-family:"Quicksand",system-ui,Segoe UI,sans-serif;-webkit-font-smoothing:antialiased}
.mono{font-family:"JetBrains Mono",ui-monospace,monospace}
.wrap{max-width:430px;margin:34px auto 60px;padding:0 18px}
.lock{text-align:center;margin-bottom:6px}.lock img{width:120px}.lock .wm{font-weight:700;font-size:28px;margin-top:2px}
.step{display:flex;justify-content:center;gap:7px;margin:14px 0 20px}.step i{width:8px;height:8px;border-radius:50%;background:var(--ls)}.step i.on{background:var(--grad);width:22px}
.s{display:none}.s.on{display:block;animation:f .25s ease}@keyframes f{from{opacity:0;transform:translateY(4px)}to{opacity:1}}
h1{font-size:21px;text-align:center;margin:0}.sub{text-align:center;color:var(--muted);font-size:13.5px;margin:6px 0 18px}
.card{background:var(--card);border:1px solid var(--line);border-radius:16px;box-shadow:0 8px 24px -12px rgba(0,0,0,.18)}.bd{padding:16px 18px}
label{display:block;margin-top:14px;font-size:12.5px;font-weight:600;color:var(--muted)}
input{width:100%;margin-top:6px;font-family:inherit;font-size:14.5px;color:var(--ink);padding:11px 12px;border:1px solid var(--ls);border-radius:11px;background:var(--inp)}
input:focus{outline:none;border-color:var(--orange);box-shadow:0 0 0 3px rgba(255,160,30,.2)}
.help{margin-top:6px;font-size:12px;color:var(--muted)}.host{color:var(--red);font-weight:500}
.btn{font-family:inherit;font-weight:700;font-size:14px;border:0;border-radius:11px;padding:12px 18px;cursor:pointer}
.btn.p{background:var(--grad);color:#fff;box-shadow:0 6px 16px -7px rgba(250,40,0,.7)}.btn.g{background:var(--card);color:var(--ink);border:1px solid var(--ls)}.blk{width:100%}
.nav{display:flex;gap:10px;margin-top:18px}.nav .btn{flex:1}
.wifi{display:flex;align-items:center;gap:12px;padding:13px 14px;border:1px solid var(--line);border-radius:12px;margin-top:9px;cursor:pointer;background:var(--card)}
.wifi.sel{border-color:var(--orange);box-shadow:0 0 0 3px rgba(255,160,30,.18)}.wifi .nm{flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;font-weight:600;font-size:14px}.wifi .lk{flex:none;font-size:11px;color:var(--muted)}
.done{width:64px;height:64px;border-radius:50%;background:var(--goodbg);color:var(--good);display:grid;place-items:center;font-size:30px;margin:0 auto 14px}
.addr{font-family:"JetBrains Mono",monospace;background:var(--addr);color:var(--addrfg);padding:11px 13px;border-radius:10px;text-align:center;font-size:13.5px;margin:12px 0;word-break:break-all}
.foot{text-align:center;color:var(--muted);opacity:.7;font-size:11.5px;margin-top:26px}
</style></head><body>
<div class="wrap">
<div class="lock"><img src="/logo.png" alt="FireLabs"><div class="wm">FireLabs</div></div>
<div class="step"><i class="on"></i><i></i><i></i><i></i></div>

<div class="s on" id="s0">
<h1>Let's set up your plug</h1><div class="sub">Three quick steps and it's on your network.</div>
<div class="card"><div class="bd" style="text-align:center;color:var(--muted);font-size:13px">Nothing's saved yet. Pick your wifi and give the plug a name.</div></div>
<div class="nav"><button class="btn p blk" onclick="go(1)">Start</button></div></div>

<div class="s" id="s1">
<h1>Choose your WiFi</h1><div class="sub">Pick the 2.4 GHz network to join.</div>
<div class="card"><div class="bd"><div id="nets" style="font-size:13px;color:var(--muted)">Loading networks…</div>
<label>Or type the network name<input type="text" id="manual" placeholder="SSID" oninput="sel=this.value"></label>
<label>Password<input type="password" id="pass" placeholder="WiFi password"></label></div></div>
<div class="nav"><button class="btn g" onclick="go(0)">Back</button><button class="btn p" onclick="go(2)">Next</button></div></div>

<div class="s" id="s2">
<h1>Name this plug</h1><div class="sub">So you can find it and label it in Home Assistant.</div>
<div class="card"><div class="bd"><label>Device name<input type="text" id="name" value="Living Room" oninput="hp()"></label>
<div class="help">Find it at <span class="host" id="hpv">fl-living-room</span>.local</div></div></div>
<div class="nav"><button class="btn g" onclick="go(1)">Back</button><button class="btn p" onclick="save()">Save &amp; connect</button></div></div>

<div class="s" id="s3">
<div class="done">✓</div><h1>Connecting…</h1>
<div class="sub">The plug is joining <b id="jn">your network</b> and will reboot. This setup network will drop in a moment.</div>
<div class="addr" id="fa">http://fl-living-room.local</div>
<div class="card"><div class="bd" style="font-size:13px;color:var(--muted)">Open that address once it's back, or look for <span class="mono" id="fh">fl-living-room</span> in your router. If it can't connect, it reopens this setup network.</div></div></div>

<form method="POST" action="/update" enctype="multipart/form-data" style="margin-top:22px;text-align:center">
<div style="font-size:11.5px;color:var(--muted);margin-bottom:8px">Firmware recovery</div>
<input type="file" name="firmware" accept=".bin" style="width:auto;font-size:12px">
<button class="btn g" style="margin-top:10px;padding:9px 16px;font-size:13px">Upload .bin</button>
</form>
<div class="foot">FireLabs S31 · first-time setup</div>
</div>
<script>
var sel="";
function go(n){document.querySelectorAll('.s').forEach(s=>s.classList.remove('on'));document.getElementById('s'+n).classList.add('on');
document.querySelectorAll('.step i').forEach((d,i)=>d.classList.toggle('on',i<=n));scrollTo(0,0);if(n==1)scan();}
function host(v){return 'fl-'+(v.toLowerCase().replace(/[^a-z0-9]+/g,'-').replace(/^-|-$/g,'')||'s31');}
function hp(){document.getElementById('hpv').textContent=host(document.getElementById('name').value);}
function scan(){fetch('/api/scan').then(r=>r.json()).then(function(l){
var h='';if(!l.length){h='No networks found. <a href="#" onclick="scan();return false" style="color:var(--red)">retry</a>';}
l.forEach(function(n){h+='<div class="wifi" onclick="pick(this,\''+n.ssid.replace(/'/g,"\\'")+'\')"><div class="nm">'+n.ssid+'</div><div class="lk">'+n.rssi+' dBm</div></div>';});
document.getElementById('nets').innerHTML=h;}).catch(function(){document.getElementById('nets').innerHTML='Scan failed. <a href="#" onclick="scan();return false" style="color:var(--red)">retry</a>';});}
function pick(el,s){sel=s;document.querySelectorAll('.wifi').forEach(w=>w.classList.remove('sel'));el.classList.add('sel');}
function save(){var name=document.getElementById('name').value,h=host(name);
document.getElementById('jn').textContent=sel||'your network';document.getElementById('fa').textContent='http://'+h+'.local';document.getElementById('fh').textContent=h;go(3);
fetch('/api/provision',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:sel,pass:document.getElementById('pass').value,name:name})});}
</script></body></html>)=====";
