'use strict';

const $ = (id) => document.getElementById(id);
const DRAFT_PREFIX = 'c-lab-draft:';
const MAX_CONSOLE_NODES = 4000;

// Classic 16-colour BGI palette, indexed by the values in graphics.h.
const PALETTE = [
  [0, 0, 0], [0, 0, 170], [0, 170, 0], [0, 170, 170], [170, 0, 0], [170, 0, 170], [170, 85, 0], [170, 170, 170],
  [85, 85, 85], [85, 85, 255], [85, 255, 85], [85, 255, 255], [255, 85, 85], [255, 85, 255], [255, 255, 85], [255, 255, 255],
];

const editor = CodeMirror.fromTextArea($('editor'), {
  mode: 'text/x-csrc',
  lineNumbers: true,
  indentUnit: 4,
  tabSize: 4,
  matchBrackets: true,
  readOnly: 'nocursor',
  screenReaderLabel: 'C source code',
  extraKeys: {
    Tab: (cm) => (cm.somethingSelected() ? cm.indentSelection('add') : cm.replaceSelection('    ')),
  },
});

const state = {
  files: [],
  current: null,
  saved: new Map(), // file name -> text on disk, with \n line endings
  crlf: new Map(),  // file name -> whether the file on disk uses \r\n
  run: null,        // { id, source }
  markedLines: [],
  loading: false,
  starting: false,
};

// ---- server ----------------------------------------------------------------

async function api(path, options = {}) {
  const init = { ...options };
  if (options.body !== undefined) {
    init.headers = { 'Content-Type': 'application/json' };
    init.body = JSON.stringify(options.body);
  }
  const res = await fetch(`/api/${path}`, init);
  const data = await res.json().catch(() => ({}));
  if (!res.ok) throw new Error(data.error || `Request failed (${res.status})`);
  return data;
}

const filePath = (name) => `files/${encodeURIComponent(name)}`;

// ---- files -----------------------------------------------------------------

const draftOf = (name) => localStorage.getItem(DRAFT_PREFIX + name);

// The editor always works in \n. Files keep whichever line endings they had on disk.
function remember(name, code) {
  state.saved.set(name, code.replace(/\r\n?/g, '\n'));
  state.crlf.set(name, code.includes('\r\n'));
}

function renderFiles() {
  const list = $('fileList');
  list.replaceChildren(...state.files.map((name) => {
    const item = document.createElement('li');
    const button = document.createElement('button');
    button.type = 'button';
    button.textContent = name;
    button.title = name;
    if (name === state.current) button.setAttribute('aria-current', 'true');
    if (draftOf(name) !== null) button.classList.add('edited');
    button.addEventListener('click', () => openFile(name));
    item.append(button);
    return item;
  }));
}

function updateDirty() {
  const name = state.current;
  if (!name) return;
  const dirty = editor.getValue() !== state.saved.get(name);
  $('dirtyMark').hidden = !dirty;
  $('saveBtn').disabled = !dirty;
  $('revertBtn').disabled = !dirty;
  const wasDraft = draftOf(name) !== null;
  if (dirty) localStorage.setItem(DRAFT_PREFIX + name, editor.getValue());
  else localStorage.removeItem(DRAFT_PREFIX + name);
  if (wasDraft !== dirty) renderFiles();
}

async function openFile(name) {
  if (name === state.current) return;
  try {
    const { code } = await api(filePath(name));
    remember(name, code);
    state.current = name;
    state.loading = true;
    editor.setOption('readOnly', false);
    editor.setValue(draftOf(name) ?? state.saved.get(name));
    editor.clearHistory();
    state.loading = false;
    clearMarks();
    $('fileName').textContent = name;
    $('runBtn').disabled = Boolean(state.run);
    $('saveNote').textContent = '';
    location.hash = encodeURIComponent(name);
    updateDirty();
    renderFiles();
  } catch (err) {
    $('saveNote').textContent = err.message;
  }
}

async function saveFile() {
  if (!state.current || $('saveBtn').disabled) return;
  const code = editor.getValue();
  try {
    const onDisk = state.crlf.get(state.current) ? code.replace(/\n/g, '\r\n') : code;
    await api(filePath(state.current), { method: 'PUT', body: { code: onDisk } });
    state.saved.set(state.current, code);
    $('saveNote').textContent = 'Saved';
    setTimeout(() => { $('saveNote').textContent = ''; }, 1500);
    updateDirty();
  } catch (err) {
    $('saveNote').textContent = `Save failed: ${err.message}`;
  }
}

async function revertFile() {
  const name = state.current;
  if (!name) return;
  const { code } = await api(filePath(name));
  remember(name, code);
  editor.setValue(state.saved.get(name));
  clearMarks();
  updateDirty();
}

async function createFile(rawName) {
  const name = rawName.trim().endsWith('.c') ? rawName.trim() : `${rawName.trim()}.c`;
  const code = '#include <stdio.h>\n\nint main() {\n    printf("Hello!\\n");\n    return 0;\n}\n';
  await api(filePath(name), { method: 'POST', body: { code } });
  state.files = (await api('files')).files;
  await openFile(name);
}

// ---- console ---------------------------------------------------------------

const consoleEl = $('console');

function appendConsole(text, className) {
  const pinned = consoleEl.scrollHeight - consoleEl.scrollTop - consoleEl.clientHeight < 40;
  const formFeed = text.lastIndexOf('\f'); // clrscr()
  if (formFeed !== -1) {
    consoleEl.replaceChildren();
    text = text.slice(formFeed + 1);
  }
  if (text) {
    const last = consoleEl.lastChild;
    if (last && last.nodeName === 'SPAN' && last.className === (className || '') && !last.dataset.diag) {
      last.append(text);
    } else {
      const span = document.createElement('span');
      span.className = className || '';
      span.textContent = text;
      consoleEl.append(span);
    }
  }
  while (consoleEl.childNodes.length > MAX_CONSOLE_NODES) consoleEl.firstChild.remove();
  if (pinned) consoleEl.scrollTop = consoleEl.scrollHeight;
}

function appendCompilerOutput(output, fileName) {
  for (const line of output.split('\n')) {
    if (!line) continue;
    const m = line.startsWith(`${fileName}:`) ? /^(\d+)(?::(\d+))?: /.exec(line.slice(fileName.length + 1)) : null;
    const span = document.createElement('span');
    span.dataset.diag = '1';
    span.className = /\berror\b/.test(line) ? 'error-line' : /\bwarning\b/.test(line) ? 'warning-line' : 'system';
    if (m) {
      const link = document.createElement('a');
      link.className = 'diag';
      link.textContent = line;
      link.addEventListener('click', () => {
        editor.setCursor({ line: Number(m[1]) - 1, ch: Number(m[2] || 1) - 1 });
        editor.focus();
      });
      span.append(link);
    } else {
      span.textContent = line;
    }
    span.append('\n');
    consoleEl.append(span);
  }
  consoleEl.scrollTop = consoleEl.scrollHeight;
}

function setStatus(text, stateName) {
  $('status').textContent = text;
  $('status').dataset.state = stateName;
}

// ---- diagnostics in the editor ---------------------------------------------

function clearMarks() {
  for (const handle of state.markedLines) {
    editor.removeLineClass(handle, 'background', 'line-error');
    editor.removeLineClass(handle, 'background', 'line-warning');
  }
  state.markedLines = [];
}

function markDiagnostics(diagnostics) {
  clearMarks();
  for (const d of diagnostics) {
    const handle = editor.getLineHandle(d.line - 1);
    if (!handle) continue;
    editor.addLineClass(handle, 'background', d.severity === 'error' ? 'line-error' : 'line-warning');
    state.markedLines.push(handle);
  }
}

// ---- graphics --------------------------------------------------------------

const canvas = $('screen');
const ctx = canvas.getContext('2d');
const gfx = { width: 0, height: 0, pixels: null, image: null, background: 0 };

function colorOf(index) {
  return PALETTE[index === 0 ? gfx.background : index & 15];
}

function resetGraphics(hasWindow) {
  gfx.pixels = null;
  gfx.image = null;
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  $('noGraphics').hidden = hasWindow;
  $('gfxSize').textContent = '';
  $('keyHint').hidden = true;
}

function paint(x, y, w, h) {
  const { pixels, image, width } = gfx;
  const out = image.data;
  for (let row = y; row < y + h; row++) {
    for (let col = x; col < x + w; col++) {
      const [r, g, b] = colorOf(pixels[row * width + col]);
      const i = (row * width + col) * 4;
      out[i] = r; out[i + 1] = g; out[i + 2] = b; out[i + 3] = 255;
    }
  }
  ctx.putImageData(image, 0, 0, x, y, w, h);
}

function decode(base64) {
  const text = atob(base64);
  const bytes = new Uint8Array(text.length);
  for (let i = 0; i < text.length; i++) bytes[i] = text.charCodeAt(i);
  return bytes;
}

function handleGraphics({ type, data }) {
  const bytes = decode(data);
  if (type === 'I') {
    const width = bytes[0] | (bytes[1] << 8);
    const height = bytes[2] | (bytes[3] << 8);
    canvas.width = gfx.width = width;
    canvas.height = gfx.height = height;
    gfx.background = 0;
    gfx.pixels = new Uint8Array(width * height);
    gfx.image = ctx.createImageData(width, height);
    paint(0, 0, width, height);
    $('noGraphics').hidden = true;
    $('gfxSize').textContent = `${width} × ${height}`;
  } else if (type === 'F' && gfx.pixels) {
    const x = bytes[0] | (bytes[1] << 8);
    const y = bytes[2] | (bytes[3] << 8);
    const w = bytes[4] | (bytes[5] << 8);
    const h = bytes[6] | (bytes[7] << 8);
    if (x + w > gfx.width || y + h > gfx.height) return;
    let col = 0;
    let row = 0;
    for (let i = 8; i + 1 < bytes.length && row < h; i += 2) {
      for (let n = bytes[i]; n > 0 && row < h; n--) {
        gfx.pixels[(y + row) * gfx.width + x + col] = bytes[i + 1];
        if (++col === w) { col = 0; row++; }
      }
    }
    paint(x, y, w, h);
  } else if (type === 'P' && gfx.pixels) {
    gfx.background = bytes[0] & 15;
    paint(0, 0, gfx.width, gfx.height);
  } else if (type === 'W') {
    $('keyHint').hidden = false;
    canvas.focus({ preventScroll: true });
  }
}

const EXTENDED_KEYS = {
  ArrowUp: 72, ArrowDown: 80, ArrowLeft: 75, ArrowRight: 77, Home: 71, End: 79, PageUp: 73, PageDown: 81, Insert: 82, Delete: 83,
};
const CONTROL_KEYS = { Enter: 13, Escape: 27, Backspace: 8, Tab: 9 };

function keyBytes(event) {
  if (event.key in EXTENDED_KEYS) return [0, EXTENDED_KEYS[event.key]];
  if (event.key in CONTROL_KEYS) return [CONTROL_KEYS[event.key]];
  if (event.key.length === 1 && !event.altKey && !event.metaKey) {
    const code = event.key.charCodeAt(0);
    if (event.ctrlKey) return code >= 97 && code <= 122 ? [code - 96] : null;
    return code < 256 ? [code] : null;
  }
  return null;
}

canvas.addEventListener('keydown', (event) => {
  if (!state.run || (event.ctrlKey && event.key === 'Enter')) return;
  const bytes = keyBytes(event);
  if (!bytes) return;
  event.preventDefault();
  $('keyHint').hidden = true;
  api(`run/${state.run.id}/key`, { method: 'POST', body: { bytes } }).catch(() => {});
});
canvas.addEventListener('click', () => canvas.focus({ preventScroll: true }));

// ---- running ---------------------------------------------------------------

function setRunning(running) {
  $('runBtn').disabled = running || !state.current;
  $('stopBtn').disabled = !running;
  $('stdinInput').disabled = !running;
}

function endRun(info) {
  if (state.run) state.run.source.close();
  state.run = null;
  setRunning(false);
  $('keyHint').hidden = true;
  if (info.compileFailed) {
    setStatus('Compile failed', 'error');
  } else if (info.crash) {
    appendConsole(`\n${info.crash}\n`, 'stderr');
    setStatus('Crashed', 'error');
  } else if (info.reason) {
    appendConsole(`\n${info.reason}\n`, 'system');
    setStatus(info.reason === 'Stopped' ? 'Stopped' : 'Ended', 'idle');
  } else if (info.code === 0) {
    appendConsole('\nProgram finished (exit code 0)\n', 'system');
    setStatus('Finished', 'ok');
  } else {
    appendConsole(`\nProgram finished (exit code ${info.code})\n`, 'system');
    setStatus(`Exit code ${info.code}`, 'error');
  }
}

async function runProgram() {
  if (!state.current || state.run || state.starting) return;
  state.starting = true;
  const name = state.current;
  consoleEl.replaceChildren();
  clearMarks();
  resetGraphics(false);
  setStatus('Compiling…', 'busy');
  setRunning(true);

  let id;
  try {
    ({ id } = await api('run', { method: 'POST', body: { name, code: editor.getValue() } }));
  } catch (err) {
    appendConsole(`${err.message}\n`, 'stderr');
    setStatus('Could not start', 'error');
    setRunning(false);
    return;
  } finally {
    state.starting = false;
  }

  const source = new EventSource(`/api/run/${id}/events`);
  state.run = { id, source };
  const on = (event, handler) => source.addEventListener(event, (e) => handler(JSON.parse(e.data)));

  on('phase', ({ phase }) => {
    if (phase === 'running') {
      setStatus('Running', 'busy');
      $('stdinInput').focus({ preventScroll: true });
    }
  });
  on('compile', ({ output, diagnostics }) => {
    markDiagnostics(diagnostics);
    appendCompilerOutput(output, name);
  });
  on('stdout', (text) => appendConsole(text));
  on('stderr', (text) => appendConsole(text, 'stderr'));
  on('gfx', handleGraphics);
  on('exit', endRun);
  source.onerror = () => {
    if (state.run && state.run.id === id) endRun({ reason: 'Lost connection to the server' });
  };
}

async function stopProgram() {
  if (state.run) await api(`run/${state.run.id}/stop`, { method: 'POST', body: {} }).catch(() => {});
}

// ---- wiring ----------------------------------------------------------------

editor.on('change', () => { if (!state.loading) updateDirty(); });
$('runBtn').addEventListener('click', runProgram);
$('stopBtn').addEventListener('click', stopProgram);
$('saveBtn').addEventListener('click', saveFile);
$('revertBtn').addEventListener('click', () => revertFile().catch((err) => { $('saveNote').textContent = err.message; }));
$('clearBtn').addEventListener('click', () => consoleEl.replaceChildren());

$('stdinForm').addEventListener('submit', (event) => {
  event.preventDefault();
  if (!state.run) return;
  const input = $('stdinInput');
  appendConsole(`${input.value}\n`, 'stdin-echo');
  api(`run/${state.run.id}/stdin`, { method: 'POST', body: { text: `${input.value}\n` } }).catch(() => {});
  input.value = '';
});
$('stdinInput').addEventListener('keydown', (event) => {
  if (event.ctrlKey && event.key === 'd' && state.run) { // end of input
    event.preventDefault();
    api(`run/${state.run.id}/stdin`, { method: 'POST', body: { eof: true } }).catch(() => {});
    appendConsole('[end of input]\n', 'system');
  }
});

$('newBtn').addEventListener('click', () => {
  const form = $('newForm');
  form.hidden = !form.hidden;
  $('newError').textContent = '';
  if (!form.hidden) $('newName').focus();
});
$('newForm').addEventListener('submit', async (event) => {
  event.preventDefault();
  const value = $('newName').value;
  if (!value.trim()) return;
  try {
    await createFile(value);
    $('newName').value = '';
    $('newForm').hidden = true;
    $('newError').textContent = '';
  } catch (err) {
    $('newError').textContent = err.message;
  }
});

document.addEventListener('keydown', (event) => {
  if (!(event.ctrlKey || event.metaKey)) return;
  if (event.key === 'Enter') { event.preventDefault(); runProgram(); }
  if (event.key.toLowerCase() === 's') { event.preventDefault(); saveFile(); }
});

(async function start() {
  try {
    state.files = (await api('files')).files;
    renderFiles();
    const wanted = decodeURIComponent(location.hash.slice(1));
    const first = state.files.includes(wanted) ? wanted : state.files[0];
    if (first) await openFile(first);
  } catch (err) {
    setStatus(err.message, 'error');
  }
})();
