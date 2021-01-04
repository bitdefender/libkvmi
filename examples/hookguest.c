/*
 * Copyright (C) 2017-2020 Bitdefender S.R.L.
 *
 * The program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * The program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * http://www.gnu.org/licenses
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <libkvmi.h>

#define MAX_VCPU 256

#define EPT_TEST_PAGES 20
#define PAGE_SIZE      4096
#define WAIT_30s       ( 30 * 1000 )

#define CR3 3
#define CR4 4

#define MSR_STAR 0xc0000081

static void *Dom;

static const char *access_str[] = {
	"---", "r--", "-w-", "rw-", "--x", "r-x", "-wx", "rwx",
};

static void die( const char *msg )
{
	perror( msg );
	exit( 1 );
}

static void setup_vcpu_reply( struct kvmi_dom_event *ev, struct kvmi_vcpu_hdr *rpl, int action )
{
	struct kvmi_event_reply *common = ( struct kvmi_event_reply * )( rpl + 1 );

	memset( rpl, 0, sizeof( *rpl ) );
	rpl->vcpu = ev->event.common.vcpu;

	memset( common, 0, sizeof( *common ) );
	common->action = action;
	common->event  = ev->event.common.event;
}

static void reply_continue( void *dom, struct kvmi_dom_event *ev, struct kvmi_vcpu_hdr *rpl, size_t rpl_size )
{
	setup_vcpu_reply( ev, rpl, KVMI_EVENT_ACTION_CONTINUE );

	printf( "Reply with CONTINUE (vcpu%u)\n", ev->event.common.vcpu );

	if ( kvmi_reply_event( dom, ev->seq, rpl, rpl_size ) )
		die( "kvmi_reply_event" );
}

static void reply_retry( void *dom, struct kvmi_dom_event *ev, struct kvmi_vcpu_hdr *rpl, size_t rpl_size )
{
	setup_vcpu_reply( ev, rpl, KVMI_EVENT_ACTION_RETRY );

	printf( "Reply with RETRY (vcpu%u)\n", ev->event.common.vcpu );

	if ( kvmi_reply_event( dom, ev->seq, rpl, rpl_size ) )
		die( "kvmi_reply_event" );
}

static void handle_cr_event( void *dom, struct kvmi_dom_event *ev )
{
	struct kvmi_event_cr *cr = &ev->event.cr;
	struct {
		struct kvmi_vcpu_hdr       hdr;
		struct kvmi_event_reply    common;
		struct kvmi_event_cr_reply cr;
	} rpl;

	memset( &rpl, 0, sizeof( rpl ) );

	printf( "CR%d 0x%llx -> 0x%llx (vcpu%u)\n", cr->cr, cr->old_value, cr->new_value, ev->event.common.vcpu );

	rpl.cr.new_val = cr->new_value;
	reply_continue( dom, ev, &rpl.hdr, sizeof( rpl ) );
}

static void handle_msr_event( void *dom, struct kvmi_dom_event *ev )
{
	struct kvmi_event_msr *msr = &ev->event.msr;
	struct {
		struct kvmi_vcpu_hdr        hdr;
		struct kvmi_event_reply     common;
		struct kvmi_event_msr_reply msr;
	} rpl;

	memset( &rpl, 0, sizeof( rpl ) );

	printf( "MSR 0x%x 0x%llx -> 0x%llx (vcpu%u)\n", msr->msr, msr->old_value, msr->new_value,
	        ev->event.common.vcpu );

	rpl.msr.new_val = msr->new_value;
	reply_continue( dom, ev, &rpl.hdr, sizeof( rpl ) );
}

static void enable_vcpu_events( void *dom, unsigned int vcpu )
{
	bool enable = true;

	printf( "Enabling CR, MSR and PF events (vcpu%u)\n", vcpu );

	if ( kvmi_control_events( dom, vcpu, KVMI_EVENT_CR, enable ) ||
	     kvmi_control_events( dom, vcpu, KVMI_EVENT_MSR, enable ) ||
	     kvmi_control_events( dom, vcpu, KVMI_EVENT_PF, enable ) )
		die( "kvmi_control_events" );

	if ( vcpu == 0 ) {
		printf( "Enabling CR3 events...\n" );

		if ( kvmi_control_cr( dom, vcpu, CR3, enable ) )
			die( "kvmi_control_cr(3)" );
	}

	printf( "Enabling CR4 events...\n" );

	if ( kvmi_control_cr( dom, vcpu, CR4, enable ) )
		die( "kvmi_control_cr(4)" );

	printf( "Enabling MSR_STAR events...\n" );

	if ( kvmi_control_msr( dom, vcpu, MSR_STAR, enable ) )
		die( "kvmi_control_msr(STAR)" );
}

static void handle_pause_vcpu_event( void *dom, struct kvmi_dom_event *ev )
{
	struct {
		struct kvmi_vcpu_hdr    hdr;
		struct kvmi_event_reply common;
	} rpl;
	unsigned int vcpu = ev->event.common.vcpu;
	static bool  events_enabled[MAX_VCPU];

	printf( "PAUSE (vcpu%u)\n", vcpu );

	if ( vcpu < MAX_VCPU && !events_enabled[vcpu] ) {
		enable_vcpu_events( dom, vcpu );
		events_enabled[vcpu] = true;
	}

	memset( &rpl, 0, sizeof( rpl ) );

	reply_continue( dom, ev, &rpl.hdr, sizeof( rpl ) );
}

static void set_page_access( void *dom, __u64 gpa, __u8 access )
{
	printf( "Set page access gpa 0x%llx access %s [0x%x]\n", gpa, access_str[access & 7], access );

	if ( kvmi_set_page_access( dom, &gpa, &access, 1, 0 ) )
		die( "kvmi_set_page_access" );
}

static void write_protect_page( void *dom, __u64 gpa )
{
	set_page_access( dom, gpa, KVMI_PAGE_ACCESS_R | KVMI_PAGE_ACCESS_X );
}

static void maybe_start_pf_test( void *dom, struct kvmi_dom_event *ev )
{
	static bool started;
	__u64       cr3  = ev->event.common.arch.sregs.cr3;
	__u16       vcpu = ev->event.common.vcpu;
	__u64       pt   = cr3 & ~0xfff;
	__u64       end;

	if ( started || !pt )
		return;

	printf( "Starting #PF test with CR3 0x%llx (vcpu%u)\n", cr3, vcpu );

	for ( end = pt + EPT_TEST_PAGES * PAGE_SIZE; pt < end; pt += PAGE_SIZE )
		write_protect_page( dom, pt );

	started = true;

	if ( ev->event.common.event == KVMI_EVENT_CR ) {
		bool enable = false;

		printf( "Disabling CR3 events (vcpu=%d)...\n", vcpu );

		if ( kvmi_control_cr( dom, vcpu, CR3, enable ) )
			die( "kvmi_control_cr(3)" );
	}
}

static void handle_pf_event( void *dom, struct kvmi_dom_event *ev )
{
	struct kvmi_event_pf *pf   = &ev->event.page_fault;
	__u16                 vcpu = ev->event.common.vcpu;
	struct {
		struct kvmi_vcpu_hdr       hdr;
		struct kvmi_event_reply    common;
		struct kvmi_event_pf_reply pf;
	} rpl;
	__u8 access = KVMI_PAGE_ACCESS_R | KVMI_PAGE_ACCESS_W | KVMI_PAGE_ACCESS_X;

	memset( &rpl, 0, sizeof( rpl ) );

	printf( "PF gva 0x%llx gpa 0x%llx access %s [0x%x] (vcpu%u)\n", pf->gva, pf->gpa, access_str[pf->access & 7],
	        pf->access, vcpu );

	set_page_access( dom, pf->gpa, access );

	reply_retry( dom, ev, &rpl.hdr, sizeof( rpl ) );
}

static void handle_event( void *dom, struct kvmi_dom_event *ev )
{
	unsigned int id = ev->event.common.event;

	switch ( id ) {
		case KVMI_EVENT_CR:
			maybe_start_pf_test( dom, ev );
			handle_cr_event( dom, ev );
			break;
		case KVMI_EVENT_MSR:
			handle_msr_event( dom, ev );
			break;
		case KVMI_EVENT_PAUSE_VCPU:
			maybe_start_pf_test( dom, ev );
			handle_pause_vcpu_event( dom, ev );
			break;
		case KVMI_EVENT_PF:
			handle_pf_event( dom, ev );
			break;
		default:
			fprintf( stderr, "Unknown event %d\n", id );
			exit( 1 );
	}
}

static void pause_vm( void *dom )
{
	unsigned int count = 0;

	if ( kvmi_get_vcpu_count( dom, &count ) )
		die( "kvmi_get_vcpu_count" );

	printf( "Sending the pause command...\n" );

	if ( kvmi_pause_all_vcpus( dom, count ) )
		die( "kvmi_pause_all_vcpus" );

	printf( "We should receive %u pause events\n", count );
}

static int new_guest( void *dom, unsigned char ( *uuid )[16], void *ctx )
{
	unsigned long long max_gfn;
	int                k;

	printf( "New guest: " );

	for ( k = 0; k < 16; k++ )
		printf( "%.2x ", ( *uuid )[k] );

	printf( "fd %d ctx %p\n", kvmi_connection_fd( dom ), ctx );

	pause_vm( dom );

	if ( kvmi_get_maximum_gfn( dom, &max_gfn ) )
		die( "kvmi_get_maximum_gfn" );

	printf( "Max gfn: 0x%llx\n", max_gfn );

	Dom = dom;

	return 0;
}

static int new_handshake( const struct kvmi_qemu2introspector *qemu, struct kvmi_introspector2qemu *intro, void *ctx )
{
	( void )intro;
	( void )ctx;
	printf( "New handshake: name '%s' start_time %ld\n", qemu->name, qemu->start_time );
	return 0;
}

static void log_cb( kvmi_log_level level, const char *s, void *ctx )
{
	( void )ctx;
	printf( "[level=%d]: %s\n", level, s );
}

static int spp_bypass = 0;

static void spp_bitmap_test( void *dom )
{
	int   ret = 0;
	__u64 cmd = 777;
	__u64 gfn;
	__u64 gpa;
	__u32 bitmap = 0;

	char buff[64] = { 0 };

	printf( "please input gfn :\n\
		777 for bypass spp test,\n\
		888 to skip this round.\n" );

	if ( fgets( buff, 63, stdin ) )
		cmd = atoll( buff );

	if ( cmd == 777 ) {
		spp_bypass = 1;
		return;
	}

	if ( cmd == 888 )
		return;

	printf( "input gfn: 0x%llx(%lld)\n", cmd, cmd );

	gfn = cmd;

	memset( buff, 0, sizeof( buff ) );

	printf( "please input spp bitmap:\n" );

	if ( fgets( buff, 63, stdin ) )
		bitmap = atoll( buff );
	printf( "input spp bitmap: 0x%x(%d)\n", bitmap, bitmap );

	/* to cheat kvmi function.*/
	gpa = gfn << 12;

	ret = kvmi_set_page_write_bitmap( dom, &gpa, &bitmap, 1 );

	if ( ret < 0 )
		printf( "failed to set spp bitmap.\n" );
	else
		printf( "set spp bit map successfully.\n" );
}

int main( int argc, char **argv )
{
	void *ctx;

	if ( argc != 2 ) {
		printf( "Usage:\n"
		        "	%s PathToSocket\n"
		        "	%s VSockPortNumber\n",
		        argv[0], argv[0] );
		return 1;
	}

	kvmi_set_log_cb( log_cb, NULL );

	if ( atoi( argv[1] ) > 0 ) {
		ctx = kvmi_init_vsock( atoi( argv[1] ), new_guest, new_handshake, NULL );
	} else {
		ctx = kvmi_init_unix_socket( argv[1], new_guest, new_handshake, NULL );
	}

	if ( !ctx ) {
		perror( "kvmi_init" );
		exit( 1 );
	}

	printf( "Waiting...\n" );

	while ( !Dom )
		sleep( 1 );

	while ( 1 ) {
		struct kvmi_dom_event *ev;

		printf( "Waiting...\n" );

		if ( kvmi_wait_event( Dom, WAIT_30s ) ) {
			if ( errno == ETIMEDOUT ) {
				printf( "No event.\n" );

				if ( !spp_bypass )
					spp_bitmap_test( Dom );

				continue;
			}
			die( "kvmi_wait_event" );
		}

		printf( "Pop event\n" );

		if ( kvmi_pop_event( Dom, &ev ) )
			die( "kvmi_pop_event" );

		handle_event( Dom, ev );

		free( ev );
	}

	kvmi_uninit( ctx );

	return 0;
}
