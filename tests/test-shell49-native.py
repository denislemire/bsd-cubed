#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
"""Exercise actual Shell49 consumer methods without a privileged D-Bus peer."""
from pathlib import Path
import subprocess
import sys

root = Path(sys.argv[1])
login = (root / 'js/misc/loginManager.js').read_text()
start = login.index('class LoginManagerConsoleKit ')
end = login.index('class LoginManagerSystemd ', start)
backend = login[start:end]
limits = (root / 'js/misc/timeLimitsManager.js').read_text()
start = limits.index('    _updateSettings() {')
end = limits.index('    async _startStateMachine()', start)
method = limits[start:end]
screencast = (root / 'js/dbusServices/screencast/screencastService.js').read_text()
start = screencast.index('    static canScreencast() {')
end = screencast.index('    constructor()', start)
screencast_method = screencast[start:end]
script = '''const Signals = {EventEmitter: class {}};
''' + backend + '''
const manager = Object.create(LoginManagerConsoleKit.prototype);
if (manager.supportsUserActivity !== false)
    throw new Error('unsupported user activity advertised');
const cap = await manager.canRebootToBootLoaderMenu();
if (cap.canRebootToBootLoaderMenu || cap.needsAuth)
    throw new Error('unsupported boot menu advertised');
let denied = false;
try { manager.setRebootToBootLoaderMenu(); }
catch (error) { denied = error.message.includes('unsupported'); }
if (!denied) throw new Error('unsupported boot operation accepted');
class Limits {
''' + method + '''
}
const limits = Object.create(Limits.prototype);
limits._loginManagerFactory = {new: () => manager};
limits._screenTimeLimitSettings = {get_boolean: () => {
    throw new Error('unsupported user history started');
}};
if (limits._updateSettings() !== false)
    throw new Error('unsupported user history reported enabled');
print('PASS: actual Shell49 ConsoleKit capabilities, boot denial and disabled user-wide history');
// The method is read from the licensed consumer at runtime. These injected
// dependencies and fixture pipeline are original test definitions.
const PIPELINES = [{pipelineString: 'fixture-source ! fixture-sink'}];
let Gst;
class Screencast {
''' + screencast_method + '''
}
function screencastFixture(initialized, missing = null) {
    const queries = [];
    let initializations = 0;
    Gst = {
        init_check(argv) {
            if (!Array.isArray(argv) || argv.length !== 0)
                throw new Error('non-null empty GStreamer argv required');
            initializations++;
            return [initialized, []];
        },
        ElementFactory: {
            find(name) {
                queries.push(name);
                return name === missing ? null : {};
            },
        },
    };
    const available = Screencast.canScreencast();
    if (initializations !== 1)
        throw new Error('unexpected GStreamer initialization count');
    return {available, queries};
}
const unavailable = screencastFixture(false);
if (unavailable.available !== false || unavailable.queries.length !== 0)
    throw new Error('false GI initialization tuple accepted');
for (const missing of ['pipewiresrc', 'filesink', 'fixture-source', 'fixture-sink']) {
    const result = screencastFixture(true, missing);
    if (result.available !== false || !result.queries.includes(missing))
        throw new Error(`missing screencast element accepted: ${missing}`);
}
const available = screencastFixture(true);
if (available.available !== true || available.queries.length !== 4)
    throw new Error('complete screencast fixture rejected');
print('PASS: actual extracted canScreencast rejects failed GI tuple and missing elements, accepts complete pipeline');
'''
# A module permits top-level await, using the installed real GJS engine.
import tempfile
with tempfile.TemporaryDirectory(prefix='bsd3-shell49-contract-') as directory:
    source = Path(directory) / 'contract.js'
    source.write_text(script)
    subprocess.run(['/usr/local/bin/gjs', '-m', str(source)], check=True, timeout=15)
subprocess.run(['/usr/local/bin/gjs', '-c',
                "imports.gi.versions.Gst='1.0'; const Gst=imports.gi.Gst; "
                "const result=Gst.init_check([]); "
                "if (!Array.isArray(result) || result.length !== 2 || "
                "typeof result[0] !== 'boolean' || !Array.isArray(result[1])) "
                "throw new Error('unexpected actual GStreamer GI tuple'); "
                "const [initialized, remaining]=result; "
                "if (initialized !== true || remaining.length !== 0) "
                "throw new Error('actual GStreamer initialization failed'); "
                "print('PASS: actual GStreamer GI tuple [true, []] initialization');"],
               check=True, timeout=15)
