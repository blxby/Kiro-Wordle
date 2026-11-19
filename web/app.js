// Vanilla JavaScript, no frameworks or modules. Talks to the C++ server's
// /api/... routes. The DOM is the page's tree of elements; we read and change it
// with document.getElementById and element.textContent / .className.

// ---- shared helpers ----------------------------------------------------

// fetch asks the server for a URL and returns a promise for the response.
// async/await lets us write it as if it were step-by-step code.
async function getJson(url) {
  const res = await fetch(url);
  return await res.json();
}

const STATES = ["b", "y", "g"];   // tile cycle order: grey -> yellow -> green

// ---- tab switching -----------------------------------------------------

const tabs = document.querySelectorAll(".tab");
for (let i = 0; i < tabs.length; i++) {
  tabs[i].onclick = function () {
    const name = this.getAttribute("data-tab");
    document.querySelectorAll(".tab").forEach(function (t) { t.classList.remove("active"); });
    document.querySelectorAll(".panel").forEach(function (p) { p.classList.remove("active"); });
    this.classList.add("active");
    document.getElementById("panel-" + name).classList.add("active");
  };
}

// ---- Solve tab ---------------------------------------------------------

// State: history is the list of submitted turns; the current row is being typed.
let history = [];                       // [{guess, feedback}]
let currentGuess = "";                  // letters typed so far on the active row
let currentStates = ["b","b","b","b","b"];  // tile colors on the active row

const gridEl = document.getElementById("grid");
const keyboardEl = document.getElementById("keyboard");

// build the 6x5 grid of tile divs once
function buildGrid(el) {
  el.innerHTML = "";
  for (let i = 0; i < 30; i++) {
    const t = document.createElement("div");
    t.className = "tile";
    el.appendChild(t);
  }
}

// draw the solve grid from history plus the row being typed
function renderGrid() {
  const tiles = gridEl.children;
  for (let i = 0; i < 30; i++) {
    tiles[i].className = "tile";
    tiles[i].textContent = "";
  }
  let row = 0;
  // submitted turns
  for (; row < history.length && row < 6; row++) {
    const h = history[row];
    for (let c = 0; c < 5; c++) {
      const t = tiles[row * 5 + c];
      t.textContent = h.guess[c];
      t.className = "tile filled " + h.feedback[c];
    }
  }
  // the active row being typed
  if (row < 6) {
    for (let c = 0; c < 5; c++) {
      const t = tiles[row * 5 + c];
      if (c < currentGuess.length) {
        t.textContent = currentGuess[c];
        t.className = "tile filled " + currentStates[c];
        // clicking a filled current tile cycles its color
        t.onclick = (function (col) {
          return function () { cycleTile(col); };
        })(c);
      } else {
        t.onclick = null;
      }
    }
  }
}

// cycle one tile on the current row: grey -> yellow -> green -> grey
function cycleTile(col) {
  const now = currentStates[col];
  const next = STATES[(STATES.indexOf(now) + 1) % 3];
  currentStates[col] = next;
  renderGrid();
}

// build the on-screen keyboard
const KB_ROWS = ["qwertyuiop", "asdfghjkl", "zxcvbnm"];
function buildKeyboard() {
  keyboardEl.innerHTML = "";
  for (let r = 0; r < KB_ROWS.length; r++) {
    const rowEl = document.createElement("div");
    rowEl.className = "kb-row";
    if (r === 2) {
      rowEl.appendChild(makeKey("Enter", true));
    }
    for (const ch of KB_ROWS[r]) {
      rowEl.appendChild(makeKey(ch, false));
    }
    if (r === 2) {
      rowEl.appendChild(makeKey("Back", true));
    }
    keyboardEl.appendChild(rowEl);
  }
  colorKeyboard();
}

function makeKey(label, wide) {
  const k = document.createElement("button");
  k.className = "key" + (wide ? " wide" : "");
  k.textContent = label;
  k.setAttribute("data-key", label);
  k.onclick = function () {
    if (label === "Enter") submitSolve();
    else if (label === "Back") typeKey("Backspace");
    else typeKey(label);
  };
  return k;
}

// color each letter key by the best state known from history (g > y > b)
function colorKeyboard() {
  const best = {};
  const rank = { b: 1, y: 2, g: 3 };
  for (const h of history) {
    for (let i = 0; i < 5; i++) {
      const letter = h.guess[i];
      const s = h.feedback[i];
      if (!best[letter] || rank[s] > rank[best[letter]]) {
        best[letter] = s;
      }
    }
  }
  const keys = keyboardEl.querySelectorAll(".key");
  keys.forEach(function (k) {
    const label = k.getAttribute("data-key");
    if (label.length === 1 && best[label]) {
      k.className = "key " + best[label];
    } else if (label.length === 1) {
      k.className = "key";
    }
  });
}

// handle a typed letter / Backspace / Enter on the current row
function typeKey(key) {
  if (history.length >= 6) return;
  if (key === "Backspace") {
    currentGuess = currentGuess.slice(0, -1);
  } else if (key.length === 1 && key >= "a" && key <= "z") {
    if (currentGuess.length < 5) currentGuess += key;
  } else if (key.length === 1 && key >= "A" && key <= "Z") {
    if (currentGuess.length < 5) currentGuess += key.toLowerCase();
  }
  renderGrid();
}

// physical keyboard fills the current row
document.addEventListener("keydown", function (e) {
  // only when the Solve panel is active and no text input is focused
  if (!document.getElementById("panel-solve").classList.contains("active")) return;
  if (document.activeElement && document.activeElement.tagName === "INPUT") return;
  if (e.key === "Enter") { submitSolve(); return; }
  if (e.key === "Backspace") { typeKey("Backspace"); return; }
  if (e.key.length === 1) typeKey(e.key);
});

// build the guesses=..&feedback=.. query from history
function historyQuery() {
  const g = history.map(function (h) { return h.guess; }).join(",");
  const f = history.map(function (h) { return h.feedback; }).join(",");
  return "guesses=" + g + "&feedback=" + f;
}

// ask the server for the candidates/suggestion for the current history
async function refreshSolve() {
  const data = await getJson("/api/solve?" + historyQuery());
  const stats = document.getElementById("solve-stats");
  if (data.error) {
    stats.textContent = "Error: " + data.error;
    return;
  }
  stats.innerHTML =
    "<b>" + data.count + "</b> words left &nbsp; " +
    "suggestion: <b>" + data.suggestion + "</b> &nbsp; " +
    "trie " + data.trie_us.toFixed(1) + " us, " + data.nodes + " nodes";
  document.getElementById("candidates").textContent = data.candidates.join(" ");
  lastSuggestion = data.suggestion;
}

let lastSuggestion = "";

function submitSolve() {
  if (currentGuess.length !== 5) return;      // row must be full
  history.push({ guess: currentGuess, feedback: currentStates.join("") });
  currentGuess = "";
  currentStates = ["b","b","b","b","b"];
  renderGrid();
  colorKeyboard();
  refreshSolve();
}

document.getElementById("btn-submit").onclick = submitSolve;

document.getElementById("btn-suggestion").onclick = function () {
  if (!lastSuggestion) return;
  currentGuess = lastSuggestion;
  currentStates = ["b","b","b","b","b"];
  renderGrid();
};

document.getElementById("btn-undo").onclick = function () {
  if (history.length > 0) history.pop();
  currentGuess = "";
  currentStates = ["b","b","b","b","b"];
  renderGrid();
  colorKeyboard();
  refreshSolve();
};

document.getElementById("btn-new").onclick = function () {
  history = [];
  currentGuess = "";
  currentStates = ["b","b","b","b","b"];
  renderGrid();
  colorKeyboard();
  refreshSolve();
};

// ---- Auto play tab -----------------------------------------------------

const autoGridEl = document.getElementById("auto-grid");

document.getElementById("btn-auto").onclick = async function () {
  buildGrid(autoGridEl);
  document.getElementById("auto-stats").textContent = "playing...";
  const answer = document.getElementById("auto-answer").value.trim();
  const url = "/api/autoplay" + (answer ? "?answer=" + answer : "");
  const data = await getJson(url);
  if (data.error) {
    document.getElementById("auto-stats").textContent = "Error: " + data.error;
    return;
  }
  revealTurns(data.turns, 0);
};

// reveal one row every 700 ms with setTimeout (runs a function after a delay)
function revealTurns(turns, i) {
  if (i >= turns.length) return;
  const tiles = autoGridEl.children;
  const t = turns[i];
  for (let c = 0; c < 5; c++) {
    const tile = tiles[i * 5 + c];
    tile.textContent = t.guess[c];
    tile.className = "tile filled reveal " + t.feedback[c];
  }
  document.getElementById("auto-stats").innerHTML =
    "Turn " + (i + 1) + ": " + t.left + " words left, " + t.us.toFixed(1) + " us";
  setTimeout(function () { revealTurns(turns, i + 1); }, 700);
}

// ---- Race tab ----------------------------------------------------------

document.querySelectorAll(".size").forEach(function (b) {
  b.onclick = async function () {
    const size = this.getAttribute("data-size");
    document.getElementById("race-info").textContent = "racing...";
    document.getElementById("race-bars").innerHTML = "";
    const data = await getJson("/api/race?size=" + size);
    if (data.error) {
      document.getElementById("race-info").textContent = "Error: " + data.error;
      return;
    }
    renderRace(data);
  };
});

function renderRace(d) {
  const speedup = (d.brute.us / d.trie.us).toFixed(1);
  document.getElementById("race-info").innerHTML =
    "size <b>" + d.size + "</b> (" + d.synthetic + " synthetic) &nbsp; " +
    "guess " + d.guess + " / " + d.feedback + " &nbsp; " +
    "trie is <b>" + speedup + "x</b> faster than brute force";

  const max = Math.max(d.trie.us, d.brute.us, d.incremental.us);
  const bars = document.getElementById("race-bars");
  bars.innerHTML = "";
  bars.appendChild(makeBar("Trie", d.trie, max, ""));
  bars.appendChild(makeBar("Brute force", d.brute, max, "brute"));
  bars.appendChild(makeBar("Incremental", d.incremental, max, "inc"));
}

// one labeled bar; width is proportional to the slowest time
function makeBar(label, m, max, cls) {
  const row = document.createElement("div");
  row.className = "bar-row";
  const lbl = document.createElement("div");
  lbl.className = "bar-label";
  lbl.textContent = label + " - " + m.us.toFixed(1) + " us, " +
    m.matches + " matches, work " + m.work;
  const track = document.createElement("div");
  track.className = "bar-track";
  const fill = document.createElement("div");
  fill.className = "bar-fill " + cls;
  const pct = max > 0 ? (m.us / max) * 100 : 0;
  fill.style.width = pct.toFixed(1) + "%";
  track.appendChild(fill);
  row.appendChild(lbl);
  row.appendChild(track);
  return row;
}

// ---- startup -----------------------------------------------------------

async function init() {
  buildGrid(gridEl);
  buildKeyboard();
  renderGrid();
  try {
    const info = await getJson("/api/info");
    document.getElementById("info").textContent =
      info.words + " words, " + info.nodes + " trie nodes";
  } catch (e) {
    document.getElementById("info").textContent = "server not reachable";
  }
  refreshSolve();
}

init();
