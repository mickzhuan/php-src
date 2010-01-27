/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Hans-Peter Oeri (University of St.Gallen) <hp@oeri.ch>      |
   +----------------------------------------------------------------------+
 */

#include <php.h>
#include <zend.h>
#include <zend_API.h>

#include "resourcebundle/resourcebundle.h"
#include "resourcebundle/resourcebundle_class.h"
#include "resourcebundle/resourcebundle_iterator.h"

/*
 * Although libicu offers iterator functions, they are not used here: libicu does iterate
 * irrespective of array indices. Those cannot be recreated afterwards. Arrays as well as tables
 * can however be accessed by numerical index, with table keys readable ex post.
 */

/* {{{ resourcebundle_iterator_read */
static void resourcebundle_iterator_read( ResourceBundle_iterator *iterator TSRMLS_DC ) 
{
	UErrorCode icuerror = U_ZERO_ERROR;
	ResourceBundle_object *rb = iterator->subject;

	rb->child = ures_getByIndex( rb->me, iterator->i, rb->child, &icuerror );

	if (U_SUCCESS(icuerror)) {
		/* ATTN: key extraction must be the first thing to do... rb->child might be reset in read! */
		if (iterator->is_table) {
			iterator->currentkey = estrdup( ures_getKey( rb->child ) );
		}
		MAKE_STD_ZVAL( iterator->current );
		resourcebundle_extract_value( iterator->current, rb TSRMLS_CC );
	}
	else {
		// zend_throw_exception( spl_ce_OutOfRangeException, "Running past end of ResourceBundle", 0 TSRMLS_CC);
		iterator->current = NULL;
	}
}
/* }}} */

/* {{{ resourcebundle_iterator_invalidate */
static void resourcebundle_iterator_invalidate( zend_object_iterator *iter TSRMLS_DC ) 
{
	ResourceBundle_iterator *iterator = (ResourceBundle_iterator *) iter;

	if (iterator->current) {
		zval_ptr_dtor( &iterator->current );
		iterator->current = NULL;
	}
	if (iterator->currentkey) {
		efree( iterator->currentkey );
		iterator->currentkey = NULL;
	}
}
/* }}} */

/* {{{ resourcebundle_iterator_dtor */
static void resourcebundle_iterator_dtor( zend_object_iterator *iter TSRMLS_DC )
{
	ResourceBundle_iterator *iterator = (ResourceBundle_iterator *) iter;
	zval                    *object = (zval *)iterator->intern.data;

	resourcebundle_iterator_invalidate( iter TSRMLS_CC );

	Z_DELREF_P(object);

	efree(iterator);
}
/* }}} */

/* {{{ resourcebundle_iterator_has_more */
static int resourcebundle_iterator_has_more( zend_object_iterator *iter TSRMLS_DC )
{
	ResourceBundle_iterator *iterator = (ResourceBundle_iterator *) iter;
	return (iterator->i < iterator->length) ? SUCCESS : FAILURE;
}
/* }}} */

/* {{{ resourcebundle_iterator_current */
static void resourcebundle_iterator_current( zend_object_iterator *iter, zval ***data TSRMLS_DC )
{
	ResourceBundle_iterator *iterator = (ResourceBundle_iterator *) iter;
	if (!iterator->current) {
		resourcebundle_iterator_read( iterator );
	}
	*data = &iterator->current;
}
/* }}} */

/* {{{ resourcebundle_iterator_key */
static int resourcebundle_iterator_key( zend_object_iterator *iter, char **str_key, uint *str_key_len, ulong *int_key TSRMLS_DC )
{
	ResourceBundle_iterator *iterator = (ResourceBundle_iterator *) iter;

	if (!iterator->current) {
		resourcebundle_iterator_read( iterator );
	}
	if (iterator->is_table) {
		*str_key = estrdup( iterator->currentkey );
		*str_key_len = strlen( iterator->currentkey ) + 1;
		return HASH_KEY_IS_STRING;
	}
	else {
		*int_key = iterator->i;
		return HASH_KEY_IS_LONG;
	}
}
/* }}} */

/* {{{ resourcebundle_iterator_has_more */
static void resourcebundle_iterator_step( zend_object_iterator *iter TSRMLS_DC )
{
	ResourceBundle_iterator *iterator = (ResourceBundle_iterator *) iter;

	iterator->i++;
	resourcebundle_iterator_invalidate( iter TSRMLS_CC );
}
/* }}} */

/* {{{ resourcebundle_iterator_has_reset */
static void resourcebundle_iterator_reset( zend_object_iterator *iter TSRMLS_DC )
{
	ResourceBundle_iterator *iterator = (ResourceBundle_iterator *) iter;

	iterator->i = 0;
	resourcebundle_iterator_invalidate( iter TSRMLS_CC );
}
/* }}} */

/* {{{ resourcebundle_iterator_funcs */
static zend_object_iterator_funcs resourcebundle_iterator_funcs = {
	resourcebundle_iterator_dtor,
	resourcebundle_iterator_has_more,
	resourcebundle_iterator_current,
	resourcebundle_iterator_key,
	resourcebundle_iterator_step,
	resourcebundle_iterator_reset,
	resourcebundle_iterator_invalidate
};
/* }}} */

/* {{{ resourcebundle_get_iterator */
zend_object_iterator *resourcebundle_get_iterator( zend_class_entry *ce, zval *object, int byref TSRMLS_DC )
{
	ResourceBundle_object   *rb = (ResourceBundle_object *) zend_object_store_get_object( object TSRMLS_CC );
	ResourceBundle_iterator *iterator = emalloc( sizeof( ResourceBundle_iterator ) );

	if (byref) {
	     php_error( E_ERROR, "ResourceBundle does not support writable iterators" );
	}

	Z_ADDREF_P(object);
	iterator->intern.data = (void *) object;
	iterator->intern.funcs = &resourcebundle_iterator_funcs;

	iterator->subject = rb;

	/* The iterated rb can only be either URES_TABLE or URES_ARRAY
	 * All other types are returned as php primitives!
	 */
	iterator->is_table = (ures_getType( rb->me ) == URES_TABLE);
	iterator->length = ures_getSize( rb->me );

	iterator->current = NULL;
	iterator->currentkey = NULL;
	iterator->i = 0;

	return (zend_object_iterator *) iterator;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
