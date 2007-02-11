#pragma once

char *SVNRevision = "$WCREV$";
char *SVNDate     = "$WCDATE$";

// This prevents the build unless all files are checked in
#if $WCMODS?1:0$
#error Source is modified
#endif
