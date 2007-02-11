#pragma once

char *SVNRevision = "$WCREV$";
char *SVNDate     = "$WCDATE$";

#if $WCMODS?1:0$
#error Source is modified
#endif
