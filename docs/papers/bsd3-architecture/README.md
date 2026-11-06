# BSD3 architecture paper

A 13-page technical study by Denis Lemire explaining the login, session/device
control and rendering paths, security boundaries, port fixes and validation.
The v0.1 draft records automatic graphical startup as verified and combined
Wi-Fi acceptance as open. It does not claim later GNOME support or measured
comparative performance. Follow the dated startup record for subsequent status.

Build with Python3 and ReportLab:

```
python3 docs/papers/bsd3-architecture/build.py output/pdf/bsd3-architecture.pdf
```

The final PDF was rendered and visually checked across all13 pages, including
its split rendering/control diagram. Text extraction and page-bound checks pass.
Original build code is BSD-2-Clause. The paper cites primary project source and
records; dependency licenses remain unchanged. Raw cores, network identifiers,
credentials and full configuration exports are excluded.
