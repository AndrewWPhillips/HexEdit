// BGPreview.cpp : implements preview of graphics files from memory - rendered in a background thread (part of CHexEditDoc)

#include "stdafx.h"
#include "HexEdit.h"

#include "HexEditDoc.h"
#include "HexEditView.h"
#include <FreeImage.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//---------------------------------------------------------------------------------------
// These 4 funcs are used with FreeImage for in-memory processing of bitmap data
static unsigned __stdcall fi_read(void *buffer, unsigned size, unsigned count, fi_handle handle)
{
	CHexEditDoc *pDoc = (CHexEditDoc *)handle;

	long addr = pDoc->GetPreviewAddress();
	size_t got = pDoc->GetData((unsigned char *)buffer, size*count, addr);
	pDoc->SetPreviewAddress(addr + got);
	return got;
}

static unsigned __stdcall fi_write(void *buffer, unsigned size, unsigned count, fi_handle handle)
{
	return 0;
}

static int __stdcall fi_seek(fi_handle handle, long offset, int origin)
{
	CHexEditDoc *pDoc = (CHexEditDoc *)handle;
	FILE_ADDRESS addr = 0;
	switch (origin)
	{
	case SEEK_SET:
		addr = offset;
		break;
	case SEEK_CUR:
		addr = pDoc->GetPreviewAddress() + offset;
		break;
	case SEEK_END:
		addr = (long)pDoc->length() + offset;
		break;
	}
	if (addr > LONG_MAX)
		return -1L;

	pDoc->SetPreviewAddress((long)addr);
	return 0;
}

static long __stdcall fi_tell(fi_handle handle)
{
	CHexEditDoc *pDoc = (CHexEditDoc *)handle;

	return pDoc->GetPreviewAddress();
}

FreeImageIO fi_funcs = { &fi_read, &fi_write, &fi_seek, &fi_tell };

void CHexEditDoc::OnTest()
{
	// Test to see if we can get the file type
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromHandle(&fi_funcs, this);
	TRACE("Image format is %d\n", fif);
}
