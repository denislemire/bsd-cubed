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
'''
# A module permits top-level await, using the installed real GJS engine.
import tempfile
with tempfile.TemporaryDirectory(prefix='bsd3-shell49-contract-') as directory:
    source = Path(directory) / 'contract.js'
    source.write_text(script)
    subprocess.run(['/usr/local/bin/gjs', '-m', str(source)], check=True, timeout=15)
subprocess.run(['/usr/local/bin/gjs', '-c',
                "imports.gi.versions.Gst='1.0'; const Gst=imports.gi.Gst; "
                "Gst.init_check([]); print('PASS: actual GStreamer non-null argv initialization');"],
               check=True, timeout=15)
