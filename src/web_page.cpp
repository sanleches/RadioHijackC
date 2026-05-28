#include "web_page.hpp"

namespace app {

const char* webPageHtml() {
  return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Pico Smart Radio</title>
<style>
:root { color-scheme: dark; --cyan:#00e5ff; --yellow:#ffeb3b; --green:#4caf50; --orange:#ff9800; }
* { box-sizing: border-box; }
body { font-family: system-ui, -apple-system, Segoe UI, sans-serif; background:#101114; color:#fff; text-align:center; margin:0; padding:18px; }
.container { max-width:480px; margin:auto; background:#1b1d22; padding:24px; border-radius:18px; box-shadow:0 16px 45px rgba(0,0,0,.45); }
h1 { color:var(--cyan); margin:0 0 6px; }
.ip { color:#8c99a8; font-size:.9rem; margin-bottom:16px; }
.freq-display { font-size:3rem; line-height:1; font-weight:800; color:var(--yellow); margin:12px 0; }
.rds-station { font-size:1.2rem; color:var(--green); font-weight:700; min-height:28px; }
.rds-song { color:#b9c3cc; font-style:italic; min-height:24px; margin-bottom:22px; }
.grid { display:grid; grid-template-columns:1fr 1fr; gap:10px; margin:14px 0; }
button { background:#2b3038; color:white; border:2px solid var(--cyan); padding:13px 14px; font-size:1rem; border-radius:12px; cursor:pointer; transition:.15s; }
button:hover { background:var(--cyan); color:#000; }
button.warn { border-color:var(--orange); }
button.warn:hover { background:var(--orange); }
input { width:100%; padding:12px; border-radius:12px; border:1px solid #343b46; background:#111318; color:white; font-size:1rem; }
input[type=range] { padding:0; accent-color:var(--cyan); }
label { display:block; margin:16px 0 8px; color:#d3d8df; }
.presets { display:grid; grid-template-columns:repeat(3, 1fr); gap:8px; margin-top:10px; }
.scan-results { display:grid; grid-template-columns:1fr 1fr; gap:8px; margin-top:10px; }
.scan-results button { width:100%; }
.small { font-size:.9rem; color:#8c99a8; margin-top:14px; }
.status { display:grid; grid-template-columns:repeat(3, 1fr); gap:8px; margin:14px 0; color:#cfd6de; font-size:.9rem; }
.status div { background:#111318; padding:8px; border-radius:10px; }
@media (max-width:430px) { .container { padding:18px; } .freq-display { font-size:2.5rem; } .presets { grid-template-columns:1fr 1fr; } }
</style>
</head>
<body>
<div class="container">
  <h1>RadioHijack</h1>
  <div class="ip" id="ip">Connecting...</div>
  <div class="rds-station" id="station">Connecting...</div>
  <div class="freq-display"><span id="freq">--.-</span> MHz</div>
  <div class="rds-song" id="song">Waiting for signal...</div>

  <div class="grid">
    <button onclick="tuneCommand('/seek?dir=down')">Seek Down</button>
    <button onclick="tuneCommand('/seek?dir=up')">Seek Up</button>
  </div>

  <label>Manual Tune</label>
  <div class="grid">
    <input id="freqInput" type="number" min="87.0" max="108.0" step="0.1" placeholder="104.5">
    <button onclick="tuneManual()">Tune</button>
  </div>
  <div class="grid">
    <button onclick="tuneCommand('/step?mhz=-0.1')">-0.1 MHz</button>
    <button onclick="tuneCommand('/step?mhz=0.1')">+0.1 MHz</button>
  </div>

  <label>Volume: <span id="vol-label">5</span></label>
  <input type="range" min="0" max="15" value="5" id="volume" oninput="setVolLabel(this.value)" onchange="runRadioCommand('/vol?v=' + this.value)">

  <div class="status">
    <div>RSSI<br><strong id="rssi">-</strong></div>
    <div>Stereo<br><strong id="stereo">-</strong></div>
    <div>RDS<br><strong id="rds">-</strong></div>
  </div>
  <button onclick="updateStatus()">Refresh Status</button>

  <div class="grid">
    <button class="warn" onclick="toggleMute()" id="muteBtn">Mute</button>
    <button class="warn" id="scanBtn" onclick="scanStations()">Scan</button>
  </div>
  <h3>Scan Results</h3>
  <div class="scan-results" id="scanResults"></div>

  <h3>Presets</h3>
  <div class="presets" id="presets"></div>
  <div class="grid">
    <input id="presetName" type="text" placeholder="Preset name">
    <button onclick="savePreset()">Save Current</button>
  </div>
  <div class="small" id="message"></div>
</div>

<script>
const state = { freq: null, muted: false, presets: {}, scanning: false };
function el(id) { return document.getElementById(id); }
function setMessage(text) { el('message').innerText = text || ''; }
function setVolLabel(value) { el('vol-label').innerText = value; }
function cacheBust(url) { return url + (url.indexOf('?') === -1 ? '?' : '&') + '_=' + Date.now(); }
async function apiGet(url) {
  const response = await fetch(cacheBust(url), { cache: 'no-store' });
  const data = await response.json();
  if (!response.ok || data.result === 'error') throw new Error(data.message || ('HTTP ' + response.status));
  return data;
}
async function apiText(url) {
  const response = await fetch(cacheBust(url), { cache: 'no-store' });
  const text = await response.text();
  if (!response.ok) throw new Error('HTTP ' + response.status);
  return text;
}
function renderFrequency(freq) {
  if (freq === undefined || freq === null || Number.isNaN(Number(freq))) return;
  state.freq = Number(freq);
  el('freq').innerText = state.freq.toFixed(1);
}
function renderVolume(vol) { if (vol !== undefined && vol !== null) { el('volume').value = vol; setVolLabel(vol); } }
function renderMute(muted) { state.muted = !!muted; el('muteBtn').innerText = state.muted ? 'Unmute' : 'Mute'; }
function renderPresets(presets) {
  state.presets = presets || {};
  const container = el('presets');
  container.innerHTML = '';
  Object.keys(state.presets).sort().forEach(name => {
    const freq = Number(state.presets[name]).toFixed(1);
    const button = document.createElement('button');
    button.className = 'warn';
    button.textContent = name + ' ' + freq;
    button.onclick = () => tuneTo(freq);
    container.appendChild(button);
  });
  if (!container.children.length) container.textContent = 'No presets saved';
}
function renderRadioText(data) {
  el('station').innerText = data.station || 'No station name';
  el('song').innerText = data.song || 'No radio text';
  el('rssi').innerText = data.rssi;
  el('stereo').innerText = data.stereo ? 'Yes' : 'No';
  el('rds').innerText = data.rds_ready ? 'Ready' : 'Wait';
}
function renderStatus(data) {
  renderFrequency(data.freq); renderVolume(data.vol); renderMute(data.muted); renderPresets(data.presets); renderRadioText(data);
  el('ip').innerText = 'Open http://' + data.ip;
}
async function updateStatus() { try { setMessage('Refreshing status...'); renderStatus(await apiGet('/status')); setMessage(''); } catch (err) { setMessage('Status error: ' + err.message); } }
async function updateRadioText() { try { renderRadioText(await apiGet('/status')); } catch (err) { console.log('Radio text update failed:', err.message); } }
async function runRadioCommand(url) {
  try {
    setMessage('Working...');
    const data = await apiGet(url);
    renderFrequency(data.freq !== undefined ? data.freq : data.frequency);
    renderVolume(data.vol !== undefined ? data.vol : data.volume);
    if (data.muted !== undefined) renderMute(data.muted);
    if (data.presets) renderPresets(data.presets);
    setMessage('');
    return data;
  } catch (err) { setMessage('Error: ' + err.message); return null; }
}
function tuneTo(freq) { return runRadioCommand('/tune?f=' + encodeURIComponent(freq)); }
function tuneCommand(url) { return runRadioCommand(url); }
function tuneManual() { const freq = el('freqInput').value; if (!freq) return setMessage('Enter a frequency first.'); tuneTo(freq); }
function toggleMute() { runRadioCommand('/mute?on=' + (state.muted ? 0 : 1)); }
async function savePreset() {
  const freq = state.freq;
  if (freq === null) return setMessage('Refresh status or tune a station first.');
  const name = el('presetName').value || (freq.toFixed(1) + ' MHz');
  await runRadioCommand('/presets?action=save&name=' + encodeURIComponent(name) + '&freq=' + encodeURIComponent(freq));
}
el('scanResults').addEventListener('click', function(event) {
  const target = event.target;
  if (target && target.getAttribute && target.getAttribute('data-freq')) tuneTo(target.getAttribute('data-freq'));
});
async function scanStations() {
  if (state.scanning) return;
  state.scanning = true;
  el('scanBtn').disabled = true;
  el('scanResults').innerHTML = '';
  setMessage('Scanning 87.0-108.0 MHz in 0.2 MHz steps...');
  try {
    el('scanResults').innerHTML = await apiText('/scanhtml?step=0.2&minrssi=10');
    setMessage(el('scanResults').querySelector('[data-freq]') ? 'Scan complete. Tap any station button to tune.' : 'Scan complete. No strong stations found.');
  } catch (err) { setMessage('Scan error: ' + err.message); }
  finally { state.scanning = false; el('scanBtn').disabled = false; }
}
updateStatus();
setInterval(updateRadioText, 2500);
</script>
</body>
</html>)HTML";
}

}  // namespace app
