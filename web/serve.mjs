// Dev server for the browser preview. Compiles lib/Effects + web/web.cpp to
// WebAssembly with Emscripten, serves web/, and on every source change rebuilds
// and reloads open pages. Needs em++ on PATH (brew install emscripten) and Node 20+.
//
//   node web/serve.mjs          build, serve on http://localhost:8000, watch
//   node web/serve.mjs build    build once and exit
//
// PORT and HOST environment variables override the defaults.

import { spawn } from 'node:child_process';
import { mkdirSync, watch } from 'node:fs';
import { readFile } from 'node:fs/promises';
import { createServer } from 'node:http';
import { dirname, extname, join, relative, resolve, sep } from 'node:path';
import { fileURLToPath } from 'node:url';

const webDir = dirname(fileURLToPath(import.meta.url));
const root = resolve(webDir, '..');
const effectsDir = join(root, 'lib/Effects/src');
const outDir = join(webDir, 'build');

const port = Number(process.env.PORT ?? 8000);
const host = process.env.HOST ?? '127.0.0.1';

const emccArgs = [
  join(effectsDir, 'Effects.cpp'),
  join(webDir, 'web.cpp'),
  '-I', effectsDir,
  '-std=c++17',
  '-O2',
  '-sMODULARIZE',
  '-sEXPORT_ES6',
  '-sEXPORT_NAME=createEffects',
  '-sENVIRONMENT=web',
  '-sEXPORTED_RUNTIME_METHODS=HEAPU8,UTF8ToString',
  '-o', join(outDir, 'effects.mjs'),
];

function build() {
  mkdirSync(outDir, { recursive: true });
  return new Promise((done) => {
    let output = '';
    const proc = spawn('em++', emccArgs, { cwd: root });
    proc.stdout.on('data', (d) => (output += d));
    proc.stderr.on('data', (d) => (output += d));
    proc.on('error', (err) => {
      const hint = err.code === 'ENOENT' ? 'em++ not found. Install it with: brew install emscripten' : err.message;
      done({ ok: false, output: hint });
    });
    proc.on('close', (code) => done({ ok: code === 0, output: output.trim() }));
  });
}

function report(result) {
  const time = new Date().toLocaleTimeString();
  if (result.ok) console.log(`[${time}] build ok${result.output ? '\n' + result.output : ''}`);
  else console.error(`[${time}] build failed\n${result.output}`);
}

if (process.argv[2] === 'build') {
  const result = await build();
  report(result);
  process.exit(result.ok ? 0 : 1);
}

// --- live reload over server-sent events ---

const clients = new Set();
let lastError = null;

function send(res, event, data = '') {
  res.write(`event: ${event}\ndata: ${JSON.stringify(data)}\n\n`);
}

function broadcast(event, data) {
  for (const res of clients) send(res, event, data);
}

let building = false;
let pending = false;

async function rebuild() {
  if (building) {
    pending = true;
    return;
  }
  building = true;
  const result = await build();
  building = false;
  report(result);
  lastError = result.ok ? null : result.output;
  broadcast(result.ok ? 'reload' : 'build-error', lastError ?? '');
  if (pending) {
    pending = false;
    rebuild();
  }
}

// --- static files ---

const types = {
  '.html': 'text/html; charset=utf-8',
  '.mjs': 'text/javascript',
  '.js': 'text/javascript',
  '.css': 'text/css',
  '.wasm': 'application/wasm',
};

const server = createServer(async (req, res) => {
  const path = decodeURIComponent(new URL(req.url, 'http://x').pathname);

  if (path === '/events') {
    res.writeHead(200, { 'Content-Type': 'text/event-stream', 'Cache-Control': 'no-cache' });
    clients.add(res);
    if (lastError) send(res, 'build-error', lastError);
    req.on('close', () => clients.delete(res));
    return;
  }

  const file = resolve(webDir, '.' + (path.endsWith('/') ? path + 'index.html' : path));
  if (file !== webDir && !file.startsWith(webDir + sep)) {
    res.writeHead(403).end();
    return;
  }
  try {
    const body = await readFile(file);
    res.writeHead(200, {
      'Content-Type': types[extname(file)] ?? 'application/octet-stream',
      'Cache-Control': 'no-store',
    });
    res.end(body);
  } catch {
    res.writeHead(404).end('Not found');
  }
});

// --- watch sources ---

let timer;
function onChange(rebuildNeeded) {
  clearTimeout(timer);
  timer = setTimeout(() => (rebuildNeeded ? rebuild() : broadcast('reload')), 100);
}

watch(effectsDir, { recursive: true }, () => onChange(true));
watch(webDir, (_, name) => {
  if (!name || name === 'build' || name === 'serve.mjs') return;
  onChange(/\.(cpp|h)$/.test(name));
});

const first = await build();
report(first);
lastError = first.ok ? null : first.output;

server.listen(port, host, () => {
  const shown = host === '127.0.0.1' || host === '0.0.0.0' ? 'localhost' : host;
  console.log(`Serving ${relative(process.cwd(), webDir) || '.'} on http://${shown}:${port}`);
  console.log(`Watching ${relative(process.cwd(), effectsDir)} and web/ for changes`);
});
