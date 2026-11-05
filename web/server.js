#!/usr/bin/env node
// C Lab: edit, compile and run the C files in this project from a browser.
// Compilation and execution both happen inside a bubblewrap sandbox.
'use strict';

const http = require('http');
const fs = require('fs');
const os = require('os');
const path = require('path');
const crypto = require('crypto');
const { spawn, spawnSync } = require('child_process');

const PORT = Number(process.env.PORT) || 8123;
const HOST = '127.0.0.1';
const WEB_DIR = __dirname;
const PROJECT_DIR = path.dirname(WEB_DIR);
const PUBLIC_DIR = path.join(WEB_DIR, 'public');
const RUNTIME_DIR = path.join(WEB_DIR, 'runtime');

const LIMITS = {
  maxBody: 2 * 1024 * 1024,
  maxConcurrentRuns: 4,
  compileSeconds: 30,
  runSeconds: 15 * 60,
  cpuSeconds: 120,
  memoryKb: 1024 * 1024,
  fileKb: 64 * 1024,
  processes: 64,
  consoleBytes: 2 * 1024 * 1024,
  gfxMessageBytes: 16 * 1024 * 1024,
  backlogBytes: 8 * 1024 * 1024,
};

const SIGNALS = {
  4: 'Illegal instruction', 6: 'Aborted', 8: 'Floating point exception (for example, dividing by zero)',
  9: 'Killed', 11: 'Segmentation fault (invalid memory access)', 24: 'CPU time limit reached',
  25: 'File size limit reached',
};

const MIME = {
  '.html': 'text/html; charset=utf-8', '.js': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
};

// ---- sandbox ---------------------------------------------------------------

function sandbox(workDir, writable, stdio, command) {
  const limits = [
    `ulimit -t ${LIMITS.cpuSeconds}`, `ulimit -v ${LIMITS.memoryKb}`,
    `ulimit -f ${LIMITS.fileKb}`, `ulimit -u ${LIMITS.processes}`, 'ulimit -c 0',
    'exec "$@"',
  ].join('; ');
  return spawn('bwrap', [
    '--unshare-all', '--die-with-parent', '--new-session', '--clearenv',
    '--setenv', 'PATH', '/usr/bin', '--setenv', 'HOME', '/work', '--setenv', 'LANG', 'C.UTF-8',
    '--ro-bind', '/usr', '/usr',
    '--symlink', 'usr/lib64', '/lib64', '--symlink', 'usr/lib', '/lib',
    '--symlink', 'usr/bin', '/bin', '--symlink', 'usr/sbin', '/sbin',
    '--proc', '/proc', '--dev', '/dev',
    '--size', String(64 * 1024 * 1024), '--tmpfs', '/tmp',
    '--ro-bind', RUNTIME_DIR, '/runtime',
    writable ? '--bind' : '--ro-bind', workDir, '/work',
    '--chdir', '/work',
    '--', 'sh', '-c', limits, 'sh', ...command,
  ], { stdio });
}

function buildRuntime() {
  fs.mkdirSync(path.join(RUNTIME_DIR, 'build'), { recursive: true });
  const result = spawnSync('gcc', [
    '-c', '-O2', '-Wall', '-Wextra', '-Iinclude', 'webbgi.c', '-o', 'build/webbgi.o',
  ], { cwd: RUNTIME_DIR, encoding: 'utf8' });
  if (result.error || result.status !== 0) {
    console.error(result.error ? result.error.message : result.stderr);
    process.exit(1);
  }
}

// ---- running a program -----------------------------------------------------

const runs = new Map();

function send(run, event, data) {
  if (!run.res || run.res.writableEnded) return true;
  return run.res.write(`event: ${event}\ndata: ${JSON.stringify(data)}\n\n`);
}

function finish(run, info) {
  if (run.finished) return;
  run.finished = true;
  clearTimeout(run.timer);
  send(run, 'exit', info);
  if (run.res) run.res.end();
  if (run.workDir) fs.rm(run.workDir, { recursive: true, force: true }, () => {});
  runs.delete(run.id);
}

function killRun(run, reason) {
  if (reason && !run.killReason) run.killReason = reason;
  if (run.child) run.child.kill('SIGKILL');
  else finish(run, { reason: run.killReason || 'Stopped' });
}

function parseDiagnostics(text, fileName) {
  const diagnostics = [];
  for (const line of text.split('\n')) {
    if (!line.startsWith(fileName + ':')) continue;
    const m = /^(\d+):(?:(\d+):)? (?:fatal )?(error|warning): (.*)$/.exec(line.slice(fileName.length + 1));
    if (m) diagnostics.push({ line: Number(m[1]), column: Number(m[2] || 1), severity: m[3], message: m[4] });
  }
  return diagnostics;
}

async function startRun(run) {
  run.workDir = await fs.promises.mkdtemp(path.join(os.tmpdir(), 'c-lab-'));
  await fs.promises.writeFile(path.join(run.workDir, run.name), run.code);
  send(run, 'phase', { phase: 'compiling' });

  const compiler = sandbox(run.workDir, true, ['ignore', 'pipe', 'pipe'], [
    'gcc', '-Wall', '-Wextra', '-g', '-fdiagnostics-color=never', '-fno-diagnostics-show-caret',
    '-I/runtime/include', run.name, '/runtime/build/webbgi.o', '-o', 'a.out', '-lm', '-fuse-ld=bfd', // /usr/bin/ld resolves through /etc, hidden here
  ]);
  run.child = compiler;
  let output = '';
  const collect = (chunk) => { if (output.length < 256 * 1024) output += chunk; };
  compiler.stdout.setEncoding('utf8').on('data', collect);
  compiler.stderr.setEncoding('utf8').on('data', collect);
  run.timer = setTimeout(() => killRun(run, 'Compiling took too long'), LIMITS.compileSeconds * 1000);

  compiler.on('error', (err) => finish(run, { reason: `Could not start the compiler: ${err.message}` }));
  compiler.on('close', (code) => {
    clearTimeout(run.timer);
    run.child = null;
    send(run, 'compile', { ok: code === 0, output, diagnostics: parseDiagnostics(output, run.name) });
    if (run.killReason) return finish(run, { reason: run.killReason });
    if (code !== 0) return finish(run, { reason: 'Compile failed', compileFailed: true });
    execute(run);
  });
}

function execute(run) {
  send(run, 'phase', { phase: 'running' });
  const child = sandbox(run.workDir, false, ['pipe', 'pipe', 'pipe', 'pipe', 'pipe'], ['./a.out']);
  run.child = child;
  run.keys = child.stdio[4];
  child.stdin.on('error', () => {});
  run.keys.on('error', () => {});
  run.timer = setTimeout(() => killRun(run, `Stopped after the ${LIMITS.runSeconds / 60} minute run limit`),
    LIMITS.runSeconds * 1000);

  let consoleBytes = 0;
  const forward = (event) => (chunk) => {
    consoleBytes += Buffer.byteLength(chunk);
    if (consoleBytes > LIMITS.consoleBytes) return killRun(run, 'Stopped: the program printed too much output');
    send(run, event, chunk);
  };
  child.stdout.setEncoding('utf8').on('data', forward('stdout'));
  child.stderr.setEncoding('utf8').on('data', forward('stderr'));

  const gfx = child.stdio[3];
  gfx.on('error', () => {});
  let pending = Buffer.alloc(0);
  gfx.on('data', (chunk) => {
    pending = pending.length ? Buffer.concat([pending, chunk]) : chunk;
    while (pending.length >= 5) {
      const length = pending.readUInt32LE(1);
      if (length > LIMITS.gfxMessageBytes) return killRun(run, 'Stopped: invalid graphics data');
      if (pending.length < 5 + length) break;
      const ok = send(run, 'gfx', {
        type: String.fromCharCode(pending[0]),
        data: pending.subarray(5, 5 + length).toString('base64'),
      });
      pending = pending.subarray(5 + length);
      // A slow page holds the program back rather than growing server memory.
      if (!ok && run.res.writableLength > LIMITS.backlogBytes && !gfx.isPaused()) {
        gfx.pause();
        run.res.once('drain', () => gfx.resume());
      }
    }
  });

  child.on('error', (err) => finish(run, { reason: `Could not start the program: ${err.message}` }));
  child.on('close', (code, signal) => {
    run.child = null;
    if (run.killReason) return finish(run, { reason: run.killReason });
    if (signal) return finish(run, { reason: `Stopped by ${signal}` });
    if (code > 128 && SIGNALS[code - 128]) return finish(run, { code, crash: SIGNALS[code - 128] });
    finish(run, { code });
  });
}

// ---- files -----------------------------------------------------------------

function sourcePath(name) {
  if (typeof name !== 'string' || !/^[^./\\\0-][^/\\\0]{0,99}\.c$/.test(name)) return null;
  return path.join(PROJECT_DIR, name);
}

async function listFiles() {
  const entries = await fs.promises.readdir(PROJECT_DIR, { withFileTypes: true });
  return entries
    .filter((e) => e.isFile() && sourcePath(e.name))
    .map((e) => e.name)
    .sort((a, b) => a.localeCompare(b, undefined, { numeric: true }));
}

async function isRegularFile(file) {
  try { return (await fs.promises.lstat(file)).isFile(); } catch { return false; }
}

// ---- http ------------------------------------------------------------------

function json(res, status, body) {
  res.writeHead(status, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify(body));
}

function readJson(req) {
  return new Promise((resolve, reject) => {
    if (!(req.headers['content-type'] || '').startsWith('application/json')) {
      return reject(Object.assign(new Error('Expected JSON'), { status: 415 }));
    }
    const chunks = [];
    let size = 0;
    req.on('data', (c) => {
      size += c.length;
      if (size > LIMITS.maxBody) {
        reject(Object.assign(new Error('Too large'), { status: 413 }));
        req.destroy();
      } else chunks.push(c);
    });
    req.on('end', () => {
      try { resolve(JSON.parse(Buffer.concat(chunks).toString('utf8'))); }
      catch { reject(Object.assign(new Error('Invalid JSON'), { status: 400 })); }
    });
    req.on('error', reject);
  });
}

// Only this machine's own page may talk to the server. Other sites open in the
// browser would otherwise be able to reach localhost and run code or edit files.
function allowedOrigin(req) {
  const hosts = [`127.0.0.1:${PORT}`, `localhost:${PORT}`];
  if (!hosts.includes(req.headers.host)) return false;
  const origin = req.headers.origin;
  return !origin || origin === `http://${req.headers.host}`;
}

async function handleApi(req, res, url) {
  const parts = url.pathname.split('/').filter(Boolean).slice(1).map(decodeURIComponent);

  if (parts[0] === 'files') {
    if (parts.length === 1 && req.method === 'GET') return json(res, 200, { files: await listFiles() });
    const file = sourcePath(parts[1]);
    if (parts.length !== 2 || !file) return json(res, 400, { error: 'File names must end in .c and contain no slashes.' });
    if (req.method === 'GET') {
      if (!(await isRegularFile(file))) return json(res, 404, { error: 'File not found.' });
      return json(res, 200, { name: parts[1], code: await fs.promises.readFile(file, 'utf8') });
    }
    if (req.method === 'PUT' || req.method === 'POST') {
      const { code } = await readJson(req);
      if (typeof code !== 'string') return json(res, 400, { error: 'Missing code.' });
      if (req.method === 'PUT' && !(await isRegularFile(file))) return json(res, 404, { error: 'File not found.' });
      try {
        await fs.promises.writeFile(file, code, { flag: req.method === 'POST' ? 'wx' : 'w' });
      } catch (err) {
        if (err.code === 'EEXIST') return json(res, 409, { error: 'A file with that name already exists.' });
        throw err;
      }
      return json(res, 200, { name: parts[1] });
    }
  }

  if (parts[0] === 'run') {
    if (parts.length === 1 && req.method === 'POST') {
      const { code, name } = await readJson(req);
      if (typeof code !== 'string') return json(res, 400, { error: 'Missing code.' });
      if (runs.size >= LIMITS.maxConcurrentRuns) return json(res, 429, { error: 'Too many programs are running. Stop one first.' });
      const run = { id: crypto.randomUUID(), code, name: sourcePath(name) ? name : 'main.c' };
      // Abandoned if the page never connects to its event stream.
      run.timer = setTimeout(() => finish(run, { reason: 'Never started' }), 15000);
      runs.set(run.id, run);
      return json(res, 200, { id: run.id });
    }
    const run = runs.get(parts[1]);
    if (!run) return json(res, 404, { error: 'That program is no longer running.' });

    if (parts[2] === 'events' && req.method === 'GET') {
      if (run.res) return json(res, 409, { error: 'Already connected.' });
      clearTimeout(run.timer);
      run.res = res;
      res.writeHead(200, { 'Content-Type': 'text/event-stream', 'Cache-Control': 'no-store', Connection: 'keep-alive' });
      res.on('close', () => { if (!run.finished) killRun(run, 'Page closed'); });
      startRun(run).catch((err) => finish(run, { reason: err.message }));
      return;
    }
    if (req.method === 'POST' && parts[2] === 'stop') {
      killRun(run, 'Stopped');
      return json(res, 200, {});
    }
    if (req.method === 'POST' && parts[2] === 'stdin') {
      const { text, eof } = await readJson(req);
      const stdin = run.child && run.child.stdin;
      if (stdin && !stdin.destroyed && run.keys) {
        if (typeof text === 'string') stdin.write(text);
        if (eof) stdin.end();
      }
      return json(res, 200, {});
    }
    if (req.method === 'POST' && parts[2] === 'key') {
      const { bytes } = await readJson(req);
      const valid = Array.isArray(bytes) && bytes.length <= 8 && bytes.every((b) => Number.isInteger(b) && b >= 0 && b < 256);
      if (!valid) return json(res, 400, { error: 'Invalid key.' });
      if (run.keys && !run.keys.destroyed) run.keys.write(Buffer.from(bytes));
      return json(res, 200, {});
    }
  }
  return json(res, 404, { error: 'Not found.' });
}

async function serveStatic(res, url) {
  const relative = url.pathname === '/' ? 'index.html' : decodeURIComponent(url.pathname).slice(1);
  const file = path.resolve(PUBLIC_DIR, relative);
  if (!file.startsWith(PUBLIC_DIR + path.sep) || !MIME[path.extname(file)]) {
    res.writeHead(404);
    return res.end('Not found');
  }
  try {
    const body = await fs.promises.readFile(file);
    res.writeHead(200, {
      'Content-Type': MIME[path.extname(file)],
      'Content-Security-Policy': "default-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data:",
      'X-Content-Type-Options': 'nosniff',
      'Cache-Control': 'no-cache',
    });
    res.end(body);
  } catch {
    res.writeHead(404);
    res.end('Not found');
  }
}

const server = http.createServer(async (req, res) => {
  try {
    if (!allowedOrigin(req)) {
      res.writeHead(403);
      return res.end('Forbidden');
    }
    const url = new URL(req.url, `http://${req.headers.host}`);
    if (url.pathname.startsWith('/api/')) await handleApi(req, res, url);
    else if (req.method === 'GET') await serveStatic(res, url);
    else { res.writeHead(405); res.end(); }
  } catch (err) {
    if (res.headersSent) return res.destroy();
    json(res, err.status || 500, { error: err.status ? err.message : 'Server error.' });
    if (!err.status) console.error(err);
  }
});

const check = spawnSync('bwrap', ['--version'], { encoding: 'utf8' });
if (check.error) {
  console.error('bubblewrap (bwrap) is required to sandbox programs. Install it with: sudo dnf install bubblewrap');
  process.exit(1);
}
buildRuntime();
server.requestTimeout = 0;
server.listen(PORT, HOST, () => console.log(`C Lab is running at http://localhost:${PORT}`));
