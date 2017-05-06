.macro GLIDIX_SYSCALL num, name
.globl \name
.type \name, @function
\name:
	mov	%rcx,	%r10
	mov	$\num,	%rax
	syscall
	ret
.size \name, .-\name
.endm

GLIDIX_SYSCALL	0,	_exit
GLIDIX_SYSCALL	1,	write
GLIDIX_SYSCALL	2,	_glidix_exec
GLIDIX_SYSCALL	3,	read
GLIDIX_SYSCALL	4,	open
GLIDIX_SYSCALL	5,	close
GLIDIX_SYSCALL	6,	getpid
GLIDIX_SYSCALL	7,	getuid
GLIDIX_SYSCALL	8,	geteuid
GLIDIX_SYSCALL	9,	_glidix_getsuid
GLIDIX_SYSCALL	10,	getgid
GLIDIX_SYSCALL	11,	getegid
GLIDIX_SYSCALL	12,	_glidix_getsgid
GLIDIX_SYSCALL	13,	sigaction
GLIDIX_SYSCALL	14,	sigprocmask
GLIDIX_SYSCALL	15,	_glidix_stat
GLIDIX_SYSCALL	16,	_glidix_getparsz
GLIDIX_SYSCALL	17,	_glidix_getpars
GLIDIX_SYSCALL	18,	raise
GLIDIX_SYSCALL	19,	_glidix_geterrno
GLIDIX_SYSCALL	20,	_glidix_seterrno
GLIDIX_SYSCALL	21,	mprotect
GLIDIX_SYSCALL	22,	lseek
GLIDIX_SYSCALL	23,	fork
GLIDIX_SYSCALL	24,	pause
GLIDIX_SYSCALL	25,	waitpid
GLIDIX_SYSCALL	26,	kill
GLIDIX_SYSCALL	27,	_glidix_insmod
GLIDIX_SYSCALL	28,	_glidix_ioctl
GLIDIX_SYSCALL	29,	_glidix_fdopendir
GLIDIX_SYSCALL	30,	_glidix_diag
GLIDIX_SYSCALL	31,	_glidix_mount
GLIDIX_SYSCALL	32,	_glidix_yield
GLIDIX_SYSCALL	33,	_glidix_time
GLIDIX_SYSCALL	34,	realpath
GLIDIX_SYSCALL	35,	chdir
GLIDIX_SYSCALL	36,	getcwd
GLIDIX_SYSCALL	37,	_glidix_fstat
GLIDIX_SYSCALL	38,	chmod
GLIDIX_SYSCALL	39,	fchmod
GLIDIX_SYSCALL	40,	fsync
GLIDIX_SYSCALL	41,	chown
GLIDIX_SYSCALL	42,	fchown
GLIDIX_SYSCALL	43,	mkdir
GLIDIX_SYSCALL	44,	ftruncate
GLIDIX_SYSCALL	45,	unlink
GLIDIX_SYSCALL	46,	dup
GLIDIX_SYSCALL	47,	dup2
GLIDIX_SYSCALL	48,	pipe
GLIDIX_SYSCALL	49,	_glidix_seterrnoptr
GLIDIX_SYSCALL	50,	_glidix_geterrnoptr
GLIDIX_SYSCALL	51,	clock
GLIDIX_SYSCALL	52,	pread
GLIDIX_SYSCALL	53,	pwrite
GLIDIX_SYSCALL	54,	mmap
GLIDIX_SYSCALL	55,	setuid
GLIDIX_SYSCALL	56,	setgid
GLIDIX_SYSCALL	57,	seteuid
GLIDIX_SYSCALL	58,	setegid
GLIDIX_SYSCALL	59,	setreuid
GLIDIX_SYSCALL	60,	setregid
GLIDIX_SYSCALL	61,	_glidix_rmmod
GLIDIX_SYSCALL	62,	link
GLIDIX_SYSCALL	63,	_glidix_unmount
GLIDIX_SYSCALL	64,	_glidix_lstat
GLIDIX_SYSCALL	65,	symlink
GLIDIX_SYSCALL	66,	readlink
GLIDIX_SYSCALL	67,	_glidix_down
GLIDIX_SYSCALL	68,	sleep
GLIDIX_SYSCALL	69,	_glidix_utime
GLIDIX_SYSCALL	70,	umask
GLIDIX_SYSCALL	71,	_glidix_routetab
GLIDIX_SYSCALL	72,	socket
GLIDIX_SYSCALL	73,	bind
GLIDIX_SYSCALL	74,	sendto
GLIDIX_SYSCALL	75,	send
GLIDIX_SYSCALL	76,	recvfrom
GLIDIX_SYSCALL	77,	recv
GLIDIX_SYSCALL	78,	_glidix_route_add
GLIDIX_SYSCALL	79,	_glidix_netconf_stat
GLIDIX_SYSCALL	80,	_glidix_netconf_getaddrs
GLIDIX_SYSCALL	81,	getsockname
GLIDIX_SYSCALL	82,	shutdown
GLIDIX_SYSCALL	83,	connect
GLIDIX_SYSCALL	84,	getpeername
GLIDIX_SYSCALL	85,	_glidix_setsockopt
GLIDIX_SYSCALL	86,	_glidix_getsockopt
GLIDIX_SYSCALL	87,	_glidix_netconf_statidx
GLIDIX_SYSCALL	88,	_glidix_pcistat
GLIDIX_SYSCALL	89,	getgroups
GLIDIX_SYSCALL	90,	_glidix_setgroups
GLIDIX_SYSCALL	91,	uname
GLIDIX_SYSCALL	92,	_glidix_netconf_addr
GLIDIX_SYSCALL	93,	_glidix_fcntl_getfd
GLIDIX_SYSCALL	94,	_glidix_fcntl_setfd
GLIDIX_SYSCALL	95,	_glidix_unique
GLIDIX_SYSCALL	96,	isatty
GLIDIX_SYSCALL	97,	_glidix_bindif
GLIDIX_SYSCALL	98,	_glidix_route_clear
GLIDIX_SYSCALL	99,	munmap
GLIDIX_SYSCALL	100,	pipe2
GLIDIX_SYSCALL	101,	getppid
GLIDIX_SYSCALL	102,	alarm
GLIDIX_SYSCALL	103,	_glidix_store_and_sleep
GLIDIX_SYSCALL	135,	_glidix_mqrecv
GLIDIX_SYSCALL	135,	_glidix_thsync
GLIDIX_SYSCALL	135,	_glidix_mqserver
GLIDIX_SYSCALL	135,	_glidix_mqclient
GLIDIX_SYSCALL	135,	_glidix_mqsend
GLIDIX_SYSCALL	107,	listen
GLIDIX_SYSCALL	108,	accept
GLIDIX_SYSCALL	109,	accept4
GLIDIX_SYSCALL	110,	setsid
GLIDIX_SYSCALL	111,	setpgid
GLIDIX_SYSCALL	112,	getsid
GLIDIX_SYSCALL	113,	getpgid
GLIDIX_SYSCALL	114,	pthread_exit
GLIDIX_SYSCALL	115,	pthread_create
GLIDIX_SYSCALL	116,	pthread_self
GLIDIX_SYSCALL	117,	pthread_join
GLIDIX_SYSCALL	118,	pthread_detach
GLIDIX_SYSCALL	119,	pthread_kill
GLIDIX_SYSCALL	120,	_glidix_kopt
GLIDIX_SYSCALL	121,	_glidix_sigwait
GLIDIX_SYSCALL	122,	_glidix_sigsuspend
GLIDIX_SYSCALL	123,	lockf
GLIDIX_SYSCALL	124,	_glidix_mcast
GLIDIX_SYSCALL	125,	_glidix_fpoll
GLIDIX_SYSCALL	126,	_glidix_oxperm
GLIDIX_SYSCALL	127,	_glidix_dxperm
GLIDIX_SYSCALL	128,	_glidix_fsinfo
GLIDIX_SYSCALL	129,	_glidix_chxperm
GLIDIX_SYSCALL	130,	_glidix_haveperm
GLIDIX_SYSCALL	131,	sync
GLIDIX_SYSCALL	132,	nice
GLIDIX_SYSCALL	133,	_glidix_procstat
GLIDIX_SYSCALL	134,	_glidix_cpuno
