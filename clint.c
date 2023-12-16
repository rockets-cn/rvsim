﻿// clint.c: CLINT, UART emulation
#include <stdint.h>
#include <windows.h>
#include <conio.h>
#include <stdio.h>

#include "clint.h"


// CLINT I/O registers
static uint32_t timer_l = 0;	// part of CLINT, mtime in SiFive doc
static uint32_t timer_h = 0;
static uint32_t timer_match_l = 0;	// part of CLINT, mtimecmp in SiFive doc
static uint32_t timer_match_h = 0;	


uint32_t io_read(uint32_t addr, uint32_t *data)
{
	switch (addr) {
	case IO_CLINT_TIMERL: *data = timer_l; break;
	case IO_CLINT_TIMERH: *data = timer_h; break;
//	case IO_CLINT_TIMERMATCHL: *data = timer_match_l; break;
//	case IO_CLINT_TIMERMATCHH: *data = timer_match_h; break;
	case IO_UART_DATA: *data = IsKBHit() ? ReadKBByte() : 0; break;
	case IO_UART_READY: *data = 0x60|IsKBHit(); break;
	default: break;
	}
	return 0;
}

uint32_t io_write(uint32_t addr, uint32_t* data)
{
	switch (addr) {
//	case IO_CLINT_TIMERL: timer_l = *data; break;
//	case IO_CLINT_TIMERH: timer_h = *data; break;
	case IO_CLINT_TIMERMATCHL: timer_match_l = *data ; break;
	case IO_CLINT_TIMERMATCHH: timer_match_h = *data; break;
	case IO_UART_DATA: printf("%c", *data); fflush(stdout);break ;
	case IO_DEBUG: debug_syscall();
	default: break;
	}
	return 0;
}



uint64_t get_microseconds()
{
	static LARGE_INTEGER lpf;
	LARGE_INTEGER li;

	if (!lpf.QuadPart)
		QueryPerformanceFrequency(&lpf);

	QueryPerformanceCounter(&li);
	return ((uint64_t)li.QuadPart * 1000000LL) / (uint64_t)lpf.QuadPart;
}


static int IsKBHit()
{
	return _kbhit();
}

static int ReadKBByte()
{
	// This code is kind of tricky, but used to convert windows arrow keys
	// to VT100 arrow keys.
	static int is_escape_sequence = 0;
	int r;
	if (is_escape_sequence == 1)
	{
		is_escape_sequence++;
		return '[';
	}

	r = _getch();

if (is_escape_sequence)
	{
		is_escape_sequence = 0;
		switch (r)
		{
		case 'H': return 'A'; // Up
		case 'P': return 'B'; // Down
		case 'K': return 'D'; // Left
		case 'M': return 'C'; // Right
		case 'G': return 'H'; // Home
		case 'O': return 'F'; // End
		default: return r; // Unknown code.
		}
	}
	else
	{
		switch (r)
		{
		case 13: return 10; //cr->lf
		case 224: is_escape_sequence = 1; return 27; // Escape arrow keys
		default: return r;
		}
	}
}

// CLINT: check to see if we should generate a timer interrupt
// return value: timer interrupt, 0 if no interrupt
// increment timer and also see if we've exceeded threshold
uint32_t run_clint()
{
	uint32_t gen_interrupt = 0;

	static uint64_t last_time = 0;
	uint64_t elapsed_time = 0;

	if (last_time == 0) {
		// initialize last_time
		last_time = get_microseconds();
		return gen_interrupt;
	} else {
		// calculate current time and update timer
		elapsed_time = get_microseconds() - last_time;
		last_time += elapsed_time;
		uint32_t new_timer = timer_l + elapsed_time;
		if (new_timer < timer_l) timer_h++;
		timer_l = new_timer;
	}


	// compare timer and generate interrupt
	// clear WFI & set MIP   or clear MIP
	uint32_t mip = read_CSR(CSR_MIP);
	if ((timer_match_h > 0 || timer_match_l > 0) && ((timer_h > timer_match_h) ||
		(timer_h == timer_match_h && timer_l >= timer_match_l))) {
		wfi = 0;
		write_CSR(CSR_MIP, mip | 0x80);	// set MIP.MTIP	
	} else {
		write_CSR(CSR_MIP, mip & 0xffffff7f);	// looks correct
	}

	uint32_t mstatus = read_CSR(CSR_MSTATUS);
	mip = read_CSR(CSR_MIP);
	uint32_t mie = read_CSR(CSR_MIE);
	// generate interrupt only if it's enabled
	// MIP.MTIP , MIE.MTIE , MSTATUS.MIE
	if ((mip&0x80) && (mie&0x80) && (mstatus & 0x8)) {
		gen_interrupt = 0x80000007;
	}
	return gen_interrupt;
}
