#pragma once

#define SVNRevision $WCREV$
#define SVNRevisionStr "$WCREV$"
char *SVNDate     = "$WCDATE$";

// This prevents the build unless all files are checked in
#if $WCMODS?1:0$
error PLease commit all files before creating a release build
#endif
