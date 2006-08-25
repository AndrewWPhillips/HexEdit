#pragma once

class CHexEditView;
class CHexEditDoc;

class CNavManager
{
public:
	CNavManager(void) : pos_(-1), in_move_(false) { }

	void Add(LPCTSTR desc, LPCTSTR info, CHexEditView *pv, FILE_ADDRESS aa, FILE_ADDRESS ee, FILE_ADDRESS as);
	void GoBack(int count = -1);
	void GoForw(int count = 1);
	bool BackAllowed() { return pos_ + pos_offset() + 1 > 0; }
	//bool BackAllowed() { return v_.size() > 0; }
	bool ForwAllowed() { return pos_ < int(v_.size()) - 1; }

	void AddItems(CBCGPopupMenu* pMenuPopup, bool forward, UINT first, int max_items);
	CHexEditView *LastView() { return pos_ >= 0 && size_t(pos_) < v_.size() ? v_[pos_].pview_ : NULL; }
	bool InMove() { return in_move_; }
	void Adjust(CHexEditDoc *pdoc, FILE_ADDRESS aa, FILE_ADDRESS diff);
	CString GetDesc(bool back, int ii);
	CString GetInfo(bool back, int ii);

private:
	void do_move(int);
    bool is_open_view(CHexEditDoc *pdoc, CHexEditView *pview);
	int pos_offset();

	// This is one entry in the nav stack
	struct nav
	{
		nav(LPCTSTR desc, LPCTSTR info,
            CHexEditView *pv, CHexEditDoc *pd,
            LPCTSTR fn, bool ro,
			FILE_ADDRESS aa, FILE_ADDRESS ee, FILE_ADDRESS as)
		:
			desc_(desc),
            info_(info),
			pview_(pv),
			pdoc_(pd),
			fname_(fn),
			read_only_(ro),
			start_addr_(aa),
			end_addr_(ee),
			scroll_(as)
		{
		}

		CString desc_;          // Something that describes the nav point or how we got to it
        CString info_;          // More info: bytes from the address

		CHexEditView *pview_;   // Ptr to the view where nav point added (if still open)
		CHexEditDoc *pdoc_;     // Ptr to the doc (if still open)
		CString fname_;         // Name of file (so we can reopen it)
		bool read_only_;        // Was the file open read only?

		FILE_ADDRESS start_addr_;     // Location of nav point in the above file
		FILE_ADDRESS end_addr_;     // Location of nav point in the above file
		FILE_ADDRESS scroll_;   // Address of top left of window (so we can try to restore scroll posn)
	};

	std::vector<nav> v_;        // Stores all the nav points in a stack
	int pos_;                   // Current pos in the stack (usually v_.size() unless user nav'd backwards)
	bool in_move_;              // Disallow adding to nav pt stack if we are currently moving
};
