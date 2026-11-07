# Public release preparation — November 6, 2026

Historical note. GitHub visibility was private at the time of writing. Later
status is in [STATUS.md](STATUS.md).

Gitleaks v8.30.1 was run locally over Git refs; redacted reports stay under
gitignored `captures/private/`. One generic API-key match was a documented
SHA256 of tested API source, not a credential. Only that exact historical
fingerprint is listed in `.gitleaksignore`. A secret scan is not a guarantee
that every private datum has been identified.

`THIRD_PARTY.md` separates original BSD-2-Clause code from GNOME GPL/LGPL
consumers, ConsoleKit, Dinit, bsdisks and inherited FreeBSD ports material.
Source hashes, native recipes and consumer licenses remain required release
assets.

No Git history rewrite is performed by this record. History still contains
earlier drafts; scrubbing it is a separate, explicit decision.
