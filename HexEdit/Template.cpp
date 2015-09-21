// Template.cpp : part of CHexEditDoc class for handling templates
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include "stdafx.h"
#include "HexEdit.h"
#include "HexEditDoc.h"
#include "SaveDffd.h"
#include "DFFDGlobal.h"
#include "DFFDMisc.h"
#include "Dialog.h"
#include "Mainfrm.h"
#include "HexFileList.h"

// Hide macros so that we can use std::min, std::max
#ifdef max
#undef max
#undef min
#endif


void CHexEditDoc::CheckSaveTemplate()
{
	// If the template for this file has been modified ask the user if they want to save it
	if (ptree_ != NULL && ptree_->IsModified())
	{
		CString path_name = ptree_->GetFileName();

		char fname[_MAX_FNAME], ext[_MAX_EXT];
		_splitpath(path_name, NULL, NULL, fname, ext);

		CString file_name = CString(fname) + ext;

		CSaveDffd dlg;
		dlg.message_.Format("The template \"%s\" has been modified.\n\n"
							"Do you want to save it?", file_name);

		if (file_name.CompareNoCase(FILENAME_DEFTEMPLATE) == 0)
		{
			if (TaskMessageBox("Save Template?", dlg.message_, MB_YESNO) != IDNO)
			{
				CHexFileDialog dlgFile("TemplateFileDlg", HIDD_FILE_TEMPLATE, FALSE, "xml", NULL, 
									OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_SHOWHELP | OFN_NOCHANGEDIR,
									"Template (XML) files - *.xml)|*.xml|All Files (*.*)|*.*||");

				// Set up the title of the dialog
				dlgFile.m_ofn.lpstrTitle = "Save Template File";
				dlgFile.m_ofn.lpstrInitialDir = theApp.xml_dir_;

				if (dlgFile.DoModal() == IDOK)
				{
					ptree_->Save(dlgFile.GetPathName());
					theApp.GetXMLFileList();        // rebuild the list with the new file name in it
					_splitpath(dlgFile.GetPathName(), NULL, NULL, fname, ext);
					xml_file_num_ = theApp.FindXMLFile(fname);
				}
			}
		}
		else
		{
			switch (dlg.DoModal())  // IDOK = Save, IDYES = SaveAs, IDCANCEL = Don't save
			{
			case IDOK:
				ptree_->Save();
				break;
			case IDYES:
				CHexFileDialog dlgFile("TemplateFileDlg", HIDD_FILE_TEMPLATE, FALSE, "xml", path_name, 
									OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_SHOWHELP | OFN_NOCHANGEDIR,
									"Template (XML) files - *.xml)|*.xml|All Files (*.*)|*.*||");

				// Set up the title of the dialog
				dlgFile.m_ofn.lpstrTitle = "Save Template File";

				if (dlgFile.DoModal() == IDOK)
				{
					ptree_->Save(dlgFile.GetPathName());
					theApp.GetXMLFileList();        // rebuild the list with the new file name in it
					_splitpath(dlgFile.GetPathName(), NULL, NULL, fname, ext);
					xml_file_num_ = theApp.FindXMLFile(fname);
				}
				break;
			}
		}
	}
}

void CHexEditDoc::OnDffdRefresh()
{
	update_needed_ = true; // force update
	CheckUpdate();
}

void CHexEditDoc::OnUpdateDffdRefresh(CCmdUI* pCmdUI)
{
	//pCmdUI->Enable(update_needed_);  // With advent of getint/bool/string the user may want to rerun the template at any time
	CHexEditView *pv = GetBestView();
	pCmdUI->Enable(pv != NULL && pv->pdfv_ != NULL);  // enable if there's a data format view
}

// New template - set template to "default" and open the template in active window
void CHexEditDoc::OnDffdNew()
{
	OpenDataFormatFile("default");
	ScanFile();
	CDFFDHint dffdh;
	UpdateAllViews(NULL, 0, &dffdh);

	CHexEditView *pv = GetBestView();
	if (pv != NULL)
		pv->ShowDffd();
}

void CHexEditDoc::OnDffdOpen(UINT nID)
{
	ASSERT(nID - ID_DFFD_OPEN_FIRST < DFFD_RESERVED);
	OpenDataFormatFile(theApp.xml_file_name_[nID - ID_DFFD_OPEN_FIRST]);
	ScanFile();
	CDFFDHint dffdh;
	UpdateAllViews(NULL, 0, &dffdh);

	CHexEditView *pv = GetBestView();
	if (pv != NULL)
		pv->ShowDffd();
}

void CHexEditDoc::OnDffdSave()
{
	if (ptree_->GetFileName().Right(11).CompareNoCase(FILENAME_DEFTEMPLATE) != 0)
		ptree_->Save();
	else
		OnDffdSaveAs();
}

void CHexEditDoc::OnDffdSaveAs()
{
	CHexFileDialog dlgFile("TemplateFileDlg", HIDD_FILE_TEMPLATE, FALSE, "xml", ptree_->GetFileName(),
							OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_SHOWHELP | OFN_NOCHANGEDIR,
							"Template (XML) files - *.xml)|*.xml|All Files (*.*)|*.*||");

	// Set up the title of the dialog
	dlgFile.m_ofn.lpstrTitle = "Save Template File";

	if (dlgFile.DoModal() == IDOK)
	{
		// Save to the new template file
		ptree_->SetFileName(dlgFile.GetPathName());
		ptree_->Save();

		theApp.GetXMLFileList();        // rebuild the list with the new file name in it

		// Remember this file as the current template
		xml_file_num_ = theApp.FindXMLFile(dlgFile.GetFileTitle());
	}
}

void CHexEditDoc::OnUpdateDffdSave(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(ptree_ != NULL && !ptree_->Error());
}

// Is editing of the active template allowed?
BOOL CHexEditDoc::DffdEditMode()
{
	return dffd_edit_mode_;
}

// Toggle editing
void CHexEditDoc::OnEditMode()
{
	dffd_edit_mode_ = !DffdEditMode();
	// We need to rebuild the tree to include/exclude edit mode stuff
	ScanFile();
	CDFFDHint dffdh;
	UpdateAllViews(NULL, 0, &dffdh);

	CHexEditView *pv = GetBestView();
	if (pv != NULL)
		pv->ShowDffd();
}

void CHexEditDoc::OnUpdateEditMode(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(ptree_ != NULL && !ptree_->Error());
	pCmdUI->SetCheck(DffdEditMode());
}

void CHexEditDoc::OnDffdOptions()
{
	if (df_elt_.size() == 0)
	{
		ASSERT(0);
		return;
	}

	CDFFDGlobal dlg(&df_elt_[0], GetBestView());
	if (dlg.DoModal() == IDOK && dlg.IsModified())
	{
		CSaveStateHint ssh;
		UpdateAllViews(NULL, 0, &ssh);
		ScanFile();
		// Update the display as how some things are shown has changed
		CDFFDHint dffdh;
		UpdateAllViews(NULL, 0, &dffdh);
		CRestoreStateHint rsh;
		UpdateAllViews(NULL, 0, &rsh);
	}
}

void CHexEditDoc::OnUpdateDffdOptions(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(DffdEditMode() && df_init_);
}

void CHexEditDoc::CheckUpdate()
{
	if (update_needed_)
	{
		CSaveStateHint ssh;
		UpdateAllViews(NULL, 0, &ssh);
		ScanFile();  // rescan doc (for tree views) since it has changed
		//CRefreshHint rh;
		//UpdateAllViews(NULL, 0, &rh);
		CDFFDHint dffdh;
		UpdateAllViews(NULL, 0, &dffdh);
		CRestoreStateHint rsh;
		UpdateAllViews(NULL, 0, &rsh);
//        update_needed_ = false;   // This is done in ScanFile()
	}
}

// Opens the XML file asociated with this file
// The XML file describes the data contained within the file.
// It opens the passed file.  If NULL it opens the previously associated XML
// file, if any, then looks for a file based on the file extension.
void CHexEditDoc::OpenDataFormatFile(LPCTSTR data_file_name /*=NULL*/)
{
	CheckSaveTemplate();   // allow user to save changes to current template (if any)

	int saved_file_num = xml_file_num_;
	xml_file_num_ = -1;  // Default to no file found

	CString filename;

	if (data_file_name == NULL)
	{
		// Work out what data format (XML) file to use
		int recent_file_index = -1;
		CHexFileList *pfl = theApp.GetFileList();

		if (pfile1_ != NULL)
			recent_file_index = pfl->GetIndex(pfile1_->GetFilePath());

		if (recent_file_index != -1)
		{
			// Get data format file from file settings for this file
			filename = pfl->GetData(recent_file_index, CHexFileList::FORMAT);
//            if (!filename.IsEmpty() && _access(theApp.xml_dir_+filename, 0) == -1)
//                filename.Empty();

			// Find this filename in the list of current XML files
			if (filename.CompareNoCase("default") != 0)
				xml_file_num_ = theApp.FindXMLFile(filename);
		}

		// If none found above then use default for this file extension
		if (xml_file_num_ == -1 && pfile1_ != NULL)
		{
			// Get file extension and change "." to "_"
			CString ss = pfile1_->GetFileName();
			if (ss.ReverseFind('.') != -1)
			{
				filename = CString("_") + ss.Mid(ss.ReverseFind('.')+1);

				// Check that a file exists for this extension
//                if (_access(theApp.xml_dir_+filename, 0) == -1)
//                    filename.Empty();            // No file so don't use this file name
				xml_file_num_ = theApp.FindXMLFile(filename);
			}
		}

		// Lastly use "default" .XML if present
		if (xml_file_num_ == -1)
		{
			filename = "default";

			// Find it in the list
			xml_file_num_ = theApp.FindXMLFile(filename);
		}
	}
	else
	{
		ASSERT(data_file_name != NULL);
		// Check if specified file name is there
//        if (_access(theApp.xml_dir_+data_file_name, 0) != -1)
//            filename = data_file_name;
		xml_file_num_ = theApp.FindXMLFile(data_file_name);
		if (xml_file_num_ != -1)
			filename = data_file_name;
	}

	// If we got a file name to use then use it
	// Note xml_file_num_ should not be -1 now but this cann occur if the "default"
	// template was not found or a template file has suddenly disappeared.
	if (xml_file_num_ != -1)
	{
		ASSERT(!filename.IsEmpty());

		CXmlTree *saved_ptree = ptree_;

		CString ss;

		ptree_ = new CXmlTree(theApp.xml_dir_ + filename + CString(".xml"));
		if (ptree_->Error())
		{
			ss.Format("XML parse error in file \"%s.XML\"\nLine %ld:%s\n%s", 
					  filename, long(ptree_->ErrorLine()),
					  ptree_->ErrorLineText(), ptree_->ErrorMessage());
			TaskMessageBox("Template Error", ss);
			delete ptree_;

			// Restore previous file
			ptree_ = saved_ptree;
			xml_file_num_ = saved_file_num;
		}
		else if (ptree_->GetDTDName() != "binary_file_format")
		{
			ss.Format("Invalid DTD used with XML file \"%s\"", ptree_->GetFileName());
			TaskMessageBox("Template Error", ss);
			delete ptree_;

			// Restore previous file
			ptree_ = saved_ptree;
			xml_file_num_ = saved_file_num;
		}
		else if (saved_ptree != NULL)
		{
			// Destroy previous file
			delete saved_ptree;
		}
	}
	else
		xml_file_num_ = saved_file_num;

	// If we are using the default template the user will more than likely want to change it so allow editing
	if (filename == "default")
		dffd_edit_mode_ = 1;
	else if (ptree_ != NULL && !ptree_->Error() && dffd_edit_mode_ == 1)
	{
		// Turn off edit mode if template says to
		CString ss;
		ss = ptree_->GetRoot().GetAttr("allow_editing");
		dffd_edit_mode_ = (ss.CompareNoCase("false") != 0);
	}
}

// Returns the (first) element that contains the passed address
// OR returns one past the last element if not found.
// [Note: This does a linear search but is very fast - it replaces
// find_address() (= binary search) which failed when JUMPs are used.]
size_t CHexEditDoc::FindDffdEltAt(FILE_ADDRESS addr)
{
	int ii, end = df_address_.size();

	for (ii = 0; ii < end; ++ii)
	{
		if (df_type_[ii] > DF_DATA)
		{
			FILE_ADDRESS aa = df_address_[ii];
			if (addr >= aa && addr < aa + df_size_[ii])
				return ii;
		}
	}
	return end;  // not found
}

void  CHexEditDoc::FindDffdEltsIn(FILE_ADDRESS start, FILE_ADDRESS end, std::vector<boost::tuple<FILE_ADDRESS, FILE_ADDRESS, COLORREF> > & retval)
{
	retval.clear();

	int ii, last_ii = df_address_.size();

	for (ii = 0; ii < last_ii; ++ii)
	{
		if (df_type_[ii] > DF_DATA)
		{
			FILE_ADDRESS ss = df_address_[ii];
			FILE_ADDRESS ee = ss  + df_size_[ii];
			if (ss < end && ee > start)
			{
				CString str = df_elt_[ii].GetAttr("color");
				if (!str.IsEmpty())
				{
					COLORREF clr = (DWORD)(strtoul(str, NULL, 16) & 0xffFFFF);
					retval.push_back(boost::tuple<FILE_ADDRESS, FILE_ADDRESS, COLORREF>(ss, ee, clr));
				}
			}
		}
	}
}


BOOL CHexEditDoc::ScanInit()
{
	if (!df_init_)          // only scan once
		return ScanFile();
	else
		return !df_mess_.IsEmpty();
}

// This function scans the (data) file based on info from the (XML data format) file.
// It loads info. into member variables ready for display in a data format tree view.
// On successful analysis TRUE is returned. If the data does not match what is expected
// from data format file info then FALSE is returned (df_mess_ describes the error):

// Various error conditions are handled as:
// - if the data is supposed to be past the physical EOF then address == -1
// - if the data element is not present (empty for or false if) then size == 0
// - if a data element is incorrect (domain error) then its size is -ve

// Note: ScanFile() is not normally called immediately after OpenDataFormatFile() to
// give MSXML (via CXmlTree) a chance to load and process the XML file asynchronously.
BOOL CHexEditDoc::ScanFile()
{
	df_init_ = TRUE;

	if (ptree_ == NULL || ptree_->Error())
	{
		df_mess_ = "No data description file is available";
		return FALSE;
	}

	CWaitCursor wait;

	// Init progress bar
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	m_last_checked = clock();

	df_mess_.Empty();
	max_indent_ = 1;

	df_type_.clear();
	df_address_.clear();
	df_size_.clear();
	df_extra_.clear();
	df_indent_.clear();
	df_elt_.clear();
	df_info_.clear();
	df_enum_.clear();
	ASSERT(ptree_->GetRoot().GetName() == "binary_file_format");

	default_byte_order_ = ptree_->GetRoot().GetAttr("default_byte_order");
	default_read_only_ = ptree_->GetRoot().GetAttr("default_read_only");
	default_char_set_ = ptree_->GetRoot().GetAttr("default_char_set");

	// Add info for root element
	df_type_.push_back(DF_FILE);            // type representing all of file
	df_address_.push_back(0);               // address is start of file
	df_size_.push_back(0);                  // size is not yet known (filled in later)
	df_extra_.push_back(-1);                // arrays only
	df_indent_.push_back(1);                // represents root of tree view
	df_elt_.push_back(ptree_->GetRoot());   // root element in CXmlTree
	df_info_.push_back(ExprStringType());
	in_jump_ = 0;                           // we are not in any JUMPs
	bits_used_ = 0;                         // we have not seen a bitfield yet
	last_size_ = 0;                         // store 0 when bits_used_ == 0

	FILE_ADDRESS size_tmp;
	CHexExpr ee(this);
	try
	{
		add_branch(ptree_->GetRoot(), 0, 2, ee, size_tmp); // process whole tree (getting size)
		ASSERT(bits_used_ == 0);                           // Bitfields should have been terminated

		df_size_[0] = size_tmp;
		if (!df_mess_.IsEmpty())
		{
			// Display any error message
			TaskMessageBox("Template Scan Error", df_mess_);
			df_address_[0] = -1;
		}
		else
		{
			CString dispStr = ptree_->GetRoot().GetAttr("expr");
			if (!dispStr.IsEmpty())
			{
				// Generate display "expr" for the whole template (displayed next to root elt name)
				int expr_ac;                            // Last node accessed by expression

				if (dispStr.Find('{') == -1)
					dispStr = CString("{") + dispStr + "}";  // This makes evaluate handle the errors

				CHexExpr::value_t tmp = Evaluate(dispStr, ee, 0, expr_ac);

				if (tmp.typ == CHexExpr::TYPE_STRING)
					df_info_[0] = *tmp.pstr;
			}
		}

		// Check that we're at EOF
		if (size_tmp < length_)
		{
			TaskMessageBox("Template Warning", "Data past expected end of file");

			df_type_.push_back(DF_EXTRA);           // represents extra unexpected data
			df_address_.push_back(size_tmp);        // address is where EOF expected
			df_size_.push_back(length_ - size_tmp); // size is distance to real EOF
			df_extra_.push_back(-1);                // used for arrays only
			df_indent_.push_back(1);                // this is the only other tree element at root
			df_elt_.push_back(ptree_->GetRoot());   // what else can we use here?
			df_info_.push_back(ExprStringType("Expected EOF"));
		}
	}
	catch (const char *mess)
	{
		(void)mess;
		TRACE1("Caught %s in ScanFile\n", mess);
	}

	mm->Progress(-1);  // Turn off progress now
	update_needed_ = false;
	return TRUE;
}

// Adds a complete branch of the display tree and returns the size (bytes) of all the data of the tree.

// The return value indicates the last vector element accessed in expression used in this branch.
// This is used for arrays to determine if its elements are of fixed size or can vary.  If all
// nodes accessed within expressions of a branch are above the current element then the branch
// cannot vary in size.  Expressions occur in the "test" attribute of the "if" tag and the "count"
// and "stop_test" attributes of the "for" tag (arrays).

// If element sizes of an array can vary then we have to read and check each element of the array
// so that we know the total size of the array and the address of anything after the array.  If 
// element sizes are fixed we only display the first few elements of the array and can work out
// the array size as the numbers of elements times the element size.

// parent = the node of the parent whose child(ren) we are processing
// addr = current address in file or -1 if we are not processing data from the file
// ind = current indent level used in the tree view
// ee = expression evaluator including resolution of indentifier values
// returned_size = size passed back to caller so it knows the size of all children
// child_num = child to process or -1 to process all children (used for IF/ELSE processing)
// ok_bitfield_at_end = false to terminate bitfield or true if bitfield can continue (ie array of bitfields)

int CHexEditDoc::add_branch(CXmlTree::CElt parent, FILE_ADDRESS addr, unsigned char ind,
							CHexExpr &ee, FILE_ADDRESS &returned_size,
							int child_num /* = -1*/, bool ok_bitfield_at_end /* = false */)
{
	ASSERT(DF_LAST < 128);

	// Keep track of the maximum indent
	if (ind > max_indent_)
	{
		ASSERT(ind < 255);
		max_indent_ = ind;
	}

	// Now do the subtree
	CXmlTree::CElt elt;
	if (child_num == -1)
		elt = parent.GetFirstChild();
	else
		elt = parent.GetChild(child_num); // Just do the specified child

	int last_ac = -1;                   // Default to no nodes accessed in expressions
	returned_size = 0;

	int ii;
	while (!elt.IsEmpty())
	{
		ii = df_address_.size();                // Index of entry we will now add

		// Add new entry in all arrays arrays (except df_type_) even if they are adjusted later
		// Note that one entry in these vectors corresp. to a row in the grid (tree display)
		df_address_.push_back(addr);
		df_size_.push_back(0);                  // Not yet known
		df_extra_.push_back(-1);                // Only used for array (below)
		df_indent_.push_back(ind);              // Indentation in tree
		df_elt_.push_back(elt);                 // Store a ptr to XML elt in case we need other stuff
		df_info_.push_back(ExprStringType());

		// Now also add the type (df_type_) and handle sub-elements (dep on the type)
		CString elt_type = elt.GetName();

		// First see if we need to terminate any preceding bitfield if the next field is not a bitfield
		if (bits_used_ > 0 && (elt_type != "data" || atoi(elt.GetAttr("bits")) == 0))
		{
			// Move to the end of this bitfield storage unit
			ASSERT(last_size_ != 0);
			returned_size += last_size_;
			if (addr != -1)
				addr += last_size_;
			df_address_[ii] = addr;
			// Indicate that there is now no bitfield in effect
			bits_used_ = 0;
			last_size_ = 0;
		}

		if (elt_type == "define_struct")
		{
			// If in edit mode show it in the tree so it can be edited
			if (DffdEditMode())
			{
				// Add sub node but use address of -1 so that file is not accessed
				df_type_.push_back(DF_DEFINE_STRUCT);

				FILE_ADDRESS size_tmp;              // Ignored since we stay at the same address after return
				last_ac = std::max(last_ac, add_branch(elt, -1, ind+1, ee, size_tmp));
				df_size_[ii] = size_tmp;            // Store size of struct
			}
			else
			{
				// Don't show struct definitions when not in edit mode
				//df_type_.pop_back(); - not pushed so don't pop it
				df_address_.pop_back();
				df_size_.pop_back();
				df_extra_.pop_back();
				df_indent_.pop_back();
				df_elt_.pop_back();
				df_info_.pop_back();
			}
		}
		else if (elt_type == "use_struct")
		{
			// Find in map and use it
			df_type_.push_back(DF_USE_STRUCT);

			// Find the define_struct that is to be used
			CString ss = elt.GetAttr("type_name");
			CXmlTree::CElt def_elt = elt.GetOwner()->GetRoot().GetFirstChild();
			for ( ; !def_elt.IsEmpty() && def_elt.GetName() == "define_struct"; ++def_elt)
				if (ss == def_elt.GetAttr("type_name"))
					break;

			if (def_elt.IsEmpty())
			{
				CString ss;
				ss.Format("Structure definition for \"%s\" was not found", elt.GetAttr("type_name"));
				HandleError(ss);
			}
			else if (addr != -1)
			{
				// Add sub-elements of the STRUCT (only if present else we get inf. recursion)
				FILE_ADDRESS size_tmp;
				last_ac = std::max(last_ac, add_branch(def_elt, addr, ind+1, ee, size_tmp));
				df_size_[ii] = size_tmp;            // size of struct is size of all contained elements
				returned_size += size_tmp;          // keep track of size of elts for our parent
				addr += size_tmp;                   // keep track of where we are now in the file

				CString dispStr = elt.GetAttr("expr");
				if (dispStr.IsEmpty())
					dispStr = def_elt.GetAttr("expr");
				if (!dispStr.IsEmpty())
				{
					int expr_ac;                            // Last node accessed by expression

					if (dispStr.Find('{') == -1)
						dispStr = CString("{") + dispStr + "}";  // This makes evaluate handle the errors

					CHexExpr::value_t tmp = Evaluate(dispStr, ee, ii, expr_ac);
					//if (expr_ac > last_ac) last_ac = expr_ac;  // Don't update last_ac as the value is only for display

					if (tmp.typ == CHexExpr::TYPE_STRING)
						df_info_[ii] = *tmp.pstr;
				}
			}
		}
		else if (elt_type == "struct")
		{
			df_type_.push_back(DF_STRUCT);

			// Add sub-elements of the STRUCT
			FILE_ADDRESS size_tmp;
			last_ac = std::max(last_ac, add_branch(elt, addr, ind+1, ee, size_tmp));
			df_size_[ii] = size_tmp;            // size of struct is size of all contained elements
			returned_size += size_tmp;          // keep track of size of elts for our parent

			// If we have not passed EOF (addr of -1 indicates we have run out of data to match the template)
			if (addr != -1)
				addr += size_tmp;               // keep track of where we are now in the file

			// Only evaluate if present (since likely to eval members that are invalid)
			if (addr != -1)
			{
				CString dispStr = elt.GetAttr("expr");
				if (!dispStr.IsEmpty())
				{
					int expr_ac;                            // Last node accessed by expression

					if (dispStr.Find('{') == -1)
						dispStr = CString("{") + dispStr + "}";  // This makes Evaluate handle the errors

					CHexExpr::value_t tmp = Evaluate(dispStr, ee, ii, expr_ac);
					//if (expr_ac > last_ac) last_ac = expr_ac;  // Don't update last_ac as the value is only for display

					if (tmp.typ == CHexExpr::TYPE_STRING)
						df_info_[ii] = *tmp.pstr;
				}
			}
		}
		else if (elt_type == "for")
		{
			df_type_.push_back(DF_FORF);        // Default to an array with fixed size elements (until we find different)

			CString strCount = elt.GetAttr("count");
			CString strTest = elt.GetAttr("stop_test");

			int elts_ac;                            // Last node accessed by elements
			int expr_ac;                            // Last node accessed by count or stop_test expression
			FILE_ADDRESS elt_size;                  // How big is one element of array
			int num_elts = INT_MAX;                 // Elts in the array (default to fill to EOF)
			if (addr == -1)
				num_elts = 0;
			else if (!strCount.IsEmpty())
			{
				CHexExpr::value_t tmp = ee.evaluate(strCount, ii, expr_ac);
				if (expr_ac > last_ac) last_ac = expr_ac;   // Keep track of what elts we access (in case parent is FOR)
				if (tmp.typ == CHexExpr::TYPE_NONE)
				{
					df_size_[ii] = 0;

					// Only show error message if we don't expect it (if past EOF we expect it)
					ASSERT(addr != -1);
					HandleError(CString(ee.get_error_message()) + "\nin \"count\" of \"for\".");
					num_elts = 0;
				}
				else if (tmp.typ != CHexExpr::TYPE_INT || tmp.int64 < 0)
				{
					df_size_[ii] = 0;

					HandleError("The \"count\" attribute expression of the\n"
								  "\"for\" tag must yield a +ve integer result.\n");
					num_elts = 0;
				}
				else
				{
					// We have found how many elts in this (instance of this) FOR
					num_elts = int(tmp.int64);
				}
			}

			if (num_elts > 0)
			{
				ASSERT(addr != -1);

				// Now get the sub-element of the FOR
				elts_ac = add_branch(elt, addr, ind+1, ee, elt_size, -1, true);
				// ASSERT(elt_size > 0 || bits_used_ > 0); // Can be zero with zero-sized string/none

				if (elts_ac > ii || !strTest.IsEmpty())
				{
					// Since the expression used within the FOR access data of the FOR (via "this")
					// each element can vary in size since the data might vary
					last_ac = ii+1;                 // Remember that array elts are accessed
					df_type_[ii] = DF_FORV;         // Signal that array elts are var size
				}

				// One or more elts in the array so get them
				// Note: we do not display all elements for arrays with fixed size elements since
				// they can often be huge and we know where the array ends since we know the number
				// of elts and their size.  "display_elts" is the number of elts to show.
				int display_elts = df_type_[ii] == DF_FORV ? num_elts : std::min(theApp.max_fix_for_elts_, num_elts);
				FILE_ADDRESS array_size;        // Total size of this array

				array_size = elt_size;          // We have already added one elt (above)
				int jj = ii + 1;                // Start at first array elt

				// Create more array elts until we have displayed them all (or as many as
				// we are going to display for fixed elt arrays) or we are told to stop (strTest)
				int elt_num;
				for (elt_num = 1; elt_num < display_elts; ++elt_num)
				{
					// Check if we need to stop at EOF
					if (strCount.IsEmpty() &&
						( /* addr == -1 || */
						 (df_type_[ii] != DF_FORV && addr + array_size + elt_size + last_size_ > length_) ||
						 (df_type_[ii] == DF_FORV && addr + array_size + last_size_ >= length_)
						)
					   )
					{
//                        ASSERT(strTest.IsEmpty());
						// If no count and not test we stop at EOF
						break;
					}
					// Check if we need to stop due to the stop_test string
					else if (!strTest.IsEmpty())
					{
						// Evaluate the stop_test expression and handle any errors
						CHexExpr::value_t tmp = ee.evaluate(strTest, jj, expr_ac);
						if (expr_ac > last_ac) last_ac = expr_ac; // Keep track of what elts we access (in case parent is also FOR)
						if (tmp.typ == CHexExpr::TYPE_NONE)
						{
							df_size_[ii] = 0;

							// Only display error if not expected (ie not past EOF)
							ASSERT(addr != -1);
							HandleError(CString(ee.get_error_message()) + "\nin \"stop_test\" of \"for\".");
							break;
						}
						else if (tmp.typ != CHexExpr::TYPE_BOOLEAN)
						{
							df_size_[ii] = 0;

							HandleError("The \"stop_test\" attribute expression of the\n"
										  "\"for\" tag must yield a boolean result.\n");
							break;
						}
						else if (tmp.boolean)
							break;              // stop_test returned true so break from loop
					}

					jj = df_address_.size();         // Move to start of next array elt

					 // Process next element of array (FOR)
					ASSERT(addr != -1);
					(void)add_branch(elt, addr + array_size, ind+1, ee, elt_size, -1, true);
					//ASSERT(elt_size > 0 || bits_used_ > 0);
					array_size += elt_size;
				}

//                if (elts_ac <= ii && !strCount.IsEmpty() && elt_num < num_elts)
				// If this is a fixed size elt array and there are more elts (but not displayed)
				if (df_type_[ii] == DF_FORF && elt_num >= display_elts && elt_num < num_elts)
				{
					ASSERT(elt_num == display_elts);

					int data_bits;

					// For array of bitfields "elt_size" is not valid (can actually contain zero or
					// size of bitfield storage unit) so we have to do calcs differently.
					if (elt.GetFirstChild().GetName() == "data" &&
						// elt.GetFirstChild().GetAttr("type") == "int" &&
						(data_bits = atoi(elt.GetFirstChild().GetAttr("bits"))) > 0)
					{
						// We need to work out the number of bitfield storage units needed for
						// the whole array and the number in the "elt_num" elts already done.
						// At this point jj is the last array element hence
						// df_size_[jj] contains the size of the bitfield storage unit,
						ASSERT(df_size_[jj] == 1 || df_size_[jj] == 2 || df_size_[jj] == 4 || df_size_[jj] == 8);
						int total_units, done_units;
						if (elt.GetFirstChild().GetAttr("straddle") == "true")
						{
							// In straddle mode we work in bits then convert back to bytes
							if (num_elts == INT_MAX)
							{
								total_units = int((length_ - addr)/df_size_[jj]);    // How many units to EOF
								num_elts = int((total_units*df_size_[jj]*8)/data_bits);
							}
							else
							{
								total_units = int((num_elts*data_bits - 1)/(df_size_[jj]*8)) + 1;
							}
							done_units = int((elt_num*data_bits - 1)/(df_size_[jj]*8)) + 1;
						}
						else
						{
							// If not straddling we work in storage units
							int elts_per_unit = int((df_size_[jj]*8)/data_bits);     // How many bitfields can we store in a unit
							if (num_elts == INT_MAX)
							{
								total_units = int((length_ - addr)/df_size_[jj]);    // How many units to EOF
								num_elts = total_units * elts_per_unit;
							}
							else
							{
								total_units = (num_elts-1)/elts_per_unit + 1;   // How many units needed for whole array
							}
							done_units = (elt_num-1)/elts_per_unit + 1;         // How many units already done
						}
						df_size_[ii] = total_units * df_size_[jj];              // Array size is no of units X unit size

						if (num_elts > elt_num)
						{
							// Add DF_MORE entry to show fixed size elts that have no entry
							df_type_.push_back(DF_MORE);
							ASSERT(addr != -1);
							df_address_.push_back(addr + done_units * df_size_[jj]);
							df_size_.push_back((total_units - done_units) * df_size_[jj]);
							df_extra_.push_back(num_elts-elt_num);
							df_indent_.push_back(ind+1);
							df_elt_.push_back(elt);
							df_info_.push_back(ExprStringType());
						}

						// Signal that we have handled bitfields (this avoids adding another storage unit to addr/returned_size below)
						bits_used_ = 0;
						last_size_ = 0;
					}
					else
					{
						// If we go all the way to EOF work out how many elts there are
						if (num_elts == INT_MAX)
						{
							ASSERT(strCount.IsEmpty());
							num_elts = int((length_ - addr)/elt_size);
						}
						// Set the size of the array to all elts (even those not displayed)
						df_size_[ii] = num_elts*elt_size;

						if (num_elts > elt_num)
						{
							// Add DF_MORE entry to show fixed size elts that have no entry
							df_type_.push_back(DF_MORE);
							ASSERT(addr != -1);
							df_address_.push_back(addr + elt_num*elt_size);
							df_size_.push_back((num_elts-elt_num)*elt_size);
							df_extra_.push_back(num_elts-elt_num);
							df_indent_.push_back(ind+1);
							df_elt_.push_back(elt);
							df_info_.push_back(ExprStringType());
						}
					}

				}
				else
				{
					// All the element (array_size) have been displayed so use 
					// their size as the total size for the array.
					df_size_[ii] = array_size;
				}

				df_extra_[ii] = elt_num;          // No of elts actually read
				returned_size += df_size_[ii];
				addr += df_size_[ii];
			}
			else if (DffdEditMode())
			{
				df_extra_[ii] = 0;

				// Get the branch for FOR so that it can be displayed and edited
				elts_ac = add_branch(elt, -1, ind+1, ee, elt_size);

				for (int jj = ii; jj < (int)df_size_.size(); ++jj)
				{
					df_size_[jj] = 0;           // signal that this elt is not present in file
					df_address_[jj] = -1;       // make all sub-elts show problem
				}
			}
			else
			{
				// Remove this empty FOR when not in edit mode
				df_type_.pop_back();
				df_address_.pop_back();
				df_size_.pop_back();
				df_extra_.pop_back();
				df_indent_.pop_back();
				df_elt_.pop_back();
				df_info_.pop_back();
			}
		} // end array "for" processing
		else if (elt_type == "switch")
		{
			BOOL show_parent_row = DffdEditMode();      // Only show SWITCH row in edit mode
			unsigned char new_ind = ind+1;

			if (!show_parent_row)
			{
				// Don't show SWITCH nodes at all, only the taken branch (case)
				df_address_.pop_back();
				df_size_.pop_back();
				df_extra_.pop_back();
				df_indent_.pop_back();
				df_elt_.pop_back();
				df_info_.pop_back();

				new_ind = ind;                      // Same indent as SWITCH would have if shown
				ii--;                               // go back since we removed the SWITCH
			}
			else
				df_type_.push_back(DF_SWITCH);

			// Check test expression
			CHexExpr::value_t switch_val;
			bool expr_ok = true;                    // Valid integer expression?

			if (addr != -1)
			{
				int expr_ac;                            // Last node accessed by test expression

				switch_val = ee.evaluate(elt.GetAttr("test"), ii, expr_ac);
				if (expr_ac > last_ac) last_ac = expr_ac;

				// Handle errors in test expression
				if (switch_val.typ != CHexExpr::TYPE_INT && switch_val.typ != CHexExpr::TYPE_STRING)
				{
					if (show_parent_row)
						df_size_[ii] = 0;       // Signal error here in case exception thrown in HandleError and code below not reached

					// Display error message unless error is due to past EOF
					if (switch_val.typ != CHexExpr::TYPE_NONE)
						HandleError("The \"test\" attribute expression of the\n"
									"\"switch\" tag must yield an integer or string result.\n");
					else
						HandleError(CString(ee.get_error_message()) + "\nin \"test\" of \"switch\".");

					expr_ok = false;
				}
			}

			bool found_it = false;                 // Did we find a case that matches the switch expression value
			FILE_ADDRESS size_tmp = 0;
			for (CXmlTree::CElt case_elt = elt.GetFirstChild(); !case_elt.IsEmpty(); ++case_elt)
			{
				bool is_valid = false;    // Is this the "taken" case?
				ASSERT(case_elt.GetName() == "case" && case_elt.GetNumChildren() == 1);

				if (addr != -1 && expr_ok && !found_it)
				{
					// check if switch expression matches this case's range
					std::istringstream strstr((const char *)case_elt.GetAttr("range"));
					if (strstr.str().empty())
					{
						// Empty string matches everything
						is_valid = true;
					}
					else if (switch_val.typ == CHexExpr::TYPE_INT)
					{
						range_set<__int64> rs;
						if (strstr >> rs && rs.find(switch_val.int64) != rs.end())
							is_valid = true;
					}
					else
					{
						ASSERT(switch_val.typ == CHexExpr::TYPE_STRING);
						if (*switch_val.pstr == (const char *)strstr.str().c_str())
							is_valid = true;
					}
				}

				if (is_valid || DffdEditMode())  // only show the taken case unless we are in edit mode
				{
					unsigned jj = df_address_.size();  // remember where we are up to
					last_ac = std::max(last_ac, add_branch(case_elt, is_valid ? addr : -1, new_ind, ee, size_tmp));
					if (is_valid)
					{
						if (show_parent_row)
							df_size_[ii] = size_tmp;   // update size for SWITCH (if present)

						returned_size += size_tmp;
						addr += size_tmp;

						found_it = true;
					}
					else
					{
						// Grey out this node and sub-nodes
						for ( ; jj < df_address_.size(); ++jj)
						{
							df_size_[jj] = 0;           // signal that this elt is not present in file
							//df_address_[jj] = -1;       // make all sub-elts show problem
						}
					}
				}
			}

			if (show_parent_row && !found_it)
			{
				df_size_[ii] = 0;           // signal that this elt is not present in file
				//df_address_[ii] = -1;       // make all sub-elts show problem
			}
		} // end "switch" processing
		else if (elt_type == "if")
		{
			ASSERT(elt.GetNumChildren() == 1 || elt.GetNumChildren() == 3);
			BOOL show_parent_row = DffdEditMode();      // Only show IF row in edit mode
			unsigned char new_ind = ind+1;

			if (!show_parent_row)
			{
				// Don't show IF nodes at all (only contained branch if condition is TRUE
				df_address_.pop_back();
				df_size_.pop_back();
				df_extra_.pop_back();
				df_indent_.pop_back();
				df_elt_.pop_back();
				df_info_.pop_back();

				new_ind = ind;                      // Same indent as IF would have if shown
				ii--;                               // go back since we removed the IF
			}
			else
				df_type_.push_back(DF_IF);

			// Check test expression
			CHexExpr::value_t if_val;
			bool expr_ok = true;                    // Expression was boolean?

			if (addr != -1)
			{
				int expr_ac;                            // Last node accessed by test expression

				if_val = ee.evaluate(elt.GetAttr("test"), ii, expr_ac);
				if (expr_ac > last_ac) last_ac = expr_ac;

				// Handle errors in test expression
				if (if_val.typ != CHexExpr::TYPE_BOOLEAN)
				{
					if (show_parent_row)
						df_size_[ii] = 0;       // Signal error here in case exception thrown in HandleError and code below not reached

					// Display error message unless error is due to past EOF
					if (if_val.typ != CHexExpr::TYPE_NONE)
						HandleError("The \"test\" attribute expression of the\n"
									"\"if\" tag must yield a boolean result.\n");
					else
						HandleError(CString(ee.get_error_message()) + "\nin \"test\" of \"if\".");

					expr_ok = false;
				}
			}

			FILE_ADDRESS size_tmp;

			// Do IF part
			if (addr != -1 && expr_ok && if_val.boolean)
			{
				// Now get the branch for true part
				last_ac = std::max(last_ac, add_branch(elt, addr, new_ind, ee, size_tmp, 0));
				if (show_parent_row)
					df_size_[ii] = size_tmp;   // update size for IF (if present)

				returned_size += size_tmp;
				addr += size_tmp;
			}
			else if (DffdEditMode())
			{
				// Grey out sub-nodes that are not present
				unsigned curr_ii = ii;
				if (show_parent_row && addr != -1 && expr_ok && !if_val.boolean && elt.GetNumChildren() >= 3)
					++curr_ii;                  // Don't grey parent node if ELSE part is valid

				// Get the branch for if so that it can be displayed and edited
				last_ac = std::max(last_ac, add_branch(elt, -1, new_ind, ee, size_tmp, 0));

				for (unsigned jj = curr_ii; jj < df_address_.size(); ++jj)
				{
					df_size_[jj] = 0;           // signal that this elt is not present in file
					//df_address_[jj] = -1;       // make all sub-elts show problem
				}
			}

			// Do ELSE part if it is present
			if (addr != -1 && expr_ok && !if_val.boolean && elt.GetNumChildren() >= 3)
			{
				ASSERT(elt.GetNumChildren() == 3);

				// Now get the branch for false part
				last_ac = std::max(last_ac, add_branch(elt, addr, new_ind, ee, size_tmp, 2));
				if (show_parent_row)
					df_size_[ii] = size_tmp;   // update size for containing IF (if present)

				returned_size += size_tmp;
				addr += size_tmp;
			}
			else if (DffdEditMode() && elt.GetNumChildren() >= 3)
			{
				unsigned curr_ii = df_address_.size();  // remember where we were up to

				// Get the branch for IF so that it can be displayed and edited
				last_ac = std::max(last_ac, add_branch(elt, -1, new_ind, ee, size_tmp, 2));

				// Mark all these sub-elements as not present (greyed)
				for (unsigned jj = curr_ii; jj < df_address_.size(); ++jj)
				{
					df_size_[jj] = 0;           // signal that this elt is not present in file
					//df_address_[jj] = -1;       // make all sub-elts show problem
				}
			}
		} // "if"
		else if (elt_type == "jump")
		{
			BOOL show_parent_row = DffdEditMode();      // Only show row in edit mode
			unsigned char new_ind = ind+1;
			if (!show_parent_row)
			{
				df_address_.pop_back();
				df_size_.pop_back();
				df_extra_.pop_back();
				df_indent_.pop_back();
				df_elt_.pop_back();
				df_info_.pop_back();

				new_ind = ind;                      // Same indent as JUMP would have if shown
				ii--;                               // go back since we removed the last row
			}
			else
				df_type_.push_back(DF_JUMP);

			FILE_ADDRESS jump_addr = -1;

			if (addr != -1)
			{
				// Get offset for new address
				int expr_ac;                            // Last node accessed by offset expression

				CHexExpr::value_t jump_val = ee.evaluate(elt.GetAttr("offset"), ii, expr_ac);
				if (expr_ac > last_ac) last_ac = expr_ac;

				// Expression must be an integer
				if (jump_val.typ != CHexExpr::TYPE_INT)
				{
					if (show_parent_row)
						df_size_[ii] = 0;       // Signal error here in case exception thrown in HandleError and code below not reached

					// Display error message unless error is due to past EOF
					if (jump_val.typ != CHexExpr::TYPE_NONE)
						HandleError("The \"expr\" address expression of the\n"
									"\"jump\" tag must yield an integer result.\n");
					else
						HandleError(CString(ee.get_error_message()) + "\nin \"expr\" of \"jump\".");
				}
				else
				{
					// Work out address jumped to
					CString origin = elt.GetAttr("origin");
					if (origin == "start")
						jump_addr = jump_val.int64;
					else if (origin == "current")
						jump_addr = addr + jump_val.int64;
					else if (origin == "end")
						jump_addr = length_ + jump_val.int64;
					else
						ASSERT(0);

					if (jump_addr < 0)
					{
						HandleError("The jump address is before the start of file.\n");
						jump_addr = -1;
					}
					else if (jump_addr >= length_)
					{
						HandleError("The jump address is past the end of file.\n");
						jump_addr = -1;
					}
				}
			}

			if (jump_addr != -1 || DffdEditMode())
			{
				++in_jump_;                     // Turn off progress in JUMP since it's based on file address
				// Add sub-element
				FILE_ADDRESS size_tmp;          // Ignored since we stay at the same address after return
				last_ac = std::max(last_ac, add_branch(elt, jump_addr, new_ind, ee, size_tmp));
				in_jump_--;

				if (show_parent_row)
				{
					df_address_[ii] = jump_addr;    // Store address & size of contained element
					df_size_[ii] = size_tmp;

					if (jump_addr != -1)
					{
						// ... but show address jumped to in the tree column
						char buf[32];
						sprintf(buf, "=> %I64d", jump_addr);   // CString::Format does not support %I64 (yet? - VS6)
						CString strAddr(buf);
						AddCommas(strAddr);
						if (jump_addr > 9)
						{
							sprintf(buf, "%I64X", jump_addr);
							strAddr += CString(" (0X") + buf + CString(")");
						}
						df_info_[ii] = strAddr;
					}
					else
						df_address_[ii] = -1;
				}
			}
		} // "jump"
		else if (elt_type == "eval")
		{
			df_type_.push_back(DF_EVAL);

			CHexExpr::value_t eval_val;
			bool display_result = false;

			if (addr != -1)
			{
				// Evaluate the expression
				int expr_ac;                            // Last node accessed by expression

				// Evaluate a simple expression or a srting containing one or more {expr;format} specs
				eval_val = Evaluate(elt.GetAttr("expr"), ee, ii, expr_ac);
				//if (expr_ac > last_ac) last_ac = expr_ac;  // Don't update last_ac as the value is only for display

				// Get options
				bool display_error = false;

				CString ss = elt.GetAttr("display_error");
				if (ss.CompareNoCase("true") == 0)
					display_error = true;
				ss = elt.GetAttr("display_result");
				if (ss.CompareNoCase("true") == 0)
					display_result = true;

				// Display an error message if the expression evaluated false
				if (eval_val.typ == CHexExpr::TYPE_NONE)
				{
					HandleError(CString(ee.get_error_message()) + "\nin EVAL \"expr\" attribute.");
					display_result = true;
				}
				else if (display_error && eval_val.typ != CHexExpr::TYPE_BOOLEAN && addr != -1)
				{
					HandleError("The \"expr\" attribute expression of the \"EVAL\"\n"
								"tag must be boolean if display_error is true.\n");

					df_address_[ii] = -1;
					// Always display the result if there is an error
					display_result = true;
				}
				else if (display_error && !eval_val.boolean)
				{
					df_address_[ii] = -1;
					// Always display the result if there is an error
					display_result = true;
				}
			}

			// Display result if requested (always display in edit mode to allow editing of EVAL)
			if (display_result || DffdEditMode())
			{
				// Save the result of the evaluation for later display
				switch (eval_val.typ)
				{
				case CHexExpr::TYPE_BOOLEAN:
					df_info_[ii] = eval_val.boolean ? "TRUE" : "FALSE";
					break;
				case CHexExpr::TYPE_INT:
					//df_info_[ii].Format("%I64d", eval_val.int64);    // CString::Format does not support %I64 yet
					{
						char buf[32];
						sprintf(buf, "%I64d", eval_val.int64);
						CString strAddr(buf);
						AddCommas(strAddr);
						df_info_[ii] = strAddr;
					}
					break;
				case CHexExpr::TYPE_DATE:
					if (eval_val.date > -1e30)
					{
						COleDateTime odt;
						odt.m_dt = eval_val.date;
						odt.m_status = COleDateTime::valid;
						df_info_[ii] = odt.Format("%#c");
					}
					else
						df_info_[ii] = "##Invalid date##";
					break;
				case CHexExpr::TYPE_REAL:
#ifdef UNICODE_TYPE_STRING
					df_info_[ii].Format(L"%g", eval_val.real64);
#else
					df_info_[ii].Format("%g", eval_val.real64);
#endif
					break;
				case CHexExpr::TYPE_STRING:
					df_info_[ii] = *eval_val.pstr;
					break;
				default:
					ASSERT(eval_val.typ == CHexExpr::TYPE_NONE);
					if (addr != -1)
					{
						HandleError(CString(ee.get_error_message()) + "\nin \"expr\" of \"eval\".");
						df_info_[ii] = CString("##") + ee.get_error_message();
					}
					df_address_[ii] = -1;
					break;
				}
			}
			else
			{
				// Remove this entry (display of result not requested and not in edit mode)
				df_type_.pop_back();
				df_address_.pop_back();
				df_size_.pop_back();
				df_extra_.pop_back();
				df_indent_.pop_back();
				df_elt_.pop_back();
				df_info_.pop_back();
			}
		} // "eval"
		else
		{
			ASSERT(elt_type == "data");
			df_type_.push_back(DF_DATA);

//            ASSERT(addr == -1 || addr < length_);
			CString data_type = elt.GetAttr("type");
			data_type.MakeLower();
			CString data_format = elt.GetAttr("format");
			data_format.MakeLower();

			// Get default char set if nec.
			if (data_format == "default")
				data_format = ptree_->GetRoot().GetAttr("default_char_set");

			// Work out length of data or 0 (not present), -1 (not known)
			CString tmp_len = elt.GetAttr("len");
			CHexExpr::value_t data_len;
			if (addr == -1)
				data_len = CHexExpr::value_t(0);     // Use zero length if not present
			else if (tmp_len.IsEmpty())
				data_len = CHexExpr::value_t(-1);    // Length not given - eg, string/none to EOF (also fixed len flds that ignore data_len)
			else
			{
				int expr_ac;                            // Last node accessed by test expression

				// Get length (which may be an expression)
				data_len = ee.evaluate(tmp_len, ii, expr_ac);
				if (expr_ac > last_ac) last_ac = expr_ac;
				if (data_len.typ != CHexExpr::TYPE_INT)
				{
					df_size_[ii] = 0;       // In case exception thrown in HandleError and code below not done

					// Display appropriate error message unless past EOF
					if (data_len.typ != CHexExpr::TYPE_NONE)
						HandleError("The length (in \"format\" attribute) for data\n"
									  "type \"none\" must yield an integer result.\n");
					else
						HandleError(CString(ee.get_error_message()) + "\nin \"length\" attribute.");

					data_len = CHexExpr::value_t(0);
				}
				else if (data_len.int64 < 0)   // -ve becomes zero so we don't go backwards
					data_len.int64 = 0;
			}

			// Work out byte order - this is only used for numeric types longer than 1 byte plus Unicode text (DF_WCHAR/DF_WSTRING)
			CString byte_order = elt.GetAttr("byte_order");
			if (byte_order == "default")
				byte_order = default_byte_order_;

			// Work out if this is bitfield and get number of bits
			int data_bits = atoi(elt.GetAttr("bits"));
			ASSERT(data_bits == 0 || data_type == "int");   // only ints can have bit-fields

			if (data_type == "none")
			{
				df_type_[ii] = DF_NO_TYPE;

				if (data_len.int64 > -1)
					df_size_[ii] = data_len.int64;
				//else if (addr == -1)
				//{
				//    df_size_[ii] = 0;          // signal that it's not present
				//    last_ac = ii;
				//}
				else
				{
					df_size_[ii] = length_ - addr;
					last_ac = ii;              // Just means that elt has variable length (dep. on file length)
				}
			}
#if _MSC_VER >= 1300
			else if (data_type.Left(6) == "string" && data_format == "unicode")
			{
				df_type_[ii] = DF_WSTRING;
				df_extra_[ii] = atoi(data_type.Mid(6));     // Terminator appended to data type eg "string13"
				if (data_len.int64 > -1)
					df_size_[ii] = data_len.int64;
				else
				{
					// Find the end of string (0) or stop at EOF
#ifdef _DEBUG
					wchar_t buf[4];  // A small buffer is more likely to catch bugs during testing
#else
					wchar_t buf[256];
#endif
					const wchar_t *pp;
					size_t got;
					wchar_t term = df_extra_[ii];
					// For big-endian Unicode strings we have to convert each wide character to little-endian
					// to compare again term, but it is simpler just to reverse bytes of term instead.
					if (byte_order == "big")
						flip_bytes((unsigned char *)&term, 2);

					df_size_[ii] = 0;
					ASSERT(sizeof(buf)%2 == 0);   // Must be even length for wide chars
					while ((got = GetData((unsigned char *)buf, sizeof(buf), addr + df_size_[ii])) > 0)
					{
						if ((pp = wmemchr(buf, term, got/2)) != NULL)
						{
							// End of string found
							df_size_[ii] += ((pp - buf) + 1)*2;  // Add one to include terminating byte, mult by 2 for wide chars
							break;
						}

						df_size_[ii] += got;
					}
					last_ac = ii;               // Indicate that we looked at the data (to find end of string)
				}
			}
#endif
			else if (data_type.Left(6) == "string")
			{
				if (data_format == "ascii")
					df_type_[ii] = DF_STRINGA;
				else if (data_format == "ansi")
					df_type_[ii] = DF_STRINGN;
				else if (data_format == "oem")
					df_type_[ii] = DF_STRINGO;
				else if (data_format == "ebcdic")
					df_type_[ii] = DF_STRINGE;
#if !(_MSC_VER >= 1300)  // handled separately above
				else if (data_format == "unicode")
					df_type_[ii] = DF_WSTRING;
#endif
				else
				{
					ASSERT(0);
					df_type_[ii] = DF_STRINGN;
				}

				df_extra_[ii] = atoi(data_type.Mid(6));     // Store string terminator

				if (data_len.int64 > -1)
					df_size_[ii] = data_len.int64;
				//else if (addr == -1)
				//{
				//    df_size_[ii] = 0;
				//    last_ac = ii;
				//}
				else
				{
					// Find the end of string (null byte) or stop at EOF
#ifdef _DEBUG
					unsigned char buf[4];  // A small buffer is more likely to catch bugs during testing
#else
					unsigned char buf[256];
#endif
					unsigned char *pp;
					size_t got;

					df_size_[ii] = 0;
					while ((got = GetData(buf, sizeof(buf), addr + df_size_[ii])) > 0)
					{
						if ((pp = (unsigned char *)memchr(buf, df_extra_[ii], got)) != NULL)
						{
							// End of string found
							df_size_[ii] += (pp - buf) + 1;  // Add one to include terminating byte
							break;
						}

						df_size_[ii] += got;
					}
					last_ac = ii;               // We had to access the data of this element to find end of string
				}
			}
			else if (data_type == "char")
			{
				df_size_[ii] = 1;
				if (data_format == "ascii")
					df_type_[ii] = DF_CHARA;
				else if (data_format == "ansi")
					df_type_[ii] = DF_CHARN;
				else if (data_format == "oem")
					df_type_[ii] = DF_CHARO;
				else if (data_format == "ebcdic")
					df_type_[ii] = DF_CHARE;
				else if (data_format == "unicode")
				{
					df_type_[ii] = DF_WCHAR;
					df_size_[ii] = 2;
				}
				else
				{
					ASSERT(0);
					df_type_[ii] = DF_CHARN;
				}
			}
			else if (data_type == "int" && data_bits > 0)
			{
				// Bitfield
				if (data_len.int64 == 1)
				{
					df_type_[ii] = DF_BITFIELD8;
					df_size_[ii] = 1;
				}
				else if (data_len.int64 == 2)
				{
					df_type_[ii] = DF_BITFIELD16;
					df_size_[ii] = 2;
				}
				else if (data_len.int64 == 8)
				{
					df_type_[ii] = DF_BITFIELD64;
					df_size_[ii] = 8;
				}
				else
				{
					df_type_[ii] = DF_BITFIELD32;
					df_size_[ii] = 4;
				}

				bool data_down = elt.GetAttr("direction") == "down";
				bool data_straddle = elt.GetAttr("straddle") == "true";

				// Check if we need to advance to the next bitfield storage unit
				if (bits_used_ > 0 && (df_size_[ii] != last_size_ || 
									   data_down != last_down_ || 
									   (bits_used_ + data_bits) > int(df_size_[ii])*8 && !data_straddle))
				{
					// Move to the end of this bitfield storage unit
					ASSERT(last_size_ != 0);
					returned_size += last_size_;
					if (addr != -1)
						addr += last_size_;
					df_address_[ii] = addr;

					// Indicate that there is now no bitfield in effect
					bits_used_ = 0;
					last_size_ = 0;
				}
				// Store number of bits in 2nd byte and position (of bottom bit of bitfield) in lowest byte
				// Note: If straddle is on position+bits could be greater than storage units size (OR position 
				// could be -ve for data_down) in which case the extra bits are taken from the next storage unit.
				if (data_down)
					df_extra_[ii] = (data_bits<<8) | (int(df_size_[ii])*8 - (bits_used_+data_bits))&0xFF;
				else
					df_extra_[ii] = (data_bits<<8) | bits_used_;

				last_size_ = df_size_[ii];
				last_down_ = data_down;
				bits_used_ += data_bits;
				if (bits_used_ >= int(df_size_[ii])*8)
				{
					// Exactly filled one storage unit or overflowed it (if straddle allowed)
					ASSERT(bits_used_ == int(df_size_[ii])*8 || data_straddle);
					returned_size += last_size_ * (bits_used_ / int(df_size_[ii]*8));
					if (addr != -1)
						addr += last_size_ * (bits_used_ / int(df_size_[ii]*8));
					bits_used_ %= int(df_size_[ii])*8;
					if (bits_used_ == 0)
						last_size_ = 0;
				}
			}
			else if (data_type == "int" && data_format.Left(1) == "u")
			{
				// Unsigned integer - now just get the size
				if (data_len.int64 == 1)
				{
					df_type_[ii] = DF_UINT8;
					df_size_[ii] = 1;
				}
				else if (data_len.int64 == 2)
				{
					df_type_[ii] = DF_UINT16;
					df_size_[ii] = 2;
				}
				else if (data_len.int64 == 8)
				{
					df_type_[ii] = DF_UINT64;
					df_size_[ii] = 8;
				}
				else
				{
					df_type_[ii] = DF_UINT32;
					df_size_[ii] = 4;
				}
			}
			else if (data_type == "int" && data_format.Left(1) == "m")
			{
				// Signed (sign & magnitude) integer - now get the size
				if (data_len.int64 == 1)
				{
					df_type_[ii] = DF_MINT8;
					df_size_[ii] = 1;
				}
				else if (data_len.int64 == 2)
				{
					df_type_[ii] = DF_MINT16;
					df_size_[ii] = 2;
				}
				else if (data_len.int64 == 8)
				{
					df_type_[ii] = DF_MINT64;
					df_size_[ii] = 8;
				}
				else
				{
					df_type_[ii] = DF_MINT32;
					df_size_[ii] = 4;
				}
			}
			else if (data_type == "int")
			{
				// Signed (2's complement) integer - now get the size
				if (data_len.int64 == 1)
				{
					df_type_[ii] = DF_INT8;
					df_size_[ii] = 1;
				}
				else if (data_len.int64 == 2)
				{
					df_type_[ii] = DF_INT16;
					df_size_[ii] = 2;
				}
				else if (data_len.int64 == 8)
				{
					df_type_[ii] = DF_INT64;
					df_size_[ii] = 8;
				}
				else
				{
					df_type_[ii] = DF_INT32;
					df_size_[ii] = 4;
				}
			}
			else if (data_type == "real" && data_format.Left(3) == "ibm")
			{
				// IBM floating point value - get whether 32 or 64 bit
				if (data_len.int64 == 4)
				{
					df_type_[ii] = DF_IBMREAL32;
					df_size_[ii] = 4;
				}
				else
				{
					df_type_[ii] = DF_IBMREAL64;
					df_size_[ii] = 8;
				}
			}
			else if (data_type == "real" && toupper(data_format[0]) == 'B')
			{
				df_type_[ii] = DF_REAL48;
				df_size_[ii] = 6;
			}
			else if (data_type == "real")
			{
				if (data_len.int64 == 4)
				{
					df_type_[ii] = DF_REAL32;
					df_size_[ii] = 4;
				}
				else
				{
					df_type_[ii] = DF_REAL64;
					df_size_[ii] = 8;
				}
			}
			else if (data_type == "date")
			{
				if (data_format == "c51")
				{
					df_type_[ii] = DF_DATEC51;
					df_size_[ii] = 4;
				}
				else if (data_format == "c7")
				{
					df_type_[ii] = DF_DATEC7;
					df_size_[ii] = 4;
				}
				else if (data_format == "cmin")
				{
					df_type_[ii] = DF_DATECMIN;
					df_size_[ii] = 4;
				}
				else if (data_format == "c64")
				{
					df_type_[ii] = DF_DATEC64;
					df_size_[ii] = 8;
				}
				else if (data_format == "ole")
				{
					df_type_[ii] = DF_DATEOLE;
					df_size_[ii] = 8;
				}
				else if (data_format == "systemtime")
				{
					df_type_[ii] = DF_DATESYSTEMTIME;
					df_size_[ii] = 16;
				}
				else if (data_format == "filetime")
				{
					df_type_[ii] = DF_DATEFILETIME;
					df_size_[ii] = 8;
				}
				else if (data_format == "msdos")
				{
					df_type_[ii] = DF_DATEMSDOS;
					df_size_[ii] = 4;
				}
				else
				{
					ASSERT(data_format == "c");
					df_type_[ii] = DF_DATEC;
					df_size_[ii] = 4;
				}
			}

			// Make sure we can actually get the data from the file
			if (addr != -1 && df_address_[ii] + df_size_[ii] > length_)
			{
//                df_size_[ii] = length_ - df_address_[ii];
				if (df_mess_.IsEmpty()) df_mess_ = "Unexpected EOF";
//                HandleError("Unexpected end of file");
				addr = -1;
			}

			// Check and store byte order
			if (byte_order == "big" && (df_type_[ii] >= DF_INT8 || df_type_[ii] == DF_WCHAR || df_type_[ii] == DF_WSTRING))
				df_type_[ii] = -df_type_[ii];   // -ve indicates big-endian byte order

			// Only check value against domain if we can read it
			if (addr != -1)
			{
				// Check if the data is within its domain
				CString strDomain = df_elt_[ii].GetAttr("domain");

				if (!strDomain.IsEmpty() && strDomain[0] == '{')
				{
					// Check the value is one of the enums
					__int64 sym_size, sym_addr;  // not used

					// Get the value of the data type and make sure it is integer data
					CHexExpr::value_t tmp = ee.get_value(ii, sym_size, sym_addr);

					if (tmp.typ != CHexExpr::TYPE_INT)
					{
						ASSERT(df_size_[ii] > 0);
						df_size_[ii] = -df_size_[ii];  // Make -ve to indicate that it's invalid

						if (tmp.typ == CHexExpr::TYPE_NONE)
							HandleError(CString(ee.get_error_message()) + "\nin \"domain\" attribute.");
						else
							HandleError("An enum domain is only valid for integer data.\n");
					}
					else
					{
						if (!add_enum(df_elt_[ii], strDomain))
						{
							// Syntax error in enum string
							ASSERT(df_size_[ii] > 0);
							df_size_[ii] = -df_size_[ii];  // Make -ve to indicate that it's invalid
							HandleError("Invalid enumeration in \"domain\" attribute.");
						}
						else
						{
							enum_t &ev = get_enum(df_elt_[ii]);
							if (ev.find(tmp.int64) == ev.end())
							{
								// Value not found
								ASSERT(df_size_[ii] > 0);
								df_size_[ii] = -df_size_[ii];  // Make -ve to indicate that it's invalid
							}
						}
					}
				}
				else if (!strDomain.IsEmpty())
				{
					int expr_ac;                            // Last node accessed by test expression

					CHexExpr::value_t tmp = ee.evaluate(strDomain, ii, expr_ac);
					if (tmp.typ != CHexExpr::TYPE_BOOLEAN || !tmp.boolean)
					{
						ASSERT(df_size_[ii] > 0);
						df_size_[ii] = -df_size_[ii];  // Make -ve to indicate that it's invalid

						if (tmp.typ == CHexExpr::TYPE_NONE)
							HandleError(CString(ee.get_error_message()) + "\nin \"domain\" attribute.");
						else if (tmp.typ != CHexExpr::TYPE_BOOLEAN)
							HandleError("The \"domain\" attribute expression of the\n"
										  "\"data\" tag must yield a boolean result.\n");
					}
				}
			}

			// Advance address unless it was a bitfield
			if (data_type != "int" || data_bits == 0)
			{
				returned_size += mac_abs(df_size_[ii]);
				if (addr != -1)
					addr += mac_abs(df_size_[ii]);
				else
					df_address_[ii] = -1;
			}

			ASSERT(df_type_[ii] != DF_DATA);  // Ensure a specific type has been assigned

			CString strName = df_elt_[ii].GetAttr("name");
			if (!DffdEditMode() && strName.IsEmpty() && parent.GetName() != "for")
			{
				// Hide nameless data elements in view mode (but show in edit mode)
				df_type_.pop_back();
				df_address_.pop_back();
				df_size_.pop_back();
				df_extra_.pop_back();
				df_indent_.pop_back();
				df_elt_.pop_back();
				df_info_.pop_back();
			}

			// Update scan progress no more than once a second
			if (length_ > 0 && addr > 0 && (clock() - m_last_checked)/CLOCKS_PER_SEC > 1 && !in_jump_)
			{
				((CMainFrame *)AfxGetMainWnd())->Progress(addr < length_ ? int((addr*100)/length_) : 100);
				m_last_checked = clock();
			}
		} // end of "data" processing

		// Make sure array lengths are all the same
		ASSERT(df_address_.size() == df_size_.size());
		ASSERT(df_address_.size() == df_indent_.size());
		ASSERT(df_address_.size() == df_elt_.size());
		ASSERT(df_address_.size() == df_type_.size());
		ASSERT(df_address_.size() == df_extra_.size());
		ASSERT(df_address_.size() == df_info_.size());

#ifdef _DEBUG
		if (false)
			dump_tree();    // Set IP here to dump the tree
#endif

		if (child_num != -1)
			break;          // if not -1 we are only doing a single child

		// Move to next sibling
		++elt;
	}

	// Terminate bitfield at end unless ok to have bitfield at end (ie we are doing an array of bitfields)
	if (!ok_bitfield_at_end && bits_used_ > 0)
	{
		// Move to the end of this bitfield storage unit
		ASSERT(last_size_ != 0);
		returned_size += last_size_;
		if (addr != -1)
			addr += last_size_;
		last_size_ = 0;
		bits_used_ = 0;                     // Indicate that there is now no bitfield in effect
	}

	return last_ac;
}

void CHexEditDoc::HandleError(const char *mess)
{
	CString ss = CString(mess) + "\n\nDo you want to continue?";

	if (TaskMessageBox("Template Error", ss, MB_YESNO) != IDYES)
		throw mess;
}

// Handle storage of enums for the current template

// Adds an enum for a template element - if enum for this element exists it just return true
// ee = template element whose enum map we are storing
// pp = string containing enum defns to be parsed eg "{BLACK, RED=3}
// Returns false if there is som sort of parse error
bool CHexEditDoc::add_enum(CXmlTree::CElt &ee, LPCTSTR pp)
{
	// Make sure we are doing an integer data element and we got the right enum string
	ASSERT(ee.GetName() == "data");
	ASSERT(ee.GetAttr("type") == "char" || ee.GetAttr("type") == "int");
	ASSERT(ee.GetAttr("domain") == pp);

	if (df_enum_.find((MSXML2::IXMLDOMElementPtr::Interface *)ee.m_pelt) != df_enum_.end())
		return true;                          // already added

	enum_t to_add;

	// Make sure the enum string starts with the flag character {
	if (pp[0] != '{')
	{
		// Add empty enum list so we don't get run-time errors
		df_enum_[(MSXML2::IXMLDOMElementPtr::Interface *)ee.m_pelt] = to_add;
		return false;
	}

	bool retval = true;

	// Find the end so we know where to stop
	const char *pend = strchr(pp, '}');
	if (pend == NULL)
	{
		pend = (char *)(pp + strlen(pp));
		retval = false;         // signal error but continue parsing
	}

	// Get string with just enums in it
	CString ss(pp+1, pend-pp-1);
	CString entry;

	__int64 enum_val = 0;
	for (int ii = 0; AfxExtractSubString(entry, ss, ii, ','); ++ii, ++enum_val)
	{
		entry.TrimLeft();
		entry.TrimRight();
		if (entry.IsEmpty())
			break;     // This is not an error as C syntax allows comma after enum list

		int eq_pos;                     // Posn of equals sign in the entry
		if ((eq_pos = entry.Find('=')) != -1)
		{
			// Separate value from the name
			CString strVal = entry.Mid(eq_pos + 1);
			errno = 0;
			// Try strtol first as it handles a 0x prefix for hex.  Admittedly this
			// could be confusing as 0x7FFFffff (32 bit) works but 0x100000000 doesn't.
			enum_val = strtol(strVal, NULL, 10);
			if (errno == ERANGE)
				enum_val = _atoi64(strVal); // if too big try to scan as 64 bit
			entry = entry.Left(eq_pos);
			entry.TrimRight();
		}
		if (!valid_id(entry))
		{
			retval = false;
			break;
		}

		to_add[enum_val] = entry;
		//TRACE2("ENUM: %s=%ld\n", entry, long(enum_val));
	}

	df_enum_[(MSXML2::IXMLDOMElementPtr::Interface *)ee.m_pelt] = to_add;
	return retval;
}

// Returns the a reference to the enum map for an element.
// The element must have already had its enums added using add_enum (above)
// ee = template element whose enum map we are getting
CHexEditDoc::enum_t &CHexEditDoc::get_enum(CXmlTree::CElt &ee)
{
	ASSERT(ee.GetName() == "data");
	ASSERT(ee.GetAttr("type") == "char" || ee.GetAttr("type") == "int");
	ASSERT(df_enum_.find((MSXML2::IXMLDOMElementPtr::Interface *)ee.m_pelt) != df_enum_.end());

	enum_t &retval = df_enum_.find((MSXML2::IXMLDOMElementPtr::Interface *)ee.m_pelt)->second;
	return retval;
}

// Return the string representation of the value, including enum string for int values (if any)
ExprStringType CHexEditDoc::get_str(CHexExpr::value_t val, int ii)
{
	if (val.typ == CHexExpr::TYPE_INT)
	{
		std::map<MSXML2::IXMLDOMElementPtr::Interface *, enum_t>::const_iterator pev =
			df_enum_.find((MSXML2::IXMLDOMElementPtr::Interface *)df_elt_[ii].m_pelt);
		if (pev != df_enum_.end())
		{
			enum_t::const_iterator pe = pev->second.find(val.int64);
			if (pe != pev->second.end())
			{
				return ExprStringType(pe->second);
			}
		}
	}

	return val.GetDataString("");
}

#ifdef _DEBUG
void CHexEditDoc::dump_tree()
{
	CString dump, ss;

	for (int ii = 0; ii < (int)df_address_.size(); ++ii)
	{
		ss.Format("%*s%5ld %5ld %-20s\n",
				  int(df_indent_[ii]-1), "",
				  long(df_address_[ii]),
				  long(df_size_[ii]),
				  df_elt_[ii].GetAttr("name"));

		dump += ss;
	}
	AfxMessageBox(dump);
}
#endif

// Evaluates a simple expression OR parses a string containing {expr;format} specs and
// replaces the specs with the values and returns the total resulting string.
// ss = string containing expression OR text with spec string(s)
// ee = used for evaluating expressions containing variables from the file
// ref = which element we are accessing from (used for getting variables)
// ref_ac = returned value indicating the last element accessed in the file using var in the expression(s)
CHexExpr::value_t CHexEditDoc::Evaluate(CString ss, CHexExpr &ee, int ref, int &ref_ac)
{
	// First check if this string contains format specs
	if (ss.Find('{') == -1)
	{
		// No format specs so assume it is a simple expression
		return ee.evaluate(ss, ref, ref_ac);
	}
	else
	{
		ExprStringType retval;
		ref_ac = -1;

		int start, end;     // Store { and } positions for current spec
		while ((start = ss.Find('{')) != -1 && (end = ss.Find('}',start)) != -1)
		{
			// We have another spec {expr;format}
			retval += ss.Left(start);       // put out the literal bit

			// Get the member name and a format string (if any)
			CString tmp = ss.Mid(start+1, end-start-1);
			CString exprStr, formatStr;
			AfxExtractSubString(exprStr,   tmp, 0, ';');
			AfxExtractSubString(formatStr, tmp, 1, ';');

			int expr_ac;                            // Last node accessed by expression

			expr_eval::value_t val = ee.evaluate(exprStr, ref, expr_ac);
			if (val.typ == CHexExpr::TYPE_NONE)
				retval += CString("##") + ee.get_error_message();
			else
				retval += val.GetDataString(formatStr);
			if (expr_ac > ref_ac) ref_ac = expr_ac;

			ss = ss.Mid(end+1);         // Continue on the next bit of the input string
		}

		retval += ss;                   // Add any left over literal stuff

		return CHexExpr::value_t(retval);
	}
}

// Get symbol type/value.  sym is the name of the symbol to find, parent is the element
// whose sub-elements are to be searched.  If sym is NULL then parent must refer to a
// "for" element and index is the sub-element to access.  If sym is not NULL then parent
// is a "struct" or "binary_file_format" element.  If sym is "this" just return parent.
// Returned values:
//   returns value of the symbol which may be TYPE_NONE if it was not found
//   pac = number of the latest symbol (sequentially within the file) accessed
//   sym_size = numbers of bytes occupied by the value
//   sym_address = address within the file of the value (0 if not part of file)
//   sym_str = string representation (currently just does enum string substitution)
// Note: variables (var_ collection) are not considered.
CHexExpr::value_t CHexExpr::find_symbol(const char *sym, value_t parent, size_t index, int *pac,
										__int64 &sym_size, __int64 &sym_address, CString &sym_str)
{
	ASSERT(pdoc->df_address_.size() == pdoc->df_size_.size());
	ASSERT(pdoc->df_address_.size() == pdoc->df_indent_.size());
	ASSERT(pdoc->df_address_.size() == pdoc->df_elt_.size());
	ASSERT(pdoc->df_address_.size() == pdoc->df_type_.size());
	ASSERT(pdoc->df_address_.size() == pdoc->df_extra_.size());
	ASSERT(pdoc->df_address_.size() == pdoc->df_info_.size());
	ASSERT(parent.typ == TYPE_NONE || parent.typ == TYPE_STRUCT || parent.typ == TYPE_ARRAY || parent.typ == TYPE_BLOB);

	CHexExpr::value_t retval;
	size_t ii = size_t(parent.int64);
	ASSERT(ii < pdoc->df_address_.size());

	retval.typ = TYPE_NONE;             // Default to symbol not found
	retval.error = false;
	retval.int64 = 0;
	sym_size = 0;
	sym_address = -1;
	sym_str.Empty();

	if (parent.typ == TYPE_NONE && _stricmp(sym, "cursor") == 0)
	{
		FILE_ADDRESS start, end;
		CHexEditView *pview = pdoc->GetBestView();
		assert(pview != NULL);
		if (pview != NULL)
		{
			pview->GetSelAddr(start, end);
			retval.typ = TYPE_INT;
			retval.int64 = start;
		}
	}
	else if (parent.typ == TYPE_NONE && _stricmp(sym, "mark") == 0)
	{
		CHexEditView *pview = pdoc->GetBestView();
		assert(pview != NULL);
		if (pview != NULL)
		{
			retval.typ = TYPE_INT;
			retval.int64 = pview->GetMark();
		}
	}
	else if (parent.typ == TYPE_NONE && _stricmp(sym, "eof") == 0)
	{
		retval.typ = TYPE_INT;
		retval.int64 = pdoc->length();
	}
	else if (parent.typ == TYPE_NONE && _stricmp(sym, "this") == 0)
	{
		// Just return the element (parent)
		if (parent.int64 > *pac) *pac = int(parent.int64);
		retval = get_value(int(parent.int64), sym_size, sym_address);
		sym_str = pdoc->get_str(retval, int(parent.int64));
	}
	else if (parent.typ == TYPE_NONE)
	{
		// Search siblings, parent siblings etc up to top level
		int curr_indent = pdoc->df_indent_[ii];

//        for (int jj = ii - 1; jj > 0; jj--)
		for (int jj = ii; jj >= 0; jj--)
		{
			if (pdoc->df_indent_[jj] < curr_indent)
			{
				ASSERT(pdoc->df_indent_[jj] == curr_indent-1);
				curr_indent = pdoc->df_indent_[jj];
			}
			if (pdoc->df_indent_[jj] == curr_indent &&
					 sym_found(sym, jj, retval, pac, sym_size, sym_address))
			{
				sym_str = pdoc->get_str(retval, jj);
				break;
			}
		}
	}
	else if (parent.typ == TYPE_STRUCT)
	{
		ASSERT(pdoc->df_type_[ii] == CHexEditDoc::DF_STRUCT || 
			   pdoc->df_type_[ii] == CHexEditDoc::DF_USE_STRUCT || 
			   pdoc->df_type_[ii] == CHexEditDoc::DF_FILE);

		// Search the sub-elements of this struct
		int curr_indent = pdoc->df_indent_[ii];
		ASSERT(ii < pdoc->df_address_.size() - 1);  // There must be at least one sub-elt

		for (int jj = ii + 1; jj < (int)pdoc->df_address_.size(); ++jj)
		{
			if (pdoc->df_indent_[jj] == curr_indent)
			{
				break;                  // End of sub-elements
			}
			else if (pdoc->df_indent_[jj] == curr_indent + 1 &&
					 sym_found(sym, jj, retval, pac, sym_size, sym_address))
			{
				sym_str = pdoc->get_str(retval, jj);
				break;
			}
		}
	}
	else if (parent.typ == TYPE_ARRAY)
	{
		ASSERT(pdoc->df_type_[ii] == CHexEditDoc::DF_FORV ||
			   pdoc->df_type_[ii] == CHexEditDoc::DF_FORF);
		ASSERT(sym == NULL);
		ASSERT(ii < pdoc->df_address_.size() - 1);  // There must be at least one sub-elt

		// Check that index is not out of range
		if (index < pdoc->df_extra_[ii])
		{
			int jj;
			int curr_indent = pdoc->df_indent_[ii];

			// Scan forward for the index'th array element
			for (jj = ii + 1; jj < (int)pdoc->df_address_.size(); ++jj)
			{
				ASSERT(pdoc->df_indent_[jj] > curr_indent); // lower indents mean we've gone past array end
				if (pdoc->df_indent_[jj] == curr_indent + 1)
					if (index-- == 0)
						break;
			}

			// Drill down to data if nec.
			while (jj < (int)pdoc->df_address_.size() && pdoc->df_type_[jj] == CHexEditDoc::DF_IF)
				++jj;
			ASSERT(jj < (int)pdoc->df_address_.size());

			if (jj > *pac) *pac = jj;
			retval = get_value(jj, sym_size, sym_address);
			sym_str = pdoc->get_str(retval, jj);
		}
	}
	else if (parent.typ == TYPE_BLOB)
	{
		ASSERT(pdoc->df_type_[ii] == CHexEditDoc::DF_NO_TYPE);  // Make sure it is a blob
		ASSERT(sym == NULL);
		ASSERT(ii < pdoc->df_address_.size() - 1);
		unsigned char val;                                      // Byte obtained from the file
		retval.typ = TYPE_INT;

		// If data element does not exist, index is past end of BLOB or just couln't read it for some reason
		if (pdoc->df_address_[ii] == -1 ||
			index > pdoc->df_size_[ii] ||
			pdoc->GetData(&val, 1, pdoc->df_address_[ii] + index) != 1)
		{
			// Set flag to say there is a problem and use null data
			retval.error = true;
			retval.int64 = -1;
		}
		else
			retval.int64 = val;
	}
	else
	{
		ASSERT(0);
	}

	return retval;
}

// Returns TRUE if the symbol is found (even if of TYPE_NONE).
// sym = name of symbol to find
// ii = index where we are looking for the symbol
// val = returned symbol value (may be TYPE_NONE)
// pac = updated to reflect the last index accessed
// sym_size = size of returned symbol
// sym_address = address of returned symbol
BOOL CHexExpr::sym_found(const char * sym, int ii, CHexExpr::value_t &val, int *pac,
						 __int64 &sym_size, __int64 &sym_address)
{
	if (ii >= (int)pdoc->df_address_.size())
		return FALSE;

	if (pdoc->df_type_[ii] == CHexEditDoc::DF_JUMP)
		return sym_found(sym, ii+1, val, pac, sym_size, sym_address);
	else if (pdoc->df_type_[ii] == CHexEditDoc::DF_IF)
	{
		++ii;                           // Move to sub-element of IF
		if (ii >= (int)pdoc->df_address_.size())
			return FALSE;

		// Check IF part first
		BOOL found_in_if = sym_found(sym, ii, val, pac, sym_size, sym_address);

		if (found_in_if && val.typ != TYPE_NONE && !val.error)
		{
			return TRUE;               // Found in IF part and its not empty
		}

		// Skip forward to ELSE part which must have same indentation as main IF part
		int curr_indent = pdoc->df_indent_[ii];
		++ii;
		while (ii < (int)pdoc->df_address_.size() && pdoc->df_indent_[ii] > curr_indent)
			++ii;

		// If no ELSE part return what we found in IF part
		if (ii == (int)pdoc->df_address_.size() || pdoc->df_indent_[ii] < curr_indent)
			return found_in_if;

		CHexExpr::value_t val2;
		int ac2;
		__int64 sym_size2, sym_address2;
		if (!sym_found(sym, ii, val2, &ac2, sym_size2, sym_address2))
			return found_in_if;
		else if (val2.typ != TYPE_NONE || !found_in_if)
		{
			// Set returned values to what we found in ELSE part
			val = val2;
			*pac = ac2;
			sym_size = sym_size2;
			sym_address = sym_address2;
			return TRUE;
		}
		else
			return found_in_if;
	}
	else if (pdoc->df_type_[ii] == CHexEditDoc::DF_SWITCH)
	{
		++ii;                                    // First case in switch is next
		if (ii >= (int)pdoc->df_address_.size())
			return FALSE;

		int curr_indent = pdoc->df_indent_[ii];  // indent of all cases in the switch

		BOOL found = FALSE;
		while (ii < (int)pdoc->df_address_.size() && pdoc->df_indent_[ii] == curr_indent)
		{
			// There are 3 situations:
			// 1. the case is the "taken" case and the symbol of the sub-element matches sym
			//   sym_found() returns TRUE and val is valid so just return TRUE
			// 2. the symbol matches but it is not the taken case or the location is invalid (eg past EOF)
			//   sym_found() returns TRUE but val is invalid so keep looking
			// 3. the symbol does not match
			if (sym_found(sym, ii, val, pac, sym_size, sym_address))
			{
				found = TRUE;
				if (val.typ != TYPE_NONE && !val.error)
					return TRUE;               // Found the case with the valid value

				// else we continue since a following case may have the same symbol name and actually be valid
			}

			// Skip children (ie higher indent level) until next case of switch or end of switch
			++ii;
			while (ii < (int)pdoc->df_address_.size() && pdoc->df_indent_[ii] > curr_indent)
				++ii;
		}
		return found;
	}
	else if (pdoc->df_elt_[ii].GetAttr("name") == sym)
	{
		val = get_value(ii, sym_size, sym_address);
		if (ii > *pac)                  // If this symbol is further forward than any seen before ...
			*pac = ii;                  // ... update the last accessed symbol ptr
		return TRUE;
	}
	else
		return FALSE;
}

// Gets the value of an element.  For data elements the type is TYPE_INT/TYPE_STRING/TYPE_REAL/
// TYPE_DATE and the value is returned in the appropriate value field. 
// If there is no actual associated data (eg if the data is in an "if" where the test failed or
// the data is past the EOF of the physical data file) then it returns a value of TYPE_NONE.
// For a "for" or "struct" the return type is set to TYPE_ARRAY or TYPE_STRUCT and the int64 field
// is set to the vector index (into df_type_ etc).
CHexExpr::value_t CHexExpr::get_value(int ii, __int64 &sym_size, __int64 &sym_address)
{
	value_t retval;
	ASSERT(retval.error == false);  // Ensure it has been initialised

	sym_size = mac_abs(pdoc->df_size_[ii]);
	sym_address = pdoc->df_address_[ii];
	if (sym_address == -1)
	{
		retval.typ = TYPE_NONE;
	}
	else if (pdoc->df_type_[ii] == CHexEditDoc::DF_STRUCT || 
		pdoc->df_type_[ii] == CHexEditDoc::DF_USE_STRUCT || 
		pdoc->df_type_[ii] == CHexEditDoc::DF_FILE)
	{
		retval.typ = TYPE_STRUCT;
		retval.int64 = ii;
	}
	else if (pdoc->df_type_[ii] == CHexEditDoc::DF_FORF ||
			 pdoc->df_type_[ii] == CHexEditDoc::DF_FORV)
	{
		retval.typ = TYPE_ARRAY;
		retval.int64 = ii;
	}
	else if (abs(pdoc->df_type_[ii]) < CHexEditDoc::DF_DATA)
	{
		// No actual data (may be within non-taken IF, etc)
		retval.typ = TYPE_NONE;
	}
	else
	{
		signed char df_type = abs(pdoc->df_type_[ii]);      // Data types may be -ve to indicate big endian
		bool big_endian = pdoc->df_type_[ii] < 0;
		FILE_ADDRESS df_size = mac_abs(pdoc->df_size_[ii]);

		ASSERT(df_type >= CHexEditDoc::DF_DATA);
		if (df_type <= CHexEditDoc::DF_NO_TYPE || pdoc->df_address_[ii] == -1)
			df_size = 0;
		unsigned char *buf = NULL;
		unsigned char small_buf[128];                      // avoid heap memory for small (most) things
		unsigned char *large_buf = NULL;                   // Only needed for long strings

		// Get the bytes for the data (except for a BLOB which could be huge)
		if (df_size > 0)
		{
			if (df_size <= sizeof(small_buf))
				buf = small_buf;
			else
			{
				large_buf = new unsigned char[size_t(df_size)];
				buf = large_buf;
			}
			if (pdoc->GetData(buf, size_t(df_size), pdoc->df_address_[ii]) != df_size)
			{
				// Set flag to say there is a problem and use null data
				retval.error = true;
				df_size = 0;
			}
			else if (big_endian && df_type == CHexEditDoc::DF_WSTRING)
				flip_each_word(buf, size_t(df_size));      // flip bytes in each word of big-endian Unicode string
			else if (big_endian && df_type == CHexEditDoc::DF_DATESYSTEMTIME)
				flip_each_word(buf, size_t(df_size));      // flip bytes of each (2 byte) field
			else if (big_endian)
				flip_bytes(buf, size_t(df_size));  // Convert big-endian to little-endian
		}

		switch (df_type)
		{
		case CHexEditDoc::DF_DATA:
			retval.typ = TYPE_NONE;
			break;
		case CHexEditDoc::DF_NO_TYPE:
			retval.typ = TYPE_BLOB;
			retval.int64 = ii;
			break;

		case CHexEditDoc::DF_CHAR:  // no longer used as a distinct type
			ASSERT(0);
			// fall through
		case CHexEditDoc::DF_CHARA:
		case CHexEditDoc::DF_CHARN:
		case CHexEditDoc::DF_CHARO:
		case CHexEditDoc::DF_CHARE:
			retval.typ = TYPE_INT;
			retval.int64 = *(unsigned char *)(buf);
			break;
		case CHexEditDoc::DF_WCHAR:
			retval.typ = TYPE_INT;
			retval.int64 = *(unsigned short *)(buf);
			break;
		case CHexEditDoc::DF_STRING:  // no longer used
			ASSERT(0);
			// fall through
		case CHexEditDoc::DF_STRINGA:
		case CHexEditDoc::DF_STRINGN:
		case CHexEditDoc::DF_STRINGO:
		case CHexEditDoc::DF_STRINGE:
			retval.typ = TYPE_STRING;
			{
				// Search for string terminator
				unsigned char *pp = (unsigned char *)memchr(buf, pdoc->df_extra_[ii], size_t(df_size));
				if (pp != NULL)
					retval.pstr = new ExprStringType((LPCTSTR)buf, pp-buf);
				else
					retval.pstr = new ExprStringType((LPCTSTR)buf, size_t(df_size));  // use full length of field
			}
			break;
		case CHexEditDoc::DF_WSTRING:
			retval.typ = TYPE_STRING;
			{
				// Search for string terminator
				ASSERT(df_size%2 == 0);  // Wide string must be even no of bytes
				const wchar_t *pp = wmemchr((wchar_t *)buf, (wchar_t)pdoc->df_extra_[ii], size_t(df_size/2));
				if (pp != NULL)
					retval.pstr = new ExprStringType((wchar_t *)buf, pp-(wchar_t *)buf);
				else
					retval.pstr = new ExprStringType((wchar_t *)buf, size_t(df_size/2));
			}
			break;
		case CHexEditDoc::DF_INT8:
			retval.typ = TYPE_INT;
			retval.int64 = *(signed char *)(buf);
			break;
		case CHexEditDoc::DF_INT16:
			retval.typ = TYPE_INT;
			retval.int64 = *(short *)(buf);
			break;
		case CHexEditDoc::DF_INT32:
			retval.typ = TYPE_INT;
			retval.int64 = *(long *)(buf);
			break;
		case CHexEditDoc::DF_INT64:
			retval.typ = TYPE_INT;
			retval.int64 = *(__int64 *)(buf);
			break;
		case CHexEditDoc::DF_UINT8:
			retval.typ = TYPE_INT;
			retval.int64 = *(unsigned char *)(buf);
			break;
		case CHexEditDoc::DF_UINT16:
			retval.typ = TYPE_INT;
			retval.int64 = *(unsigned short *)(buf);
			break;
		case CHexEditDoc::DF_UINT32:
			retval.typ = TYPE_INT;
			retval.int64 = *(unsigned long *)(buf);
			break;
		case CHexEditDoc::DF_UINT64:
			retval.typ = TYPE_INT;
			retval.int64 = *(unsigned __int64 *)(buf);
			break;
		case CHexEditDoc::DF_MINT8:
			retval.typ = TYPE_INT;
			retval.int64 = *(signed char *)(buf);
			if (retval.int64 < 0)
				retval.int64 = -(retval.int64&0x7F);
			break;
		case CHexEditDoc::DF_MINT16:
			retval.typ = TYPE_INT;
			retval.int64 = *(short *)(buf);
			if (retval.int64 < 0)
				retval.int64 = -(retval.int64&0x7FFF);
			break;
		case CHexEditDoc::DF_MINT32:
			retval.typ = TYPE_INT;
			retval.int64 = *(long *)(buf);
			if (retval.int64 < 0)
				retval.int64 = -(retval.int64&0x7fffFFFFL);
			break;
		case CHexEditDoc::DF_MINT64:
			retval.typ = TYPE_INT;
			retval.int64 = *(__int64 *)(buf);
			if (retval.int64 < 0)
				retval.int64 = -(retval.int64&0x7fffFFFFffffFFFFi64);
			break;

		// Bitfields do not handle straddle - too hard
		case CHexEditDoc::DF_BITFIELD8:
			retval.typ = TYPE_INT;
			ASSERT(signed char(pdoc->df_extra_[ii]) >= 0 && (pdoc->df_extra_[ii]&0xFF)+(pdoc->df_extra_[ii]>>8) <= 8);
			retval.int64 = (*((unsigned char *)buf)>>(pdoc->df_extra_[ii]&0xFF)) & ((1<<(pdoc->df_extra_[ii]>>8))-1);
			break;
		case CHexEditDoc::DF_BITFIELD16:
			retval.typ = TYPE_INT;
			ASSERT(signed char(pdoc->df_extra_[ii]) >= 0 && (pdoc->df_extra_[ii]&0xFF)+(pdoc->df_extra_[ii]>>8) <= 16);
			retval.int64 = (*((unsigned short *)buf)>>(pdoc->df_extra_[ii]&0xFF)) & ((1<<(pdoc->df_extra_[ii]>>8))-1);
			break;
		case CHexEditDoc::DF_BITFIELD32:
			retval.typ = TYPE_INT;
			ASSERT(signed char(pdoc->df_extra_[ii]) >= 0 && (pdoc->df_extra_[ii]&0xFF)+(pdoc->df_extra_[ii]>>8) <= 32);
			retval.int64 = (*((unsigned long *)buf)>>(pdoc->df_extra_[ii]&0xFF)) & ((1<<(pdoc->df_extra_[ii]>>8))-1);
			break;
		case CHexEditDoc::DF_BITFIELD64:
			retval.typ = TYPE_INT;
			ASSERT(signed char(pdoc->df_extra_[ii]) >= 0 && (pdoc->df_extra_[ii]&0xFF)+(pdoc->df_extra_[ii]>>8) <= 64);
			retval.int64 = (*((unsigned __int64 *)buf)>>(pdoc->df_extra_[ii]&0xFF)) & ((1<<(pdoc->df_extra_[ii]>>8))-1);
			break;

		case CHexEditDoc::DF_REAL32:
			retval.typ = TYPE_REAL;
			retval.real64 = *(float *)(buf);
			break;
		case CHexEditDoc::DF_REAL64:
			retval.typ = TYPE_REAL;
			retval.real64 = *(double *)(buf);
			break;

		case CHexEditDoc::DF_IBMREAL32:
			retval.typ = TYPE_REAL;
			retval.real64 = ::ibm_fp32(buf);
			break;
		case CHexEditDoc::DF_IBMREAL64:
			retval.typ = TYPE_REAL;
			retval.real64 = ::ibm_fp64(buf);
			break;
		case CHexEditDoc::DF_REAL48:
			retval.typ = TYPE_REAL;
			retval.real64 = ::real48(buf);
			break;

		case CHexEditDoc::DF_DATEC:
			{
				retval.typ = TYPE_DATE;
				COleDateTime odt(*((time_t *)buf));
				retval.date = (odt.m_status == 0 ? odt.m_dt : -1e30);
			}
			break;
#ifdef TIME64_T
		case CHexEditDoc::DF_DATEC64:
			{
				retval.typ = TYPE_DATE;
				COleDateTime odt(*((__time64_t *)buf));
				retval.date = (odt.m_status == 0 ? odt.m_dt : -1e30);
			}
			break;
#endif
		case CHexEditDoc::DF_DATEC51:
			{
				retval.typ = TYPE_DATE;
				COleDateTime odt(time_t(*((time_t *)buf) + (365*10 + 2)*24L*60L*60L));
				retval.date = (odt.m_status == 0 ? odt.m_dt : -1e30);
			}
			break;
		case CHexEditDoc::DF_DATEC7:
			{
				retval.typ = TYPE_DATE;
				COleDateTime odt(time_t(*((time_t *)buf) - (365*70 + 17 + 2)*24UL*60UL*60UL));
				retval.date = (odt.m_status == 0 ? odt.m_dt : -1e30);
			}
			break;
		case CHexEditDoc::DF_DATECMIN:
			{
				retval.typ = TYPE_DATE;
				COleDateTime odt(time_t(*((time_t *)buf) * 60));
				retval.date = (odt.m_status == 0 ? odt.m_dt : -1e30);
			}
			break;
		case CHexEditDoc::DF_DATEOLE:
			retval.typ = TYPE_DATE;
			retval.date = *((DATE *)buf);
			break;
		case CHexEditDoc::DF_DATESYSTEMTIME:
			{
				retval.typ = TYPE_DATE;
				COleDateTime odt(*((SYSTEMTIME *)buf));
				retval.date = (odt.m_status == 0 ? odt.m_dt : -1e30);
			}
			break;
		case CHexEditDoc::DF_DATEFILETIME:
			{
				retval.typ = TYPE_DATE;
				COleDateTime odt(*((FILETIME *)buf));
				retval.date = (odt.m_status == 0 ? odt.m_dt : -1e30);
			}
			break;
		case CHexEditDoc::DF_DATEMSDOS:
			{
				retval.typ = TYPE_DATE;
				FILETIME ft;
				if (DosDateTimeToFileTime(*(LPWORD(buf+2)), *(LPWORD(buf)), &ft))
				{
					COleDateTime odt(ft);
					retval.date = (odt.m_status == 0 ? odt.m_dt : -1e30);
				}
				else
					retval.date = -1e30;
			}
			break;
		default:
			ASSERT(0);
			break;
		}

		// If real check that its not invalid
		if (retval.typ == TYPE_REAL)
		{
			switch (_fpclass(retval.real64))
			{
			case _FPCLASS_SNAN:
			case _FPCLASS_QNAN:
				// df_mess_ = "Invalid floating point number";
				retval.error = true;
				break;
			case _FPCLASS_NINF:
			case _FPCLASS_PINF:
				// df_mess_ = "Infinite floating point number";
				retval.error = true;
				break;
			}
		}
		if (large_buf != NULL)
			delete[] large_buf;
	}

	return retval;
}

#if 0  // This was in the DTD but caused problems
/*
<!--
General attributes
 name = uniquely identifies this data element -- a name is not required if an element is a child of a "for" tag
 comment = can be used to describe the element or attach information that cannot be placed elsewehere
 type_name = a name that may not be unique (eg. may be name of C/C++ struct, class or typedef)

Attributes of binary_file_format
 default_byte_order = default byte order of data elements unless otherwise specified with byte_order attribute
 default_read_only = read only attribute of all data elements unless otherwsie specified with read_only
 default_char_set = which is the default character set used to display char data

Struct attributes
 name = structure name -- members are referenced using a dot then the member name (eg name.member)

For attributes
 name = array (for) name -- sub-elements are referenced using square brackets and an integer index (eg name[2])
 count = an expression that evaluates to an integer specifying the number of elements (may be zero)
 stop_test = an expression that evaluates to a Boolean (true/false) value saying this is the last element of the array
 if a count and a stop_test are specified whichever occurs first determines the size
 if neither are specified then the array goes all the way to the end of file

Data attributes
 type = any supported primitive data type (see below)
 format = with the type. length and byte_order specifies exactly how the data is stored and how big it is
 length = specifies the size if the type can have more than one size
 byte_order = big- or little-endian -- if not specified or "default" then default_byte_order is used
 read_only = determines whether these bytes can be modified -- if not specified then default_read_only is used
 domain = determines the set of valid values -- this may be individual values or ranges
 units = for numeric values this says what units are used if any
 display = determines how the values are shown to the user (see below)

Note that C/C++ printf format specifies must use the dollar character (|) rather
than the percent character (%) as XML DTDs use % for entity parameters.

				  TYPE   FORMAT  LENGTH    DISPLAY
DF_NO_TYPE        none   -       <length>  none
DF_INT8           int    -       8         C/C++ printf format specifiers (%d, %x, %X etc)
DF_INT16          int    -       16
DF_INT32          int    -       -
DF_INT64          int    -       64
DF_UINT8          int    u       8
DF_UINT16         int    u       16
DF_UINT32         int    u       -
DF_UINT64         int    u       64
DF_MINT8          int    m       8
DF_MINT16         int    m       16
DF_MINT32         int    m       -
DF_MINT64         int    m       64
DF_CHAR           char   -
DF_CHARA          char   -       ascii
DF_CHARN          char   -       ansi
DF_CHARO          char   -       oem
DF_CHARE          char   -       ebcdic
DF_WCHAR          char   -       unicode
DF_STRING         string -       <length>
DF_STRINGA        string ascii   <length>
DF_STRINGN        string ansi    <length>
DF_STRINGO        string oem     <length>
DF_STRINGE        string ebcdic  <length>
DF_WSTRING        string unicode <length>
DF_REAL32         real   -       32        C/C++ printf format specifiers (%g, %e, %f etc)
DF_REAL64         real   -       -
DF_IBMREAL32      real   ibm     32
DF_IBMREAL64      real   ibm     -
DF_REAL48         real   borland -
DF_DATEC          date   -       c         C/C++ strftime format (%c, %#c, %#x etc)
DF_DATEC51        date   -       c51
DF_DATEC7         date   -       c7
DF_DATECMIN       date   -       cmin
DF_DATEOLE        date   -       ole
DF_DATESYSTEMTIME date   -       systemtime
DF_DATEFILETIME   date   -       filetime
DF_DATEMSDOS      date   -       msdos
DF_DATEC          date   -       c64
-->
*/
#endif

