// clang++ -stdlib=libc++ -std=c++1y  tests/lazy_ptr_test.cc && ./a.out
#include <iostream>
#include "../src/lazy_ptr.h"
#include <string>
#include <ostream>

using std::cout;
using std::shared_ptr;
using std::string;

class A
{
	virtual int x() { return 1; }
};
class B : public A
{
	virtual int x() { return 2; }
};

void print(lazy_ptr<std::string> &s)
{
	if (!s.get()) {
		cout << "null\n";
	}
	else {
		cout << *s.get() << "\n";
	}
}
int main()
{
	cout << "OK\n";

	{
		lazy_ptr<string> p1 = new string("foo");
		lazy_ptr<string> p2 = new string("bar");
		lazy_ptr<string> ps =
				lazy_ptr_op<string>([p1, p2]() -> string * { return new string(*p1 + *p2); });
		print(ps);
	}
	{
		lazy_ptr<string> s;
		s = new string("123");

		print(s);

		shared_ptr<string> sp = s;
		string &rs = *s.get();
	}

	{
		lazy_ptr<string> ps;
		lazy_ptr<const string> pcs;
		pcs = ps;
	}
	{
		shared_ptr<string> sp;
		shared_ptr<const string> scp;
		scp = sp;
	}
	{
		shared_ptr<string> sp;
		lazy_ptr<const string> fcp;
		fcp = sp;
	}
	{
		string *ps;
		const string *pcs;
		pcs = ps;
	}
	{
		lazy_ptr<A> pa;
		lazy_ptr<B> pb;
		pb = dynamic_pointer_cast<B>(pa);
	}
	{
		lazy_ptr<A> pa;
		lazy_ptr<B> pb;
		pb = static_pointer_cast<B>(pa);
	}
	{
		lazy_ptr<A> fp;
		A *f;
		f = fp;
		fp = f;
		f = fp.get();

		A &r = *fp;
	}
	{
		lazy_ptr<string> p = lazy_ptr_op<string>([] { return new string("lazy_ptr_op"); });
		print(p);
	}
}
