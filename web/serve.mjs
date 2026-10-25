// Dev server for the browser preview. Compiles lib/Effects + web/web.cpp to
// WebAssembly with Emscripten, serves web/, and on every source change rebuilds
// and reloads open pages. Needs em++ on PATH (brew install emscripten) and Node 20+.
// Each build also writes build/info.json (git revision, toolchain, output sizes,
// the firmware's platformio.ini settings), which the page shows in its info panel.
//
//   node web/serve.mjs          build, serve on http://localhost:8000, watch
//   node web/serve.mjs build    build once and exit
//
// PORT and HOST environment variables override the defaults.

import { execFile, spawn } from 'node:child_process';
import { mkdirSync, watch } from 'node:fs';
import { readFile, stat, writeFile } from 'node:fs/promises';
import { createServer } from 'node:http';
import { basename, dirname, extname, join, relative, resolve, sep } from 'node:path';
import { fileURLToPath } from 'node:url';

const webDir = dirname(fileURLToPath(import.meta.url));
const root = resolve(webDir, '..');
const effectsDir = join(root, 'lib/Effects/src');
const outDir = join(webDir, 'build');
const configH = join(root, 'src/config.h');  // web.cpp includes it for the device info

const port = Number(process.env.PORT ?? 8000);
const host = process.env.HOST ?? '127.0.0.1';

const sources = [join(effectsDir, 'Effects.cpp'), join(webDir, 'web.cpp')];
const flags = ['-std=c++17', '-O2'];
const emccArgs = [
  ...sources,
  '-I', effectsDir,
  ...flags,
  '-sMODULARIZE',
  '-sEXPORT_ES6',
  '-sEXPORT_NAME=createEffects',
  '-sENVIRONMENT=web',
  '-sEXPORTED_RUNTIME_METHODS=HEAPU8,UTF8ToString',
  '-o', join(outDir, 'effects.mjs'),
];

function compile() {
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

// --- build info ---

// Resolves to a command's trimmed stdout, or null if it fails (no git, not a checkout).
const run = (cmd, args) =>
  new Promise((done) => execFile(cmd, args, { cwd: root }, (err, out) => done(err ? null : out.trim())));

let compilerVersion;  // em++ doesn't change while the server runs, so ask once
async function gitInfo() {
  const [commit, branch, subject, date, status] = await Promise.all([
    run('git', ['rev-parse', '--short', 'HEAD']),
    run('git', ['rev-parse', '--abbrev-ref', 'HEAD']),
    run('git', ['log', '-1', '--format=%s']),
    run('git', ['log', '-1', '--format=%cI']),
    run('git', ['status', '--porcelain']),
  ]);
  return commit && { commit, branch, subject, date, dirty: status !== '' };
}

// Just enough of PlatformIO's INI dialect for our platformio.ini: sections,
// indented continuation lines, ; comments, `extends`, and ${section.key}.
function parseIni(text) {
  const sections = {};
  let section, key;
  for (const raw of text.split('\n')) {
    const line = raw.replace(/(^|\s);.*$/, '').trimEnd();
    if (!line.trim()) continue;
    let m;
    if ((m = line.match(/^\[(.+)\]$/))) sections[(section = m[1])] = {};
    else if (/^\s/.test(line) && key) sections[section][key] += '\n' + line.trim();
    else if ((m = line.match(/^([^=]+?)\s*=\s*(.*)$/))) sections[section][(key = m[1])] = m[2];
  }
  const get = (name, k) => {
    const s = sections[name] ?? {};
    const v = s[k] ?? (s.extends ? get(s.extends.trim(), k) : undefined);
    return v?.replace(/\$\{([^.}]+)\.([^}]+)\}/g, (_, n, kk) => get(n, kk) ?? '');
  };
  return { sections, get };
}

async function firmwareInfo() {
  try {
    const ini = parseIni(await readFile(join(root, 'platformio.ini'), 'utf8'));
    const envs = Object.keys(ini.sections).filter((s) => s.startsWith('env:')).map((s) => s.slice(4));
    const env = ini.get('platformio', 'default_envs')?.split(/[\s,]+/)[0] ?? envs[0];
    const get = (k) => ini.get(`env:${env}`, k);
    const list = (k) => (get(k) ?? '').split('\n').map((v) => v.trim()).filter(Boolean);
    return {
      env, envs,
      platform: get('platform'),
      board: get('board'),
      framework: get('framework'),
      flashSize: get('board_upload.flash_size'),
      memoryType: get('board_build.arduino.memory_type'),
      partitions: get('board_build.partitions'),
      monitorSpeed: Number(get('monitor_speed')) || null,
      libDeps: list('lib_deps'),
      buildFlags: list('build_flags'),
    };
  } catch {
    return null;
  }
}

const fileSize = (name) => stat(join(outDir, name)).then((s) => s.size, () => null);

async function build() {
  const started = Date.now();
  const result = await compile();
  if (!result.ok) return result;
  compilerVersion ??= (await run('em++', ['--version']))?.split('\n')[0] ?? null;
  const [git, firmware, wasm, mjs] = await Promise.all([
    gitInfo(), firmwareInfo(), fileSize('effects.wasm'), fileSize('effects.mjs'),
  ]);
  const info = {
    builtAt: new Date().toISOString(),
    durationMs: Date.now() - started,
    compiler: compilerVersion,
    flags,
    sources: sources.map((s) => relative(root, s)),
    outputs: { 'effects.wasm': wasm, 'effects.mjs': mjs },
    git,
    firmware,
  };
  await writeFile(join(outDir, 'info.json'), JSON.stringify(info, null, 2) + '\n');
  return result;
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
  '.json': 'application/json',
};

const server = createServer(async (req, res) => {
  const path = decodeURIComponent(new URL(req.url, 'http://x').pathname);

  if (path === '/events') {
    res.writeHead(200, { 'Content-Type': 'text/event-stream', 'Cache-Control': 'no-cache' });
    res.write(': connected\n\n');  // flushes the headers, so the page's EventSource opens now
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
// Watch the folder, not the file: editors that save by renaming would end a file watch.
watch(dirname(configH), (_, name) => name === basename(configH) && onChange(true));
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
  console.log(`Watching ${relative(process.cwd(), effectsDir)}, ${relative(process.cwd(), configH)} and web/ for changes`);
});
