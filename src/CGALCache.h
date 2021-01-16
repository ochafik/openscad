#pragma once

#include "cache.h"
#include "memory.h"
#include "lazy_ptr.h"

/*!
*/
class CGALCache
{
public:
	CGALCache(size_t limit = 100*1024*1024);

	static CGALCache *instance() { if (!inst) inst = new CGALCache; return inst; }

	bool contains(const std::string &id) const { return this->cache.contains(id); }
	lazy_ptr<const class CGAL_Nef_polyhedron> get(const std::string &id) const;
	bool insert(const std::string &id, const lazy_ptr<const CGAL_Nef_polyhedron> &N);
	size_t maxSizeMB() const;
	void setMaxSizeMB(size_t limit);
	void clear();
	void print();

private:
	static CGALCache *inst;

	struct cache_entry {
		lazy_ptr<const CGAL_Nef_polyhedron> N;
		std::string msg;
		cache_entry(const lazy_ptr<const CGAL_Nef_polyhedron> &N);
		~cache_entry() { }
	};

	Cache<std::string, cache_entry> cache;
};
