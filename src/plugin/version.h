// Version of both DLLs, shared by the C++ code (tf3snap.h) and the version resources (tf3snap.rc,
// ../proxy/proxy.rc).  Keep the two lines in step.  The installer (release/tools/install.ps1) recognises our
// DLLs by the resource's ProductName and shows ProductVersion.
#pragma once
#define TF3SNAP_VERSION "0.3.2"
#define TF3SNAP_VERSION_RC 0, 3, 2, 0
// Renamed in 0.3.0 (was "TF3 Snap Options"): the installer accepts both names, see $LegacyProducts there.
#define TF3SNAP_PRODUCT "Better Snap Option"
