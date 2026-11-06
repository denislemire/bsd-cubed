#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
"""Build the BSD3 architecture paper with ReportLab. No network access required."""
from pathlib import Path
import sys
from xml.sax.saxutils import escape
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.utils import simpleSplit
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, PageBreak, Table, TableStyle, Flowable
from reportlab.lib.pagesizes import letter

INK = colors.HexColor('#173247')
TEAL = colors.HexColor('#087F8C')
MUTED = colors.HexColor('#536370')
PALE = colors.HexColor('#F1F5F6')
styles = {
 'title': ParagraphStyle('title',fontName='Helvetica-Bold',fontSize=30,leading=35,textColor=INK,spaceAfter=20),
 'deck': ParagraphStyle('deck',fontName='Helvetica',fontSize=15,leading=22,textColor=MUTED,spaceAfter=16),
 'h1': ParagraphStyle('h1',fontName='Helvetica-Bold',fontSize=21,leading=26,textColor=INK,spaceAfter=16),
 'h2': ParagraphStyle('h2',fontName='Helvetica-Bold',fontSize=12,leading=16,textColor=TEAL,spaceBefore=12,spaceAfter=7),
 'body': ParagraphStyle('body',fontName='Times-Roman',fontSize=11.5,leading=16,spaceAfter=10),
 'small': ParagraphStyle('small',fontName='Helvetica',fontSize=9,leading=13,textColor=MUTED,spaceAfter=8),
 'code': ParagraphStyle('code',fontName='Courier',fontSize=9,leading=14,spaceAfter=9),
}

def p(text, style='body'):
 return Paragraph(text,styles[style])

class Stack(Flowable):
 def __init__(self, rows):
  Flowable.__init__(self); self.rows=rows; self.width=504; self.height=len(rows)*48+12
 def draw(self):
  c=self.canv
  for i,(title,detail) in enumerate(self.rows):
   y=self.height-42-i*48
   c.setFillColor(PALE); c.roundRect(0,y,504,38,4,fill=1,stroke=0)
   c.setFillColor(INK); c.setFont('Helvetica-Bold',10); c.drawString(12,y+23,title)
   c.setFillColor(MUTED); c.setFont('Helvetica',9); c.drawString(12,y+9,detail)
   if i<len(self.rows)-1:
    c.setStrokeColor(TEAL); c.line(252,y,252,y-10); c.line(249,y-7,252,y-10); c.line(255,y-7,252,y-10)

class Split(Flowable):
 def __init__(self):
  Flowable.__init__(self); self.width=504; self.height=228
 def draw(self):
  c=self.canv
  def box(x,y,w,title,detail):
   c.setFillColor(PALE); c.roundRect(x,y,w,42,4,fill=1,stroke=0)
   c.setFillColor(INK); c.setFont('Helvetica-Bold',10); c.drawString(x+10,y+26,title)
   c.setFillColor(MUTED); c.setFont('Helvetica',9); c.drawString(x+10,y+11,detail)
  def line(x,y,x2,y2):
   c.setStrokeColor(TEAL); c.line(x,y,x2,y2)
  box(0,182,504,'Applications','Surfaces travel to the compositor over Wayland')
  box(0,126,504,'GNOME Shell / Mutter','The compositor draws the desktop and routes input')
  line(252,182,252,168)
  line(126,126,126,112); line(378,126,378,112)
  box(0,62,240,'Rendering path','EGL/GBM; DRM/KMS presentation')
  box(264,62,240,'Session and device control','BSD3 compatibility -> ConsoleKit')
  line(126,62,126,48); line(378,62,378,48)
  box(0,0,504,'Native FreeBSD kernel and devices','i915 graphics, input descriptors, virtual-terminal controls')

sections = [
('1', 'What we actually built', [
 ('p','BSD3 is a narrow compatibility project that lets an existing GNOME desktop use native FreeBSD session and device services. Its daemon is named bsddd: BSD Desktop Daemon, with three consecutive d characters. The original code is C11 under BSD-2-Clause. It does not become PID 1, parse arbitrary systemd units, or turn FreeBSD into a Linux system. [1]'),
 ('h','The compatibility gap'),
 ('p','A desktop compositor needs more than a drawing API. It must identify the logged-in user and seat, obtain controlled access to input and graphics devices, respond when another virtual terminal becomes active, and release devices when the session ends. GNOME components commonly speak interfaces associated with systemd-logind. FreeBSD has different facilities underneath. The project translates a selected part of that contract rather than importing the entire service manager.'),
 ('h','Three deliverables, not one magic daemon'),
 ('p','First, a small library implements selected sd-login discovery functions over ConsoleKit. Second, a privileged daemon offers selected login1 D-Bus objects and controlled device operations. Third, consumer port patches and a strict session launcher connect those interfaces to Mutter and the actual FreeBSD login path. ConsoleKit itself also required focused native device and virtual-terminal corrections. [1-3]'),
 ('p','That distinction explains why owning the expected D-Bus name did not immediately produce a desktop. The compositor still needed its native backend enabled, a working device lifecycle, correct GPU enumeration and a valid frame-presentation path. A shim that merely returns plausible answers would hide failures rather than solve them.'),
 ('h','Scope of the result'),
 ('p','The demonstrated desktop is GNOME 47, using Mutter 47.10 and the published beta5 package set. Physical input, usable rendering, Web pages and normal logout have been observed. Later GNOME generations have additional session-startup dependencies; this result does not establish their support. An automatically displayed login screen is also a different result from a fully qualified daily-use system. [4,5]'),
 ]),
('2', 'The stack, from the screen down', [
 ('p','GNOME Shell provides the visible desktop. Mutter, integrated into that process, is both its window manager and its Wayland compositor. Applications submit surfaces; Mutter combines them and arranges presentation on the monitor. The kernel driver controls the actual graphics hardware. BSD3 is involved in obtaining and managing the right access, not in drawing each window.'),
 ('split', None),
 ('h','Two paths must be kept separate'),
 ('p','The control path answers questions such as who owns a session and whether it may use a device. The rendering path submits graphics work and presents frames. Once Mutter has an appropriate descriptor, BSD3 is not a frame-by-frame pixel proxy. This is one reason a small compatibility layer can coexist with a responsive native desktop. It is an architectural explanation, not a measured overhead claim.'),
 ('p','SDDM is the login manager in this deployment. Its greeter currently uses Xorg, while the selected user session is native Wayland. A greeter using Xorg does not make the logged-in GNOME session an Xorg desktop or a nested Wayland demonstration. [4,6]'),
 ]),
('3', 'A login, step by step', [
 ('p','At boot, FreeBSD rc starts the system message bus, then bsddd, then SDDM. The native rcorder result was checked on both the STABLE VM and the laptop. SDDM presents a login form and authenticates through PAM, the system framework for authentication and session setup. Starting the greeter automatically does not bypass the password. [6,7]'),
 ('h','Register a real local user session'),
 ('p','The SDDM session command runs bsd3-session as the ordinary user. This launcher checks the real controlling terminal and uses ConsoleKit to register or validate a local Wayland user session. It checks the user ID, virtual-terminal number, local/active state, session type and class. It also verifies that the runtime directory belongs to the user and has mode 0700. An SSH login is not silently relabeled as a local graphical session. [1]'),
 ('h','Create the application bus and start GNOME'),
 ('p','The adapter creates a per-session D-Bus with dbus-run-session and invokes SDDM\'s Wayland session script. The persistent profile wrapper supplies FreeBSD resource paths, a separate settings/data/cache location, and the verified WebKit compatibility setting. It then executes gnome-session-binary in its built-in manager mode. GNOME 47 still provides that mode; this is not a replacement implementation of the newer GNOME user-service graph. [1,4]'),
 ('h','Why locality is a security property'),
 ('p','Being the same Unix user is not enough to grant control of a physical seat. The process must belong to the claimed session, that session must correspond to the local terminal, and the controller relationship must remain valid. Otherwise a background or remote process could impersonate the foreground graphical session. The checks are designed to fail when this evidence is missing.'),
 ('p','The session launcher also supervises descendants with native process-reaper facilities. Cleanup has an owned scope: it must not indiscriminately kill other sessions belonging to the same user. A terminal, a login session, and a process tree are related concepts, but they are not interchangeable. [1,3]'),
 ]),
('4', 'What the two compatibility interfaces do', [
 ('h','A library is an in-process API'),
 ('p','libbsd3-login.a provides selected sd-login functions with the expected calling conventions and ownership/error contracts. For example, a caller may ask which session contains a process or which graphical session belongs to a user. The answers come from typed ConsoleKit calls. Returned strings are owned according to the API contract; errors use negative errno-style results. [1]'),
 ('p','The installed header is namespaced as bsd3/sd-login.h. Consumer ports explicitly opt in. The project does not install a counterfeit systemd header tree or a libsystemd.pc file claiming broad compatibility. Linking the BSD3 definitions alongside another library exporting the same sd_* symbols is unsupported.'),
 ('h','D-Bus is a cross-process protocol'),
 ('p','The daemon owns org.freedesktop.login1 on the system bus and exports only the supported object/method subset. D-Bus describes typed method arguments and authenticates connections. This is separate from the library ABI: one boundary is a function call inside a process, the other is a message exchanged with a service. A consumer may use both. [1,8]'),
 ('h','Names and credentials have lifetimes'),
 ('p','A well-known bus name can move to a replacement process after a restart. A unique bus name identifies a particular connection. The bridge pins backend ownership during authorization and control. It must not authorize against one ConsoleKit instance and then accidentally execute a privileged operation against a different instance that acquired the same name.'),
 ('p','The privileged service refuses to replace an existing login1 owner. Its rc readiness check confirms that the exact daemon PID owns the name, rather than accepting any service with that name. Unsupported operations return errors. An implemented subset is useful precisely because its boundary is explicit; a successful probe is not proof of an operational desktop. [1]'),
 ]),
('5', 'Device ownership without a root desktop', [
 ('p','A file descriptor is a process-held reference to an open kernel object. For a compositor, descriptors can represent an input device or a DRM graphics device. Passing a descriptor lets a trusted broker open the appropriate object without running all of GNOME as root. D-Bus carries descriptors as ancillary Unix-socket data; the integer in the message is an index into that attached list, not an open descriptor by itself. [1,8]'),
 ('h','Authenticate the original caller'),
 ('p','The bridge obtains the original frontend connection\'s Unix UID and PID from the message bus, and checks ownership and PID-to-session membership through ConsoleKit. It does not trust a UID supplied in a request. Merely forwarding as a privileged daemon would substitute the daemon\'s credentials for the caller\'s, so authorization is performed explicitly at the frontend. [1]'),
 ('h','Keep controllers independent'),
 ('p','Each accepted frontend controller receives its own backend bus connection. ConsoleKit tracks that unique backend sender. When the frontend disconnects, the bridge closes that controller\'s backend connection, allowing the backend to release its devices. Sharing one backend connection among every desktop client would merge their lifetimes and make cleanup ambiguous. Forced takeover is refused.'),
 ('h','Validate transport and ordering'),
 ('p','Device replies and resume messages are checked for the expected typed body and descriptor bundle. Resume descriptors are directed only to the authenticated controller. Malformed messages or impossible state ordering invalidate control rather than preserving a guessed ownership state. These are operational correctness rules as well as security boundaries. [1]'),
 ('p','The design does not make every input node world-readable and does not launch the desktop as root. ConsoleKit is the selected device/session backend for this path. An installed seatd service is not evidence that seatd owns the same devices; competing ownership models must not be inferred from service presence. [2,3]'),
 ]),
('6', 'Leaving and returning to a virtual terminal', [
 ('p','FreeBSD virtual terminals let the console switch among sessions. A compositor must relinquish meaningful access when its session becomes inactive. Sending a friendly pause notification alone is insufficient: an old input descriptor that remains usable would still grant access to another session\'s input. [3]'),
 ('diagram',[('Active local session','Controller holds a usable input/device descriptor'),('Switch away','Pause is signaled; backend revokes the old input descriptor'),('Inactive session','Old input ioctl fails; inactive switch request is rejected'),('Return and resume','Controller receives a fresh descriptor; old one stays revoked'),('Release or disconnect','Backend releases control and revokes the remaining input handle')]),
 ('h','What was actually measured'),
 ('p','The STABLE VM test used a real controlling terminal and kernel device operations. For inactive input, an ioctl returned ENODEV and poll reported POLLHUP. On return, a fresh descriptor worked while the old descriptor remained revoked. Final release revoked the fresh one. The test deliberately did not read input events. [3]'),
 ('h','The ConsoleKit console-selection fix'),
 ('p','ConsoleKit was not tracking the active VT because it lacked a usable console descriptor. Typed ioctl probes distinguished /dev/console, /dev/consolectl and /dev/ttyv0. Only the appropriate ttyv0 control supported the required active-VT query in this test. A focused FreeBSD fallback opened that terminal, and a failed console lookup correctly returned -1. That restored backend tracking without pretending a device path implied the right ioctl contract. [2,3]'),
 ]),
('7', 'Why compiling Mutter was not enough', [
 ('h','Restore the native backend deliberately'),
 ('p','The FreeBSD Mutter port needed its native backend restored and adapted. That path controls real KMS/DRM display output and native input. A headless backend is useful for session and Wayland-client tests, but it does not exercise a laptop\'s physical GPU presentation. Native CI and a working headless window therefore preceded, rather than replaced, hardware trials. [2,4]'),
 ('h','First hardware boundary: no GPUs found'),
 ('p','The initial strict GNOME login authenticated and registered correctly, but GPU discovery returned no cards. FreeBSD\'s udev-compatible stack exposed the DRM character device without the Linux-style seat tag and DEVTYPE metadata Mutter expected. A comparison using the actual enumerator found a card without the tag filter and none with it. [2]'),
 ('p','The correction is FreeBSD-specific: avoid requiring that absent tag, and infer an absent DRM device type only for strictly validated cardN or renderDN character devices in the expected DRM namespace. Explicit wrong types remain rejected, and Linux enumeration keeps its existing behavior. This is an adaptation to real metadata, not an unrestricted device search.'),
 ('h','Second boundary: display creation is not presentation'),
 ('p','Beta4 then found the Intel i915 card, selected atomic KMS and created a GBM renderer. It still crashed on the first frame. This separated device discovery from presentation and falsified the idea that the earlier discovery fix alone established desktop readiness. Each milestone removed one failure boundary; the next could only be observed once the earlier one was crossed. [4]'),
 ('p','Resource-path configuration also mattered: FreeBSD\'s installed cursor theme was outside Mutter\'s bundled default search locations. Adding the existing icon path corrected a configuration mismatch. The cursor warning was not treated as evidence of the independent first-frame crash.'),
 ]),
('8', 'The first-frame crash and its regression', [
 ('p','The beta4 crash was preserved before replacing the affected package: the core, all mapped object files, matching fully patched source, package identities and SHA256 manifests were retained locally. The old core was replayed against immutable old objects. A current library with the same filename would not be a valid substitute. Raw memory stayed out of Git and the paper. [4]'),
 ('h','The decisive observation'),
 ('p','The stack reached a null call underneath the EGL region-swap path. The port-applied stage code forced a nonempty region for a full redraw. Without buffer age, that selected a region-swap callback unavailable to this native presentation path. Disassembly and the actual patched source agreed with that mechanism; stripped symbols limited precise source-line attribution.'),
 ('h','The smallest controlled change'),
 ('p','For native stages only, the overlay restores the upstream full-redraw empty-region behavior. That selects the native buffer-swap path. Non-native stages and builds without native support retain the previous port workaround. Mutter\'s package revision becomes 47.10_5. The change does not delete a workaround globally or assume its original rationale was irrelevant. [2,4]'),
 ('h','A test that distinguishes old and new'),
 ('p','The regression executes the actual region-construction and swap-dispatch branches with a deliberately missing region callback. The beta4 source fails that fixture; the correction passes native/non-native build modes and the relevant redraw, buffer-age, clipping and shadow-buffer combinations. Native CI 45 and independent STABLE qualification then produced the beta5 candidate.'),
 ('p','The released package was downloaded, checksum-verified and installed on hardware. Denis then observed a usable GNOME Wayland desktop and responsive input. That physical result adds evidence the headless fixture could never supply. It does not establish every GPU, monitor configuration or suspend path.'),
 ]),
('9', 'A desktop includes logout and applications', [
 ('h','Why the first logout looked frozen'),
 ('p','The initial trial wrapper deleted its disposable profile in an EXIT trap while applications could still write into it. Cleanup reported a nonempty directory and returned status1. SDDM treats that helper status as an authentication error and does not restart the display on that path. GNOME had shut down, leaving its last frame visible. The original pre-trap GNOME exit code was not separately captured, which limits a stronger causal claim. [5]'),
 ('p','The persistent wrapper removes that cleanup trap and executes GNOME directly. Settings are kept in a private persistent profile, and the session\'s real status reaches the login manager. Two subsequent normal logouts returned to a freshly started graphical greeter, with successful helper exits in SDDM\'s log. A timer-induced return was not counted as a logout success.'),
 ('h','Why Web needed a separate workaround'),
 ('p','Konqueror rendered webpages while GNOME Web initially displayed blank content. GTK reported DMA-BUF texture import disabled at compile time. Force-shared-memory transport alone did not resolve startup; a local browser core showed an abort through graphics preparation and WebKit acceleration setup. The installed package integrity check passed, but its precise runtime/source correspondence was not fully established. [5]'),
 ('p','WEBKIT_DISABLE_COMPOSITING_MODE=1 allowed the browser to stay running and render pages. The persistent session exports that WebKit setting. Denis then confirmed an ordinary fresh login and application-launcher Web launch worked. This disables WebKit application compositing for compatibility; it does not disable Mutter\'s Intel GPU compositor or the browser sandbox.'),
 ('p','Application services remain a separate acceptance surface. Earlier bus-activated search and preview processes reported missing display connections. Sound has now been reported working, but lock/unlock, suspend, long-duration use and the wider application set still need their own evidence. [5]'),
 ]),
('10', 'A reproducible FreeBSD deployment', [
 ('h','Packages first, hardware second'),
 ('p','The workflow builds and tests on native FreeBSD CI in the EhWS KubeVirt lab. The existing CI image is15.1-based, so a separate15-STABLE VM qualifies the exact candidate packages against the intended running system. A package building successfully under one image does not establish runtime compatibility with another. [2,6]'),
 ('p','Published beta5 has signed source/tag provenance, the native Mutter package, matching source/recipe artifacts and checksums. The unchanged BSD3 and ConsoleKit dependencies use the exact already released CI 36 bytes, not a silent rebuild with the same version. Hardware receives the downloaded release assets, verified before installation. Modified dependencies retain their own licenses and matching source distribution. [2,4]'),
 ('h','Turn the working session into a bootable system'),
 ('p','The desktop environment is cloned from the working STABLE system rather than assembled with a new kernel. FreeBSD\'s existing rc.conf switches enable dbus, bsddd and sddm. The optional helper verifies actual service ordering and backs up configuration first. The one-shot boot keeps the former normal environment as the persistent fallback until combined acceptance passes. [6,7]'),
 ('p','A boot-return guard is tested through its real rc.d invocation, and a separate observer demonstrates live output before reboot. Once remote access, service ownership and identities are verified, the guard is acknowledged. Final adoption must remove temporary test controls and select the intended default explicitly. A timer cannot recover a kernel that has frozen completely.'),
 ('h','Preserve the qualified package set'),
 ('p','The custom BSD3, ConsoleKit and Mutter packages are locked against accidental replacement by stock upgrades. A later qualified rollout must deliberately unlock and replace that set. The retained boot environment provides another recovery route; it is not an independent off-machine backup. [6]'),
 ]),
('11', 'Evidence, performance and the next boundary', [
 ('h','An acceptance ladder'),
 ('p','Private-bus tests check message types, permissions and lifetimes. Native package checks verify staged paths and dependencies. The STABLE VM exercises actual VT/input revocation and complete GNOME session startup, window updates and logout. Hardware then verifies real GPU presentation and user-visible behavior. Finally, reboot acceptance checks automatic services together with networking. Passing one rung is not a substitute for the next. [1-6]'),
 ('h','What feels fast, and what is measured'),
 ('p','Denis reports that this GNOME Wayland environment feels dramatically more responsive than the prior stock15.1 experience. Native Intel KMS/GBM presentation is verified. There is no controlled graphics benchmark comparing the old and new systems, so neither a speedup factor nor a single-cause performance claim is warranted. Kernel, driver, userland and desktop changes can all affect an uncontrolled comparison.'),
 ('h','Networking must pass in the same boot'),
 ('p','The automatic graphical boot retained byte-identical enhanced Wi-Fi components and enabled HE support, yet the user reported poor Wi-Fi. Live inspection found association on2.4 GHz HT/20 rather than the previously qualified5 GHz HE/40 path. A fresh scan sees 5 GHz candidates. A temporary local frequency filter and reassociation then reported no carrier; the user chose a recovery reboot. This has not established band selection as the complete cause. Combined acceptance remains unresolved, and matching binaries do not guarantee matching runtime behavior. Permanent default promotion is held pending that investigation. [6]'),
 ('h','What remains outside the claim'),
 ('p','The project is not a full logind or libsystemd replacement. Newer GNOME session-manager service graphs, broad power operations, every multi-session case, long-term recovery behavior and other hardware remain separate work. Kernel Wi-Fi development, console true color and a GNOME network menu are separate projects. The useful outcome is a bounded, inspectable native path with failures that can be explained and retested.'),
 ('p','The next meaningful completion is a repeatable ordinary boot with the intended graphics, network mode, input, browser, sound and clean logout together, followed by a retained recovery option. This paper describes that engineering boundary rather than declaring the larger desktop finished.'),
 ]),
]

refs = [
 ('1','BSD3 source, reviewed baseline 9d1adb8; src/login.c, bsddd.c, control.c, seats.c and session.c.','https://github.com/denislemire/bsd-cubed/tree/9d1adb83da2830c3af71920ab4ff0590217fcc7a/src'),
 ('2','BSD3 native port overlays: explicit consumer opt-in, ConsoleKit device patches and Mutter native backend.','https://github.com/denislemire/bsd-cubed/tree/b61cbb6313084e02dd28f77ff6eb0cb9cf7883db/packaging/freebsd'),
 ('3','Native VT/device regression, recorded 2026-11-05; real input revocation and recovery boundaries.','https://github.com/denislemire/bsd-cubed/blob/9d1adb83da2830c3af71920ab4ff0590217fcc7a/docs/native-vt-regression-2026-11-05.md'),
 ('4','Mutter presentation correction, PR19 and published v0.1.0-beta5.','https://github.com/denislemire/bsd-cubed/pull/19'),
 ('5','Physical desktop acceptance and persistent profile integration, PR20.','https://github.com/denislemire/bsd-cubed/pull/20'),
 ('6','Automatic startup and combined acceptance record; companion source in this review branch.','docs/automatic-startup-2026-11-05.md'),
 ('7','FreeBSD Handbook: managing services and the booting process.','https://docs.freebsd.org/en/books/handbook/config/#configtuning-rcd'),
 ('8','D-Bus specification: authenticated connections, bus names and Unix file-descriptor transport.','https://dbus.freedesktop.org/doc/dbus-specification.html'),
]

def footer(c, doc):
 c.saveState(); w,h=letter
 c.setStrokeColor(colors.HexColor('#D6E0E4')); c.line(54,h-39,w-54,h-39)
 c.setFillColor(MUTED); c.setFont('Helvetica',8)
 c.drawString(54,h-30,'BSD3 / NATIVE FREEBSD DESKTOP ARCHITECTURE')
 c.drawRightString(w-54,h-30,'Denis Lemire')
 c.drawString(54,30,'Technical study draft / 5 November 2026 / v0.1')
 c.drawRightString(w-54,30,str(doc.page)); c.restoreState()

def build(target):
 target=Path(target); target.parent.mkdir(parents=True,exist_ok=True)
 doc=SimpleDocTemplate(str(target),pagesize=letter,rightMargin=54,leftMargin=54,topMargin=58,bottomMargin=54,title='BSD3: A Native FreeBSD Path to GNOME on Wayland',author='Denis Lemire',subject='Architecture, failure analysis and reproducible desktop integration')
 story=[Spacer(1,68),p('BSD<super>3</super>','title'),p('A native FreeBSD path<br/>to GNOME on Wayland','title'),p('Architecture, failure analysis, and<br/>reproducible desktop integration','deck'),Spacer(1,20),p('Denis Lemire','h2'),p('Aspiring independent contributor and FreeBSD enthusiast','small'),p('5 November 2026 | Technical study draft v0.1','small'),Spacer(1,30),p('Abstract','h2'),p('This study explains a small C11 compatibility bridge that connects selected GNOME session and device expectations to native FreeBSD services. It follows the complete path from password login to controlled device access and GPU presentation, then analyzes the failures that separated a compiled prototype from a usable GNOME 47 Wayland desktop. Native VM evidence, signed package provenance and physical observations are distinguished. Automatic graphical startup is demonstrated; combined Wi-Fi acceptance remains under investigation in this draft.'),p('Reading guide: start with sections 1-3 for the overall model, sections 4-8 for the implementation and failure analysis, and sections 9-11 for daily-use behavior and evidence limits.','small'),p('Original project code: BSD-2-Clause. Dependencies retain their licenses. Developed with AI coding assistance; the human author remains responsible for review and factual claims. This is a technical study, not a peer-reviewed publication.','small')]
 for number,title,content in sections:
  story.append(PageBreak()); story.append(p(number+' / '+title,'h1'))
  for typ,text in content:
   if typ=='diagram': story += [Stack(text),Spacer(1,10)]
   elif typ=='split': story += [Split(),Spacer(1,10)]
   else: story.append(p(text,'h2' if typ=='h' else 'body'))
 story += [PageBreak(),p('References and reproducibility','h1'),p('Repository links may require access to the private project. Numbered citations identify source and evidence, not independent third-party review. Earlier dated notes retain their original pending statements; later physical and startup records supersede those specific status claims.','small')]
 for n,title,url in refs:
  story.append(p('<b>['+n+']</b> '+escape(title)))
  if url.startswith('https:'): story.append(p('<link href="'+escape(url)+'" color="#087F8C">'+escape(url)+'</link>','small'))
  else: story.append(p(escape(url),'small'))
 story += [p('Rebuild this paper','h2'),p('python3 build.py /path/to/bsd3-architecture.pdf','code'),p('Requires ReportLab. Sources contain no credentials, packet captures, raw memory or identifying configuration exports. Raw evidence remains protected on the test machine. The study is intentionally bounded by the package/source revisions and observations described here.','small')]
 doc.build(story,onFirstPage=footer,onLaterPages=footer)
 print(target)

if __name__=='__main__':
 build(sys.argv[1] if len(sys.argv)>1 else 'bsd3-architecture.pdf')
