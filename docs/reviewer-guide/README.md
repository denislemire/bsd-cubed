# BSD³ reviewer guide

The [PDF](../../output/pdf/bsd-cubed-reviewer-guide.pdf) is a source-pinned
architecture and validation guide, authored under Denis Lemire's name for review.
It separates private-bus tests, native VM results, exact CI artifacts and physical
acceptance. It is a review draft, not a desktop-readiness certificate.

The implementation reference is commit
8bfa868f3a1c485954d35b9be28b2347b2e4bf30. The November 5 final reusable runner
passed on stable/15 with the recorded local package; CI36 and exact-package STABLE
qualification now pass. The signed beta3 is published and its downloads verified;
physical acceptance remains a separate gate. Appendix D identifies the current
release and external cold-test correction. Historical CI33 hashes in Appendix A
must not be used as the final SDDM candidate manifest.

## Sources and build

Edit `guide.md` and render with `build.py` from any directory:

```sh
python3 -m pip install reportlab
python3 docs/reviewer-guide/build.py
```

The renderer uses ReportLab and TrueType fonts, not the desktop's runtime SDK.
Set `BSD3_DOC_FONT_DIR` to a directory containing `DejaVuSans.ttf`,
`DejaVuSans-Bold.ttf`, `DejaVuSansMono.ttf` and either the Noto Serif regular,
bold and italic faces or the equivalent DejaVu Serif faces. The default search
also supports the author's bundled document runtime and common Linux DejaVu
paths. Fonts are embedded in the PDF; font files are not vendored here.

`build-manifest.json` records the implementation pin, source/PDF hashes,
renderer version and selected fonts. A checksum proves artifact identity, not
correctness. Review the prose against the pinned implementation before publishing.

## Review the result

```sh
mkdir -p tmp/pdfs
pdftoppm -scale-to 1200 -png output/pdf/bsd-cubed-reviewer-guide.pdf tmp/pdfs/bsd3-review
```

Inspect every page after edits, especially the cover, contents, diagrams, long
references and table breaks. The generated PNGs are ignored. The final PDF is a
committed review artifact. The source guide, renderer and original diagrams are
BSD-2-Clause; linked dependencies retain their own licenses.
