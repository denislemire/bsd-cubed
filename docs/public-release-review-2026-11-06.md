# Public release preparation — November6,2026

Status: in progress. GitHub repository visibility remains private. The EhWS
beta2 catalogue, nine packages and corresponding source/recipe bundles are
publicly readable and were verified through anonymous downloads plus an isolated
native FreeBSD package client. Alpha2 is a private GitHub prerelease whose exact
packages passed the recorded hardware trial.

## Source and history check

Gitleaks v8.30.1 was built from its published Go module and run locally over all
Git refs with redacted reports kept under gitignored `captures/private/`.
The scan examined72commits/~1.16MB of historical content. One generic API-key
match was the documented SHA256 of tested API source in a historical session
validation record. Manual review confirmed the text identifies a source digest,
not an authentication value. Only that exact historical fingerprint is listed
in `.gitleaksignore`; no broad rule was disabled. The repeated history scan had
no remaining findings. This is a secret-scan result, not a guarantee that every
possible private datum has been identified.

GitHub issue/PR bodies, issue/review comments and release descriptions were
exported only to the private review directory and scanned locally with redaction.
That metadata scan had no findings. Tracked files contained neither the known
real network name nor MAC-address patterns in documents. These checks do not
replace review of the final public contents.

## License and disclosure review

The README now points to actual accepted GNOME49 hardware results and distinguishes
that package set from the next ecosystem candidate. `THIRD_PARTY.md` separates
original BSD2 code from GNOME GPL/LGPL consumers, ConsoleKit, Dinit, bsdisks and
inherited FreeBSD ports material. Source hashes, native recipes and consumer
licenses remain required release assets.

Before changing repository visibility, finish review of tracked/current new files,
GitHub issue/PR/release descriptions and history for identifying captures or
private operational material; verify the final patch/source/license inventory;
and present the concrete reviewed publication state to the owner. No Git history
rewrite or repository visibility change is performed by this record.

## Active package work and priority

The next GNOME49 ecosystem candidate includes Settings49.9, SettingsDaemon49.1
and the independently versioned apps from the pinned GNOME49 manifest. New
consumer overlays remain experimental until their native manifest/build/runtime
checks and combined release CI pass. The accepted alpha2 laptop environment is
preserved during VM development.

The owner explicitly queued the Wayland-native greeter after the remaining
GNOME49 packages. The current SDDM greeter still uses Xorg. Xwayland compatibility
for older apps is a separate dependency from the Xorg server/greeter.

## Ecosystem candidate audit update

Signed candidate `5a680405523106a12212d4d7717f97166698ad14` adds the manually
verified 23 ecosystem ports and their serial CI controller. CI61 is running;
this does not qualify an alpha3 release or hardware installation. The source
export scan found only the previously reviewed documented source digest and
EDS's Kerberos-disable build option. Neither is a credential.
The 78-commit all-ref history scan found that same EDS option in its original
source-preparation commit. Only its exact historical fingerprint is added to
`.gitleaksignore`; the generic API-key rule remains enabled.

The existing draft PR description now reflects Session49.3, the native ecosystem
results, the corrected browser ownership path and the actual alpha2 hardware
acceptance. It explicitly keeps new CI/release/hardware work open. No repository
visibility change or history rewrite has occurred.

The same harmless option text was also detected when repeated in this audit
and its ignore-file explanation. Those two exact historical fingerprints are
reviewed as well; the current explanation avoids repeating the scanner trigger.
