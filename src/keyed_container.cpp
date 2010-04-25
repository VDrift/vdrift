#include "keyed_container.h"
#include "unittest.h"
#include "quickprof.h"

#include <stdint.h>

#ifdef USE_COUNTING_INT_FOR_KEYED_CONTAINER

QT_TEST(counting_unique_generator_test)
{
	counting_unique_generator <unsigned int> counter;
	QT_CHECK_EQUAL(counter.num_allocated_keys(), 0);
	QT_CHECK_EQUAL(counter.num_free_keys(), 0);
	
	unsigned int mykey1 = counter.allocate();
	
	QT_CHECK_EQUAL(mykey1, 0);
	QT_CHECK_EQUAL(counter.num_allocated_keys(), 1);
	QT_CHECK_EQUAL(counter.num_free_keys(), 0);
	
	counter.release(mykey1);
	
	QT_CHECK_EQUAL(counter.num_allocated_keys(), 0);
	QT_CHECK_EQUAL(counter.num_free_keys(), 1);
	
	counter.release(mykey1);
	
	QT_CHECK_EQUAL(counter.num_allocated_keys(), 0);
	QT_CHECK_EQUAL(counter.num_free_keys(), 1);
	
	mykey1 = counter.allocate();
	
	QT_CHECK_EQUAL(counter.num_allocated_keys(), 1);
	QT_CHECK_EQUAL(counter.num_free_keys(), 0);
	
	unsigned int mykey2 = counter.allocate();
	
	QT_CHECK_EQUAL(counter.num_allocated_keys(), 2);
	QT_CHECK_EQUAL(counter.num_free_keys(), 0);
	QT_CHECK(mykey1 != mykey2);
}

QT_TEST(fast_counting_unique_generator_test)
{
	fast_counting_unique_generator <unsigned int> counter;
	
	unsigned int mykey1 = counter.allocate();
	
	QT_CHECK_EQUAL(mykey1, 0);
	
	counter.release(mykey1);
	
	counter.release(mykey1);
	
	mykey1 = counter.allocate();
	
	unsigned int mykey2 = counter.allocate();

	QT_CHECK(mykey1 != mykey2);
}

QT_TEST(keyed_container_test)
{
	QT_CHECK_EQUAL(sizeof(size_t),sizeof(uint32_t));
	QT_CHECK_EQUAL(sizeof(unsigned int),sizeof(uint32_t));
	
	keyed_container <int> data;
	QT_CHECK_EQUAL(sizeof(unsigned int),sizeof(keyed_container <int>::handle));
	QT_CHECK(data.empty());
	QT_CHECK_EQUAL(data.size(), 0);
	QT_CHECK(data.begin() == data.end());
	QT_CHECK(data.find(keyed_container <int>::handle()) == data.end());
	QT_CHECK(!data.contains(keyed_container <int>::handle()));
	
	keyed_container <int>::handle handle1 = data.insert(1);
	keyed_container <int>::handle handle2 = data.insert(2);
	keyed_container <int>::handle handle3 = data.insert(3);
	
	QT_CHECK(!(handle1 == handle2));
	QT_CHECK(!(handle1 == handle3));
	QT_CHECK(!(handle3 == handle2));
	QT_CHECK_EQUAL(data.size(), 3);
	QT_CHECK(!data.empty());
	QT_CHECK(data.contains(handle1));
	QT_CHECK(data.contains(handle2));
	QT_CHECK(data.contains(handle3));
	QT_CHECK(!data.contains(keyed_container <int>::handle()));
	QT_CHECK(data.find(keyed_container <int>::handle()) == data.end());
	QT_CHECK_EQUAL(data.get(handle1), 1);
	QT_CHECK_EQUAL(data.get(handle2), 2);
	QT_CHECK_EQUAL(data.get(handle3), 3);
	QT_CHECK_EQUAL(data.find(handle2)->second, 2);
	
	data.erase(handle2);
	data.erase(handle1);
	
	QT_CHECK_EQUAL(data.size(), 1);
	QT_CHECK(!data.empty());
	QT_CHECK(!data.contains(handle1));
	QT_CHECK(!data.contains(handle2));
	QT_CHECK(data.contains(handle3));
	QT_CHECK(!data.contains(keyed_container <int>::handle()));
	QT_CHECK(data.find(handle1) == data.end());
	QT_CHECK_EQUAL(data.get(handle3), 3);
	QT_CHECK_EQUAL(data.find(handle3)->second, 3);
	
	int count = 0;
	for (keyed_container <int>::iterator i = data.begin(); i != data.end(); i++)
	{
		QT_CHECK_EQUAL(i->second, 3);
		count++;
	}
	QT_CHECK_EQUAL(count, 1);
	
	handle1 = data.insert(1);
	
	QT_CHECK_EQUAL(data.size(), 2);
	QT_CHECK(!data.empty());
	QT_CHECK(data.contains(handle1));
	QT_CHECK(!data.contains(handle2));
	QT_CHECK(data.contains(handle3));
	QT_CHECK(!data.contains(keyed_container <int>::handle()));
	QT_CHECK(data.find(handle2) == data.end());
	QT_CHECK_EQUAL(data.get(handle1), 1);
	QT_CHECK_EQUAL(data.get(handle3), 3);
	QT_CHECK_EQUAL(data.find(handle3)->second, 3);
	
	data.clear();
	
	QT_CHECK(data.empty());
	QT_CHECK_EQUAL(data.size(), 0);
	QT_CHECK(data.begin() == data.end());
	QT_CHECK(data.find(keyed_container <int>::handle()) == data.end());
	QT_CHECK(!data.contains(keyed_container <int>::handle()));
}

#else //#ifndef 

QT_TEST(keyed_container_test)
{
	keyed_container <int> data;
	QT_CHECK(data.empty());
	QT_CHECK_EQUAL(data.size(), 0);
	QT_CHECK(data.begin() == data.end());
	#ifdef USEBOOST_FOR_KEYED_CONTAINER
	QT_CHECK(data.find(keyed_container <int>::handle()) == data.end());
	QT_CHECK(!data.contains(keyed_container <int>::handle()));
	#endif
	
	keyed_container <int>::handle handle1 = data.insert(1);
	keyed_container <int>::handle handle2 = data.insert(2);
	keyed_container <int>::handle handle3 = data.insert(3);
	
	QT_CHECK(!(handle1 == handle2));
	QT_CHECK(!(handle1 == handle3));
	QT_CHECK(!(handle3 == handle2));
	QT_CHECK_EQUAL(data.size(), 3);
	QT_CHECK(!data.empty());
	QT_CHECK(data.contains(handle1));
	QT_CHECK(data.contains(handle2));
	QT_CHECK(data.contains(handle3));
	#ifdef USEBOOST_FOR_KEYED_CONTAINER
	QT_CHECK(!data.contains(keyed_container <int>::handle()));
	QT_CHECK(data.find(keyed_container <int>::handle()) == data.end());
	#endif
	QT_CHECK_EQUAL(data.get(handle1), 1);
	QT_CHECK_EQUAL(data.get(handle2), 2);
	QT_CHECK_EQUAL(data.get(handle3), 3);
	
	#ifdef USEBOOST_FOR_KEYED_CONTAINER
	QT_CHECK_EQUAL(**data.find(handle2), 2);
	#else
	QT_CHECK_EQUAL(*data.find(handle2), 2);
	#endif
	
	data.erase(handle2);
	data.erase(handle1);
	
	QT_CHECK_EQUAL(data.size(), 1);
	QT_CHECK(!data.empty());
	QT_CHECK(!data.contains(handle1));
	QT_CHECK(!data.contains(handle2));
	QT_CHECK(data.contains(handle3));
	#ifdef USEBOOST_FOR_KEYED_CONTAINER
	QT_CHECK(!data.contains(keyed_container <int>::handle()));
	#endif
	QT_CHECK(data.find(handle1) == data.end());
	QT_CHECK_EQUAL(data.get(handle3), 3);
	#ifdef USEBOOST_FOR_KEYED_CONTAINER
	QT_CHECK_EQUAL(**data.find(handle3), 3);
	#else
	QT_CHECK_EQUAL(*data.find(handle3), 3);
	#endif
	
	int count = 0;
	for (keyed_container <int>::iterator i = data.begin(); i != data.end(); i++)
	{
		#ifdef USEBOOST_FOR_KEYED_CONTAINER
		QT_CHECK_EQUAL(**i, 3);
		#else
		QT_CHECK_EQUAL(*i, 3);
		#endif
		
		count++;
	}
	QT_CHECK_EQUAL(count, 1);
	
	handle1 = data.insert(1);
	
	QT_CHECK_EQUAL(data.size(), 2);
	QT_CHECK(!data.empty());
	QT_CHECK(data.contains(handle1));
	QT_CHECK(!data.contains(handle2));
	QT_CHECK(data.contains(handle3));
	#ifdef USEBOOST_FOR_KEYED_CONTAINER
	QT_CHECK(!data.contains(keyed_container <int>::handle()));
	#endif
	QT_CHECK(data.find(handle2) == data.end());
	QT_CHECK_EQUAL(data.get(handle1), 1);
	QT_CHECK_EQUAL(data.get(handle3), 3);
	#ifdef USEBOOST_FOR_KEYED_CONTAINER
	QT_CHECK_EQUAL(**data.find(handle3), 3);
	#else
	QT_CHECK_EQUAL(*data.find(handle3), 3);
	#endif
	
	data.clear();
	
	QT_CHECK(data.empty());
	QT_CHECK_EQUAL(data.size(), 0);
	QT_CHECK(data.begin() == data.end());
	#ifdef USEBOOST_FOR_KEYED_CONTAINER
	QT_CHECK(data.find(keyed_container <int>::handle()) == data.end());
	QT_CHECK(!data.contains(keyed_container <int>::handle()));
	#endif
}

#endif
