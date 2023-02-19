/**
 * repeatedptrfield.traits.hxx
 */
#ifndef ODB_CONTAINER_REPEATEDPTRFIELD_TRAITS_HXX
#define ODB_CONTAINER_REPEATEDPTRFIELD_TRAITS_HXX

#include <odb/pre.hxx>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/repeated_field.h>
#include <odb/container-traits.hxx>

namespace odb
{

template<typename V>
class access::container_traits<google::protobuf::RepeatedPtrField<V> >
{
public:
	static const container_kind kind = ck_ordered;
	static const bool smart = false;

	typedef google::protobuf::RepeatedPtrField<V> container_type;

	typedef V value_type;
	typedef int index_type;
	typedef ordered_functions<index_type, value_type> functions;
public:
	static void persist(const container_type& c, const functions& f)
	{
		// Index based access is just as fast as iterator based access for google::protobuf::RepeatedPtrField
		for (index_type i(0), n(c.size()); i < n; ++i)
			f.insert(i, c.Get(i));
	}

	static void load(container_type& c, bool more, const functions& f)
	{
		c.Clear();
		while (more)
		{
			index_type dummy;
			value_type *v = c.Add();
			more = f.select(dummy, *v);
		}
	}

	static void update(const container_type& c, const functions& f)
	{
		index_type n(c.size());
		f.delete_();
		for (index_type i(0); i < n; ++i)
		{
			f.insert(i, c.Get(i));
		}
	}

	static void erase(const functions& f)
	{
		f.delete_();
	}
};
}

#include <odb/post.hxx>

#endif
