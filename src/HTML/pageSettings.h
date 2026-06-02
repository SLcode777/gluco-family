//************************************************
// Settings page: edit per-person Dexcom/Libre config from the web
//************************************************
const char *SettingsHtml = R"====(
<!DOCTYPE html>
<html>

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { background:#111; color:#fff; font-family:Arial; margin:0; }
        a { color:#fff; text-decoration:none; }
        .nav { display:flex; gap:8px; padding:8px 12px; background:#000; font-size:16px; }
        .nav a { padding:5px 10px; border:1px solid #444; border-radius:6px; }
        .wrap { max-width:540px; margin:0 auto; padding:0 12px 30px; }
        h2 { text-align:center; }
        fieldset { border:1px solid #444; border-radius:8px; margin:14px 0; padding:10px 12px; }
        legend { color:#9cf; padding:0 6px; }
        label { display:inline-block; width:42%; color:#ccc; margin:6px 0; vertical-align:middle; }
        input, select { width:54%; padding:6px; box-sizing:border-box; background:#222; color:#fff;
                        border:1px solid #555; border-radius:5px; }
        input[type=number] { width:26%; }
        .save { display:block; width:100%; padding:12px; font-size:18px; margin-top:10px;
                background:#2563eb; color:#fff; border:none; border-radius:8px; }
        #status { display:block; text-align:center; margin-top:10px; min-height:22px; }
    </style>
    <title>Gluco-Family - Settings</title>
    <script src="/JS_Commun"></script>
    <script src="/JS_Traduction"></script>
</head>

<body onload="initS();">
    <div class="nav">
        <a href="/" data-i18n="Glucose">Home</a>
    </div>
    <div class="wrap">
        <h2 data-i18n="Settings">Settings</h2>
        <div id="persons"></div>
        <fieldset>
            <legend data-i18n="Compte">Account (global)</legend>
            <label data-i18n="Region">Dexcom region</label>
            <select id="region"><option>Non-US</option><option>US</option><option>JP</option></select><br>
            <label data-i18n="EmailLinkUp">LibreLinkUp email</label><input id="lemail"><br>
            <label data-i18n="PasseLinkUp">LibreLinkUp password</label><input id="lpass" type="password"><br>
            <label data-i18n="ServerZone">LibreLinkUp zone</label><input id="lzone">
        </fieldset>
        <button class="save" onclick="save()" data-i18n="Save">Save</button>
        <span id="status"></span>
    </div>

    <script>
        const NB = 3;
        function tr(k, f) { return (typeof Traduction !== "undefined" && Traduction[k]) ? Traduction[k] : f; }
        function esc(s) { return (s || "").toString().replace(/&/g, "&amp;").replace(/"/g, "&quot;").replace(/</g, "&lt;"); }

        function personBlock(i, p) {
            return `<fieldset>
                <legend>${tr("Person", "Person")} ${i + 1}</legend>
                <label data-i18n="FirstName">Name</label><input id="name${i}" value="${esc(p.name)}"><br>
                <label data-i18n="SensorType">Sensor</label>
                <select id="sensor${i}"><option value="1">Dexcom</option><option value="0">LibreLinkUp</option></select><br>
                <label data-i18n="UsernameDexcom">Dexcom username</label><input id="duser${i}" value="${esc(p.dexcomUsername)}"><br>
                <label data-i18n="PasseDexcom">Dexcom password</label><input id="dpass${i}" type="password"><br>
                <label data-i18n="TargetLow">Target low</label><input id="low${i}" type="number" value="${p.targetLow}">
                <label data-i18n="TargetHigh">Target high</label><input id="high${i}" type="number" value="${p.targetHigh}">
            </fieldset>`;
        }

        function initS() {
            fetch("/api/config")
                .then(r => r.json())
                .then(cfg => {
                    let html = "";
                    for (let i = 0; i < NB; i++) html += personBlock(i, cfg.persons[i]);
                    GID("persons").innerHTML = html;
                    for (let i = 0; i < NB; i++) {
                        GID("sensor" + i).value = cfg.persons[i].sensorType;
                        if (cfg.persons[i].hasDexcomPass) GID("dpass" + i).placeholder = tr("PwdUnchanged", "(leave blank = unchanged)");
                    }
                    GID("region").value = cfg.dexcomRegion || "Non-US";
                    GID("lemail").value = cfg.libreEmail || "";
                    GID("lzone").value = cfg.libreZone || "";
                    if (cfg.hasLibrePass) GID("lpass").placeholder = tr("PwdUnchanged", "(leave blank = unchanged)");
                    SetTraduction();
                })
                .catch(() => { GID("status").textContent = "Error loading config"; });
        }

        function save() {
            const body = new URLSearchParams();
            for (let i = 0; i < NB; i++) {
                body.set("name" + i, GID("name" + i).value);
                body.set("sensor" + i, GID("sensor" + i).value);
                body.set("duser" + i, GID("duser" + i).value);
                body.set("dpass" + i, GID("dpass" + i).value);
                body.set("low" + i, GID("low" + i).value);
                body.set("high" + i, GID("high" + i).value);
            }
            body.set("region", GID("region").value);
            body.set("lemail", GID("lemail").value);
            body.set("lpass", GID("lpass").value);
            body.set("lzone", GID("lzone").value);
            GID("status").textContent = "...";
            fetch("/api/settings", { method: "POST", body })
                .then(r => r.json())
                .then(o => { GID("status").textContent = (o.status === "ok") ? tr("Saved", "Saved") : "Error"; })
                .catch(() => { GID("status").textContent = "Error"; });
        }
    </script>
</body>

</html>
)====";
