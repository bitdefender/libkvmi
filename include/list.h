/* SPDX-License-Identifier: LGPL-3.0 OR BSD-3-Clause */
/*
 * Copyright (C) 2020 Bitdefender S.R.L.
 */
#ifndef __LIST_H__
#define __LIST_H__

#include <stdlib.h>
#include <stddef.h>

typedef struct __list {
	struct __list *prev;
	struct __list *next;
} list_t;

#define LIST_HEAD_INIT( name ) { &( name ), &( name ) }

#define INIT_LIST_HEAD( ptr )            \
	do {                             \
		( ptr )->next = ( ptr ); \
		( ptr )->prev = ( ptr ); \
	} while ( 0 )

#define list_container( _addr, _type, _field ) ( ( _type * )( ( char * )( _addr ) - offsetof( _type, _field ) ) )

#define list_for_each_safe( pos, n, head ) \
        for ( pos = ( head )->next, n = pos->next; pos != ( head ); pos = n, n = pos->next )

#define list_for_each( pos, head ) \
        for ( pos = ( head )->next; pos != ( head ); pos = pos->next )

#define list_for_each_safe_from( pos, n, head, start ) \
        for ( pos = ( start )->next, n = pos->next; pos != ( head ); pos = n, n = pos->next )

#define list_for_each_safe_reverse( pos, p, head ) \
        for ( pos = ( head )->prev, p = pos->prev; pos != ( head ); pos = p, p = pos->prev )

#define list_init( _list ) \
	( _list )->prev = ( _list )->next = ( _list )

#define list_is_empty( _list ) \
	( ( _list )->next == ( _list ) && ( _list )->prev == ( _list ) )

#define list_add_head( _list, _new ) ( {          \
		( _new )->prev = ( _list );       \
		( _new )->next = ( _list )->next; \
		( _list )->next->prev = ( _new ); \
		( _list )->next = ( _new );       \
	})

#define list_add_tail( _list, _new ) ( {          \
		( _new )->prev = ( _list )->prev; \
		( _new )->next = ( _list );       \
		( _list )->prev->next = ( _new ); \
		( _list )->prev = ( _new );       \
	})

#define list_add_before( _old, _new ) list_add_tail( _old, _new )

#define list_move_to_head( _list1,_list2 ) ({                      \
		if ( !list_is_empty( _list2 ) ) {                  \
			( _list2 )->next->prev = ( _list1 );       \
			( _list2 )->prev->next = ( _list1 )->next; \
			( _list1 )->next->prev = ( _list2 )->prev; \
			( _list1 )->next = ( _list2 )->next;       \
			list_init( _list2 );                       \
		}                                                  \
	})

#define list_move_to_tail( _list1, _list2 ) ({                     \
		if ( !list_is_empty( _list2 ) ) {                  \
			( _list2 )->prev->next = ( _list1 );       \
			( _list2 )->next->prev = ( _list1 )->prev; \
			( _list1 )->prev->next = ( _list2 )->next; \
			( _list1 )->prev = ( _list2 )->prev;       \
			list_init( _list2 );                       \
		}                                                  \
	})

#define list_remove_head( _list ) ({                             \
		list_t *_head  = NULL;                           \
		if ( !list_is_empty( _list ) ) {                 \
			_head  = ( _list )->next;                \
			( _list )->next = ( _list )->next->next; \
			( _list )->next->prev = ( _list );       \
			_head->prev = _head;                     \
			_head->next = _head;                     \
		}                                                \
		_head;                                           \
	})

#define list_remove_tail( _list ) ({                             \
		list_t *_tail = NULL;                            \
		if ( !list_is_empty( _list ) ) {                 \
			_tail = ( _list )->prev;                 \
			( _list )->prev = ( _list )->prev->prev; \
			( _list )->prev->next = ( _list );       \
			_tail->prev = _tail;                     \
			_tail->next = _tail;                     \
		}                                                \
		_tail;                                           \
	})

#define list_remove_item( _list ) ({                           \
		( _list )->prev->next = ( _list )->next;       \
		( _list )->next->prev = ( _list )->prev;       \
		( _list )->next = ( _list )->prev = ( _list ); \
		( _list );                                     \
	})

#define list_del( _list ) ({                           \
		( _list )->prev->next = ( _list )->next;       \
		( _list )->next->prev = ( _list )->prev;       \
	})

#define list_del_entry( _item ) list_remove_item( _item )

static inline void list_ins_head( list_t *head, list_t *phead )
{
	head->next = phead;
	head->prev = phead->prev;
	phead->prev->next = head;
	phead->prev = head;
}

#endif /* __LIST_H__ */
