// Listing 1

#ifndef RANGE_SET_H
#define RANGE_SET_H

// Copyright 1999 (c) Andrew W. Phillips
// You can freely use this software for any purpose
// as long as you preserve this copyright notice.

#include <list>
#include <iterator>
#include <utility>                  // for make_pair<T1,T2>()   
#include <functional>               // for less<T>
#include <istream>                  // used in << and >>
#include <ios>
#include <cassert>                  // for assert()

template <class T, class Pred = std::less<T>,
		  class Alloc = std::allocator<T> >
class range_set
{
public:
	// Define standard types for STL container
	typedef typename Alloc allocator_type;
	typedef typename Alloc::size_type size_type;
	typedef typename Alloc::difference_type difference_type;
	typedef typename Alloc::pointer pointer;
	typedef typename Alloc::const_pointer const_pointer;
	typedef typename Alloc::reference reference;
	typedef typename Alloc::const_reference const_reference;
	typedef typename Alloc::value_type value_type;
	typedef typename Pred value_compare;
	typedef typename value_type key_type;        // Value == key for sets
	typedef typename value_compare key_compare;

	// All iterators are const (mods via iterators not allowed)
	class const_iterator;
	typedef const_iterator iterator;

	// Class segment: one contiguous block from the set
	// Note: we could have just used std::pair<T,T> rather
	// than creating this class, but this may have hindered
	// future additions.
	struct segment
	{
		T sfirst;           // Bottom of segment
		T slast;            // One past the top of the segment

		// Constructor
		segment(T fst, T lst) : sfirst(fst), slast(lst) { }
	};
	typedef std::list<segment> range_t;
	mutable range_t range_; // All segments for this range_set

private:
	friend class const_iterator;

	Pred compare_;          // Object used for comparing elts
	Alloc allocator_;       // Object used for allocating elts
	bool increasing_;       // compare_ uses increasing order?

	// NOTE: When member variables are added: member swap() must
	//     be updated, perhaps and operator==() and operator<().

	std::pair<const_iterator, const_iterator>
	insert_helper(typename range_t::iterator pp, const T &ss,const T &ee)
	{
		if (!compare_(ss, ee))
			return std::make_pair(end(), end());

		T new_end = ee;     // Keep end since we have to modify

		// Make sure hint is at or before the correct segment
		range_t::iterator prev = pp;
		for ( ; pp != range_.begin(); pp = prev)
		{
			--prev;
			if (compare_(prev->slast, ss))
				break;
		}

		// Search until we find where the new range fits or
		// we reach the end of the list
		for ( ; pp != range_.end() && 
				!compare_(new_end, pp->sfirst); ++pp)
		{
			if (!compare_(pp->slast, ss))
			{
				// The new range overlaps this one.  At least
				// we don't need to add any list elements
				// (although we may need to delete some).

				// Make sure we retain all of the current seg
				if (compare_(ss, pp->sfirst))
					pp->sfirst = ss;
				if (compare_(new_end, pp->slast))
					new_end = pp->slast;

				// Delete following ranges subsumed (if any)
				range_t::iterator pnext;
				for (pnext = pp, ++pnext;
					 pnext != range_.end() && 
						 !compare_(new_end,pnext->sfirst);
					 pnext = pp, ++pnext)
				{
					// Delete the segment but make sure we
					// retain up to the end of it
					if (compare_(new_end, pnext->slast))
						new_end = pnext->slast;
					range_.erase(pnext);
				}

				pp->slast = new_end;
				return std::make_pair(
					const_iterator(pp, this, ss),
					const_iterator(pnext, this,
								   pnext==range_.end() ? 
									   T() : pnext->sfirst));
			}
		}

		// Add a new range in the gap before pp or at the end
		range_.insert(pp, segment(ss, new_end));
		range_t::iterator pprev = pp; --pprev;

		// pprev now points to the new seg inserted before pp
		return std::make_pair(
			const_iterator(pprev, this, ss),
			const_iterator(pp, this,
						   pp==range_.end() ? T() :pp->sfirst));
	}
	const_iterator erase_helper(typename range_t::iterator pp, 
								const T &ss, const T &ee)
	{
		if (!compare_(ss, ee)) return lower_bound(ss);

		// Make sure hint is at or before the correct segment
		range_t::iterator prev = pp;
		for ( ; pp != range_.begin(); pp = prev)
		{
			--prev;
			if (compare_(prev->slast, ss))
				break;
		}

		for ( ; pp != range_.end(); ++pp)
		{
			if (!compare_(pp->sfirst, ee))
			{
				// Nothing to erase (all in gap before this seg)
				return const_iterator(pp, this, pp->sfirst);
			}
			else if (!compare_(pp->sfirst, ss) && 
					 compare_(ee, pp->slast))
			{
				// Chop off bottom of current range only
				pp->sfirst = ee;
				return const_iterator(pp, this, ee);
			}
			else if (compare_(ee, pp->slast))
			{
				// Split this range in twain
				range_.insert(pp, segment(pp->sfirst, ss));
				pp->sfirst = ee;
				return const_iterator(pp, this, ee);
			}
			else if (compare_(ss, pp->slast))
			{
				// Chops off top of this range
				if (compare_(pp->sfirst, ss))
				{
					pp->slast = ss;
					++pp;
				}

				// Delete ranges that are totally enclosed
				while (pp != range_.end() && 
					   !compare_(ee, pp->slast))
				{
					range_t::iterator pnext = pp; ++pnext;
					range_.erase(pp);
					pp = pnext;
				}

				// Chop off bottom of last if necessary
				if (pp != range_.end() && 
					compare_(pp->sfirst, ee))
				{
					pp->sfirst = ee;
				}

				if (pp != range_.end())
					return const_iterator(pp, this, pp->sfirst);
				else
					return end();
			}
		} /* for */

		// Nothing to erase (all are past the last segment)
		return end();
	}

	T one_more(const T &v) const
	{
		// Since we store "half-open" ranges we need a way to
		// work out what is "one more" than a value.  If we're
		// using less<T> for comparisons this is done by adding
		// 1 but if using greater<T> then we must subtract.
		return increasing_ ? v + T(1) : v - T(1);
	}
	T one_less(const T &v) const
	{
		// We similarly obtain the value "before" another value.
		return increasing_ ? v - T(1) : v + T(1);
	}

public:
	// Iterators
	class const_iterator :
		public std::iterator<std::bidirectional_iterator_tag,
							 T, difference_type>
	{
	private:
		// Since the container does not "contain" all the actual
		// values the iterator itself must contain the current
		// value pointed to.  We must also store an iterator
		// (pr_) into the list of segments so we know what the
		// valid values are.  Note that pr_ is declared "mutable"
		// to avoid having a separate iterator & const_iterator.
		T value_;                   // The value "pointed" to
		mutable typename range_t::iterator pr_;// Segment with the value

		// We must also access a few things in the container.
		const range_set *pc_;       // Pointer to container

		const_iterator(const typename range_t::iterator &p,
					   const range_set *pc, const T &v)
			: pr_(p), pc_(pc), value_(v)
		{
			// Check that we are creating a valid iterator
			assert(range_t::const_iterator(pr_) == 
										pc_->range_.end() || 
				   (!pc_->compare_(v, pr_->sfirst) && 
					pc_->compare_(v, pr_->slast)));
		}

	public:
		friend class range_set<T,Pred,Alloc>;
		const_iterator() : pc_(0)   // Default constructor
		{
		}

		// Use compiler generated copy constructor,
		// copy assignment operator, and destructor

		const_reference operator*() const
		{
			// Check that the iterator is valid and
			// that we don't dereference end()
			assert(pc_ != 0);
			assert(pr_ != pc_->range_.end());

			return value_;
		}
		const_pointer operator->() const
		{
			assert(pc_ != 0);
			assert(pr_ != pc_->range_.end());

			return &value_;
		}
		const_iterator &operator++()
		{
			// Check that the iterator is valid and
			// that we don't try to move past the end()
			assert(pc_ != 0);
			assert(pr_ != pc_->range_.end());

			value_ = pc_->one_more(value_);
			if (!pc_->compare_(value_,pr_->slast) &&
				++pr_ != pc_->range_.end())
			{
				value_ = pr_->sfirst;
			}
			return *this;
		}
		const_iterator operator++(int)
		{
			assert(pc_ != 0);           // Ensure iter is valid
			const_iterator tmp(*this);
			++*this;
			return tmp;
		}
		const_iterator &operator--()
		{
			assert(pc_ != 0);           // Ensure iter is valid

			// Check that we don't move back before start
			assert(pc_->range_.begin() != pc_->range_.end());
			assert(pr_ != pc_->range_.begin() ||
				   pc_->compare_(pr_->sfirst, value_));

			value_ = pc_->one_less(value_);
			if (pr_ == pc_->range_.end() || 
				pc_->compare_(value_, pr_->sfirst))
			{
				--pr_;
				value_ = pc_->one_less(pr_->slast);
			}
			return *this;
		}
		const_iterator operator--(int)
		{
			assert(pc_ != 0);           // Ensure iter is valid
			const_iterator tmp(*this);
			--*this;
			return tmp;
		}
		bool operator==(const const_iterator& p) const
		{
			assert(pc_ != 0);           // Ensure iter is valid
			assert(pc_ == p.pc_);       // Same container?

			if (pr_ != p.pr_)
				// Different segments so they must be different
				return false;
			else if (pr_ == pc_->range_.end())
				// Both are at end so they compare the same
				return true;
			else
				// The iterators are now the same if their 
				// value_s are the same.  The following tests
				// for equality using compare_.
				// [Note that A == B is equivalent to
				//  A >= B && A <= B  or  !(A < B) && !(B < A)]
				return !pc_->compare_(value_, p.value_) && 
					   !pc_->compare_(p.value_, value_);
		}
		bool operator!=(const const_iterator& p) const
		{
			return !operator==(p);
		}
	};

	// Create a reverse iterator based on normal (forward) iter
	typedef std::reverse_bidirectional_iterator<const_iterator,
								value_type, const_reference,
								const_pointer, difference_type>
			const_reverse_iterator;
	typedef const_reverse_iterator reverse_iterator;

	// Constructors
	explicit range_set(const Pred &p = Pred(),
					   const Alloc &a = Alloc())
		: compare_(p), allocator_(a)
	{
		increasing_ = compare_(T(0), T(1));
	}
#ifdef MEMBER_TEMPLATES
	template<Init> range_set(Init p1, const Init &p2,
		const Pred &p = Pred(), const Alloc &a = Alloc())
		: compare_(p), allocator_(a)
	{
		increasing_ = compare_(T(0), T(1));

		const_iterator last_pos = begin();
		while (p1 != p2)
			last_pos = insert(last_pos, *p1++);
	}
#else
	range_set(const T *p1, const T *p2,
			  const Pred &p = Pred(), const Alloc &a = Alloc())
		: compare_(p), allocator_(a)
	{
		increasing_ = compare_(T(0), T(1));

		const_iterator last_pos = begin();
		while (p1 != p2)
			last_pos = insert(last_pos, *p1++);
	}
	range_set(const_iterator p1, const const_iterator &p2,
			  const Pred &p = Pred(), const Alloc &a = Alloc())
		: compare_(p), allocator_(a)
	{
		assert(p1.pc_ == p2.pc_);       // Same container?
		assert(p1.pc_ != 0);            // ... and valid?

		increasing_ = compare_(T(0), T(1));

		const_iterator last_pos = begin();
		while (p1 != p2)
			last_pos = insert(last_pos, *p1++);
	}
#endif

	// Use compiler generated copy constructor,
	// copy assignment operator, and destructor

	const_iterator begin() const
	{
		if (range_.empty())
			return const_iterator(range_.end(), this, T());
		else
			return const_iterator(range_.begin(), this, 
								  range_.begin()->sfirst);
	}
	const_iterator end() const
	{
		return const_iterator(range_.end(), this, T());
	}
	const_reverse_iterator rbegin() const
	{
		return const_reverse_iterator(end());
	}
	const_reverse_iterator rend() const
	{
		return const_reverse_iterator(begin());
	}

	// Allocator
	Alloc get_allocator() const
	{
		return allocator_;
	}

	// Size info
	size_type max_size() const
	{
		// The worst case max is the same as the range_t max
		return range_.max_size();
	}
	size_type size() const
	{
		range_t::const_iterator pp;
		size_type total = 0;

		if (increasing_)
			for (pp = range_.begin(); pp != range_.end(); ++pp)
				total +=  size_type(pp->slast - pp->sfirst);
		else
			for (pp = range_.begin(); pp != range_.end(); ++pp)
				total +=  size_type(pp->sfirst - pp->slast);
		return total;
	}
	bool empty() const
	{
		return (size() == 0);
	}

	// Modifiers
	std::pair<const_iterator, bool> insert(const value_type &v)
	{
		const_iterator p;
		if ((p = find(v)) != end())
			return std::make_pair(p, false);   // Already in set
		else
			return std::make_pair(
					insert_helper(p.pr_, v, one_more(v)).first,
					true);
	}
	const_iterator insert(const const_iterator &p,
						  const value_type &v)
	{
		assert(p.pc_ == this);          // For this container?
		return insert_helper(p.pr_, v, one_more(v)).first;
	}
#ifdef MEMBER_TEMPLATES
	template<Init> void insert(Init p1, const Init &p2)
	{
		const_iterator last_pos = begin();
		while (p1 != p2)
			last_pos = insert(last_pos, *p1++);
	}
#else
	void insert(const T* p1, const T* p2)
	{
		const_iterator last_pos = begin();
		while (p1 != p2)
			last_pos = insert(last_pos, *p1++);
	}
	void insert(const_iterator p1, const const_iterator &p2)
	{
		assert(p1.pc_ == p2.pc_);            // Same container?
		assert(p1.pc_ != this);              // but not this one
		assert(p1.pc_ != 0);                 // Ensure valid.

		const_iterator last_pos = begin();
		while (p1 != p2)
			last_pos = insert(last_pos, *p1++);
	}
#endif
	std::pair<const_iterator, const_iterator>
	insert_range(const value_type &ss, const value_type &ee)
	{
		return insert_helper(range_.begin(), ss, ee);
	}
	std::pair<const_iterator, const_iterator>
	insert_range(const const_iterator &p, const value_type &ss,
				 const value_type &ee)
	{
		assert(p.pc_ == this);          // For this container?
		return insert_helper(p.pr_, ss, ee);
	}

	const_iterator erase(const const_iterator &p)
	{
		assert(p.pc_ == this);         // For this container?
		return erase_helper(p.pr_, p.value_,one_more(p.value_));
	}
	const_iterator erase(const const_iterator &p1,
						 const const_iterator &p2)
	{
		// Check that the iterators are for this container
		assert(p1.pc_ == this && p2.pc_ == this);

		if (p1 == end())
			return end();

		T end_value;
		if (p2 == end())
		{
			// Note: if p2 == end() && the set is empty then we
			// should not use --tmp (p2.pr_ - 1) but in that
			// case p1 should == end() which is handled above.
			assert(begin() != end());

			// Erase everything up to the last value in the set.
			// Work out (one past) the last value in the set.
			range_t::iterator tmp = p2.pr_;
			--tmp;                 // tmp now points to last seg
			end_value = tmp->slast;
		}
		else
			end_value = p2.value_;
		return erase_helper(p1.pr_, p1.value_, end_value);
	}
	size_type erase(const key_type &k)
	{
		// Returns 0 if the value (k) was not present;
		// returns 1 (and removes it) if it was
		const_iterator p;
		if ((p = find(k)) != end())
		{
			(void)erase_helper(p.pr_, k, one_more(k));
			return 1;
		}
		else
			return 0;
	}
	const_iterator erase_range(const key_type &ss,
							   const key_type &ee)
	{
		return erase_helper(range_.begin(), ss, ee);
	}
	const_iterator erase_range(const const_iterator &p,
					   const key_type &ss, const key_type &ee)
	{
		assert(p.pc_ == this);         // For this container?
		return erase_helper(p.pr_, ss, ee);
	}

	void clear()
	{
		range_.clear();
	}
	void swap(range_set &rr)
	{
		range_.swap(rr.range_);
		std::swap(compare_, rr.compare_);
		std::swap(allocator_, rr.allocator_);
		std::swap(increasing_, rr.increasing_);
	}

	// Comparison
	key_compare key_comp() const
	{
		return compare_;
	}
	value_compare value_comp() const
	{
		return compare_;
	}

	// Searches
	const_iterator find(const key_type &k) const
	{
		for (range_t::iterator pp = range_.begin();
			 pp != range_.end(); ++pp)
		{
			if (compare_(k, pp->sfirst))
				break;                      // Return end()
			else if (compare_(k, pp->slast))
				return const_iterator(pp, this, k); // Found!
		}

		return const_iterator(range_.end(), this, T());
	}
	size_type count(const key_type &k) const
	{
		return (find(k) == end()) ? 0 : 1;
	}

	const_iterator lower_bound(const key_type &k) const
	{
		range_t::iterator pp;
		for (pp = range_.begin();
			 pp != range_.end(); ++pp)
		{
			if (compare_(k, pp->sfirst))
				return const_iterator(pp, this, pp->sfirst);
			else if (compare_(k, pp->slast))
				return const_iterator(pp, this, k);    // Found!
		}

		return const_iterator(pp, this, T());   // Return end()
	}
	const_iterator upper_bound(const key_type &k) const
	{
		return lower_bound(one_more(k));
	}
	std::pair<const_iterator, const_iterator>
	equal_range(const key_type &k) const
	{
		const_iterator low = lower_bound(k);
		const_iterator up = low;

		// If lower bound == value then upper bound is next,
		// else lower == upper
		if (low != end() && !compare_(k, *low))
			++up;
		return std::make_pair(low, up);
	}

	// Get a range_set as zero or more comma-separated
	// ranges (segments). Each segment is a single value
	// or 2 values separated by ':' or '-'.
	friend std::istream &operator>>(std::istream &ss, 
									range_set &rr)
	{
		rr.clear();
		range_set::const_iterator pp = rr.end();
		T v1, v2;

		// If the set is non-empty
		if (ss >> v1)
		{
			v2 = v1;                    // May be a single value
			char cc = '\0';
			ss >> cc;

			// While we havent reached the end (unexpected char)
			while (cc == '-' || cc == ':' || cc == ',')
			{
				// Allow colon or minus as range character
				if (cc == ':' || cc == '-')
				{
					// Get the end value of the segment and
					// check its >= the start value
					if (!(ss >> v2) || rr.compare_(v2, v1))
					{
						ss.clear(std::ios_base::failbit);
						return ss;
					}
				}
				else
				{
					// This is the start of new segment so we 
					// can save the previous one.
					pp = rr.insert_range(pp, v1, 
										rr.one_more(v2)).second;

					if (!(ss >> v1))
					{
						ss.clear(std::ios_base::failbit);
						return ss;
					}

					v2 = v1;            // May be a single value
				}

				cc = '\0';
				ss >> cc;
			}
			(void)rr.insert_range(pp, v1, rr.one_more(v2));
			if (ss && cc != '\0')
				ss.putback(cc);
			else if (!ss && cc == '\0')
				ss.clear();
		}
		return ss;
	}
	friend std::ostream &operator<<(std::ostream &ss,
									const range_set &rr)
	{
		if (rr.range_.begin() != rr.range_.end())
		{
			range_set::range_t::const_iterator pp;
			for (pp = rr.range_.begin(); ; )
			{
				T vv = rr.one_less(pp->slast); // Last seg value
				if (!rr.compare_(pp->sfirst,vv) && 
					!rr.compare_(vv,pp->sfirst))
				{
					ss << pp->sfirst;
				}
				else
					ss << pp->sfirst << ":" << vv;

				if (++pp == rr.range_.end())
					break;
				ss << ",";
			}
		}
		return ss;
	}

	friend bool operator==(const range_set &rs1,
						   const range_set &rs2)
	{
		range_t::const_iterator p1, p2;
		for (p1 = rs1.range_.begin(), p2 = rs2.range_.begin(); 
			 p1 != rs1.range_.end() && p2 != rs2.range_.end();
			 ++p1, ++p2)
		{
			// Check that the next segments are the same
			// Note that A != B is equiv. to (A < B) || (B < A)
			if (rs1.compare_((*p1).sfirst, (*p2).sfirst) ||
				rs1.compare_((*p2).sfirst, (*p1).sfirst) ||
				rs1.compare_((*p1).slast,  (*p2).slast)  ||
				rs1.compare_((*p2).slast,  (*p1).slast)  )
			{
				return false;
			}
		}
		// All segments are the same to here, but 
		// are we at the end of both?
		return p1 == rs1.range_.end() && p2 == rs2.range_.end();
	}

	friend bool operator!=(const range_set &rs1, 
						   const range_set &rs2)
	{
		return !(rs1 == rs2);
	}

	friend bool operator<(const range_set &rs1,
						  const range_set &rs2)
	{
		range_t::const_iterator p1, p2;
		for (p1 = rs1.range_.begin(), p2 = rs2.range_.begin();
			 p1 != rs1.range_.end() && p2 != rs2.range_.end();
			 ++p1, ++p2)
		{
			// Compare the next elt of each set
			if (rs1.compare_((*p1).sfirst, (*p2).sfirst))
				return true;
			else if (rs1.compare_((*p2).sfirst, (*p1).sfirst))
				return false;

			if (rs1.compare_((*p1).slast, (*p2).slast))
			{
				// The rs1 segment is shorter than the rs2
				// segment so it seems that rs2 < rs1, UNLESS
				// we have reached the end of rs1
				range_t::const_iterator t = p1;
				++t;
				return t == rs1.range_.end();
			}
			else if (rs1.compare_((*p2).slast, (*p1).slast))
			{
				// The rs2 segment is shorter than the rs1
				// segment so it seems that rs1 < rs2, UNLESS
				// we have reached the end of rs2
				range_t::const_iterator t = p2;
				++t;
				return t != rs2.range_.end();
			}
		}
		// If we haven't reached the end of rs2 then rs2 >= rs1
		return p2 != rs2.range_.end();
	}

	friend bool operator>(const range_set &rs1,
						  const range_set &rs2)
	{
		return rs2 < rs1;
	}

	friend bool operator<=(const range_set &rs1,
						   const range_set &rs2)
	{
		return !(rs2 < rs1);
	}

	friend bool operator>=(const range_set &rs1,
						   const range_set &rs2)
	{
		return !(rs1 < rs2);
	}
};

#endif
