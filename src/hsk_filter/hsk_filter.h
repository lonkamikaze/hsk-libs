/** \file
 * HSK Filter generator
 *
 * This file offers preprocessor macros to filter analogue values, by
 * calculating the average of a set of a given length.
 *
 * The buffer for the filter is stored in xdata memory.
 *
 * @author kami
 */

#ifndef _HSK_FILTER_H_
#define _HSK_FILTER_H_

#include <Infineon/XC878.h>

#include <string.h> /* memset() */

/**
 * Generates a filter.
 *
 * The filter can be accessed with:
 * - void \<prefix\>_init(void)
 *	- Initializes the filter with 0
 * - \<valueType\> \<prefix\>_update(const \<valueType\> value)
 *	- Update the filter and return the current average
 *
 * @param prefix
 *	A prefix for the generated internals and functions
 * @param valueType
 *	The data type of the stored values
 * @param sumType
 *	A data type that can contain the sum of all buffered values
 * @param sizeType
 *	A data type that can hold the length of the buffer
 * @param size
 *	The length of the buffer
 */
#define FILTER_FACTORY(prefix, valueType, sumType, sizeType, size) \
	\
	/**
	 * Holds the buffer and its current state.
	 */ \
	struct { \
		/**
		 * The value buffer.
		 */ \
		valueType values[size]; \
	\
		/**
		 * The sum of the buffered values.
		 */ \
		sumType sum; \
	\
		/**
		 * The index of the oldest buffered value.
		 */ \
		sizeType current; \
	} xdata prefix; \
	\
	/**
	 * Initializes the buffer with 0.
	 */ \
	void prefix##_init(void) { \
		memset(&prefix, 0, sizeof(prefix)); \
	} \
	\
	/**
	 * Updates the filter and returns the current sliding average of
	 * buffered values.
	 *
	 * @param value
	 *	The value to add to the buffer
	 * @return
	 *	The average of the buffed values
	 */ \
	valueType prefix##_update(const valueType value) { \
		prefix.sum -= prefix.values[prefix.current]; \
		prefix.values[prefix.current++] = value; \
		prefix.sum += value; \
		prefix.current %= size; \
		return prefix.sum / size; \
	} \


/**
 * Generates a group of filters.
 *
 * The filters can be accessed with:
 * - void \<prefix\>_init(void)
 *	- Initializes all filters with 0
 * - \<valueType\> \<prefix\>_update(const ubyte filter, const \<valueType\> value)
 *	- Update the given filter and return the current average
 *
 * @param prefix
 *	A prefix for the generated internals and functions
 * @param filters
 *	The number of filters
 * @param valueType
 *	The data type of the stored values
 * @param sumType
 *	A data type that can contain the sum of all buffered values
 * @param sizeType
 *	A data type that can hold the length of the buffer
 * @param size
 *	The length of the buffer
 */
#define FILTER_GROUP_FACTORY(prefix, filters, valueType, sumType, sizeType, size) \
	\
	/**
	 * Holds the buffers and their current states.
	 */ \
	struct { \
		/**
		 * The value buffer.
		 */ \
		valueType values[size]; \
	\
		/**
		 * The sum of the buffered values.
		 */ \
		sumType sum; \
	\
		/**
		 * The index of the oldest buffered value.
		 */ \
		sizeType current; \
	} xdata prefix[filters]; \
	\
	/**
	 * Initializes all buffers with 0.
	 */ \
	void prefix##_init(void) { \
		memset(&prefix, 0, sizeof(prefix)); \
	} \
	\
	/**
	 * Updates the given filter and returns the current sliding average of
	 * buffered values.
	 *
	 * @param filter
	 *	The filter to update
	 * @param value
	 *	The value to add to the buffer
	 * @return
	 *	The average of the buffed values
	 */ \
	valueType prefix##_update(const ubyte filter, const valueType value) { \
		prefix[filter].sum -= prefix[filter].values[prefix[filter].current]; \
		prefix[filter].values[prefix[filter].current++] = value; \
		prefix[filter].sum += value; \
		prefix[filter].current %= size; \
		return prefix[filter].sum / size; \
	} \


#endif /* _HSK_FILTER_H_ */

