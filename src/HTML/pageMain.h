//************************************************
// Page principale HTML et Javascript
//************************************************
const char *MainHtml = R"====(
<!DOCTYPE html>
<html>

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { background:#111; color:#fff; font-family:Arial; margin:0; }
        a { color:#fff; text-decoration:none; }
        .nav { display:flex; gap:8px; justify-content:flex-end; padding:8px 12px; background:#000; font-size:16px; }
        .nav a { padding:5px 10px; border:1px solid #444; border-radius:6px; }
        h1.brand { text-align:center; margin:10px 0; font-size:24px; }
        .zones { max-width:540px; margin:0 auto; }
        .zone { display:flex; align-items:center; border-top:1px solid #333; min-height:120px; padding:6px 12px; box-sizing:border-box; }
        .zone:first-child { border-top:none; }
        .zinfo { flex:1; min-width:0; }
        .zname { font-size:18px; color:#ccc; }
        .zval { font-size:64px; font-weight:bold; line-height:1; }
        .zunit { font-size:16px; color:#aaa; margin-left:8px; }
        .zempty { font-size:18px; color:#777; font-style:italic; }
        .zarrow { width:70px; text-align:center; }
        .zarrow svg { width:60px; height:60px; }
        .zage { width:10px; height:96px; background:#333; border-radius:4px; overflow:hidden; position:relative; margin-left:10px; }
        .zagefill { position:absolute; bottom:0; left:0; width:100%; height:0%; background:#fff; transition:height .5s linear; }
        .LeBas { display:flex; justify-content:space-between; color:#888; margin:16px 12px; font-size:13px; }
    </style>
    <title>Gluco-Family</title>
    <script src="/JS_Commun"></script>
    <script src="/JS_Main"></script>
    <script src="/JS_Traduction"></script>
</head>

<body onload="init();">
    <div class="nav">
        <a href="/Settings" data-i18n="Settings">Settings</a>
        <a href="/Brute" data-i18n="Historique">Data</a>
        <a href="/OTA" data-i18n="Update">Update</a>
        <a href="/Restart" data-i18n="Restart">Restart</a>
    </div>
    <h1 class="brand">Gluco-Family</h1>
    <div class="zones" id="zones"></div>
    <div class="LeBas">
        <div>Version : <span id="version"></span></div>
        <div><a href="https://github.com/SLcode777/gluco-family">github.com/SLcode777/gluco-family</a></div>
    </div>
</body>

</html>


)====";

const char *RestartHtml = R"====(
<!DOCTYPE html>
<html>

<head>
    <meta charset="UTF-8">
    <title>Restart</title>
</head>

<body>
    <h1>Restart</h1>
</body>

</html>

)====";

// icône 64pixels
const char * Favicon = R"====(
<svg width="64" height="64" viewBox="0 20 180 90" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <style>
      .gp { fill: none; stroke-width: 25; stroke-linecap: butt; }
    </style>
  </defs>
  <path class="gp" stroke="blue" d="M 22.5,80 A 67.5,67.5 0 0,1 42.27,32.27" />
  <path class="gp" stroke="green" d="M 42.27,32.27 A 67.5,67.5 0 0,1 90,12.5" />
  <path class="gp" stroke="orange" d="M 90,12.5 A 67.5,67.5 0 0,1 137.73,32.27" />
  <path class="gp" stroke="red" d="M 137.73,32.27 A 67.5,67.5 0 0,1 157.5,80" />
</svg>
)====";
// icône 192pixels
const char * Favicon192 = R"====(
<svg width="192" height="192" viewBox="0 20 180 90" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <style>
      .gp { fill: none; stroke-width: 25; stroke-linecap: butt; }
    </style>
  </defs>
  <path class="gp" stroke="blue" d="M 22.5,80 A 67.5,67.5 0 0,1 42.27,32.27" />
  <path class="gp" stroke="green" d="M 42.27,32.27 A 67.5,67.5 0 0,1 90,12.5" />
  <path class="gp" stroke="orange" d="M 90,12.5 A 67.5,67.5 0 0,1 137.73,32.27" />
  <path class="gp" stroke="red" d="M 137.73,32.27 A 67.5,67.5 0 0,1 157.5,80" />
</svg>
)====";
// Manifest pour Android
const char * Manifest = R"====(
{
  "name": "Routeur F1ATB",
  "short_name": "Routeur",
  "start_url": "/",
  "display": "standalone",
  "icons": [
    {
      "src": "/favicon192.ico",
      "sizes": "192x192",
      "type": "image/svg+xml"
    },
    {
      "src": "/favicon.ico",
      "sizes": "64x64",
      "type": "image/svg+xml"
    }
  ]
}
)====";