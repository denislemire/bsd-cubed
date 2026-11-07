#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2026 Denis Lemire
# One serial continuation AFTER ci-gnome49.sh, on its disposable native executor.
set -eu
umask 077
program=$(mktemp)
trap 'rm -f "$program"' EXIT
cat > "$program" <<'PY'
import contextlib
import hashlib
import json
import os
from pathlib import Path
import pwd
import re
import shutil
import shlex
import signal
import stat
import subprocess
import sys
import tempfile
import time
import tomllib
import xml.etree.ElementTree as ET

REVISION = '2072800715389ae49066946c960e6521bac37e6a'
# Canonical origins are essential: dependency lookup must see these recipes too.
COMPONENTS = [
 ('gnome-online-accounts49','net/gnome-online-accounts'),
 ('evolution-data-server49','databases/evolution-data-server'),
 ('libshumate49','x11-toolkits/libshumate'), ('tecla49','x11/tecla'),
 ('gnome-backgrounds49','x11-themes/gnome-backgrounds'),
 ('gnome-settings-daemon49','sysutils/gnome-settings-daemon'),
 ('bsdisks','sysutils/bsdisks'), ('gnome-control-center49','sysutils/gnome-control-center'),
 ('gnome-calculator49','math/gnome-calculator'), ('gnome-clocks49','deskutils/gnome-clocks'),
 ('gnome-characters49','deskutils/gnome-characters'), ('gnome-font-viewer49','deskutils/gnome-font-viewer'),
 ('baobab49','deskutils/baobab'), ('gnome-text-editor49','editors/gnome-text-editor'),
 ('gnome-system-monitor49','sysutils/gnome-system-monitor'), ('nautilus49','x11-fm/nautilus'),
 ('epiphany49','www/epiphany'), ('gnome-calendar49','deskutils/gnome-calendar'),
 ('gnome-console49','x11/gnome-console'), ('gnome-maps49','deskutils/gnome-maps'),
 ('papers49','graphics/papers'), ('gnome-shell-extensions49','x11/gnome-shell-extensions'),
 ('evolution49','mail/evolution')]
BASE = ['consolekit2','dinit','bsd-cubed','gjs','gsettings-desktop-schemas','mutter','gnome-shell','gnome-session']
CLOCK = float('inf')
INHIBITOR = None


def require(test, message):
    if not test:
        raise RuntimeError(message)


def digest(path):
    h = hashlib.sha256()
    with open(path, 'rb') as stream:
        for chunk in iter(lambda: stream.read(1024*1024), b''):
            h.update(chunk)
    return h.hexdigest()


def halt(process):
    if process.poll() is None:
        os.killpg(process.pid, signal.SIGTERM)
        try:
            process.wait(timeout=8)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait(timeout=5)


def run(argv, log=None, timeout=600, env=None, cwd=None):
    output = open(log, 'wb') if log else tempfile.TemporaryFile()
    process = subprocess.Popen([str(a) for a in argv], stdout=output, stderr=subprocess.STDOUT,
                               env=env, cwd=cwd, start_new_session=True)
    try:
        deadline = min(CLOCK, time.monotonic()+timeout)
        tick = 0
        while process.poll() is None:
            require(time.monotonic() < deadline, 'bounded ecosystem command timed out')
            require(INHIBITOR is None or INHIBITOR.poll() is None, 'bounded inhibitor lost')
            if tick % 120 == 0:
                print('Running serial ecosystem command; diagnostics retained', flush=True)
            tick += 1
            time.sleep(.25)
        output.flush()
        if process.returncode:
            if log:
                print(''.join(Path(log).read_text(errors='replace').splitlines(True)[-60:]), flush=True)
            raise subprocess.CalledProcessError(process.returncode, argv)
        if not log:
            output.seek(0)
            return output.read().decode()
        return ''
    finally:
        halt(process)
        output.close()


def trusted(path):
    path = Path(path)
    require(not path.is_symlink(), 'controller/recipe links refused')
    for entry in [path] + list(path.parents):
        info = entry.stat()
        require(info.st_uid == 0 and (not info.st_mode & 0o022 or
                (entry.is_dir() and info.st_mode & stat.S_ISVTX)), 'root command input is actor-writable')


def freeze(path, gid):
    for entry in [Path(path)] + list(Path(path).rglob('*')):
        require(not entry.is_symlink(), 'snapshot links require explicit review')
        os.chown(entry, 0, gid)
        entry.chmod(0o750 if entry.is_dir() else 0o640)


def copy_output(actor_root, relative, destination, uid):
    # Root never follows actor-controlled paths or reads actor hardlinks.
    flags = os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW
    fd = os.open(actor_root, flags)
    try:
        parts = Path(relative).parts
        require(parts and all(p not in ('.','..','/') for p in parts), 'invalid receipt path')
        for part in parts[:-1]:
            following = os.open(part, flags, dir_fd=fd)
            os.close(fd)
            fd = following
        source_fd = os.open(parts[-1], os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK, dir_fd=fd)
        with os.fdopen(source_fd, 'rb') as source:
            info = os.fstat(source.fileno())
            require(stat.S_ISREG(info.st_mode) and info.st_uid == uid and info.st_nlink == 1,
                    'artifact must be a single-link actor-owned regular file')
            require(info.st_size <= 4*1024**3, 'unexpected artifact size')
            try:
                with open(destination, 'xb') as target:
                    remaining = info.st_size
                    while remaining:
                        require(time.monotonic() < CLOCK, 'artifact collection deadline exceeded')
                        block = source.read(min(1024*1024,remaining))
                        require(block, 'artifact truncated during collection')
                        target.write(block)
                        remaining -= len(block)
            except BaseException:
                Path(destination).unlink(missing_ok=True)
                raise
    finally:
        os.close(fd)


def make(port, ports, work):
    return ['/usr/bin/make','-C',port,'PORTSDIR='+str(ports),'BATCH=yes','DEVELOPER=yes',
            'USE_PACKAGE_DEPENDS_ONLY=yes','WRKDIRPREFIX='+str(work/'obj'),
            'DISTDIR='+str(work/'distfiles'),'PACKAGES='+str(work/'packages'),
            'MAKE_JOBS_NUMBER='+('2' if port.name == 'papers' else '3')]


def qa_ok(log):
    require(not re.search(r'^Error:', Path(log).read_text(errors='replace'), re.M),
            'stage QA emitted Error: despite command status')


def meson_results(path, expected):
    records = [json.loads(line) for line in Path(path).read_text().splitlines() if line.strip()]
    require(len(records) == expected, 'actual Meson result count differs')
    require(all(record.get('result') == 'OK' and record.get('returncode') == 0 for record in records),
            'Meson skipped, failed or expected-failure result refused')
    names = [record.get('name') for record in records]
    require(all(isinstance(name,str) and name for name in names) and len(set(names)) == expected,
            'duplicate/missing actual Meson test identity')


def ctest_results(path, expected):
    root = ET.parse(path).getroot()
    local = lambda element: element.tag.rsplit('}',1)[-1]
    cases = [element for element in root.iter() if local(element) == 'testcase']
    require(len(cases) == expected, 'actual CTest result count differs')
    for element in root.iter():
        if local(element) in ('testsuite','testsuites'):
            for field in ('failures','errors','skipped','disabled'):
                require(int(element.get(field,'0')) == 0, 'CTest summary includes nonpassing cases')
    identities = []
    for case in cases:
        require(case.get('status','run') == 'run', 'CTest did not run a required case')
        require(not any(local(element) in ('skipped','error','failure') for element in case.iter()),
                'CTest skipped/error/failure case refused')
        require(bool(case.get('name')), 'CTest result lacks identity')
        identities.append((case.get('classname',''),case.get('name')))
    require(len(set(identities)) == expected, 'duplicate CTest result identity')


def worker(control, ports, work, key, origin):
    require(os.geteuid() != 0 and sys.platform.startswith('freebsd'), 'ordinary FreeBSD actor required')
    work.mkdir(mode=0o700)
    receipts = work/'receipts'
    receipts.mkdir()
    # bsd.port.mk enables repository output only if PACKAGES already exists.
    (work/'packages').mkdir()
    command = make(ports/origin, ports, work)
    query = lambda name: run(command+['-V',name], timeout=120).strip()
    wrksrc = Path(query('WRKSRC'))
    require(wrksrc.is_relative_to(work), 'source workdir escaped actor root')
    if key == 'papers49':
        command += ['CARGO_VENDOR_DIR='+str(wrksrc/'vendor')]
        os.environ.update(CARGO_BUILD_JOBS='2', CARGO_NET_OFFLINE='true')
    buildlog = receipts/'build.log'
    run(command+['stage','check-plist','stage-qa','package'], log=buildlog, timeout=4800)
    qa_ok(buildlog)
    build = Path(query('TEST_WRKSRC'))
    if key in ('gnome-calculator49','gnome-system-monitor49','libshumate49','nautilus49'):
        expected = {'gnome-calculator49':7,'gnome-system-monitor49':3,'libshumate49':16,'nautilus49':19}[key]
        tests = json.loads(run(['meson','introspect','--tests',build], timeout=30))
        require(len(tests) == expected, 'upstream Meson suite count differs from qualified recipe')
        failure = None
        try:
            run([sys.executable,control/'tests/native-wayland-tests.py','--output',receipts/'upstream',
                 '--cwd',build,'--timeout','600','--','meson','test','-C',build,
                 '--print-errorlogs','--num-processes='+('1' if key == 'nautilus49' else '2')],
                log=receipts/'tests.log', timeout=700)
        except BaseException as error:
            failure = error
        try:
            shutil.copyfile(build/'meson-logs/testlog.json',receipts/'meson-testlog.json')
        except BaseException as error:
            if failure is None: failure = error
        if failure is not None: raise failure
        meson_results(receipts/'meson-testlog.json',expected)
    elif key in ('evolution-data-server49','bsdisks'):
        expected = 100 if key == 'evolution-data-server49' else 1
        tests = json.loads(run(['ctest','--show-only=json-v1'], cwd=build, timeout=30))
        require(len(tests['tests']) == expected, 'upstream CTest suite count differs')
        run([sys.executable,control/'tests/native-wayland-tests.py','--output',receipts/'upstream',
             '--cwd',build,'--timeout','900','--','ctest','--output-on-failure','-j1','--timeout','90',
             '--output-junit',receipts/'ctest-results.xml'], log=receipts/'tests.log', timeout=1000)
        ctest_results(receipts/'ctest-results.xml',expected)
    if key == 'nautilus49':
        run([sys.executable,control/'tests/native-nautilus49-autofs.py',wrksrc],
            log=receipts/'autofs-contract.log', timeout=120)
    if key == 'epiphany49':
        # This probes the installed WebKit library, not Epiphany's page/quit path.
        shutil.copyfile(control/'tests/native-webkit-view.c',receipts/'native-webkit-view.c')
        flags = shlex.split(run(['pkg-config','--cflags','--libs','webkitgtk-6.0'],timeout=30))
        compile_command = ['cc','-std=c11','-Wall','-Wextra','-Werror',
            str(control/'tests/native-webkit-view.c'),'-o',str(work/'native-webkit-view')]+flags
        (receipts/'webkit-compile.json').write_text(json.dumps({'command':compile_command,
            'webkit_version':run(['pkg-config','--modversion','webkitgtk-6.0'],timeout=20).strip(),
            'WEBKIT_DISABLE_COMPOSITING_MODE':'1','claim':'local library DOM/render probe only'},indent=2)+'\n')
        run(compile_command,log=receipts/'webkit-compile.log',timeout=120)
        run(['/usr/bin/env','WEBKIT_DISABLE_COMPOSITING_MODE=1',sys.executable,
            control/'tests/native-wayland-tests.py','--output',receipts/'webkit-probe',
            '--cwd',work,'--timeout','45','--',work/'native-webkit-view',
            receipts/'webkit-probe/content.png'],log=receipts/'webkit-probe.log',timeout=90)
    source_name = wrksrc.name
    run(['tar','--exclude='+source_name+'/_build','--exclude='+source_name+'/target',
         '-C',wrksrc.parent,'-cJf',receipts/'source.tar.xz',source_name], timeout=600)
    run(['tar','-C',ports,'-czf',receipts/'recipe.tar.gz',origin], timeout=120)
    run(['tar','-C',work/'distfiles','-cJf',receipts/'upstream-distfiles.tar.xz','.'], timeout=600)
    if key == 'papers49':
        require((wrksrc/'vendor').is_dir() and not (wrksrc/'vendor').is_symlink(), 'Papers vendoring escaped WRKSRC')
        lock = tomllib.loads((wrksrc/'Cargo.lock').read_text())
        crates = [p for p in lock['package'] if p.get('source','').startswith('registry+')]
        require(len(crates) == 253, 'Papers locked crate set changed')
        names = []
        for crate in crates:
            require(crate['source'] == 'registry+https://github.com/rust-lang/crates.io-index', 'unsupported Cargo registry')
            name = crate['name']+'-'+crate['version']+'.crate'
            require(re.fullmatch(r'[A-Za-z0-9_.+-]+\.crate',name), 'unsafe Cargo filename')
            path = work/'distfiles/gnome/rust/crates'/name
            require(digest(path) == crate['checksum'], 'locked Cargo source checksum mismatch')
            names.append(name)
        require(not any(p.get('source','').startswith('git+') for p in lock['package']), 'unpreserved Git Cargo input')
        run(['tar','-C',work/'distfiles/gnome/rust/crates','-cJf',receipts/'locked-crates.tar.xz']+names, timeout=300)
        (receipts/'Cargo.lock').write_bytes((wrksrc/'Cargo.lock').read_bytes())
    package = work/'packages/All'/(query('PKGNAME')+'.pkg')
    shutil.copyfile(package, receipts/'package.pkg')
    (receipts/'provenance.json').write_text(json.dumps({'key':key,'origin':origin,
        'framework':REVISION,'pkgname':query('PKGNAME'),'source':source_name,
        'make_command':[str(arg) for arg in command],'meson_args':query('MESON_ARGS'), 'cmake_args':query('CMAKE_ARGS'),
        'compiler':run(['cc','--version'],timeout=20),'freebsd':run(['freebsd-version','-kru'],timeout=20),
        'abi':run(['/usr/local/sbin/pkg','config','ABI'],timeout=20),
        'rustc':run(['/usr/local/bin/rustc','--version'],timeout=20).strip() if key=='papers49' else None,
        'native_tests':'recorded separately; no hardware acceptance claim'},indent=2)+'\n')
    names = ['source.tar.xz','recipe.tar.gz','upstream-distfiles.tar.xz','package.pkg','provenance.json']
    if key == 'papers49': names += ['locked-crates.tar.xz','Cargo.lock']
    (receipts/'complete.json').write_text(json.dumps(names)+'\n')


def complete_receipts(receipt, key):
    # The controller defines the contract; an actor manifest cannot weaken it.
    expected = ['source.tar.xz','recipe.tar.gz','upstream-distfiles.tar.xz',
                'package.pkg','provenance.json']
    if key == 'papers49': expected += ['locked-crates.tar.xz','Cargo.lock']
    require(json.loads((receipt/'complete.json').read_text()) == expected,
            'source/package completion receipt differs')
    for name in expected:
        path = receipt/name
        info = path.lstat()
        require(stat.S_ISREG(info.st_mode) and info.st_size > 0,
                'required source/package receipt missing or empty: '+name)


def installed(name):
    answer = subprocess.run(['/usr/local/sbin/pkg','query','%n-%v %o',name],
                            capture_output=True, text=True, timeout=30)
    require(answer.returncode in (0,1), 'package inventory query failed')
    return answer.stdout.strip() if answer.returncode == 0 else None


def actor_run(control, account, argv, log, cwd, timeout=600):
    env = ['HOME='+str(cwd),'USER='+account.pw_name,'LOGNAME='+account.pw_name,
           'PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin',
           'LANG=C.UTF-8','PYTHONDONTWRITEBYTECODE=1']
    return run(['sudo','-H','-u',account.pw_name,'env','-i']+env+list(argv),
               cwd=cwd, log=log, timeout=timeout)


def collect_tree(actor, relative, target, uid):
    source = actor/relative
    if not source.exists(): return
    count = 0
    for directory, dirs, files in os.walk(source, followlinks=False):
        require(not any((Path(directory)/d).is_symlink() for d in dirs), 'artifact directory link refused')
        for name in files:
            count += 1
            require(count < 1000, 'unexpected diagnostic artifact count')
            path = Path(directory)/name
            destination = target/path.relative_to(source)
            destination.parent.mkdir(parents=True, exist_ok=True)
            copy_output(actor, path.relative_to(actor), destination, uid)


def inventory(directory):
    sums = {str(p.relative_to(directory)):digest(p) for p in directory.rglob('*')
            if p.is_file() and p.name != 'COMPONENT_SHA256SUMS.json'}
    (directory/'COMPONENT_SHA256SUMS.json').write_text(json.dumps(sums,sort_keys=True,indent=2)+'\n')


def direct_dependencies(metadata):
    return sorted(set(run(metadata+['build-depends-list','run-depends-list'],timeout=180).splitlines()))


def provision_timezone(output, localtime=Path('/etc/localtime'), zoneinfo=Path('/var/db/zoneinfo')):
    # Preserve existing configuration, including a dangling symlink requiring
    # separate diagnosis; only a truly absent file authorizes this CI default.
    timezone = {'action':'preserved','localtime_present':os.path.lexists(localtime)}
    if not timezone['localtime_present']:
        run(['/usr/sbin/tzsetup','-s','UTC'],log=output/'timezone-provision.log',timeout=30)
        require(localtime.is_file(),'UTC provisioning did not create localtime')
        timezone['action'] = 'initialized-absent-localtime-to-UTC'
    if localtime.is_file(): timezone['localtime_sha256'] = digest(localtime)
    if zoneinfo.is_file(): timezone['zoneinfo'] = zoneinfo.read_text().strip()
    (output/'timezone.json').write_text(json.dumps(timezone,indent=2)+'\n')


def provision_papers_rust(control, directory):
    # The pinned cargo.mk requires Rust >= 1.99.0; the executor's quarterly
    # catalogue supplied 1.98.1 in CI76. Scope latest to this toolchain request.
    minimum = '1.99.0'
    pkg = '/usr/local/sbin/pkg'
    current = installed('rust')
    if current:
        require(re.fullmatch(r'rust-[A-Za-z0-9_.,+-]+ lang/rust',current),
                'unexpected installed Rust provider')
        version = current.split()[0][len('rust-'):]
        if run([pkg,'version','-t',version,minimum],timeout=20).strip() in ('=','>'):
            (directory/'toolchain.json').write_text(json.dumps({'minimum':minimum,
                'provider':current,'source':'already installed',
                'rustc':run(['/usr/local/bin/rustc','--version'],timeout=20).strip()},indent=2)+'\n')
            return
    keys = '/usr/share/keys/pkg'
    require((Path(keys)/'trusted').is_dir(), 'official FreeBSD package trust unavailable')
    repo_dir = control/'papers-rust-repo'
    repo_dir.mkdir(mode=0o700)
    repo_name = 'BSD3_Papers_Rust'
    url = 'pkg+https://pkg.FreeBSD.org/${ABI}/latest'
    (repo_dir/'FreeBSD-latest.conf').write_text(repo_name+' {\n'
        '  url: "'+url+'",\n  mirror_type: "srv",\n'
        '  signature_type: "fingerprints",\n  fingerprints: "'+keys+'",\n'
        '  enabled: yes\n}\n')
    command = [pkg,'-o','REPOS_DIR='+str(repo_dir)]
    run(command+['update','-f','-r',repo_name],log=directory/'latest-catalogue.log',timeout=600)
    selected = run(command+['rquery','-r',repo_name,'%n %v %o','rust'],timeout=30).split()
    require(len(selected)==3 and selected[0]=='rust' and selected[2]=='lang/rust' and
            re.fullmatch(r'[A-Za-z0-9_.,+-]+',selected[1]),'unexpected signed Rust catalogue identity')
    require(run([pkg,'version','-t',selected[1],minimum],timeout=20).strip() in ('=','>'),
            'signed latest Rust cannot satisfy pinned cargo.mk')
    # A latest-repo install can upgrade pkg itself. Fetch only the compiler
    # through the signed catalogue, then install that file without any repo.
    fetch_dir = control/'papers-rust-fetch'
    fetch_dir.mkdir(mode=0o700)
    run(command+['fetch','-y','-U','-r',repo_name,'-o',fetch_dir,'rust'],
        log=directory/'toolchain-fetch.log',timeout=600)
    # pkg releases use either plain or hashed paths under the fetch root.
    packages = [path for path in fetch_dir.rglob('*.pkg') if path.is_file() and not path.is_symlink()]
    require(len(packages)==1, 'expected exactly one signed Rust package')
    package = packages[0]
    trusted(package)
    expected = 'rust-'+selected[1]+' lang/rust'
    require(run([pkg,'query','-F',package,'%n-%v %o'],timeout=30).strip()==expected,
            'fetched Rust differs from signed catalogue selection')
    checksum = digest(package)
    def other_packages():
        return {line for line in run([pkg,'query','-a','%n %v %o'],timeout=60).splitlines()
                if not line.startswith('rust ')}
    before = other_packages()
    run([pkg,'-o','REPOS_DIR=/var/empty','-o','HANDLE_RC_SCRIPTS=false',
         'add','-A','-f',package],log=directory/'toolchain-install.log',timeout=900)
    require(installed('rust')==expected,'installed Rust differs from signed catalogue selection')
    require(other_packages()==before,'toolchain install changed a non-Rust package')
    (directory/'toolchain.json').write_text(json.dumps({'minimum':minimum,
        'provider':expected,'source':url,'signature_type':'fingerprints','package_sha256':checksum,
        'rustc':run(['/usr/local/bin/rustc','--version'],timeout=20).strip()},indent=2)+'\n')
    shutil.rmtree(fetch_dir)


def root_main(repo):
    global CLOCK, INHIBITOR
    require(os.geteuid() == 0 and sys.platform.startswith('freebsd') and os.environ.get('CIRCLECI') == 'true',
            'root disposable native CI controller required')
    framework_input = os.environ.get('BSD3_CI_PORTS_ROOT','')
    os.environ.clear()
    os.environ.update(PATH='/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin',
                      HOME='/root',LANG='C.UTF-8',CIRCLECI='true',BSD3_CI_PORTS_ROOT=framework_input)
    repo = repo.resolve()
    trusted(repo)
    for daemon in ['gdm','sddm']:
        answer = subprocess.run(['pgrep','-x',daemon], capture_output=True, timeout=5)
        require(answer.returncode == 1, 'active display manager or process-query failure')
    account = pwd.getpwnam('circleci')
    require(account.pw_uid > 0, 'ordinary CI actor missing')
    ports = Path(os.environ['BSD3_CI_PORTS_ROOT']).resolve(strict=True)
    require(run(['git','-C',ports,'rev-parse','HEAD'], timeout=30).strip() == REVISION,
            'framework revision differs')
    run(['git','-C',ports,'diff','--quiet','HEAD','--'], timeout=60)
    # All root-evaluated framework code must be immutable to the build actor.
    for path in [ports/'Mk',ports/'Tools']:
        for entry in [path]+list(path.rglob('*')):
            trusted(entry)
    control = Path(tempfile.mkdtemp(prefix='bsd3-ecosystem49-',dir='/var/tmp')).resolve()
    os.chown(control,0,account.pw_gid)
    control.chmod(0o710)
    shutil.copyfile(__file__,control/'driver.py')
    shutil.copytree(repo/'packaging/freebsd',control/'recipes')
    shutil.copytree(repo/'tests',control/'tests')
    shutil.copyfile(repo/'tools/ci-ecosystem49.sh',control/'ci-ecosystem49.sh')
    for path in [control/'recipes',control/'tests']:
        freeze(path,account.pw_gid)
    for name in ['driver.py','ci-ecosystem49.sh']:
        os.chown(control/name,0,account.pw_gid)
        (control/name).chmod(0o640)
    actor = control/'actor'
    actor.mkdir(mode=0o700)
    os.chown(actor,account.pw_uid,account.pw_gid)
    output = repo/'dist/ecosystem49'
    require(not output.exists(), 'new ecosystem artifact directory required')
    output.mkdir(parents=True)
    trusted(output)
    provision_timezone(output)
    shutil.copyfile(control/'ci-ecosystem49.sh',output/'ci-ecosystem49.sh')
    shutil.copyfile(control/'tests/native-wayland-tests.py',output/'native-wayland-tests.py')
    (output/'checkout.txt').write_text(run(['git','-C',repo,'rev-parse','HEAD'],timeout=30))
    run(['pkg-config','--atleast-version=0.2.0.a3','bsd3-user-session'], timeout=20)
    reserved = set(BASE)
    for key,origin in COMPONENTS:
        source = control/'recipes'/key
        require((source/'Makefile').is_file(), 'candidate recipe missing: '+key)
        destination = ports/origin
        trusted(destination.parent)
        require(not destination.is_symlink(), 'canonical origin link refused')
        if destination.exists(): shutil.rmtree(destination)
        shutil.copytree(source,destination)
        freeze(destination,account.pw_gid)
        base = run(['make','-C',destination,'PORTSDIR='+str(ports),'-V','PKGBASE'],timeout=60).strip()
        require(re.fullmatch(r'[A-Za-z0-9_+.-]+',base), 'invalid package base')
        reserved.add(base)
    protected = {}
    for name in BASE:
        identity = installed(name)
        require(identity, 'preceding qualified provider missing: '+name)
        run(['pkg','lock','-y',name],timeout=30)
        protected[name] = identity
    require(protected['consolekit2'] == 'consolekit2-2.0.0_5 sysutils/consolekit2',
            'native consolekit2-2.0.0_5 required')
    (output/'initial-providers.json').write_text(json.dumps(protected,indent=2)+'\n')
    (output/'framework-overlay.diff').write_text(run(['git','-C',ports,'diff','HEAD','--'],timeout=60))
    # Compile only trusted, snapshotted BSD fixtures under root provisioning.
    gio = run(['pkg-config','--cflags','--libs','gio-unix-2.0'],timeout=20).split()
    run(['cc','-std=c11',control/'tests/native-inhibit.c','-o',control/'native-inhibit']+gio,timeout=60)
    login = run(['pkg-config','--cflags','--libs','bsd3-login'],timeout=20).split()
    gtk = run(['pkg-config','--cflags','--libs','gtk+-3.0'],timeout=20).split()
    run(['cc','-std=c11',control/'tests/native-session.c','-o',control/'native-session']+login,timeout=60)
    run(['cc','-std=c11',control/'tests/native-window.c','-o',control/'native-window']+gtk,timeout=60)
    for name in ['native-inhibit','native-session','native-window']:
        os.chown(control/name,0,account.pw_gid)
        (control/name).chmod(0o550)
    @contextlib.contextmanager
    def inhibited(directory):
        global CLOCK, INHIBITOR
        CLOCK = time.monotonic()+6600
        with (directory/'inhibitor.log').open('wb') as log:
            INHIBITOR = subprocess.Popen([control/'native-inhibit','7200'],stdout=log,
                stderr=subprocess.STDOUT,env={'PATH':'/usr/local/bin:/usr/bin:/bin'})
            try:
                until = time.monotonic()+5
                while 'BOUNDED_INHIBITOR_READY' not in (directory/'inhibitor.log').read_text():
                    require(INHIBITOR.poll() is None and time.monotonic()<until,'inhibitor readiness failure')
                    time.sleep(.1)
                listed = run(['gdbus','call','--system','--dest','org.freedesktop.ConsoleKit',
                    '--object-path','/org/freedesktop/ConsoleKit/Manager','--method',
                    'org.freedesktop.ConsoleKit.Manager.ListInhibitors'],timeout=5)
                require(re.search(r'\b'+str(INHIBITOR.pid)+r'\)',listed),'inhibitor PID absent')
                (directory/'inhibitors.txt').write_text(listed)
                yield
            finally:
                if INHIBITOR.poll() is None:
                    INHIBITOR.terminate()
                    try: INHIBITOR.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        INHIBITOR.kill(); INHIBITOR.wait(timeout=5)
                INHIBITOR = None
                CLOCK = float('inf')
    def preserve_providers():
        for name,identity in protected.items():
            require(installed(name)==identity,'custom provider replaced: '+name)
        run(['pkg','check','-s']+list(protected),timeout=180)
    toolchain = output/'papers-toolchain'
    toolchain.mkdir()
    with inhibited(toolchain):
        preserve_providers()
        provision_papers_rust(control,toolchain)
        preserve_providers()
    for key,origin in COMPONENTS:
        directory = output/key
        directory.mkdir()
        with inhibited(directory):
            preserve_providers()
            port = ports/origin
            metadata = ['make','-C',port,'PORTSDIR='+str(ports),'BATCH=yes','DEPENDS_SHOW_FLAVOR=yes']
            deps = direct_dependencies(metadata)
            missing = set()
            for line in deps:
                path,at,flavor = line.partition('@')
                dependency = Path(path)
                require(dependency.is_relative_to(ports),'dependency origin escaped pinned framework')
                trusted(dependency)
                for entry in dependency.rglob('*'): trusted(entry)
                query = ['make','-C',dependency,'PORTSDIR='+str(ports),'BATCH=yes']
                if at:
                    require(re.fullmatch(r'[A-Za-z0-9_+-]+',flavor),'invalid dependency flavor')
                    query += ['FLAVOR='+flavor]
                name = run(query+['-V','PKGBASE'],timeout=60).strip()
                require(re.fullmatch(r'[A-Za-z0-9_+.-]+',name),'invalid stock dependency name')
                if name not in reserved and not installed(name): missing.add(name)
            (directory/'missing-stock-dependencies.json').write_text(json.dumps(sorted(missing))+'\n')
            if missing: run(['pkg','install','-yA']+sorted(missing),log=directory/'stock-install.log',timeout=600)
            preserve_providers()
            home = actor/(key+'-home')
            home.mkdir(mode=0o700)
            os.chown(home,account.pw_uid,account.pw_gid)
            work = actor/key
            failure = None
            try:
                actor_run(control,account,['dbus-run-session','--',sys.executable,control/'driver.py',
                    '--worker',control,ports,work,key,origin],directory/'worker.log',home,timeout=6000)
            except BaseException as error:
                failure = error
            try:
                collect_tree(actor,Path(key)/'receipts',directory/'receipts',account.pw_uid)
            except BaseException as error:
                (directory/'collection-error.txt').write_text(type(error).__name__+': '+str(error)+'\n')
                if failure is None: failure = error
            if failure is not None:
                inventory(directory)
                raise failure
            receipt = directory/'receipts'
            complete_receipts(receipt,key)
            qa_ok(receipt/'build.log')
            expected = run(metadata+['-V','PKGNAME'],timeout=60).strip()
            require(re.fullmatch(r'[A-Za-z0-9_+.,-]+',expected),'invalid expected package name')
            actual = run(['pkg','query','-F',receipt/'package.pkg','%n-%v %o'],timeout=30).strip()
            require(actual == expected+' '+origin,'built package identity differs from trusted recipe')
            package = directory/(expected+'.pkg')
            (receipt/'package.pkg').rename(package)
            run(['pkg','-o','REPOS_DIR=/var/empty','add','-f',package],log=directory/'install.log',timeout=300)
            name = expected.rsplit('-',1)[0]
            protected[name] = installed(name)
            require(protected[name],'built provider not installed')
            run(['pkg','lock','-y',name],timeout=30)
            preserve_providers()
            (directory/'installed-packages.txt').write_text(run(['pkg','query','%n %v %o'],timeout=30))
            (directory/'providers.json').write_text(json.dumps(protected,indent=2)+'\n')
            actor_run(control,account,['/bin/rm','-rf',work,home],directory/'cleanup.log',actor,timeout=180)
            inventory(directory)
    graph = output/'session-graph'
    graph.mkdir()
    with inhibited(graph):
        preserve_providers()
        for system in [False,True]:
            runtime = actor/('graph-system' if system else 'graph-isolated')
            runtime.mkdir(mode=0o700)
            os.chown(runtime,account.pw_uid,account.pw_gid)
            args = [control/'native-session','dbus-run-session','--','env',
                'BSD3_TEST_GNOME_APPS=yes','BSD3_TEST_SETTINGS_UI=yes',
                'BSD3_TEST_GNOME_APPS_EXTRA=nautilus,gnome-console,gnome-maps,epiphany,gnome-calendar,papers,evolution',
                'BSD3_EXPECT_GSD_VERSION='+run(['pkg','query','%v','gnome-settings-daemon'],timeout=30).strip(),
                sys.executable,control/'tests/native-gnome49-graph.py',control/'native-window']
            if system: args += ['--system-autostart']
            failure = None
            try:
                actor_run(control,account,args,graph/(runtime.name+'.log'),runtime,timeout=600)
            except BaseException as error: failure = error
            try:
                collect_tree(actor,runtime.relative_to(actor)/'dist',graph/runtime.name,account.pw_uid)
            except BaseException as error:
                if failure is None: failure = error
            if failure is not None: raise failure
    (output/'qualification.json').write_text(json.dumps({'native_pipeline':'passed this run',
        'hardware':'NOT TESTED','greeter':'NOT TESTED','network_services':'NOT QUALIFIED'},indent=2)+'\n')
    sums = {str(path.relative_to(output)):digest(path) for path in output.rglob('*') if path.is_file()}
    (output/'SHA256SUMS.json').write_text(json.dumps(sums,sort_keys=True,indent=2)+'\n')


def self_test():
    require(len(COMPONENTS)==23 and len({o for k,o in COMPONENTS})==23,'component table collision')
    order = [k for k,o in COMPONENTS]
    for a,b in [('gnome-online-accounts49','evolution-data-server49'),
                ('evolution-data-server49','evolution49'),('evolution-data-server49','gnome-calendar49'),
                ('libshumate49','gnome-maps49'),('nautilus49','gnome-console49'),('nautilus49','papers49')]:
        require(order.index(a)<order.index(b),'consumer scheduled before provider')
    previous_run = run
    observed = []
    try:
        def fake_query(argv,**kwargs):
            observed.append(argv)
            return '/ports/devel/example@py312\n/ports/devel/example@py312\n/ports/net/other\n'
        globals()['run'] = fake_query
        require(direct_dependencies(['make','-C','/ports/app']) ==
                ['/ports/devel/example@py312','/ports/net/other'], 'direct origin/flavor deduplication failed')
        require(observed == [['make','-C','/ports/app','build-depends-list','run-depends-list']],
                'recursive dependency enumeration returned')
    finally:
        globals()['run'] = previous_run
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        log = root/'log'
        log.write_text('compiler succeeded\nError: actual stage QA failure\n')
        try: qa_ok(log)
        except RuntimeError: pass
        else: raise AssertionError('exit-zero QA Error accepted')
        localtime, zoneinfo = root/'localtime', root/'zoneinfo'
        previous_run = run
        calls = []
        try:
            def fake_tzsetup(argv,**kwargs):
                calls.append(argv)
                localtime.write_bytes(b'TZif offline test fixture')
                zoneinfo.write_text('UTC\n')
                Path(kwargs['log']).write_text('offline fixture; no system provisioning\n')
            globals()['run'] = fake_tzsetup
            provision_timezone(root,localtime,zoneinfo)
            require(calls == [['/usr/sbin/tzsetup','-s','UTC']], 'absent timezone not provisioned once')
            before = localtime.read_bytes()
            calls.clear()
            provision_timezone(root,localtime,zoneinfo)
            require(not calls and localtime.read_bytes() == before,'configured timezone modified')
            localtime.unlink()
            localtime.symlink_to('existing-but-missing-configured-zone')
            provision_timezone(root,localtime,zoneinfo)
            require(not calls and localtime.is_symlink(),'dangling configured timezone overwritten')
        finally:
            globals()['run'] = previous_run
        meson = root/'meson.json'
        okay = [{'name':'a','result':'OK','returncode':0},{'name':'b','result':'OK','returncode':0}]
        meson.write_text('\n'.join(json.dumps(record) for record in okay)+'\n')
        meson_results(meson,2)
        for result in ['SKIP','EXPECTEDFAIL','UNEXPECTEDPASS','FAIL','TIMEOUT']:
            bad = [okay[0],dict(okay[1],result=result)]
            meson.write_text('\n'.join(json.dumps(record) for record in bad)+'\n')
            try: meson_results(meson,2)
            except RuntimeError: pass
            else: raise AssertionError('nonpassing Meson result accepted: '+result)
        meson.write_text(json.dumps(okay[0])+'\n')
        try: meson_results(meson,2)
        except RuntimeError: pass
        else: raise AssertionError('missing actual Meson execution accepted')
        junit = root/'ctest.xml'
        junit.write_text('<testsuite tests="2"><testcase name="a" status="run"/><testcase name="b"/></testsuite>')
        ctest_results(junit,2)
        for bad_case in ['<testcase name="b"><skipped/></testcase>',
                         '<testcase name="b"><failure/></testcase>',
                         '<testcase name="b"><error/></testcase>',
                         '<testcase name="b" status="notrun"/>',
                         '<testcase name="a"/>']:
            junit.write_text('<testsuite><testcase name="a"/>'+bad_case+'</testsuite>')
            try: ctest_results(junit,2)
            except RuntimeError: pass
            else: raise AssertionError('nonpassing/duplicate CTest execution accepted')
        junit.write_text('<testsuite skipped="1"><testcase name="a"/><testcase name="b"/></testsuite>')
        try: ctest_results(junit,2)
        except RuntimeError: pass
        else: raise AssertionError('CTest skipped summary accepted')
        receipt = root/'receipts'; receipt.mkdir()
        required = ['source.tar.xz','recipe.tar.gz','upstream-distfiles.tar.xz',
                    'package.pkg','provenance.json','locked-crates.tar.xz','Cargo.lock']
        (receipt/'complete.json').write_text(json.dumps(required))
        for name in required: (receipt/name).write_bytes(b'fixture')
        complete_receipts(receipt,'papers49')
        for name in required:
            path = receipt/name
            path.write_bytes(b'')
            try: complete_receipts(receipt,'papers49')
            except RuntimeError: pass
            else: raise AssertionError('empty required source receipt accepted: '+name)
            path.unlink()
            try: complete_receipts(receipt,'papers49')
            except FileNotFoundError: pass
            else: raise AssertionError('missing required source receipt accepted: '+name)
            path.write_bytes(b'fixture')
        (receipt/'complete.json').write_text(json.dumps(required[:-2]))
        try: complete_receipts(receipt,'papers49')
        except RuntimeError: pass
        else: raise AssertionError('actor removed required Cargo source contract')
        actor = root/'actor'; actor.mkdir()
        (actor/'log').write_text('safe failed-build log')
        copy_output(actor,'log',root/'copy',os.getuid())
        (actor/'link').symlink_to(log)
        try: copy_output(actor,'link',root/'bad',os.getuid())
        except OSError: pass
        else: raise AssertionError('unsafe actor link collected')
    print('PASS: dependency order, actual results, timezone preservation, QA and artifact boundaries; native pipeline not run')


if __name__ == '__main__':
    def interrupted(_sig,_frame): raise KeyboardInterrupt()
    signal.signal(signal.SIGTERM,interrupted)
    if sys.argv[1:] == ['--self-test']: self_test()
    elif sys.argv[1:2] == ['--worker']:
        worker(Path(sys.argv[2]),Path(sys.argv[3]),Path(sys.argv[4]),sys.argv[5],sys.argv[6])
    else:
        root_main(Path.cwd())

PY
python3.12 "$program" "$@"
