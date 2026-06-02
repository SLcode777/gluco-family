// language=JavaScript
static const char *JS_Main = R"rawliteral(

let UNIT = "mg/dL";
let PERSONS = [];   // latest data from /ajaxGlycemie
let Boucle1s = 0;

// ---- helpers ----
function polar(cx, cy, r, angle) {
    let rad = (angle - 90) * Math.PI / 180;
    return { x: cx + r * Math.cos(rad), y: cy + r * Math.sin(rad) };
}

// Glucose value color, same thresholds as the device.
function glucoColor(v, low, high) {
    if (v < low) return "#3b9dff";
    if (v < high) return "#39d353";
    if (v < 300) return "orange";
    return "#ff4d4d";
}

// Build one trend arrow as an SVG string (white). Trend codes match the device:
// -1 DoubleDown, 1 Down, 2 DownRight, 3 Flat, 4 UpRight, 5 Up, 6 DoubleUp, 0 unknown.
function arrowSVG(Tendance) {
    if (Tendance === 0 || Tendance === undefined || Tendance === null) return "";
    let Teta, isDouble = false;
    if (Tendance === -1) { Teta = 180; isDouble = true; }
    else if (Tendance === 6) { Teta = 0; isDouble = true; }
    else { Teta = 45 * (5 - Tendance); }

    function oneArrow(cx, cy) {
        let s = "";
        let p1 = polar(cx, cy, 20, Teta);
        let p2 = polar(cx, cy, 15, Teta + 90);
        let p3 = polar(cx, cy, 15, Teta - 90);
        s += `<polyline points="${p1.x},${p1.y} ${p2.x},${p2.y} ${p3.x},${p3.y}" style="fill:white;" />`;
        p2 = polar(cx, cy, 30, Teta + 170);
        p3 = polar(cx, cy, 30, Teta - 170);
        s += `<polyline points="${p1.x},${p1.y} ${p2.x},${p2.y} ${p3.x},${p3.y}" style="fill:white;" />`;
        return s;
    }

    let S = `<svg viewBox="0 0 60 60">`;
    S += isDouble ? (oneArrow(15, 30) + oneArrow(45, 30)) : oneArrow(30, 30);
    S += `</svg>`;
    return S;
}

// Freshness bar: fills white over 0-5 min, then orange (5-15 min), red (>15 min or no data).
function ageBar(sec, hasMeasure) {
    let h, col;
    if (!hasMeasure) { h = 100; col = "#ff4d4d"; }
    else if (sec < 300) { h = Math.round(sec / 300 * 100); col = "#fff"; }
    else if (sec < 900) { h = 100; col = "orange"; }
    else { h = 100; col = "#ff4d4d"; }
    return `<div class="zage"><div class="zagefill" style="height:${h}%;background:${col}"></div></div>`;
}

function tr(key, fallback) {
    return (typeof Traduction !== "undefined" && Traduction[key]) ? Traduction[key] : fallback;
}

// Render all person zones from the latest data.
function render() {
    let html = "";
    let now = Math.floor(Date.now() / 1000);
    for (let i = 0; i < PERSONS.length; i++) {
        let p = PERSONS[i];
        let name = (p.name && p.name.length > 0) ? p.name : ("Person " + (i + 1));
        if (!p.configured) {
            html += `<div class="zone"><div class="zinfo"><div class="zname">${name}</div>`
                 +  `<div class="zempty">${tr("PersonEmpty", "Not configured")}</div></div></div>`;
            continue;
        }
        let has = p.GlycemieVal > 0;
        let info, arrow = "", age;
        if (has) {
            let val = (UNIT === "mmol/L") ? (p.GlycemieVal / 18.0).toFixed(1) : Math.round(p.GlycemieVal);
            let col = glucoColor(p.GlycemieVal, p.targetLow, p.targetHigh);
            info = `<div class="zname">${name}</div>`
                 + `<div class="zval" style="color:${col}">${val}<span class="zunit">${UNIT}</span></div>`;
            arrow = `<div class="zarrow">${arrowSVG(p.TrendArrow)}</div>`;
            let sec = (p.lastGlyUnixTime > 0) ? (now - p.lastGlyUnixTime) : 999999;
            age = ageBar(sec, true);
        } else {
            info = `<div class="zname">${name}</div>`
                 + `<div class="zempty">${tr("WaitGluco", "Waiting...")}</div>`;
            age = ageBar(0, false);
        }
        html += `<div class="zone"><div class="zinfo">${info}</div>${arrow}${age}</div>`;
    }
    GH("zones", html);
}

function LoadLGlycemie() {
    fetch("/ajaxGlycemie")
        .then(r => r.json())
        .then(obj => {
            UNIT = obj.GlucoseUnitLabel || "mg/dL";
            PERSONS = obj.persons || [];
            render();
        })
        .catch(() => {});
}

function Sequenceur1s() {
    if (Boucle1s % 10 === 0) LoadLGlycemie();
    Boucle1s = (1 + Boucle1s) % 100;
    render(); // refresh age bars/text every second
}

function init() {
    SetTraduction();
    GH("version", Version);
    LoadLGlycemie();
    setInterval(Sequenceur1s, 1000);
}

)rawliteral";
