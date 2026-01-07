
userspace/lib9p_syscall/init:     file format elf64-x86-64


Disassembly of section .text:

0000000000200000 <_start>:
  200000:	48 c7 c4 00 80 20 00 	mov    $0x208000,%rsp
  200007:	e8 14 02 00 00       	call   200220 <main>
  20000c:	f4                   	hlt
  20000d:	eb fd                	jmp    20000c <_start+0xc>
  20000f:	90                   	nop

0000000000200010 <init_print>:
  200010:	80 3f 00             	cmpb   $0x0,(%rdi)
  200013:	48 89 fe             	mov    %rdi,%rsi
  200016:	74 23                	je     20003b <init_print+0x2b>
  200018:	b8 01 00 00 00       	mov    $0x1,%eax
  20001d:	0f 1f 00             	nopl   (%rax)
  200020:	48 89 c2             	mov    %rax,%rdx
  200023:	48 83 c0 01          	add    $0x1,%rax
  200027:	80 7c 06 ff 00       	cmpb   $0x0,-0x1(%rsi,%rax,1)
  20002c:	75 f2                	jne    200020 <init_print+0x10>
  20002e:	48 63 d2             	movslq %edx,%rdx
  200031:	bf 01 00 00 00       	mov    $0x1,%edi
  200036:	e9 d5 05 00 00       	jmp    200610 <sys_write>
  20003b:	31 d2                	xor    %edx,%edx
  20003d:	eb ef                	jmp    20002e <init_print+0x1e>
  20003f:	90                   	nop

0000000000200040 <register_service>:
  200040:	41 54                	push   %r12
  200042:	55                   	push   %rbp
  200043:	53                   	push   %rbx
  200044:	48 81 ec 80 01 00 00 	sub    $0x180,%rsp
  20004b:	0f b6 17             	movzbl (%rdi),%edx
  20004e:	c7 04 24 2f 73 72 76 	movl   $0x7672732f,(%rsp)
  200055:	c6 44 24 04 2f       	movb   $0x2f,0x4(%rsp)
  20005a:	84 d2                	test   %dl,%dl
  20005c:	0f 84 55 01 00 00    	je     2001b7 <register_service+0x177>
  200062:	b8 06 00 00 00       	mov    $0x6,%eax
  200067:	48 8d 4c 24 ff       	lea    -0x1(%rsp),%rcx
  20006c:	eb 10                	jmp    20007e <register_service+0x3e>
  20006e:	66 90                	xchg   %ax,%ax
  200070:	48 83 c0 01          	add    $0x1,%rax
  200074:	48 83 f8 79          	cmp    $0x79,%rax
  200078:	0f 84 25 01 00 00    	je     2001a3 <register_service+0x163>
  20007e:	88 14 01             	mov    %dl,(%rcx,%rax,1)
  200081:	0f b6 54 07 fb       	movzbl -0x5(%rdi,%rax,1),%edx
  200086:	84 d2                	test   %dl,%dl
  200088:	75 e6                	jne    200070 <register_service+0x30>
  20008a:	0f b6 16             	movzbl (%rsi),%edx
  20008d:	48 98                	cltq
  20008f:	c7 84 24 80 00 00 00 	movl   $0x63657865,0x80(%rsp)
  200096:	65 78 65 63 
  20009a:	c6 04 04 00          	movb   $0x0,(%rsp,%rax,1)
  20009e:	c6 84 24 84 00 00 00 	movb   $0x3d,0x84(%rsp)
  2000a5:	3d 
  2000a6:	84 d2                	test   %dl,%dl
  2000a8:	0f 84 13 01 00 00    	je     2001c1 <register_service+0x181>
  2000ae:	b8 06 00 00 00       	mov    $0x6,%eax
  2000b3:	48 8d 4c 24 7f       	lea    0x7f(%rsp),%rcx
  2000b8:	eb 16                	jmp    2000d0 <register_service+0x90>
  2000ba:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  2000c0:	48 83 c0 01          	add    $0x1,%rax
  2000c4:	48 3d fb 00 00 00    	cmp    $0xfb,%rax
  2000ca:	0f 84 dd 00 00 00    	je     2001ad <register_service+0x16d>
  2000d0:	88 14 01             	mov    %dl,(%rcx,%rax,1)
  2000d3:	0f b6 54 06 fb       	movzbl -0x5(%rsi,%rax,1),%edx
  2000d8:	84 d2                	test   %dl,%dl
  2000da:	75 e4                	jne    2000c0 <register_service+0x80>
  2000dc:	49 89 e4             	mov    %rsp,%r12
  2000df:	48 98                	cltq
  2000e1:	ba b6 01 00 00       	mov    $0x1b6,%edx
  2000e6:	be 01 00 00 00       	mov    $0x1,%esi
  2000eb:	4c 89 e7             	mov    %r12,%rdi
  2000ee:	c6 84 04 80 00 00 00 	movb   $0x0,0x80(%rsp,%rax,1)
  2000f5:	00 
  2000f6:	e8 05 09 00 00       	call   200a00 <sys_create>
  2000fb:	89 c3                	mov    %eax,%ebx
  2000fd:	85 c0                	test   %eax,%eax
  2000ff:	0f 88 7c 00 00 00    	js     200181 <register_service+0x141>
  200105:	48 8d 8c 24 81 00 00 	lea    0x81(%rsp),%rcx
  20010c:	00 
  20010d:	31 ed                	xor    %ebp,%ebp
  20010f:	80 bc 24 80 00 00 00 	cmpb   $0x0,0x80(%rsp)
  200116:	00 
  200117:	48 89 c8             	mov    %rcx,%rax
  20011a:	74 16                	je     200132 <register_service+0xf2>
  20011c:	0f 1f 40 00          	nopl   0x0(%rax)
  200120:	48 89 c2             	mov    %rax,%rdx
  200123:	48 83 c0 01          	add    $0x1,%rax
  200127:	80 78 ff 00          	cmpb   $0x0,-0x1(%rax)
  20012b:	75 f3                	jne    200120 <register_service+0xe0>
  20012d:	29 ca                	sub    %ecx,%edx
  20012f:	8d 6a 01             	lea    0x1(%rdx),%ebp
  200132:	48 63 ed             	movslq %ebp,%rbp
  200135:	48 8d b4 24 80 00 00 	lea    0x80(%rsp),%rsi
  20013c:	00 
  20013d:	89 df                	mov    %ebx,%edi
  20013f:	48 89 ea             	mov    %rbp,%rdx
  200142:	e8 c9 04 00 00       	call   200610 <sys_write>
  200147:	48 39 c5             	cmp    %rax,%rbp
  20014a:	75 13                	jne    20015f <register_service+0x11f>
  20014c:	89 df                	mov    %ebx,%edi
  20014e:	e8 ad 03 00 00       	call   200500 <sys_close>
  200153:	48 81 c4 80 01 00 00 	add    $0x180,%rsp
  20015a:	5b                   	pop    %rbx
  20015b:	5d                   	pop    %rbp
  20015c:	41 5c                	pop    %r12
  20015e:	c3                   	ret
  20015f:	48 8d 3d aa 34 00 00 	lea    0x34aa(%rip),%rdi        # 203610 <_syscall+0xbb>
  200166:	e8 a5 fe ff ff       	call   200010 <init_print>
  20016b:	4c 89 e7             	mov    %r12,%rdi
  20016e:	e8 9d fe ff ff       	call   200010 <init_print>
  200173:	48 8d 3d 48 34 00 00 	lea    0x3448(%rip),%rdi        # 2035c2 <_syscall+0x6d>
  20017a:	e8 91 fe ff ff       	call   200010 <init_print>
  20017f:	eb cb                	jmp    20014c <register_service+0x10c>
  200181:	48 8d 3d d0 33 00 00 	lea    0x33d0(%rip),%rdi        # 203558 <_syscall+0x3>
  200188:	e8 83 fe ff ff       	call   200010 <init_print>
  20018d:	4c 89 e7             	mov    %r12,%rdi
  200190:	e8 7b fe ff ff       	call   200010 <init_print>
  200195:	48 8d 3d 26 34 00 00 	lea    0x3426(%rip),%rdi        # 2035c2 <_syscall+0x6d>
  20019c:	e8 6f fe ff ff       	call   200010 <init_print>
  2001a1:	eb b0                	jmp    200153 <register_service+0x113>
  2001a3:	b8 78 00 00 00       	mov    $0x78,%eax
  2001a8:	e9 dd fe ff ff       	jmp    20008a <register_service+0x4a>
  2001ad:	b8 fa 00 00 00       	mov    $0xfa,%eax
  2001b2:	e9 25 ff ff ff       	jmp    2000dc <register_service+0x9c>
  2001b7:	b8 05 00 00 00       	mov    $0x5,%eax
  2001bc:	e9 c9 fe ff ff       	jmp    20008a <register_service+0x4a>
  2001c1:	b8 05 00 00 00       	mov    $0x5,%eax
  2001c6:	e9 11 ff ff ff       	jmp    2000dc <register_service+0x9c>
  2001cb:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)

00000000002001d0 <print>:
  2001d0:	53                   	push   %rbx
  2001d1:	80 3f 00             	cmpb   $0x0,(%rdi)
  2001d4:	48 89 fe             	mov    %rdi,%rsi
  2001d7:	74 2f                	je     200208 <print+0x38>
  2001d9:	b8 01 00 00 00       	mov    $0x1,%eax
  2001de:	66 90                	xchg   %ax,%ax
  2001e0:	48 89 c2             	mov    %rax,%rdx
  2001e3:	48 83 c0 01          	add    $0x1,%rax
  2001e7:	80 7c 06 ff 00       	cmpb   $0x0,-0x1(%rsi,%rax,1)
  2001ec:	75 f2                	jne    2001e0 <print+0x10>
  2001ee:	89 d3                	mov    %edx,%ebx
  2001f0:	bf 02 00 00 00       	mov    $0x2,%edi
  2001f5:	48 63 d2             	movslq %edx,%rdx
  2001f8:	e8 13 04 00 00       	call   200610 <sys_write>
  2001fd:	89 d8                	mov    %ebx,%eax
  2001ff:	5b                   	pop    %rbx
  200200:	c3                   	ret
  200201:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  200208:	31 db                	xor    %ebx,%ebx
  20020a:	31 d2                	xor    %edx,%edx
  20020c:	bf 02 00 00 00       	mov    $0x2,%edi
  200211:	e8 fa 03 00 00       	call   200610 <sys_write>
  200216:	89 d8                	mov    %ebx,%eax
  200218:	5b                   	pop    %rbx
  200219:	c3                   	ret
  20021a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)

0000000000200220 <main>:
  200220:	48 83 ec 08          	sub    $0x8,%rsp
  200224:	48 8d 3d 45 33 00 00 	lea    0x3345(%rip),%rdi        # 203570 <_syscall+0x1b>
  20022b:	e8 e0 fd ff ff       	call   200010 <init_print>
  200230:	48 8d 3d 01 34 00 00 	lea    0x3401(%rip),%rdi        # 203638 <_syscall+0xe3>
  200237:	e8 d4 fd ff ff       	call   200010 <init_print>
  20023c:	bf 10 00 00 00       	mov    $0x10,%edi
  200241:	e8 1a 09 00 00       	call   200b60 <sys_rfork>
  200246:	85 c0                	test   %eax,%eax
  200248:	0f 84 89 00 00 00    	je     2002d7 <main+0xb7>
  20024e:	0f 88 9d 00 00 00    	js     2002f1 <main+0xd1>
  200254:	48 8d 3d 25 34 00 00 	lea    0x3425(%rip),%rdi        # 203680 <_syscall+0x12b>
  20025b:	e8 b0 fd ff ff       	call   200010 <init_print>
  200260:	48 8d 3d 5a 33 00 00 	lea    0x335a(%rip),%rdi        # 2035c1 <_syscall+0x6c>
  200267:	e8 a4 fd ff ff       	call   200010 <init_print>
  20026c:	48 8d 3d 35 34 00 00 	lea    0x3435(%rip),%rdi        # 2036a8 <_syscall+0x153>
  200273:	e8 98 fd ff ff       	call   200010 <init_print>
  200278:	48 8d 35 45 33 00 00 	lea    0x3345(%rip),%rsi        # 2035c4 <_syscall+0x6f>
  20027f:	48 8d 3d 44 33 00 00 	lea    0x3344(%rip),%rdi        # 2035ca <_syscall+0x75>
  200286:	e8 b5 fd ff ff       	call   200040 <register_service>
  20028b:	48 8d 35 44 33 00 00 	lea    0x3344(%rip),%rsi        # 2035d6 <_syscall+0x81>
  200292:	48 8d 3d 43 33 00 00 	lea    0x3343(%rip),%rdi        # 2035dc <_syscall+0x87>
  200299:	e8 a2 fd ff ff       	call   200040 <register_service>
  20029e:	48 8d 35 40 33 00 00 	lea    0x3340(%rip),%rsi        # 2035e5 <_syscall+0x90>
  2002a5:	48 8d 3d 3f 33 00 00 	lea    0x333f(%rip),%rdi        # 2035eb <_syscall+0x96>
  2002ac:	e8 8f fd ff ff       	call   200040 <register_service>
  2002b1:	48 8d 3d 10 34 00 00 	lea    0x3410(%rip),%rdi        # 2036c8 <_syscall+0x173>
  2002b8:	e8 53 fd ff ff       	call   200010 <init_print>
  2002bd:	48 8d 3d 31 33 00 00 	lea    0x3331(%rip),%rdi        # 2035f5 <_syscall+0xa0>
  2002c4:	e8 47 fd ff ff       	call   200010 <init_print>
  2002c9:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  2002d0:	e8 2b 0b 00 00       	call   200e00 <sys_wait>
  2002d5:	eb f9                	jmp    2002d0 <main+0xb0>
  2002d7:	48 8d 3d ae 32 00 00 	lea    0x32ae(%rip),%rdi        # 20358c <_syscall+0x37>
  2002de:	e8 bd 08 00 00       	call   200ba0 <sys_exec>
  2002e3:	48 8d 3d 76 33 00 00 	lea    0x3376(%rip),%rdi        # 203660 <_syscall+0x10b>
  2002ea:	e8 21 fd ff ff       	call   200010 <init_print>
  2002ef:	eb fe                	jmp    2002ef <main+0xcf>
  2002f1:	48 8d 3d aa 32 00 00 	lea    0x32aa(%rip),%rdi        # 2035a2 <_syscall+0x4d>
  2002f8:	e8 13 fd ff ff       	call   200010 <init_print>
  2002fd:	48 8d 3d b1 32 00 00 	lea    0x32b1(%rip),%rdi        # 2035b5 <_syscall+0x60>
  200304:	e8 17 06 00 00       	call   200920 <sys_exit>
  200309:	e9 46 ff ff ff       	jmp    200254 <main+0x34>
  20030e:	66 90                	xchg   %ax,%ax

0000000000200310 <lux_call>:
  200310:	f3 0f 1e fa          	endbr64
  200314:	55                   	push   %rbp
  200315:	ba 00 0f 00 00       	mov    $0xf00,%edx
  20031a:	48 bd 00 f0 ef fe ff 	movabs $0x7ffffeeff000,%rbp
  200321:	7f 00 00 
  200324:	53                   	push   %rbx
  200325:	48 89 f3             	mov    %rsi,%rbx
  200328:	48 89 ee             	mov    %rbp,%rsi
  20032b:	48 83 ec 08          	sub    $0x8,%rsp
  20032f:	e8 2c 25 00 00       	call   202860 <convS2M>
  200334:	85 c0                	test   %eax,%eax
  200336:	7e 40                	jle    200378 <lux_call+0x68>
  200338:	e8 18 32 00 00       	call   203555 <_syscall>
  20033d:	31 f6                	xor    %esi,%esi
  20033f:	ba 98 01 00 00       	mov    $0x198,%edx
  200344:	48 89 df             	mov    %rbx,%rdi
  200347:	e8 d4 13 00 00       	call   201720 <memset>
  20034c:	48 89 da             	mov    %rbx,%rdx
  20034f:	be 00 0f 00 00       	mov    $0xf00,%esi
  200354:	48 89 ef             	mov    %rbp,%rdi
  200357:	e8 c4 15 00 00       	call   201920 <convM2S>
  20035c:	85 c0                	test   %eax,%eax
  20035e:	74 18                	je     200378 <lux_call+0x68>
  200360:	31 c0                	xor    %eax,%eax
  200362:	80 3b 6b             	cmpb   $0x6b,(%rbx)
  200365:	0f 94 c0             	sete   %al
  200368:	f7 d8                	neg    %eax
  20036a:	48 83 c4 08          	add    $0x8,%rsp
  20036e:	5b                   	pop    %rbx
  20036f:	5d                   	pop    %rbp
  200370:	c3                   	ret
  200371:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  200378:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  20037d:	eb eb                	jmp    20036a <lux_call+0x5a>
  20037f:	90                   	nop

0000000000200380 <do_syscall>:
  200380:	41 56                	push   %r14
  200382:	41 55                	push   %r13
  200384:	41 89 fd             	mov    %edi,%r13d
  200387:	41 54                	push   %r12
  200389:	49 89 f4             	mov    %rsi,%r12
  20038c:	31 f6                	xor    %esi,%esi
  20038e:	55                   	push   %rbp
  20038f:	89 d5                	mov    %edx,%ebp
  200391:	ba 98 01 00 00       	mov    $0x198,%edx
  200396:	53                   	push   %rbx
  200397:	48 89 cb             	mov    %rcx,%rbx
  20039a:	48 81 ec 40 03 00 00 	sub    $0x340,%rsp
  2003a1:	49 89 e6             	mov    %rsp,%r14
  2003a4:	4c 89 f7             	mov    %r14,%rdi
  2003a7:	e8 74 13 00 00       	call   201720 <memset>
  2003ac:	b8 01 00 00 00       	mov    $0x1,%eax
  2003b1:	4c 89 f7             	mov    %r14,%rdi
  2003b4:	c6 04 24 82          	movb   $0x82,(%rsp)
  2003b8:	48 8d b4 24 a0 01 00 	lea    0x1a0(%rsp),%rsi
  2003bf:	00 
  2003c0:	66 89 44 24 08       	mov    %ax,0x8(%rsp)
  2003c5:	44 89 6c 24 10       	mov    %r13d,0x10(%rsp)
  2003ca:	c7 44 24 14 00 00 00 	movl   $0x0,0x14(%rsp)
  2003d1:	00 
  2003d2:	4c 89 64 24 18       	mov    %r12,0x18(%rsp)
  2003d7:	89 6c 24 20          	mov    %ebp,0x20(%rsp)
  2003db:	e8 30 ff ff ff       	call   200310 <lux_call>
  2003e0:	85 c0                	test   %eax,%eax
  2003e2:	78 22                	js     200406 <do_syscall+0x86>
  2003e4:	48 85 db             	test   %rbx,%rbx
  2003e7:	74 0b                	je     2003f4 <do_syscall+0x74>
  2003e9:	48 8b 84 24 c8 01 00 	mov    0x1c8(%rsp),%rax
  2003f0:	00 
  2003f1:	48 89 03             	mov    %rax,(%rbx)
  2003f4:	31 c0                	xor    %eax,%eax
  2003f6:	48 81 c4 40 03 00 00 	add    $0x340,%rsp
  2003fd:	5b                   	pop    %rbx
  2003fe:	5d                   	pop    %rbp
  2003ff:	41 5c                	pop    %r12
  200401:	41 5d                	pop    %r13
  200403:	41 5e                	pop    %r14
  200405:	c3                   	ret
  200406:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  20040b:	eb e9                	jmp    2003f6 <do_syscall+0x76>
  20040d:	0f 1f 00             	nopl   (%rax)

0000000000200410 <sys_open>:
  200410:	f3 0f 1e fa          	endbr64
  200414:	41 54                	push   %r12
  200416:	41 89 f4             	mov    %esi,%r12d
  200419:	55                   	push   %rbp
  20041a:	53                   	push   %rbx
  20041b:	48 81 ec 40 07 00 00 	sub    $0x740,%rsp
  200422:	80 3f 00             	cmpb   $0x0,(%rdi)
  200425:	0f 84 b5 00 00 00    	je     2004e0 <sys_open+0xd0>
  20042b:	b8 01 00 00 00       	mov    $0x1,%eax
  200430:	48 89 c2             	mov    %rax,%rdx
  200433:	48 8d 40 01          	lea    0x1(%rax),%rax
  200437:	80 3c 17 00          	cmpb   $0x0,(%rdi,%rdx,1)
  20043b:	75 f3                	jne    200430 <sys_open+0x20>
  20043d:	8d 5a 02             	lea    0x2(%rdx),%ebx
  200440:	89 d1                	mov    %edx,%ecx
  200442:	0f b6 c6             	movzbl %dh,%eax
  200445:	48 63 db             	movslq %ebx,%rbx
  200448:	48 8d ac 24 40 03 00 	lea    0x340(%rsp),%rbp
  20044f:	00 
  200450:	88 84 24 41 03 00 00 	mov    %al,0x341(%rsp)
  200457:	48 8d 84 24 42 03 00 	lea    0x342(%rsp),%rax
  20045e:	00 
  20045f:	48 89 fe             	mov    %rdi,%rsi
  200462:	48 01 eb             	add    %rbp,%rbx
  200465:	48 89 c7             	mov    %rax,%rdi
  200468:	88 8c 24 40 03 00 00 	mov    %cl,0x340(%rsp)
  20046f:	e8 5c 12 00 00       	call   2016d0 <memmove>
  200474:	44 88 23             	mov    %r12b,(%rbx)
  200477:	49 89 e4             	mov    %rsp,%r12
  20047a:	31 f6                	xor    %esi,%esi
  20047c:	4c 89 e7             	mov    %r12,%rdi
  20047f:	48 83 c3 01          	add    $0x1,%rbx
  200483:	ba 98 01 00 00       	mov    $0x198,%edx
  200488:	48 29 eb             	sub    %rbp,%rbx
  20048b:	e8 90 12 00 00       	call   201720 <memset>
  200490:	b8 01 00 00 00       	mov    $0x1,%eax
  200495:	4c 89 e7             	mov    %r12,%rdi
  200498:	c6 04 24 82          	movb   $0x82,(%rsp)
  20049c:	48 8d b4 24 a0 01 00 	lea    0x1a0(%rsp),%rsi
  2004a3:	00 
  2004a4:	66 89 44 24 08       	mov    %ax,0x8(%rsp)
  2004a9:	48 c7 44 24 10 01 00 	movq   $0x1,0x10(%rsp)
  2004b0:	00 00 
  2004b2:	48 89 6c 24 18       	mov    %rbp,0x18(%rsp)
  2004b7:	89 5c 24 20          	mov    %ebx,0x20(%rsp)
  2004bb:	e8 50 fe ff ff       	call   200310 <lux_call>
  2004c0:	85 c0                	test   %eax,%eax
  2004c2:	78 2c                	js     2004f0 <sys_open+0xe0>
  2004c4:	8b 84 24 c8 01 00 00 	mov    0x1c8(%rsp),%eax
  2004cb:	48 81 c4 40 07 00 00 	add    $0x740,%rsp
  2004d2:	5b                   	pop    %rbx
  2004d3:	5d                   	pop    %rbp
  2004d4:	41 5c                	pop    %r12
  2004d6:	c3                   	ret
  2004d7:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  2004de:	00 00 
  2004e0:	31 c0                	xor    %eax,%eax
  2004e2:	31 c9                	xor    %ecx,%ecx
  2004e4:	bb 02 00 00 00       	mov    $0x2,%ebx
  2004e9:	31 d2                	xor    %edx,%edx
  2004eb:	e9 58 ff ff ff       	jmp    200448 <sys_open+0x38>
  2004f0:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  2004f5:	eb d4                	jmp    2004cb <sys_open+0xbb>
  2004f7:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  2004fe:	00 00 

0000000000200500 <sys_close>:
  200500:	f3 0f 1e fa          	endbr64
  200504:	48 83 ec 18          	sub    $0x18,%rsp
  200508:	31 c9                	xor    %ecx,%ecx
  20050a:	ba 04 00 00 00       	mov    $0x4,%edx
  20050f:	89 3c 24             	mov    %edi,(%rsp)
  200512:	48 89 e6             	mov    %rsp,%rsi
  200515:	bf 02 00 00 00       	mov    $0x2,%edi
  20051a:	e8 61 fe ff ff       	call   200380 <do_syscall>
  20051f:	48 83 c4 18          	add    $0x18,%rsp
  200523:	c3                   	ret
  200524:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  20052b:	00 00 00 00 
  20052f:	90                   	nop

0000000000200530 <sys_read>:
  200530:	f3 0f 1e fa          	endbr64
  200534:	41 55                	push   %r13
  200536:	41 54                	push   %r12
  200538:	55                   	push   %rbp
  200539:	48 89 f5             	mov    %rsi,%rbp
  20053c:	31 f6                	xor    %esi,%esi
  20053e:	53                   	push   %rbx
  20053f:	48 89 d3             	mov    %rdx,%rbx
  200542:	48 81 ec 68 03 00 00 	sub    $0x368,%rsp
  200549:	4c 8d 64 24 20       	lea    0x20(%rsp),%r12
  20054e:	89 3c 24             	mov    %edi,(%rsp)
  200551:	4c 8d ac 24 c0 01 00 	lea    0x1c0(%rsp),%r13
  200558:	00 
  200559:	89 54 24 0c          	mov    %edx,0xc(%rsp)
  20055d:	4c 89 e7             	mov    %r12,%rdi
  200560:	ba 98 01 00 00       	mov    $0x198,%edx
  200565:	48 c7 44 24 04 00 00 	movq   $0x0,0x4(%rsp)
  20056c:	00 00 
  20056e:	e8 ad 11 00 00       	call   201720 <memset>
  200573:	31 f6                	xor    %esi,%esi
  200575:	4c 89 ef             	mov    %r13,%rdi
  200578:	ba 98 01 00 00       	mov    $0x198,%edx
  20057d:	e8 9e 11 00 00       	call   201720 <memset>
  200582:	b8 01 00 00 00       	mov    $0x1,%eax
  200587:	4c 89 ee             	mov    %r13,%rsi
  20058a:	4c 89 e7             	mov    %r12,%rdi
  20058d:	66 89 44 24 28       	mov    %ax,0x28(%rsp)
  200592:	48 89 e0             	mov    %rsp,%rax
  200595:	c6 44 24 20 82       	movb   $0x82,0x20(%rsp)
  20059a:	c7 44 24 30 03 00 00 	movl   $0x3,0x30(%rsp)
  2005a1:	00 
  2005a2:	48 89 44 24 38       	mov    %rax,0x38(%rsp)
  2005a7:	c7 44 24 40 10 00 00 	movl   $0x10,0x40(%rsp)
  2005ae:	00 
  2005af:	e8 5c fd ff ff       	call   200310 <lux_call>
  2005b4:	85 c0                	test   %eax,%eax
  2005b6:	78 4c                	js     200604 <sys_read+0xd4>
  2005b8:	8b 94 24 d8 01 00 00 	mov    0x1d8(%rsp),%edx
  2005bf:	48 89 d0             	mov    %rdx,%rax
  2005c2:	48 39 da             	cmp    %rbx,%rdx
  2005c5:	7e 0c                	jle    2005d3 <sys_read+0xa3>
  2005c7:	89 9c 24 d8 01 00 00 	mov    %ebx,0x1d8(%rsp)
  2005ce:	89 da                	mov    %ebx,%edx
  2005d0:	48 89 d0             	mov    %rdx,%rax
  2005d3:	85 c0                	test   %eax,%eax
  2005d5:	74 1c                	je     2005f3 <sys_read+0xc3>
  2005d7:	48 8b b4 24 d8 01 00 	mov    0x1d8(%rsp),%rsi
  2005de:	00 
  2005df:	48 85 f6             	test   %rsi,%rsi
  2005e2:	74 0f                	je     2005f3 <sys_read+0xc3>
  2005e4:	48 89 ef             	mov    %rbp,%rdi
  2005e7:	e8 e4 10 00 00       	call   2016d0 <memmove>
  2005ec:	8b 94 24 d8 01 00 00 	mov    0x1d8(%rsp),%edx
  2005f3:	48 89 d0             	mov    %rdx,%rax
  2005f6:	48 81 c4 68 03 00 00 	add    $0x368,%rsp
  2005fd:	5b                   	pop    %rbx
  2005fe:	5d                   	pop    %rbp
  2005ff:	41 5c                	pop    %r12
  200601:	41 5d                	pop    %r13
  200603:	c3                   	ret
  200604:	48 c7 c0 ff ff ff ff 	mov    $0xffffffffffffffff,%rax
  20060b:	eb e9                	jmp    2005f6 <sys_read+0xc6>
  20060d:	0f 1f 00             	nopl   (%rax)

0000000000200610 <sys_write>:
  200610:	f3 0f 1e fa          	endbr64
  200614:	41 55                	push   %r13
  200616:	49 89 f5             	mov    %rsi,%r13
  200619:	41 54                	push   %r12
  20061b:	55                   	push   %rbp
  20061c:	89 fd                	mov    %edi,%ebp
  20061e:	48 8d 7a 10          	lea    0x10(%rdx),%rdi
  200622:	53                   	push   %rbx
  200623:	48 89 d3             	mov    %rdx,%rbx
  200626:	48 81 ec 58 07 00 00 	sub    $0x758,%rsp
  20062d:	48 c7 44 24 08 00 00 	movq   $0x0,0x8(%rsp)
  200634:	00 00 
  200636:	48 81 ff 00 04 00 00 	cmp    $0x400,%rdi
  20063d:	76 21                	jbe    200660 <sys_write+0x50>
  20063f:	48 8d 74 24 08       	lea    0x8(%rsp),%rsi
  200644:	e8 67 0e 00 00       	call   2014b0 <pebble_alloc>
  200649:	85 c0                	test   %eax,%eax
  20064b:	0f 88 c2 00 00 00    	js     200713 <sys_write+0x103>
  200651:	4c 8b 64 24 08       	mov    0x8(%rsp),%r12
  200656:	eb 10                	jmp    200668 <sys_write+0x58>
  200658:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
  20065f:	00 
  200660:	4c 8d a4 24 50 03 00 	lea    0x350(%rsp),%r12
  200667:	00 
  200668:	41 89 2c 24          	mov    %ebp,(%r12)
  20066c:	49 8d 6c 24 10       	lea    0x10(%r12),%rbp
  200671:	48 89 da             	mov    %rbx,%rdx
  200674:	4c 89 ee             	mov    %r13,%rsi
  200677:	41 89 5c 24 0c       	mov    %ebx,0xc(%r12)
  20067c:	48 89 ef             	mov    %rbp,%rdi
  20067f:	48 01 dd             	add    %rbx,%rbp
  200682:	48 8d 5c 24 10       	lea    0x10(%rsp),%rbx
  200687:	49 c7 44 24 04 00 00 	movq   $0x0,0x4(%r12)
  20068e:	00 00 
  200690:	4c 29 e5             	sub    %r12,%rbp
  200693:	e8 38 10 00 00       	call   2016d0 <memmove>
  200698:	31 f6                	xor    %esi,%esi
  20069a:	48 89 df             	mov    %rbx,%rdi
  20069d:	ba 98 01 00 00       	mov    $0x198,%edx
  2006a2:	e8 79 10 00 00       	call   201720 <memset>
  2006a7:	b8 01 00 00 00       	mov    $0x1,%eax
  2006ac:	48 89 df             	mov    %rbx,%rdi
  2006af:	48 8d b4 24 b0 01 00 	lea    0x1b0(%rsp),%rsi
  2006b6:	00 
  2006b7:	c6 44 24 10 82       	movb   $0x82,0x10(%rsp)
  2006bc:	66 89 44 24 18       	mov    %ax,0x18(%rsp)
  2006c1:	48 c7 44 24 20 04 00 	movq   $0x4,0x20(%rsp)
  2006c8:	00 00 
  2006ca:	4c 89 64 24 28       	mov    %r12,0x28(%rsp)
  2006cf:	89 6c 24 30          	mov    %ebp,0x30(%rsp)
  2006d3:	e8 38 fc ff ff       	call   200310 <lux_call>
  2006d8:	85 c0                	test   %eax,%eax
  2006da:	78 28                	js     200704 <sys_write+0xf4>
  2006dc:	48 8b 7c 24 08       	mov    0x8(%rsp),%rdi
  2006e1:	48 8b 9c 24 d8 01 00 	mov    0x1d8(%rsp),%rbx
  2006e8:	00 
  2006e9:	48 85 ff             	test   %rdi,%rdi
  2006ec:	74 05                	je     2006f3 <sys_write+0xe3>
  2006ee:	e8 4d 0f 00 00       	call   201640 <pebble_free>
  2006f3:	48 89 d8             	mov    %rbx,%rax
  2006f6:	48 81 c4 58 07 00 00 	add    $0x758,%rsp
  2006fd:	5b                   	pop    %rbx
  2006fe:	5d                   	pop    %rbp
  2006ff:	41 5c                	pop    %r12
  200701:	41 5d                	pop    %r13
  200703:	c3                   	ret
  200704:	48 8b 7c 24 08       	mov    0x8(%rsp),%rdi
  200709:	48 85 ff             	test   %rdi,%rdi
  20070c:	74 05                	je     200713 <sys_write+0x103>
  20070e:	e8 2d 0f 00 00       	call   201640 <pebble_free>
  200713:	48 c7 c0 ff ff ff ff 	mov    $0xffffffffffffffff,%rax
  20071a:	eb da                	jmp    2006f6 <sys_write+0xe6>
  20071c:	0f 1f 40 00          	nopl   0x0(%rax)

0000000000200720 <sys_pwrite>:
  200720:	f3 0f 1e fa          	endbr64
  200724:	41 57                	push   %r15
  200726:	41 56                	push   %r14
  200728:	41 89 fe             	mov    %edi,%r14d
  20072b:	48 8d 7a 10          	lea    0x10(%rdx),%rdi
  20072f:	41 55                	push   %r13
  200731:	49 89 cd             	mov    %rcx,%r13
  200734:	41 54                	push   %r12
  200736:	55                   	push   %rbp
  200737:	48 89 f5             	mov    %rsi,%rbp
  20073a:	53                   	push   %rbx
  20073b:	48 89 d3             	mov    %rdx,%rbx
  20073e:	48 81 ec 68 07 00 00 	sub    $0x768,%rsp
  200745:	48 c7 44 24 18 00 00 	movq   $0x0,0x18(%rsp)
  20074c:	00 00 
  20074e:	48 81 ff 00 04 00 00 	cmp    $0x400,%rdi
  200755:	76 19                	jbe    200770 <sys_pwrite+0x50>
  200757:	48 8d 74 24 18       	lea    0x18(%rsp),%rsi
  20075c:	e8 4f 0d 00 00       	call   2014b0 <pebble_alloc>
  200761:	85 c0                	test   %eax,%eax
  200763:	0f 88 a0 01 00 00    	js     200909 <sys_pwrite+0x1e9>
  200769:	4c 8b 64 24 18       	mov    0x18(%rsp),%r12
  20076e:	eb 08                	jmp    200778 <sys_pwrite+0x58>
  200770:	4c 8d a4 24 60 03 00 	lea    0x360(%rsp),%r12
  200777:	00 
  200778:	4d 89 eb             	mov    %r13,%r11
  20077b:	89 d8                	mov    %ebx,%eax
  20077d:	45 89 f2             	mov    %r14d,%r10d
  200780:	44 89 f1             	mov    %r14d,%ecx
  200783:	49 c1 eb 10          	shr    $0x10,%r11
  200787:	c1 f8 18             	sar    $0x18,%eax
  20078a:	41 89 d9             	mov    %ebx,%r9d
  20078d:	4d 89 e8             	mov    %r13,%r8
  200790:	4c 89 da             	mov    %r11,%rdx
  200793:	45 0f b6 db          	movzbl %r11b,%r11d
  200797:	41 89 c7             	mov    %eax,%r15d
  20079a:	4c 89 e8             	mov    %r13,%rax
  20079d:	81 e2 00 ff 00 00    	and    $0xff00,%edx
  2007a3:	0f b6 c4             	movzbl %ah,%eax
  2007a6:	41 c1 fa 18          	sar    $0x18,%r10d
  2007aa:	4c 89 ef             	mov    %r13,%rdi
  2007ad:	4c 09 da             	or     %r11,%rdx
  2007b0:	45 0f b6 dd          	movzbl %r13b,%r11d
  2007b4:	45 0f b6 d2          	movzbl %r10b,%r10d
  2007b8:	c1 f9 10             	sar    $0x10,%ecx
  2007bb:	48 c1 e2 08          	shl    $0x8,%rdx
  2007bf:	0f b6 c9             	movzbl %cl,%ecx
  2007c2:	41 c1 f9 10          	sar    $0x10,%r9d
  2007c6:	4c 89 ee             	mov    %r13,%rsi
  2007c9:	49 c1 e8 30          	shr    $0x30,%r8
  2007cd:	48 09 c2             	or     %rax,%rdx
  2007d0:	44 89 f0             	mov    %r14d,%eax
  2007d3:	45 0f b6 c9          	movzbl %r9b,%r9d
  2007d7:	48 c1 e2 08          	shl    $0x8,%rdx
  2007db:	45 0f b6 c0          	movzbl %r8b,%r8d
  2007df:	48 c1 ef 28          	shr    $0x28,%rdi
  2007e3:	4c 09 da             	or     %r11,%rdx
  2007e6:	40 0f b6 ff          	movzbl %dil,%edi
  2007ea:	48 c1 ee 20          	shr    $0x20,%rsi
  2007ee:	48 c1 e2 08          	shl    $0x8,%rdx
  2007f2:	40 0f b6 f6          	movzbl %sil,%esi
  2007f6:	4c 09 d2             	or     %r10,%rdx
  2007f9:	48 c1 e2 08          	shl    $0x8,%rdx
  2007fd:	48 09 ca             	or     %rcx,%rdx
  200800:	0f b6 cc             	movzbl %ah,%ecx
  200803:	41 0f b6 c7          	movzbl %r15b,%eax
  200807:	48 c1 e0 08          	shl    $0x8,%rax
  20080b:	48 c1 e2 08          	shl    $0x8,%rdx
  20080f:	4c 09 c8             	or     %r9,%rax
  200812:	48 09 ca             	or     %rcx,%rdx
  200815:	41 0f b6 ce          	movzbl %r14b,%ecx
  200819:	49 89 c7             	mov    %rax,%r15
  20081c:	0f b6 c7             	movzbl %bh,%eax
  20081f:	48 c1 e2 08          	shl    $0x8,%rdx
  200823:	49 89 c1             	mov    %rax,%r9
  200826:	4c 89 f8             	mov    %r15,%rax
  200829:	48 c1 e0 08          	shl    $0x8,%rax
  20082d:	4c 09 c8             	or     %r9,%rax
  200830:	44 0f b6 cb          	movzbl %bl,%r9d
  200834:	48 c1 e0 08          	shl    $0x8,%rax
  200838:	4c 09 c8             	or     %r9,%rax
  20083b:	4c 0f a4 e8 08       	shld   $0x8,%r13,%rax
  200840:	4d 8d 6c 24 10       	lea    0x10(%r12),%r13
  200845:	48 c1 e0 08          	shl    $0x8,%rax
  200849:	4c 09 c0             	or     %r8,%rax
  20084c:	48 c1 e0 08          	shl    $0x8,%rax
  200850:	48 09 f8             	or     %rdi,%rax
  200853:	4c 89 ef             	mov    %r13,%rdi
  200856:	48 c1 e0 08          	shl    $0x8,%rax
  20085a:	48 09 ca             	or     %rcx,%rdx
  20085d:	49 01 dd             	add    %rbx,%r13
  200860:	48 09 f0             	or     %rsi,%rax
  200863:	48 89 14 24          	mov    %rdx,(%rsp)
  200867:	48 89 ee             	mov    %rbp,%rsi
  20086a:	48 89 da             	mov    %rbx,%rdx
  20086d:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
  200872:	66 0f 6f 04 24       	movdqa (%rsp),%xmm0
  200877:	48 8d 5c 24 20       	lea    0x20(%rsp),%rbx
  20087c:	4d 29 e5             	sub    %r12,%r13
  20087f:	41 0f 11 04 24       	movups %xmm0,(%r12)
  200884:	e8 47 0e 00 00       	call   2016d0 <memmove>
  200889:	31 f6                	xor    %esi,%esi
  20088b:	48 89 df             	mov    %rbx,%rdi
  20088e:	ba 98 01 00 00       	mov    $0x198,%edx
  200893:	e8 88 0e 00 00       	call   201720 <memset>
  200898:	b8 01 00 00 00       	mov    $0x1,%eax
  20089d:	48 89 df             	mov    %rbx,%rdi
  2008a0:	48 8d b4 24 c0 01 00 	lea    0x1c0(%rsp),%rsi
  2008a7:	00 
  2008a8:	c6 44 24 20 82       	movb   $0x82,0x20(%rsp)
  2008ad:	66 89 44 24 28       	mov    %ax,0x28(%rsp)
  2008b2:	48 c7 44 24 30 06 00 	movq   $0x6,0x30(%rsp)
  2008b9:	00 00 
  2008bb:	4c 89 64 24 38       	mov    %r12,0x38(%rsp)
  2008c0:	44 89 6c 24 40       	mov    %r13d,0x40(%rsp)
  2008c5:	e8 46 fa ff ff       	call   200310 <lux_call>
  2008ca:	85 c0                	test   %eax,%eax
  2008cc:	78 2c                	js     2008fa <sys_pwrite+0x1da>
  2008ce:	48 8b 7c 24 18       	mov    0x18(%rsp),%rdi
  2008d3:	48 8b 9c 24 e8 01 00 	mov    0x1e8(%rsp),%rbx
  2008da:	00 
  2008db:	48 85 ff             	test   %rdi,%rdi
  2008de:	74 05                	je     2008e5 <sys_pwrite+0x1c5>
  2008e0:	e8 5b 0d 00 00       	call   201640 <pebble_free>
  2008e5:	48 89 d8             	mov    %rbx,%rax
  2008e8:	48 81 c4 68 07 00 00 	add    $0x768,%rsp
  2008ef:	5b                   	pop    %rbx
  2008f0:	5d                   	pop    %rbp
  2008f1:	41 5c                	pop    %r12
  2008f3:	41 5d                	pop    %r13
  2008f5:	41 5e                	pop    %r14
  2008f7:	41 5f                	pop    %r15
  2008f9:	c3                   	ret
  2008fa:	48 8b 7c 24 18       	mov    0x18(%rsp),%rdi
  2008ff:	48 85 ff             	test   %rdi,%rdi
  200902:	74 05                	je     200909 <sys_pwrite+0x1e9>
  200904:	e8 37 0d 00 00       	call   201640 <pebble_free>
  200909:	48 c7 c0 ff ff ff ff 	mov    $0xffffffffffffffff,%rax
  200910:	eb d6                	jmp    2008e8 <sys_pwrite+0x1c8>
  200912:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  200919:	00 00 00 00 
  20091d:	0f 1f 00             	nopl   (%rax)

0000000000200920 <sys_exit>:
  200920:	f3 0f 1e fa          	endbr64
  200924:	41 54                	push   %r12
  200926:	55                   	push   %rbp
  200927:	53                   	push   %rbx
  200928:	48 81 ec 40 04 00 00 	sub    $0x440,%rsp
  20092f:	48 85 ff             	test   %rdi,%rdi
  200932:	0f 84 a0 00 00 00    	je     2009d8 <sys_exit+0xb8>
  200938:	80 3f 00             	cmpb   $0x0,(%rdi)
  20093b:	48 89 fe             	mov    %rdi,%rsi
  20093e:	0f 84 ab 00 00 00    	je     2009ef <sys_exit+0xcf>
  200944:	b8 01 00 00 00       	mov    $0x1,%eax
  200949:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  200950:	48 89 c2             	mov    %rax,%rdx
  200953:	48 8d 40 01          	lea    0x1(%rax),%rax
  200957:	80 3c 16 00          	cmpb   $0x0,(%rsi,%rdx,1)
  20095b:	75 f3                	jne    200950 <sys_exit+0x30>
  20095d:	8d 6a 02             	lea    0x2(%rdx),%ebp
  200960:	89 d1                	mov    %edx,%ecx
  200962:	0f b6 c6             	movzbl %dh,%eax
  200965:	48 8d 7c 24 02       	lea    0x2(%rsp),%rdi
  20096a:	48 8d 9c 24 00 01 00 	lea    0x100(%rsp),%rbx
  200971:	00 
  200972:	88 0c 24             	mov    %cl,(%rsp)
  200975:	49 89 e4             	mov    %rsp,%r12
  200978:	88 44 24 01          	mov    %al,0x1(%rsp)
  20097c:	e8 4f 0d 00 00       	call   2016d0 <memmove>
  200981:	31 f6                	xor    %esi,%esi
  200983:	48 89 df             	mov    %rbx,%rdi
  200986:	ba 98 01 00 00       	mov    $0x198,%edx
  20098b:	e8 90 0d 00 00       	call   201720 <memset>
  200990:	b8 01 00 00 00       	mov    $0x1,%eax
  200995:	48 89 df             	mov    %rbx,%rdi
  200998:	48 8d b4 24 a0 02 00 	lea    0x2a0(%rsp),%rsi
  20099f:	00 
  2009a0:	c6 84 24 00 01 00 00 	movb   $0x82,0x100(%rsp)
  2009a7:	82 
  2009a8:	66 89 84 24 08 01 00 	mov    %ax,0x108(%rsp)
  2009af:	00 
  2009b0:	48 c7 84 24 10 01 00 	movq   $0x8,0x110(%rsp)
  2009b7:	00 08 00 00 00 
  2009bc:	4c 89 a4 24 18 01 00 	mov    %r12,0x118(%rsp)
  2009c3:	00 
  2009c4:	89 ac 24 20 01 00 00 	mov    %ebp,0x120(%rsp)
  2009cb:	e8 40 f9 ff ff       	call   200310 <lux_call>
  2009d0:	eb fe                	jmp    2009d0 <sys_exit+0xb0>
  2009d2:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  2009d8:	31 c0                	xor    %eax,%eax
  2009da:	31 c9                	xor    %ecx,%ecx
  2009dc:	bd 02 00 00 00       	mov    $0x2,%ebp
  2009e1:	31 d2                	xor    %edx,%edx
  2009e3:	48 8d 35 d9 2b 00 00 	lea    0x2bd9(%rip),%rsi        # 2035c3 <_syscall+0x6e>
  2009ea:	e9 76 ff ff ff       	jmp    200965 <sys_exit+0x45>
  2009ef:	31 c0                	xor    %eax,%eax
  2009f1:	31 c9                	xor    %ecx,%ecx
  2009f3:	bd 02 00 00 00       	mov    $0x2,%ebp
  2009f8:	31 d2                	xor    %edx,%edx
  2009fa:	e9 66 ff ff ff       	jmp    200965 <sys_exit+0x45>
  2009ff:	90                   	nop

0000000000200a00 <sys_create>:
  200a00:	f3 0f 1e fa          	endbr64
  200a04:	41 55                	push   %r13
  200a06:	41 89 d5             	mov    %edx,%r13d
  200a09:	41 54                	push   %r12
  200a0b:	41 89 f4             	mov    %esi,%r12d
  200a0e:	55                   	push   %rbp
  200a0f:	53                   	push   %rbx
  200a10:	48 81 ec 48 07 00 00 	sub    $0x748,%rsp
  200a17:	80 3f 00             	cmpb   $0x0,(%rdi)
  200a1a:	0f 84 28 01 00 00    	je     200b48 <sys_create+0x148>
  200a20:	b8 01 00 00 00       	mov    $0x1,%eax
  200a25:	0f 1f 00             	nopl   (%rax)
  200a28:	48 89 c2             	mov    %rax,%rdx
  200a2b:	48 8d 40 01          	lea    0x1(%rax),%rax
  200a2f:	80 3c 17 00          	cmpb   $0x0,(%rdi,%rdx,1)
  200a33:	75 f3                	jne    200a28 <sys_create+0x28>
  200a35:	8d 5a 02             	lea    0x2(%rdx),%ebx
  200a38:	89 d1                	mov    %edx,%ecx
  200a3a:	0f b6 c6             	movzbl %dh,%eax
  200a3d:	48 63 db             	movslq %ebx,%rbx
  200a40:	88 84 24 41 03 00 00 	mov    %al,0x341(%rsp)
  200a47:	48 8d 84 24 42 03 00 	lea    0x342(%rsp),%rax
  200a4e:	00 
  200a4f:	48 89 fe             	mov    %rdi,%rsi
  200a52:	48 8d ac 24 40 03 00 	lea    0x340(%rsp),%rbp
  200a59:	00 
  200a5a:	48 89 c7             	mov    %rax,%rdi
  200a5d:	88 8c 24 40 03 00 00 	mov    %cl,0x340(%rsp)
  200a64:	48 01 eb             	add    %rbp,%rbx
  200a67:	e8 64 0c 00 00       	call   2016d0 <memmove>
  200a6c:	44 89 e8             	mov    %r13d,%eax
  200a6f:	44 89 ee             	mov    %r13d,%esi
  200a72:	44 89 e1             	mov    %r12d,%ecx
  200a75:	c1 f8 18             	sar    $0x18,%eax
  200a78:	c1 fe 10             	sar    $0x10,%esi
  200a7b:	44 89 e2             	mov    %r12d,%edx
  200a7e:	48 83 c3 08          	add    $0x8,%rbx
  200a82:	c1 f9 18             	sar    $0x18,%ecx
  200a85:	0f b6 c0             	movzbl %al,%eax
  200a88:	40 0f b6 f6          	movzbl %sil,%esi
  200a8c:	c1 fa 10             	sar    $0x10,%edx
  200a8f:	48 c1 e0 08          	shl    $0x8,%rax
  200a93:	0f b6 c9             	movzbl %cl,%ecx
  200a96:	0f b6 d2             	movzbl %dl,%edx
  200a99:	48 09 f0             	or     %rsi,%rax
  200a9c:	48 89 c7             	mov    %rax,%rdi
  200a9f:	44 89 e8             	mov    %r13d,%eax
  200aa2:	45 0f b6 ed          	movzbl %r13b,%r13d
  200aa6:	0f b6 f4             	movzbl %ah,%esi
  200aa9:	48 89 f8             	mov    %rdi,%rax
  200aac:	48 c1 e0 08          	shl    $0x8,%rax
  200ab0:	48 09 f0             	or     %rsi,%rax
  200ab3:	31 f6                	xor    %esi,%esi
  200ab5:	48 c1 e0 08          	shl    $0x8,%rax
  200ab9:	4c 09 e8             	or     %r13,%rax
  200abc:	48 c1 e0 08          	shl    $0x8,%rax
  200ac0:	48 09 c8             	or     %rcx,%rax
  200ac3:	44 89 e1             	mov    %r12d,%ecx
  200ac6:	45 0f b6 e4          	movzbl %r12b,%r12d
  200aca:	48 c1 e0 08          	shl    $0x8,%rax
  200ace:	48 09 d0             	or     %rdx,%rax
  200ad1:	0f b6 d5             	movzbl %ch,%edx
  200ad4:	48 c1 e0 08          	shl    $0x8,%rax
  200ad8:	48 09 d0             	or     %rdx,%rax
  200adb:	ba 98 01 00 00       	mov    $0x198,%edx
  200ae0:	48 c1 e0 08          	shl    $0x8,%rax
  200ae4:	4c 09 e0             	or     %r12,%rax
  200ae7:	49 89 e4             	mov    %rsp,%r12
  200aea:	48 89 43 f8          	mov    %rax,-0x8(%rbx)
  200aee:	4c 89 e7             	mov    %r12,%rdi
  200af1:	48 29 eb             	sub    %rbp,%rbx
  200af4:	e8 27 0c 00 00       	call   201720 <memset>
  200af9:	b8 01 00 00 00       	mov    $0x1,%eax
  200afe:	4c 89 e7             	mov    %r12,%rdi
  200b01:	c6 04 24 82          	movb   $0x82,(%rsp)
  200b05:	48 8d b4 24 a0 01 00 	lea    0x1a0(%rsp),%rsi
  200b0c:	00 
  200b0d:	66 89 44 24 08       	mov    %ax,0x8(%rsp)
  200b12:	48 c7 44 24 10 07 00 	movq   $0x7,0x10(%rsp)
  200b19:	00 00 
  200b1b:	48 89 6c 24 18       	mov    %rbp,0x18(%rsp)
  200b20:	89 5c 24 20          	mov    %ebx,0x20(%rsp)
  200b24:	e8 e7 f7 ff ff       	call   200310 <lux_call>
  200b29:	85 c0                	test   %eax,%eax
  200b2b:	78 2b                	js     200b58 <sys_create+0x158>
  200b2d:	8b 84 24 c8 01 00 00 	mov    0x1c8(%rsp),%eax
  200b34:	48 81 c4 48 07 00 00 	add    $0x748,%rsp
  200b3b:	5b                   	pop    %rbx
  200b3c:	5d                   	pop    %rbp
  200b3d:	41 5c                	pop    %r12
  200b3f:	41 5d                	pop    %r13
  200b41:	c3                   	ret
  200b42:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  200b48:	31 c0                	xor    %eax,%eax
  200b4a:	31 c9                	xor    %ecx,%ecx
  200b4c:	bb 02 00 00 00       	mov    $0x2,%ebx
  200b51:	31 d2                	xor    %edx,%edx
  200b53:	e9 e8 fe ff ff       	jmp    200a40 <sys_create+0x40>
  200b58:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  200b5d:	eb d5                	jmp    200b34 <sys_create+0x134>
  200b5f:	90                   	nop

0000000000200b60 <sys_rfork>:
  200b60:	f3 0f 1e fa          	endbr64
  200b64:	48 83 ec 28          	sub    $0x28,%rsp
  200b68:	ba 04 00 00 00       	mov    $0x4,%edx
  200b6d:	89 7c 24 10          	mov    %edi,0x10(%rsp)
  200b71:	48 8d 4c 24 08       	lea    0x8(%rsp),%rcx
  200b76:	48 8d 74 24 10       	lea    0x10(%rsp),%rsi
  200b7b:	bf 13 00 00 00       	mov    $0x13,%edi
  200b80:	e8 fb f7 ff ff       	call   200380 <do_syscall>
  200b85:	85 c0                	test   %eax,%eax
  200b87:	78 09                	js     200b92 <sys_rfork+0x32>
  200b89:	8b 44 24 08          	mov    0x8(%rsp),%eax
  200b8d:	48 83 c4 28          	add    $0x28,%rsp
  200b91:	c3                   	ret
  200b92:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  200b97:	eb f4                	jmp    200b8d <sys_rfork+0x2d>
  200b99:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)

0000000000200ba0 <sys_exec>:
  200ba0:	f3 0f 1e fa          	endbr64
  200ba4:	41 54                	push   %r12
  200ba6:	ba 98 01 00 00       	mov    $0x198,%edx
  200bab:	31 f6                	xor    %esi,%esi
  200bad:	55                   	push   %rbp
  200bae:	53                   	push   %rbx
  200baf:	48 89 fb             	mov    %rdi,%rbx
  200bb2:	48 81 ec 40 03 00 00 	sub    $0x340,%rsp
  200bb9:	48 89 e5             	mov    %rsp,%rbp
  200bbc:	4c 8d a4 24 a0 01 00 	lea    0x1a0(%rsp),%r12
  200bc3:	00 
  200bc4:	48 89 ef             	mov    %rbp,%rdi
  200bc7:	e8 54 0b 00 00       	call   201720 <memset>
  200bcc:	4c 89 e7             	mov    %r12,%rdi
  200bcf:	ba 98 01 00 00       	mov    $0x198,%edx
  200bd4:	31 f6                	xor    %esi,%esi
  200bd6:	e8 45 0b 00 00       	call   201720 <memset>
  200bdb:	b8 01 00 00 00       	mov    $0x1,%eax
  200be0:	4c 89 e6             	mov    %r12,%rsi
  200be3:	48 89 ef             	mov    %rbp,%rdi
  200be6:	48 89 5c 24 18       	mov    %rbx,0x18(%rsp)
  200beb:	c6 04 24 80          	movb   $0x80,(%rsp)
  200bef:	66 89 44 24 08       	mov    %ax,0x8(%rsp)
  200bf4:	e8 17 f7 ff ff       	call   200310 <lux_call>
  200bf9:	48 81 c4 40 03 00 00 	add    $0x340,%rsp
  200c00:	5b                   	pop    %rbx
  200c01:	5d                   	pop    %rbp
  200c02:	41 5c                	pop    %r12
  200c04:	c3                   	ret
  200c05:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  200c0c:	00 00 00 00 

0000000000200c10 <sys_pipe>:
  200c10:	f3 0f 1e fa          	endbr64
  200c14:	41 54                	push   %r12
  200c16:	31 f6                	xor    %esi,%esi
  200c18:	ba 98 01 00 00       	mov    $0x198,%edx
  200c1d:	55                   	push   %rbp
  200c1e:	53                   	push   %rbx
  200c1f:	48 89 fb             	mov    %rdi,%rbx
  200c22:	48 81 ec 40 03 00 00 	sub    $0x340,%rsp
  200c29:	48 89 e5             	mov    %rsp,%rbp
  200c2c:	4c 8d a4 24 a0 01 00 	lea    0x1a0(%rsp),%r12
  200c33:	00 
  200c34:	48 89 ef             	mov    %rbp,%rdi
  200c37:	e8 e4 0a 00 00       	call   201720 <memset>
  200c3c:	31 f6                	xor    %esi,%esi
  200c3e:	4c 89 e7             	mov    %r12,%rdi
  200c41:	ba 98 01 00 00       	mov    $0x198,%edx
  200c46:	e8 d5 0a 00 00       	call   201720 <memset>
  200c4b:	b8 01 00 00 00       	mov    $0x1,%eax
  200c50:	4c 89 e6             	mov    %r12,%rsi
  200c53:	48 89 ef             	mov    %rbp,%rdi
  200c56:	c6 04 24 82          	movb   $0x82,(%rsp)
  200c5a:	66 89 44 24 08       	mov    %ax,0x8(%rsp)
  200c5f:	48 c7 44 24 10 15 00 	movq   $0x15,0x10(%rsp)
  200c66:	00 00 
  200c68:	48 c7 44 24 18 00 00 	movq   $0x0,0x18(%rsp)
  200c6f:	00 00 
  200c71:	c7 44 24 20 00 00 00 	movl   $0x0,0x20(%rsp)
  200c78:	00 
  200c79:	e8 92 f6 ff ff       	call   200310 <lux_call>
  200c7e:	85 c0                	test   %eax,%eax
  200c80:	78 36                	js     200cb8 <sys_pipe+0xa8>
  200c82:	83 bc 24 c0 01 00 00 	cmpl   $0x7,0x1c0(%rsp)
  200c89:	07 
  200c8a:	76 2c                	jbe    200cb8 <sys_pipe+0xa8>
  200c8c:	48 8b 84 24 b8 01 00 	mov    0x1b8(%rsp),%rax
  200c93:	00 
  200c94:	48 85 c0             	test   %rax,%rax
  200c97:	74 1f                	je     200cb8 <sys_pipe+0xa8>
  200c99:	8b 10                	mov    (%rax),%edx
  200c9b:	89 13                	mov    %edx,(%rbx)
  200c9d:	8b 40 04             	mov    0x4(%rax),%eax
  200ca0:	89 43 04             	mov    %eax,0x4(%rbx)
  200ca3:	31 c0                	xor    %eax,%eax
  200ca5:	48 81 c4 40 03 00 00 	add    $0x340,%rsp
  200cac:	5b                   	pop    %rbx
  200cad:	5d                   	pop    %rbp
  200cae:	41 5c                	pop    %r12
  200cb0:	c3                   	ret
  200cb1:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  200cb8:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  200cbd:	eb e6                	jmp    200ca5 <sys_pipe+0x95>
  200cbf:	90                   	nop

0000000000200cc0 <sys_seek>:
  200cc0:	f3 0f 1e fa          	endbr64
  200cc4:	41 56                	push   %r14
  200cc6:	41 89 d1             	mov    %edx,%r9d
  200cc9:	41 89 d2             	mov    %edx,%r10d
  200ccc:	89 d0                	mov    %edx,%eax
  200cce:	41 54                	push   %r12
  200cd0:	49 89 f4             	mov    %rsi,%r12
  200cd3:	c1 f8 18             	sar    $0x18,%eax
  200cd6:	41 89 fb             	mov    %edi,%r11d
  200cd9:	49 c1 ec 10          	shr    $0x10,%r12
  200cdd:	55                   	push   %rbp
  200cde:	41 89 c6             	mov    %eax,%r14d
  200ce1:	89 fd                	mov    %edi,%ebp
  200ce3:	4c 89 e2             	mov    %r12,%rdx
  200ce6:	45 0f b6 e4          	movzbl %r12b,%r12d
  200cea:	53                   	push   %rbx
  200ceb:	48 89 f3             	mov    %rsi,%rbx
  200cee:	81 e2 00 ff 00 00    	and    $0xff00,%edx
  200cf4:	0f b6 c7             	movzbl %bh,%eax
  200cf7:	c1 fd 18             	sar    $0x18,%ebp
  200cfa:	89 f9                	mov    %edi,%ecx
  200cfc:	4c 09 e2             	or     %r12,%rdx
  200cff:	44 0f b6 e3          	movzbl %bl,%r12d
  200d03:	40 0f b6 ed          	movzbl %bpl,%ebp
  200d07:	41 c1 fb 10          	sar    $0x10,%r11d
  200d0b:	48 c1 e2 08          	shl    $0x8,%rdx
  200d0f:	45 0f b6 db          	movzbl %r11b,%r11d
  200d13:	41 c1 fa 10          	sar    $0x10,%r10d
  200d17:	49 89 d8             	mov    %rbx,%r8
  200d1a:	48 09 c2             	or     %rax,%rdx
  200d1d:	0f b6 c5             	movzbl %ch,%eax
  200d20:	45 0f b6 d2          	movzbl %r10b,%r10d
  200d24:	49 c1 e8 30          	shr    $0x30,%r8
  200d28:	48 c1 e2 08          	shl    $0x8,%rdx
  200d2c:	48 89 df             	mov    %rbx,%rdi
  200d2f:	45 0f b6 c0          	movzbl %r8b,%r8d
  200d33:	48 83 ec 48          	sub    $0x48,%rsp
  200d37:	48 c1 ef 28          	shr    $0x28,%rdi
  200d3b:	4c 09 e2             	or     %r12,%rdx
  200d3e:	48 c1 ee 20          	shr    $0x20,%rsi
  200d42:	0f b6 c9             	movzbl %cl,%ecx
  200d45:	48 c1 e2 08          	shl    $0x8,%rdx
  200d49:	40 0f b6 ff          	movzbl %dil,%edi
  200d4d:	40 0f b6 f6          	movzbl %sil,%esi
  200d51:	48 09 ea             	or     %rbp,%rdx
  200d54:	48 c1 e2 08          	shl    $0x8,%rdx
  200d58:	4c 09 da             	or     %r11,%rdx
  200d5b:	48 c1 e2 08          	shl    $0x8,%rdx
  200d5f:	48 09 c2             	or     %rax,%rdx
  200d62:	41 0f b6 c6          	movzbl %r14b,%eax
  200d66:	48 c1 e0 08          	shl    $0x8,%rax
  200d6a:	48 c1 e2 08          	shl    $0x8,%rdx
  200d6e:	4c 09 d0             	or     %r10,%rax
  200d71:	49 89 c6             	mov    %rax,%r14
  200d74:	44 89 c8             	mov    %r9d,%eax
  200d77:	45 0f b6 c9          	movzbl %r9b,%r9d
  200d7b:	0f b6 c4             	movzbl %ah,%eax
  200d7e:	49 89 c2             	mov    %rax,%r10
  200d81:	4c 89 f0             	mov    %r14,%rax
  200d84:	48 c1 e0 08          	shl    $0x8,%rax
  200d88:	4c 09 d0             	or     %r10,%rax
  200d8b:	48 c1 e0 08          	shl    $0x8,%rax
  200d8f:	4c 09 c8             	or     %r9,%rax
  200d92:	48 0f a4 d8 08       	shld   $0x8,%rbx,%rax
  200d97:	48 c1 e0 08          	shl    $0x8,%rax
  200d9b:	4c 09 c0             	or     %r8,%rax
  200d9e:	48 c1 e0 08          	shl    $0x8,%rax
  200da2:	48 09 f8             	or     %rdi,%rax
  200da5:	48 09 ca             	or     %rcx,%rdx
  200da8:	48 8d 4c 24 18       	lea    0x18(%rsp),%rcx
  200dad:	bf 27 00 00 00       	mov    $0x27,%edi
  200db2:	48 c1 e0 08          	shl    $0x8,%rax
  200db6:	48 89 14 24          	mov    %rdx,(%rsp)
  200dba:	ba 10 00 00 00       	mov    $0x10,%edx
  200dbf:	48 09 f0             	or     %rsi,%rax
  200dc2:	48 8d 74 24 20       	lea    0x20(%rsp),%rsi
  200dc7:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
  200dcc:	66 0f 6f 04 24       	movdqa (%rsp),%xmm0
  200dd1:	0f 29 44 24 20       	movaps %xmm0,0x20(%rsp)
  200dd6:	e8 a5 f5 ff ff       	call   200380 <do_syscall>
  200ddb:	85 c0                	test   %eax,%eax
  200ddd:	78 10                	js     200def <sys_seek+0x12f>
  200ddf:	48 8b 44 24 18       	mov    0x18(%rsp),%rax
  200de4:	48 83 c4 48          	add    $0x48,%rsp
  200de8:	5b                   	pop    %rbx
  200de9:	5d                   	pop    %rbp
  200dea:	41 5c                	pop    %r12
  200dec:	41 5e                	pop    %r14
  200dee:	c3                   	ret
  200def:	48 c7 c0 ff ff ff ff 	mov    $0xffffffffffffffff,%rax
  200df6:	eb ec                	jmp    200de4 <sys_seek+0x124>
  200df8:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
  200dff:	00 

0000000000200e00 <sys_wait>:
  200e00:	f3 0f 1e fa          	endbr64
  200e04:	48 83 ec 18          	sub    $0x18,%rsp
  200e08:	31 d2                	xor    %edx,%edx
  200e0a:	31 f6                	xor    %esi,%esi
  200e0c:	bf a6 00 00 00       	mov    $0xa6,%edi
  200e11:	48 8d 4c 24 08       	lea    0x8(%rsp),%rcx
  200e16:	e8 65 f5 ff ff       	call   200380 <do_syscall>
  200e1b:	85 c0                	test   %eax,%eax
  200e1d:	78 09                	js     200e28 <sys_wait+0x28>
  200e1f:	8b 44 24 08          	mov    0x8(%rsp),%eax
  200e23:	48 83 c4 18          	add    $0x18,%rsp
  200e27:	c3                   	ret
  200e28:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  200e2d:	eb f4                	jmp    200e23 <sys_wait+0x23>
  200e2f:	90                   	nop

0000000000200e30 <sys_pread>:
  200e30:	f3 0f 1e fa          	endbr64
  200e34:	49 89 c9             	mov    %rcx,%r9
  200e37:	41 56                	push   %r14
  200e39:	41 89 d3             	mov    %edx,%r11d
  200e3c:	89 d0                	mov    %edx,%eax
  200e3e:	41 55                	push   %r13
  200e40:	4d 89 cd             	mov    %r9,%r13
  200e43:	89 f9                	mov    %edi,%ecx
  200e45:	c1 f8 18             	sar    $0x18,%eax
  200e48:	49 c1 ed 10          	shr    $0x10,%r13
  200e4c:	41 54                	push   %r12
  200e4e:	41 89 fc             	mov    %edi,%r12d
  200e51:	41 89 fa             	mov    %edi,%r10d
  200e54:	55                   	push   %rbp
  200e55:	48 89 d5             	mov    %rdx,%rbp
  200e58:	4c 89 ea             	mov    %r13,%rdx
  200e5b:	45 0f b6 ed          	movzbl %r13b,%r13d
  200e5f:	81 e2 00 ff 00 00    	and    $0xff00,%edx
  200e65:	53                   	push   %rbx
  200e66:	4c 89 cb             	mov    %r9,%rbx
  200e69:	41 c1 fc 18          	sar    $0x18,%r12d
  200e6d:	4c 09 ea             	or     %r13,%rdx
  200e70:	0f b6 df             	movzbl %bh,%ebx
  200e73:	45 0f b6 e9          	movzbl %r9b,%r13d
  200e77:	45 0f b6 e4          	movzbl %r12b,%r12d
  200e7b:	48 c1 e2 08          	shl    $0x8,%rdx
  200e7f:	c1 f9 10             	sar    $0x10,%ecx
  200e82:	0f b6 c0             	movzbl %al,%eax
  200e85:	4d 89 c8             	mov    %r9,%r8
  200e88:	48 09 da             	or     %rbx,%rdx
  200e8b:	41 c1 fb 10          	sar    $0x10,%r11d
  200e8f:	44 89 d3             	mov    %r10d,%ebx
  200e92:	0f b6 c9             	movzbl %cl,%ecx
  200e95:	48 c1 e2 08          	shl    $0x8,%rdx
  200e99:	45 0f b6 db          	movzbl %r11b,%r11d
  200e9d:	48 c1 e0 08          	shl    $0x8,%rax
  200ea1:	4c 89 cf             	mov    %r9,%rdi
  200ea4:	4c 09 ea             	or     %r13,%rdx
  200ea7:	4c 09 d8             	or     %r11,%rax
  200eaa:	49 c1 e8 30          	shr    $0x30,%r8
  200eae:	49 89 f6             	mov    %rsi,%r14
  200eb1:	48 c1 e2 08          	shl    $0x8,%rdx
  200eb5:	48 c1 e0 08          	shl    $0x8,%rax
  200eb9:	45 0f b6 c0          	movzbl %r8b,%r8d
  200ebd:	4c 89 ce             	mov    %r9,%rsi
  200ec0:	4c 09 e2             	or     %r12,%rdx
  200ec3:	48 c1 ef 28          	shr    $0x28,%rdi
  200ec7:	48 81 ec 70 03 00 00 	sub    $0x370,%rsp
  200ece:	48 c1 e2 08          	shl    $0x8,%rdx
  200ed2:	40 0f b6 ff          	movzbl %dil,%edi
  200ed6:	48 c1 ee 20          	shr    $0x20,%rsi
  200eda:	4c 8d 64 24 30       	lea    0x30(%rsp),%r12
  200edf:	48 09 ca             	or     %rcx,%rdx
  200ee2:	0f b6 cf             	movzbl %bh,%ecx
  200ee5:	48 89 eb             	mov    %rbp,%rbx
  200ee8:	40 0f b6 f6          	movzbl %sil,%esi
  200eec:	0f b6 df             	movzbl %bh,%ebx
  200eef:	48 c1 e2 08          	shl    $0x8,%rdx
  200ef3:	4c 8d ac 24 d0 01 00 	lea    0x1d0(%rsp),%r13
  200efa:	00 
  200efb:	48 09 d8             	or     %rbx,%rax
  200efe:	48 09 ca             	or     %rcx,%rdx
  200f01:	41 0f b6 ca          	movzbl %r10b,%ecx
  200f05:	44 0f b6 d5          	movzbl %bpl,%r10d
  200f09:	48 c1 e0 08          	shl    $0x8,%rax
  200f0d:	48 c1 e2 08          	shl    $0x8,%rdx
  200f11:	4c 09 d0             	or     %r10,%rax
  200f14:	4c 0f a4 c8 08       	shld   $0x8,%r9,%rax
  200f19:	48 c1 e0 08          	shl    $0x8,%rax
  200f1d:	4c 09 c0             	or     %r8,%rax
  200f20:	48 c1 e0 08          	shl    $0x8,%rax
  200f24:	48 09 f8             	or     %rdi,%rax
  200f27:	48 09 ca             	or     %rcx,%rdx
  200f2a:	4c 89 e7             	mov    %r12,%rdi
  200f2d:	48 c1 e0 08          	shl    $0x8,%rax
  200f31:	48 89 14 24          	mov    %rdx,(%rsp)
  200f35:	ba 98 01 00 00       	mov    $0x198,%edx
  200f3a:	48 09 f0             	or     %rsi,%rax
  200f3d:	31 f6                	xor    %esi,%esi
  200f3f:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
  200f44:	66 0f 6f 04 24       	movdqa (%rsp),%xmm0
  200f49:	0f 29 44 24 10       	movaps %xmm0,0x10(%rsp)
  200f4e:	e8 cd 07 00 00       	call   201720 <memset>
  200f53:	31 f6                	xor    %esi,%esi
  200f55:	4c 89 ef             	mov    %r13,%rdi
  200f58:	ba 98 01 00 00       	mov    $0x198,%edx
  200f5d:	e8 be 07 00 00       	call   201720 <memset>
  200f62:	b8 01 00 00 00       	mov    $0x1,%eax
  200f67:	4c 89 ee             	mov    %r13,%rsi
  200f6a:	4c 89 e7             	mov    %r12,%rdi
  200f6d:	66 89 44 24 38       	mov    %ax,0x38(%rsp)
  200f72:	48 8d 44 24 10       	lea    0x10(%rsp),%rax
  200f77:	c6 44 24 30 82       	movb   $0x82,0x30(%rsp)
  200f7c:	c7 44 24 40 05 00 00 	movl   $0x5,0x40(%rsp)
  200f83:	00 
  200f84:	48 89 44 24 48       	mov    %rax,0x48(%rsp)
  200f89:	c7 44 24 50 10 00 00 	movl   $0x10,0x50(%rsp)
  200f90:	00 
  200f91:	e8 7a f3 ff ff       	call   200310 <lux_call>
  200f96:	85 c0                	test   %eax,%eax
  200f98:	78 4e                	js     200fe8 <sys_pread+0x1b8>
  200f9a:	8b 94 24 e8 01 00 00 	mov    0x1e8(%rsp),%edx
  200fa1:	48 89 d0             	mov    %rdx,%rax
  200fa4:	48 39 ea             	cmp    %rbp,%rdx
  200fa7:	7e 0c                	jle    200fb5 <sys_pread+0x185>
  200fa9:	89 ac 24 e8 01 00 00 	mov    %ebp,0x1e8(%rsp)
  200fb0:	89 ea                	mov    %ebp,%edx
  200fb2:	48 89 d0             	mov    %rdx,%rax
  200fb5:	85 c0                	test   %eax,%eax
  200fb7:	74 1c                	je     200fd5 <sys_pread+0x1a5>
  200fb9:	48 8b b4 24 e8 01 00 	mov    0x1e8(%rsp),%rsi
  200fc0:	00 
  200fc1:	48 85 f6             	test   %rsi,%rsi
  200fc4:	74 0f                	je     200fd5 <sys_pread+0x1a5>
  200fc6:	4c 89 f7             	mov    %r14,%rdi
  200fc9:	e8 02 07 00 00       	call   2016d0 <memmove>
  200fce:	8b 94 24 e8 01 00 00 	mov    0x1e8(%rsp),%edx
  200fd5:	48 89 d0             	mov    %rdx,%rax
  200fd8:	48 81 c4 70 03 00 00 	add    $0x370,%rsp
  200fdf:	5b                   	pop    %rbx
  200fe0:	5d                   	pop    %rbp
  200fe1:	41 5c                	pop    %r12
  200fe3:	41 5d                	pop    %r13
  200fe5:	41 5e                	pop    %r14
  200fe7:	c3                   	ret
  200fe8:	48 c7 c0 ff ff ff ff 	mov    $0xffffffffffffffff,%rax
  200fef:	eb e7                	jmp    200fd8 <sys_pread+0x1a8>
  200ff1:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  200ff8:	00 00 00 00 
  200ffc:	0f 1f 40 00          	nopl   0x0(%rax)

0000000000201000 <sys_nsec>:
  201000:	f3 0f 1e fa          	endbr64
  201004:	55                   	push   %rbp
  201005:	ba 98 01 00 00       	mov    $0x198,%edx
  20100a:	31 f6                	xor    %esi,%esi
  20100c:	53                   	push   %rbx
  20100d:	48 81 ec 48 03 00 00 	sub    $0x348,%rsp
  201014:	48 89 e3             	mov    %rsp,%rbx
  201017:	48 8d ac 24 a0 01 00 	lea    0x1a0(%rsp),%rbp
  20101e:	00 
  20101f:	48 89 df             	mov    %rbx,%rdi
  201022:	e8 f9 06 00 00       	call   201720 <memset>
  201027:	ba 98 01 00 00       	mov    $0x198,%edx
  20102c:	31 f6                	xor    %esi,%esi
  20102e:	48 89 ef             	mov    %rbp,%rdi
  201031:	e8 ea 06 00 00       	call   201720 <memset>
  201036:	b8 01 00 00 00       	mov    $0x1,%eax
  20103b:	48 89 ee             	mov    %rbp,%rsi
  20103e:	48 89 df             	mov    %rbx,%rdi
  201041:	c6 04 24 82          	movb   $0x82,(%rsp)
  201045:	66 89 44 24 08       	mov    %ax,0x8(%rsp)
  20104a:	c7 44 24 10 35 00 00 	movl   $0x35,0x10(%rsp)
  201051:	00 
  201052:	c7 44 24 20 00 00 00 	movl   $0x0,0x20(%rsp)
  201059:	00 
  20105a:	48 c7 44 24 18 00 00 	movq   $0x0,0x18(%rsp)
  201061:	00 00 
  201063:	e8 a8 f2 ff ff       	call   200310 <lux_call>
  201068:	89 c2                	mov    %eax,%edx
  20106a:	31 c0                	xor    %eax,%eax
  20106c:	85 d2                	test   %edx,%edx
  20106e:	78 08                	js     201078 <sys_nsec+0x78>
  201070:	48 8b 84 24 c8 01 00 	mov    0x1c8(%rsp),%rax
  201077:	00 
  201078:	48 81 c4 48 03 00 00 	add    $0x348,%rsp
  20107f:	5b                   	pop    %rbx
  201080:	5d                   	pop    %rbp
  201081:	c3                   	ret
  201082:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  201089:	00 00 00 00 
  20108d:	0f 1f 00             	nopl   (%rax)

0000000000201090 <sys_stat>:
  201090:	f3 0f 1e fa          	endbr64
  201094:	41 54                	push   %r12
  201096:	48 89 fe             	mov    %rdi,%rsi
  201099:	55                   	push   %rbp
  20109a:	53                   	push   %rbx
  20109b:	48 81 ec 40 07 00 00 	sub    $0x740,%rsp
  2010a2:	80 3f 00             	cmpb   $0x0,(%rdi)
  2010a5:	0f 84 95 00 00 00    	je     201140 <sys_stat+0xb0>
  2010ab:	b8 01 00 00 00       	mov    $0x1,%eax
  2010b0:	48 89 c2             	mov    %rax,%rdx
  2010b3:	48 8d 40 01          	lea    0x1(%rax),%rax
  2010b7:	80 3c 16 00          	cmpb   $0x0,(%rsi,%rdx,1)
  2010bb:	75 f3                	jne    2010b0 <sys_stat+0x20>
  2010bd:	44 8d 62 02          	lea    0x2(%rdx),%r12d
  2010c1:	89 d1                	mov    %edx,%ecx
  2010c3:	0f b6 c6             	movzbl %dh,%eax
  2010c6:	48 89 e3             	mov    %rsp,%rbx
  2010c9:	48 8d bc 24 42 03 00 	lea    0x342(%rsp),%rdi
  2010d0:	00 
  2010d1:	88 8c 24 40 03 00 00 	mov    %cl,0x340(%rsp)
  2010d8:	48 8d ac 24 40 03 00 	lea    0x340(%rsp),%rbp
  2010df:	00 
  2010e0:	88 84 24 41 03 00 00 	mov    %al,0x341(%rsp)
  2010e7:	e8 e4 05 00 00       	call   2016d0 <memmove>
  2010ec:	48 89 df             	mov    %rbx,%rdi
  2010ef:	ba 98 01 00 00       	mov    $0x198,%edx
  2010f4:	31 f6                	xor    %esi,%esi
  2010f6:	e8 25 06 00 00       	call   201720 <memset>
  2010fb:	b8 01 00 00 00       	mov    $0x1,%eax
  201100:	48 89 df             	mov    %rbx,%rdi
  201103:	48 8d b4 24 a0 01 00 	lea    0x1a0(%rsp),%rsi
  20110a:	00 
  20110b:	48 89 6c 24 18       	mov    %rbp,0x18(%rsp)
  201110:	44 89 64 24 20       	mov    %r12d,0x20(%rsp)
  201115:	c6 04 24 82          	movb   $0x82,(%rsp)
  201119:	66 89 44 24 08       	mov    %ax,0x8(%rsp)
  20111e:	48 c7 44 24 10 0a 00 	movq   $0xa,0x10(%rsp)
  201125:	00 00 
  201127:	e8 e4 f1 ff ff       	call   200310 <lux_call>
  20112c:	48 81 c4 40 07 00 00 	add    $0x740,%rsp
  201133:	5b                   	pop    %rbx
  201134:	c1 f8 1f             	sar    $0x1f,%eax
  201137:	5d                   	pop    %rbp
  201138:	41 5c                	pop    %r12
  20113a:	c3                   	ret
  20113b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  201140:	31 c0                	xor    %eax,%eax
  201142:	31 c9                	xor    %ecx,%ecx
  201144:	41 bc 02 00 00 00    	mov    $0x2,%r12d
  20114a:	31 d2                	xor    %edx,%edx
  20114c:	e9 75 ff ff ff       	jmp    2010c6 <sys_stat+0x36>
  201151:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  201158:	00 00 00 00 
  20115c:	0f 1f 40 00          	nopl   0x0(%rax)

0000000000201160 <sys_wstat>:
  201160:	f3 0f 1e fa          	endbr64
  201164:	41 56                	push   %r14
  201166:	41 55                	push   %r13
  201168:	4c 63 ea             	movslq %edx,%r13
  20116b:	41 54                	push   %r12
  20116d:	55                   	push   %rbp
  20116e:	48 89 f5             	mov    %rsi,%rbp
  201171:	53                   	push   %rbx
  201172:	48 89 fb             	mov    %rdi,%rbx
  201175:	48 81 ec 50 07 00 00 	sub    $0x750,%rsp
  20117c:	80 3f 00             	cmpb   $0x0,(%rdi)
  20117f:	48 c7 44 24 08 00 00 	movq   $0x0,0x8(%rsp)
  201186:	00 00 
  201188:	0f 84 12 01 00 00    	je     2012a0 <sys_wstat+0x140>
  20118e:	31 c0                	xor    %eax,%eax
  201190:	48 89 c2             	mov    %rax,%rdx
  201193:	48 83 c0 01          	add    $0x1,%rax
  201197:	80 3c 03 00          	cmpb   $0x0,(%rbx,%rax,1)
  20119b:	75 f3                	jne    201190 <sys_wstat+0x30>
  20119d:	41 8d 7c 15 05       	lea    0x5(%r13,%rdx,1),%edi
  2011a2:	81 ff 00 04 00 00    	cmp    $0x400,%edi
  2011a8:	77 0e                	ja     2011b8 <sys_wstat+0x58>
  2011aa:	4c 8d b4 24 50 03 00 	lea    0x350(%rsp),%r14
  2011b1:	00 
  2011b2:	eb 27                	jmp    2011db <sys_wstat+0x7b>
  2011b4:	0f 1f 40 00          	nopl   0x0(%rax)
  2011b8:	48 8d 74 24 08       	lea    0x8(%rsp),%rsi
  2011bd:	48 63 ff             	movslq %edi,%rdi
  2011c0:	e8 eb 02 00 00       	call   2014b0 <pebble_alloc>
  2011c5:	85 c0                	test   %eax,%eax
  2011c7:	0f 88 17 01 00 00    	js     2012e4 <sys_wstat+0x184>
  2011cd:	80 3b 00             	cmpb   $0x0,(%rbx)
  2011d0:	4c 8b 74 24 08       	mov    0x8(%rsp),%r14
  2011d5:	0f 84 ee 00 00 00    	je     2012c9 <sys_wstat+0x169>
  2011db:	b8 01 00 00 00       	mov    $0x1,%eax
  2011e0:	48 89 c2             	mov    %rax,%rdx
  2011e3:	48 8d 40 01          	lea    0x1(%rax),%rax
  2011e7:	80 3c 13 00          	cmpb   $0x0,(%rbx,%rdx,1)
  2011eb:	75 f3                	jne    2011e0 <sys_wstat+0x80>
  2011ed:	44 8d 62 02          	lea    0x2(%rdx),%r12d
  2011f1:	89 d1                	mov    %edx,%ecx
  2011f3:	0f b6 c6             	movzbl %dh,%eax
  2011f6:	4d 63 e4             	movslq %r12d,%r12
  2011f9:	41 88 0e             	mov    %cl,(%r14)
  2011fc:	48 89 de             	mov    %rbx,%rsi
  2011ff:	49 8d 7e 02          	lea    0x2(%r14),%rdi
  201203:	4b 8d 1c 26          	lea    (%r14,%r12,1),%rbx
  201207:	41 88 46 01          	mov    %al,0x1(%r14)
  20120b:	48 83 c3 02          	add    $0x2,%rbx
  20120f:	e8 bc 04 00 00       	call   2016d0 <memmove>
  201214:	66 44 89 6b fe       	mov    %r13w,-0x2(%rbx)
  201219:	48 89 ee             	mov    %rbp,%rsi
  20121c:	48 89 df             	mov    %rbx,%rdi
  20121f:	4c 89 ea             	mov    %r13,%rdx
  201222:	48 8d 6c 24 10       	lea    0x10(%rsp),%rbp
  201227:	4c 01 eb             	add    %r13,%rbx
  20122a:	4c 29 f3             	sub    %r14,%rbx
  20122d:	e8 9e 04 00 00       	call   2016d0 <memmove>
  201232:	ba 98 01 00 00       	mov    $0x198,%edx
  201237:	31 f6                	xor    %esi,%esi
  201239:	48 89 ef             	mov    %rbp,%rdi
  20123c:	e8 df 04 00 00       	call   201720 <memset>
  201241:	b8 01 00 00 00       	mov    $0x1,%eax
  201246:	48 89 ef             	mov    %rbp,%rdi
  201249:	48 8d b4 24 b0 01 00 	lea    0x1b0(%rsp),%rsi
  201250:	00 
  201251:	c6 44 24 10 82       	movb   $0x82,0x10(%rsp)
  201256:	66 89 44 24 18       	mov    %ax,0x18(%rsp)
  20125b:	48 c7 44 24 20 0b 00 	movq   $0xb,0x20(%rsp)
  201262:	00 00 
  201264:	4c 89 74 24 28       	mov    %r14,0x28(%rsp)
  201269:	89 5c 24 30          	mov    %ebx,0x30(%rsp)
  20126d:	e8 9e f0 ff ff       	call   200310 <lux_call>
  201272:	48 8b 7c 24 08       	mov    0x8(%rsp),%rdi
  201277:	85 c0                	test   %eax,%eax
  201279:	78 5f                	js     2012da <sys_wstat+0x17a>
  20127b:	48 85 ff             	test   %rdi,%rdi
  20127e:	74 05                	je     201285 <sys_wstat+0x125>
  201280:	e8 bb 03 00 00       	call   201640 <pebble_free>
  201285:	31 c0                	xor    %eax,%eax
  201287:	48 81 c4 50 07 00 00 	add    $0x750,%rsp
  20128e:	5b                   	pop    %rbx
  20128f:	5d                   	pop    %rbp
  201290:	41 5c                	pop    %r12
  201292:	41 5d                	pop    %r13
  201294:	41 5e                	pop    %r14
  201296:	c3                   	ret
  201297:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  20129e:	00 00 
  2012a0:	41 8d 7d 04          	lea    0x4(%r13),%edi
  2012a4:	81 ff 00 04 00 00    	cmp    $0x400,%edi
  2012aa:	0f 87 08 ff ff ff    	ja     2011b8 <sys_wstat+0x58>
  2012b0:	31 c0                	xor    %eax,%eax
  2012b2:	31 c9                	xor    %ecx,%ecx
  2012b4:	41 bc 02 00 00 00    	mov    $0x2,%r12d
  2012ba:	31 d2                	xor    %edx,%edx
  2012bc:	4c 8d b4 24 50 03 00 	lea    0x350(%rsp),%r14
  2012c3:	00 
  2012c4:	e9 30 ff ff ff       	jmp    2011f9 <sys_wstat+0x99>
  2012c9:	31 c0                	xor    %eax,%eax
  2012cb:	31 c9                	xor    %ecx,%ecx
  2012cd:	41 bc 02 00 00 00    	mov    $0x2,%r12d
  2012d3:	31 d2                	xor    %edx,%edx
  2012d5:	e9 1f ff ff ff       	jmp    2011f9 <sys_wstat+0x99>
  2012da:	48 85 ff             	test   %rdi,%rdi
  2012dd:	74 05                	je     2012e4 <sys_wstat+0x184>
  2012df:	e8 5c 03 00 00       	call   201640 <pebble_free>
  2012e4:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  2012e9:	eb 9c                	jmp    201287 <sys_wstat+0x127>
  2012eb:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)

00000000002012f0 <sys_mount>:
  2012f0:	f3 0f 1e fa          	endbr64
  2012f4:	41 89 f1             	mov    %esi,%r9d
  2012f7:	41 55                	push   %r13
  2012f9:	48 89 d6             	mov    %rdx,%rsi
  2012fc:	89 fa                	mov    %edi,%edx
  2012fe:	44 89 c8             	mov    %r9d,%eax
  201301:	41 54                	push   %r12
  201303:	c1 fa 10             	sar    $0x10,%edx
  201306:	55                   	push   %rbp
  201307:	c1 f8 18             	sar    $0x18,%eax
  20130a:	4c 89 c5             	mov    %r8,%rbp
  20130d:	45 89 c8             	mov    %r9d,%r8d
  201310:	41 c1 f8 10          	sar    $0x10,%r8d
  201314:	0f b6 c0             	movzbl %al,%eax
  201317:	53                   	push   %rbx
  201318:	89 cb                	mov    %ecx,%ebx
  20131a:	45 0f b6 c0          	movzbl %r8b,%r8d
  20131e:	48 c1 e0 08          	shl    $0x8,%rax
  201322:	89 f9                	mov    %edi,%ecx
  201324:	0f b6 d2             	movzbl %dl,%edx
  201327:	4c 09 c0             	or     %r8,%rax
  20132a:	c1 f9 18             	sar    $0x18,%ecx
  20132d:	49 89 c2             	mov    %rax,%r10
  201330:	44 89 c8             	mov    %r9d,%eax
  201333:	45 0f b6 c9          	movzbl %r9b,%r9d
  201337:	0f b6 c9             	movzbl %cl,%ecx
  20133a:	0f b6 c4             	movzbl %ah,%eax
  20133d:	48 81 ec 48 07 00 00 	sub    $0x748,%rsp
  201344:	49 89 c0             	mov    %rax,%r8
  201347:	4c 89 d0             	mov    %r10,%rax
  20134a:	48 c1 e0 08          	shl    $0x8,%rax
  20134e:	4c 09 c0             	or     %r8,%rax
  201351:	48 c1 e0 08          	shl    $0x8,%rax
  201355:	4c 09 c8             	or     %r9,%rax
  201358:	48 c1 e0 08          	shl    $0x8,%rax
  20135c:	48 09 c8             	or     %rcx,%rax
  20135f:	89 f9                	mov    %edi,%ecx
  201361:	40 0f b6 ff          	movzbl %dil,%edi
  201365:	48 c1 e0 08          	shl    $0x8,%rax
  201369:	48 09 d0             	or     %rdx,%rax
  20136c:	0f b6 d5             	movzbl %ch,%edx
  20136f:	48 c1 e0 08          	shl    $0x8,%rax
  201373:	48 09 d0             	or     %rdx,%rax
  201376:	48 c1 e0 08          	shl    $0x8,%rax
  20137a:	48 09 f8             	or     %rdi,%rax
  20137d:	80 3e 00             	cmpb   $0x0,(%rsi)
  201380:	48 89 84 24 40 03 00 	mov    %rax,0x340(%rsp)
  201387:	00 
  201388:	0f 84 f2 00 00 00    	je     201480 <sys_mount+0x190>
  20138e:	b8 01 00 00 00       	mov    $0x1,%eax
  201393:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  201398:	48 89 c2             	mov    %rax,%rdx
  20139b:	48 8d 40 01          	lea    0x1(%rax),%rax
  20139f:	80 3c 16 00          	cmpb   $0x0,(%rsi,%rdx,1)
  2013a3:	75 f3                	jne    201398 <sys_mount+0xa8>
  2013a5:	44 8d 62 02          	lea    0x2(%rdx),%r12d
  2013a9:	89 d1                	mov    %edx,%ecx
  2013ab:	0f b6 c6             	movzbl %dh,%eax
  2013ae:	4d 63 e4             	movslq %r12d,%r12
  2013b1:	48 8d bc 24 4a 03 00 	lea    0x34a(%rsp),%rdi
  2013b8:	00 
  2013b9:	88 8c 24 48 03 00 00 	mov    %cl,0x348(%rsp)
  2013c0:	4e 8d a4 24 48 03 00 	lea    0x348(%rsp,%r12,1),%r12
  2013c7:	00 
  2013c8:	88 84 24 49 03 00 00 	mov    %al,0x349(%rsp)
  2013cf:	e8 fc 02 00 00       	call   2016d0 <memmove>
  2013d4:	80 7d 00 00          	cmpb   $0x0,0x0(%rbp)
  2013d8:	41 89 1c 24          	mov    %ebx,(%r12)
  2013dc:	0f 84 b6 00 00 00    	je     201498 <sys_mount+0x1a8>
  2013e2:	b8 01 00 00 00       	mov    $0x1,%eax
  2013e7:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  2013ee:	00 00 
  2013f0:	48 89 c2             	mov    %rax,%rdx
  2013f3:	48 8d 40 01          	lea    0x1(%rax),%rax
  2013f7:	80 7c 15 00 00       	cmpb   $0x0,0x0(%rbp,%rdx,1)
  2013fc:	75 f2                	jne    2013f0 <sys_mount+0x100>
  2013fe:	4c 8d 6a 06          	lea    0x6(%rdx),%r13
  201402:	89 d1                	mov    %edx,%ecx
  201404:	0f b6 c6             	movzbl %dh,%eax
  201407:	41 88 4c 24 04       	mov    %cl,0x4(%r12)
  20140c:	48 89 e3             	mov    %rsp,%rbx
  20140f:	49 8d 7c 24 06       	lea    0x6(%r12),%rdi
  201414:	48 89 ee             	mov    %rbp,%rsi
  201417:	41 88 44 24 05       	mov    %al,0x5(%r12)
  20141c:	4d 01 ec             	add    %r13,%r12
  20141f:	e8 ac 02 00 00       	call   2016d0 <memmove>
  201424:	48 89 df             	mov    %rbx,%rdi
  201427:	ba 98 01 00 00       	mov    $0x198,%edx
  20142c:	31 f6                	xor    %esi,%esi
  20142e:	e8 ed 02 00 00       	call   201720 <memset>
  201433:	b8 01 00 00 00       	mov    $0x1,%eax
  201438:	48 89 df             	mov    %rbx,%rdi
  20143b:	c6 04 24 82          	movb   $0x82,(%rsp)
  20143f:	66 89 44 24 08       	mov    %ax,0x8(%rsp)
  201444:	48 8d 84 24 40 03 00 	lea    0x340(%rsp),%rax
  20144b:	00 
  20144c:	48 8d b4 24 a0 01 00 	lea    0x1a0(%rsp),%rsi
  201453:	00 
  201454:	49 29 c4             	sub    %rax,%r12
  201457:	48 89 44 24 18       	mov    %rax,0x18(%rsp)
  20145c:	44 89 64 24 20       	mov    %r12d,0x20(%rsp)
  201461:	48 c7 44 24 10 2e 00 	movq   $0x2e,0x10(%rsp)
  201468:	00 00 
  20146a:	e8 a1 ee ff ff       	call   200310 <lux_call>
  20146f:	48 81 c4 48 07 00 00 	add    $0x748,%rsp
  201476:	5b                   	pop    %rbx
  201477:	c1 f8 1f             	sar    $0x1f,%eax
  20147a:	5d                   	pop    %rbp
  20147b:	41 5c                	pop    %r12
  20147d:	41 5d                	pop    %r13
  20147f:	c3                   	ret
  201480:	31 c0                	xor    %eax,%eax
  201482:	31 c9                	xor    %ecx,%ecx
  201484:	41 bc 02 00 00 00    	mov    $0x2,%r12d
  20148a:	31 d2                	xor    %edx,%edx
  20148c:	e9 20 ff ff ff       	jmp    2013b1 <sys_mount+0xc1>
  201491:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  201498:	31 c0                	xor    %eax,%eax
  20149a:	31 c9                	xor    %ecx,%ecx
  20149c:	41 bd 06 00 00 00    	mov    $0x6,%r13d
  2014a2:	31 d2                	xor    %edx,%edx
  2014a4:	e9 5e ff ff ff       	jmp    201407 <sys_mount+0x117>
  2014a9:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)

00000000002014b0 <pebble_alloc>:
  2014b0:	f3 0f 1e fa          	endbr64
  2014b4:	41 57                	push   %r15
  2014b6:	ba 98 01 00 00       	mov    $0x198,%edx
  2014bb:	41 56                	push   %r14
  2014bd:	41 55                	push   %r13
  2014bf:	49 89 fd             	mov    %rdi,%r13
  2014c2:	41 54                	push   %r12
  2014c4:	4d 89 ef             	mov    %r13,%r15
  2014c7:	4d 89 ee             	mov    %r13,%r14
  2014ca:	55                   	push   %rbp
  2014cb:	49 c1 ef 30          	shr    $0x30,%r15
  2014cf:	49 c1 ee 28          	shr    $0x28,%r14
  2014d3:	53                   	push   %rbx
  2014d4:	48 89 f3             	mov    %rsi,%rbx
  2014d7:	31 f6                	xor    %esi,%esi
  2014d9:	45 0f b6 ff          	movzbl %r15b,%r15d
  2014dd:	45 0f b6 f6          	movzbl %r14b,%r14d
  2014e1:	48 81 ec 68 03 00 00 	sub    $0x368,%rsp
  2014e8:	48 8d 6c 24 20       	lea    0x20(%rsp),%rbp
  2014ed:	4c 8d a4 24 c0 01 00 	lea    0x1c0(%rsp),%r12
  2014f4:	00 
  2014f5:	48 89 ef             	mov    %rbp,%rdi
  2014f8:	e8 23 02 00 00       	call   201720 <memset>
  2014fd:	31 f6                	xor    %esi,%esi
  2014ff:	4c 89 e7             	mov    %r12,%rdi
  201502:	ba 98 01 00 00       	mov    $0x198,%edx
  201507:	e8 14 02 00 00       	call   201720 <memset>
  20150c:	4c 89 ea             	mov    %r13,%rdx
  20150f:	4d 89 eb             	mov    %r13,%r11
  201512:	b8 01 00 00 00       	mov    $0x1,%eax
  201517:	48 c1 ea 38          	shr    $0x38,%rdx
  20151b:	49 c1 eb 20          	shr    $0x20,%r11
  20151f:	4c 89 e9             	mov    %r13,%rcx
  201522:	66 89 44 24 28       	mov    %ax,0x28(%rsp)
  201527:	48 c1 e2 08          	shl    $0x8,%rdx
  20152b:	45 0f b6 db          	movzbl %r11b,%r11d
  20152f:	48 8d 44 24 10       	lea    0x10(%rsp),%rax
  201534:	49 89 da             	mov    %rbx,%r10
  201537:	4c 09 fa             	or     %r15,%rdx
  20153a:	48 c1 e9 18          	shr    $0x18,%rcx
  20153e:	48 89 44 24 38       	mov    %rax,0x38(%rsp)
  201543:	4c 89 e8             	mov    %r13,%rax
  201546:	48 c1 e2 08          	shl    $0x8,%rdx
  20154a:	0f b6 c9             	movzbl %cl,%ecx
  20154d:	48 c1 e8 10          	shr    $0x10,%rax
  201551:	49 89 d9             	mov    %rbx,%r9
  201554:	4c 09 f2             	or     %r14,%rdx
  201557:	0f b6 c0             	movzbl %al,%eax
  20155a:	49 c1 ea 30          	shr    $0x30,%r10
  20155e:	48 89 de             	mov    %rbx,%rsi
  201561:	48 c1 e2 08          	shl    $0x8,%rdx
  201565:	45 0f b6 d2          	movzbl %r10b,%r10d
  201569:	48 89 df             	mov    %rbx,%rdi
  20156c:	49 89 d8             	mov    %rbx,%r8
  20156f:	49 c1 e9 28          	shr    $0x28,%r9
  201573:	4c 09 da             	or     %r11,%rdx
  201576:	48 c1 ee 10          	shr    $0x10,%rsi
  20157a:	c6 44 24 20 82       	movb   $0x82,0x20(%rsp)
  20157f:	48 c1 e2 08          	shl    $0x8,%rdx
  201583:	48 c1 ef 18          	shr    $0x18,%rdi
  201587:	45 0f b6 c9          	movzbl %r9b,%r9d
  20158b:	40 0f b6 f6          	movzbl %sil,%esi
  20158f:	48 09 ca             	or     %rcx,%rdx
  201592:	49 c1 e8 20          	shr    $0x20,%r8
  201596:	40 0f b6 ff          	movzbl %dil,%edi
  20159a:	41 0f b6 cd          	movzbl %r13b,%ecx
  20159e:	48 c1 e2 08          	shl    $0x8,%rdx
  2015a2:	45 0f b6 c0          	movzbl %r8b,%r8d
  2015a6:	c7 44 24 30 3b 00 00 	movl   $0x3b,0x30(%rsp)
  2015ad:	00 
  2015ae:	48 09 c2             	or     %rax,%rdx
  2015b1:	4c 89 e8             	mov    %r13,%rax
  2015b4:	c7 44 24 40 10 00 00 	movl   $0x10,0x40(%rsp)
  2015bb:	00 
  2015bc:	0f b6 c4             	movzbl %ah,%eax
  2015bf:	48 c1 e2 08          	shl    $0x8,%rdx
  2015c3:	48 09 c2             	or     %rax,%rdx
  2015c6:	48 89 d8             	mov    %rbx,%rax
  2015c9:	48 c1 e8 38          	shr    $0x38,%rax
  2015cd:	48 c1 e2 08          	shl    $0x8,%rdx
  2015d1:	48 c1 e0 08          	shl    $0x8,%rax
  2015d5:	4c 09 d0             	or     %r10,%rax
  2015d8:	48 c1 e0 08          	shl    $0x8,%rax
  2015dc:	48 09 ca             	or     %rcx,%rdx
  2015df:	4c 09 c8             	or     %r9,%rax
  2015e2:	48 89 14 24          	mov    %rdx,(%rsp)
  2015e6:	48 c1 e0 08          	shl    $0x8,%rax
  2015ea:	4c 09 c0             	or     %r8,%rax
  2015ed:	48 c1 e0 08          	shl    $0x8,%rax
  2015f1:	48 09 f8             	or     %rdi,%rax
  2015f4:	48 89 ef             	mov    %rbp,%rdi
  2015f7:	48 c1 e0 08          	shl    $0x8,%rax
  2015fb:	48 09 f0             	or     %rsi,%rax
  2015fe:	0f b6 f7             	movzbl %bh,%esi
  201601:	0f b6 db             	movzbl %bl,%ebx
  201604:	48 c1 e0 08          	shl    $0x8,%rax
  201608:	48 09 f0             	or     %rsi,%rax
  20160b:	4c 89 e6             	mov    %r12,%rsi
  20160e:	48 c1 e0 08          	shl    $0x8,%rax
  201612:	48 09 c3             	or     %rax,%rbx
  201615:	48 89 5c 24 08       	mov    %rbx,0x8(%rsp)
  20161a:	66 0f 6f 04 24       	movdqa (%rsp),%xmm0
  20161f:	0f 29 44 24 10       	movaps %xmm0,0x10(%rsp)
  201624:	e8 e7 ec ff ff       	call   200310 <lux_call>
  201629:	48 81 c4 68 03 00 00 	add    $0x368,%rsp
  201630:	5b                   	pop    %rbx
  201631:	c1 f8 1f             	sar    $0x1f,%eax
  201634:	5d                   	pop    %rbp
  201635:	41 5c                	pop    %r12
  201637:	41 5d                	pop    %r13
  201639:	41 5e                	pop    %r14
  20163b:	41 5f                	pop    %r15
  20163d:	c3                   	ret
  20163e:	66 90                	xchg   %ax,%ax

0000000000201640 <pebble_free>:
  201640:	f3 0f 1e fa          	endbr64
  201644:	41 54                	push   %r12
  201646:	ba 98 01 00 00       	mov    $0x198,%edx
  20164b:	31 f6                	xor    %esi,%esi
  20164d:	55                   	push   %rbp
  20164e:	53                   	push   %rbx
  20164f:	48 89 fb             	mov    %rdi,%rbx
  201652:	48 81 ec 50 03 00 00 	sub    $0x350,%rsp
  201659:	48 8d 6c 24 10       	lea    0x10(%rsp),%rbp
  20165e:	4c 8d a4 24 b0 01 00 	lea    0x1b0(%rsp),%r12
  201665:	00 
  201666:	48 89 ef             	mov    %rbp,%rdi
  201669:	e8 b2 00 00 00       	call   201720 <memset>
  20166e:	4c 89 e7             	mov    %r12,%rdi
  201671:	ba 98 01 00 00       	mov    $0x198,%edx
  201676:	31 f6                	xor    %esi,%esi
  201678:	e8 a3 00 00 00       	call   201720 <memset>
  20167d:	b8 01 00 00 00       	mov    $0x1,%eax
  201682:	4c 89 e6             	mov    %r12,%rsi
  201685:	48 89 ef             	mov    %rbp,%rdi
  201688:	66 89 44 24 18       	mov    %ax,0x18(%rsp)
  20168d:	48 8d 44 24 08       	lea    0x8(%rsp),%rax
  201692:	48 89 5c 24 08       	mov    %rbx,0x8(%rsp)
  201697:	c6 44 24 10 82       	movb   $0x82,0x10(%rsp)
  20169c:	c7 44 24 20 3c 00 00 	movl   $0x3c,0x20(%rsp)
  2016a3:	00 
  2016a4:	48 89 44 24 28       	mov    %rax,0x28(%rsp)
  2016a9:	c7 44 24 30 08 00 00 	movl   $0x8,0x30(%rsp)
  2016b0:	00 
  2016b1:	e8 5a ec ff ff       	call   200310 <lux_call>
  2016b6:	48 81 c4 50 03 00 00 	add    $0x350,%rsp
  2016bd:	5b                   	pop    %rbx
  2016be:	c1 f8 1f             	sar    $0x1f,%eax
  2016c1:	5d                   	pop    %rbp
  2016c2:	41 5c                	pop    %r12
  2016c4:	c3                   	ret
  2016c5:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  2016cc:	00 00 00 
  2016cf:	90                   	nop

00000000002016d0 <memmove>:
  2016d0:	f3 0f 1e fa          	endbr64
  2016d4:	48 89 f8             	mov    %rdi,%rax
  2016d7:	48 39 f7             	cmp    %rsi,%rdi
  2016da:	73 24                	jae    201700 <memmove+0x30>
  2016dc:	31 c9                	xor    %ecx,%ecx
  2016de:	48 85 d2             	test   %rdx,%rdx
  2016e1:	74 3a                	je     20171d <memmove+0x4d>
  2016e3:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  2016e8:	0f b6 3c 0e          	movzbl (%rsi,%rcx,1),%edi
  2016ec:	40 88 3c 08          	mov    %dil,(%rax,%rcx,1)
  2016f0:	48 83 c1 01          	add    $0x1,%rcx
  2016f4:	48 39 d1             	cmp    %rdx,%rcx
  2016f7:	75 ef                	jne    2016e8 <memmove+0x18>
  2016f9:	c3                   	ret
  2016fa:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  201700:	48 85 d2             	test   %rdx,%rdx
  201703:	74 18                	je     20171d <memmove+0x4d>
  201705:	48 83 ea 01          	sub    $0x1,%rdx
  201709:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  201710:	0f b6 0c 16          	movzbl (%rsi,%rdx,1),%ecx
  201714:	88 0c 10             	mov    %cl,(%rax,%rdx,1)
  201717:	48 83 ea 01          	sub    $0x1,%rdx
  20171b:	73 f3                	jae    201710 <memmove+0x40>
  20171d:	c3                   	ret
  20171e:	66 90                	xchg   %ax,%ax

0000000000201720 <memset>:
  201720:	f3 0f 1e fa          	endbr64
  201724:	48 89 f8             	mov    %rdi,%rax
  201727:	41 89 f0             	mov    %esi,%r8d
  20172a:	48 8d 3c 17          	lea    (%rdi,%rdx,1),%rdi
  20172e:	48 89 c1             	mov    %rax,%rcx
  201731:	48 85 d2             	test   %rdx,%rdx
  201734:	74 2a                	je     201760 <memset+0x40>
  201736:	48 89 fa             	mov    %rdi,%rdx
  201739:	48 29 c2             	sub    %rax,%rdx
  20173c:	83 e2 01             	and    $0x1,%edx
  20173f:	74 0f                	je     201750 <memset+0x30>
  201741:	48 8d 48 01          	lea    0x1(%rax),%rcx
  201745:	40 88 71 ff          	mov    %sil,-0x1(%rcx)
  201749:	48 39 cf             	cmp    %rcx,%rdi
  20174c:	74 13                	je     201761 <memset+0x41>
  20174e:	66 90                	xchg   %ax,%ax
  201750:	44 88 01             	mov    %r8b,(%rcx)
  201753:	48 83 c1 02          	add    $0x2,%rcx
  201757:	44 88 41 ff          	mov    %r8b,-0x1(%rcx)
  20175b:	48 39 cf             	cmp    %rcx,%rdi
  20175e:	75 f0                	jne    201750 <memset+0x30>
  201760:	c3                   	ret
  201761:	c3                   	ret
  201762:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  201769:	00 00 00 00 
  20176d:	0f 1f 00             	nopl   (%rax)

0000000000201770 <strlen>:
  201770:	f3 0f 1e fa          	endbr64
  201774:	80 3f 00             	cmpb   $0x0,(%rdi)
  201777:	74 17                	je     201790 <strlen+0x20>
  201779:	48 89 f8             	mov    %rdi,%rax
  20177c:	0f 1f 40 00          	nopl   0x0(%rax)
  201780:	48 83 c0 01          	add    $0x1,%rax
  201784:	80 38 00             	cmpb   $0x0,(%rax)
  201787:	75 f7                	jne    201780 <strlen+0x10>
  201789:	48 29 f8             	sub    %rdi,%rax
  20178c:	c3                   	ret
  20178d:	0f 1f 00             	nopl   (%rax)
  201790:	31 c0                	xor    %eax,%eax
  201792:	c3                   	ret
  201793:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  20179a:	00 00 00 00 
  20179e:	66 90                	xchg   %ax,%ax

00000000002017a0 <malloc>:
  2017a0:	f3 0f 1e fa          	endbr64
  2017a4:	48 83 ec 08          	sub    $0x8,%rsp
  2017a8:	48 8d 3d 41 1f 00 00 	lea    0x1f41(%rip),%rdi        # 2036f0 <_syscall+0x19b>
  2017af:	e8 6c f1 ff ff       	call   200920 <sys_exit>
  2017b4:	31 c0                	xor    %eax,%eax
  2017b6:	48 83 c4 08          	add    $0x8,%rsp
  2017ba:	c3                   	ret
  2017bb:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)

00000000002017c0 <free>:
  2017c0:	f3 0f 1e fa          	endbr64
  2017c4:	48 8d 3d 55 1f 00 00 	lea    0x1f55(%rip),%rdi        # 203720 <_syscall+0x1cb>
  2017cb:	e9 50 f1 ff ff       	jmp    200920 <sys_exit>

00000000002017d0 <atoi>:
  2017d0:	f3 0f 1e fa          	endbr64
  2017d4:	0f b6 07             	movzbl (%rdi),%eax
  2017d7:	3c 2d                	cmp    $0x2d,%al
  2017d9:	74 45                	je     201820 <atoi+0x50>
  2017db:	8d 50 d0             	lea    -0x30(%rax),%edx
  2017de:	31 f6                	xor    %esi,%esi
  2017e0:	80 fa 09             	cmp    $0x9,%dl
  2017e3:	77 5b                	ja     201840 <atoi+0x70>
  2017e5:	31 d2                	xor    %edx,%edx
  2017e7:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  2017ee:	00 00 
  2017f0:	83 e8 30             	sub    $0x30,%eax
  2017f3:	8d 14 92             	lea    (%rdx,%rdx,4),%edx
  2017f6:	48 83 c7 01          	add    $0x1,%rdi
  2017fa:	0f be c0             	movsbl %al,%eax
  2017fd:	8d 14 50             	lea    (%rax,%rdx,2),%edx
  201800:	0f b6 07             	movzbl (%rdi),%eax
  201803:	8d 48 d0             	lea    -0x30(%rax),%ecx
  201806:	80 f9 09             	cmp    $0x9,%cl
  201809:	76 e5                	jbe    2017f0 <atoi+0x20>
  20180b:	89 d0                	mov    %edx,%eax
  20180d:	f7 d8                	neg    %eax
  20180f:	85 f6                	test   %esi,%esi
  201811:	0f 45 d0             	cmovne %eax,%edx
  201814:	89 d0                	mov    %edx,%eax
  201816:	c3                   	ret
  201817:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  20181e:	00 00 
  201820:	0f b6 47 01          	movzbl 0x1(%rdi),%eax
  201824:	48 8d 4f 01          	lea    0x1(%rdi),%rcx
  201828:	8d 50 d0             	lea    -0x30(%rax),%edx
  20182b:	80 fa 09             	cmp    $0x9,%dl
  20182e:	77 10                	ja     201840 <atoi+0x70>
  201830:	48 89 cf             	mov    %rcx,%rdi
  201833:	be 01 00 00 00       	mov    $0x1,%esi
  201838:	eb ab                	jmp    2017e5 <atoi+0x15>
  20183a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  201840:	31 d2                	xor    %edx,%edx
  201842:	89 d0                	mov    %edx,%eax
  201844:	c3                   	ret
  201845:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  20184c:	00 00 00 00 

0000000000201850 <strtoull>:
  201850:	f3 0f 1e fa          	endbr64
  201854:	48 89 f9             	mov    %rdi,%rcx
  201857:	49 89 f1             	mov    %rsi,%r9
  20185a:	89 d7                	mov    %edx,%edi
  20185c:	0f b6 01             	movzbl (%rcx),%eax
  20185f:	85 d2                	test   %edx,%edx
  201861:	75 09                	jne    20186c <strtoull+0x1c>
  201863:	bf 0a 00 00 00       	mov    $0xa,%edi
  201868:	3c 30                	cmp    $0x30,%al
  20186a:	74 64                	je     2018d0 <strtoull+0x80>
  20186c:	31 c0                	xor    %eax,%eax
  20186e:	4c 63 c7             	movslq %edi,%r8
  201871:	eb 1a                	jmp    20188d <strtoull+0x3d>
  201873:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  201878:	83 ea 30             	sub    $0x30,%edx
  20187b:	39 fa                	cmp    %edi,%edx
  20187d:	7d 2a                	jge    2018a9 <strtoull+0x59>
  20187f:	49 0f af c0          	imul   %r8,%rax
  201883:	48 63 d2             	movslq %edx,%rdx
  201886:	48 83 c1 01          	add    $0x1,%rcx
  20188a:	48 01 d0             	add    %rdx,%rax
  20188d:	0f be 11             	movsbl (%rcx),%edx
  201890:	8d 72 d0             	lea    -0x30(%rdx),%esi
  201893:	40 80 fe 09          	cmp    $0x9,%sil
  201897:	76 df                	jbe    201878 <strtoull+0x28>
  201899:	8d 72 9f             	lea    -0x61(%rdx),%esi
  20189c:	40 80 fe 05          	cmp    $0x5,%sil
  2018a0:	77 16                	ja     2018b8 <strtoull+0x68>
  2018a2:	83 ea 57             	sub    $0x57,%edx
  2018a5:	39 fa                	cmp    %edi,%edx
  2018a7:	7c d6                	jl     20187f <strtoull+0x2f>
  2018a9:	4d 85 c9             	test   %r9,%r9
  2018ac:	74 03                	je     2018b1 <strtoull+0x61>
  2018ae:	49 89 09             	mov    %rcx,(%r9)
  2018b1:	c3                   	ret
  2018b2:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  2018b8:	8d 72 bf             	lea    -0x41(%rdx),%esi
  2018bb:	40 80 fe 05          	cmp    $0x5,%sil
  2018bf:	77 e8                	ja     2018a9 <strtoull+0x59>
  2018c1:	83 ea 37             	sub    $0x37,%edx
  2018c4:	eb b5                	jmp    20187b <strtoull+0x2b>
  2018c6:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  2018cd:	00 00 00 
  2018d0:	0f b6 41 01          	movzbl 0x1(%rcx),%eax
  2018d4:	83 e0 df             	and    $0xffffffdf,%eax
  2018d7:	3c 58                	cmp    $0x58,%al
  2018d9:	75 91                	jne    20186c <strtoull+0x1c>
  2018db:	48 83 c1 02          	add    $0x2,%rcx
  2018df:	bf 10 00 00 00       	mov    $0x10,%edi
  2018e4:	eb 86                	jmp    20186c <strtoull+0x1c>
  2018e6:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  2018ed:	00 00 00 

00000000002018f0 <gqid>:
  2018f0:	48 8d 47 0d          	lea    0xd(%rdi),%rax
  2018f4:	48 39 f0             	cmp    %rsi,%rax
  2018f7:	77 17                	ja     201910 <gqid+0x20>
  2018f9:	0f b6 0f             	movzbl (%rdi),%ecx
  2018fc:	88 4a 10             	mov    %cl,0x10(%rdx)
  2018ff:	48 63 4f 01          	movslq 0x1(%rdi),%rcx
  201903:	48 89 4a 08          	mov    %rcx,0x8(%rdx)
  201907:	48 8b 4f 05          	mov    0x5(%rdi),%rcx
  20190b:	48 89 0a             	mov    %rcx,(%rdx)
  20190e:	c3                   	ret
  20190f:	90                   	nop
  201910:	31 c0                	xor    %eax,%eax
  201912:	c3                   	ret
  201913:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  20191a:	00 00 00 00 
  20191e:	66 90                	xchg   %ax,%ax

0000000000201920 <convM2S>:
  201920:	f3 0f 1e fa          	endbr64
  201924:	41 57                	push   %r15
  201926:	89 f6                	mov    %esi,%esi
  201928:	41 56                	push   %r14
  20192a:	41 55                	push   %r13
  20192c:	4c 8d 6f 07          	lea    0x7(%rdi),%r13
  201930:	41 54                	push   %r12
  201932:	55                   	push   %rbp
  201933:	48 8d 2c 37          	lea    (%rdi,%rsi,1),%rbp
  201937:	53                   	push   %rbx
  201938:	48 89 fb             	mov    %rdi,%rbx
  20193b:	48 83 ec 18          	sub    $0x18,%rsp
  20193f:	4c 39 ed             	cmp    %r13,%rbp
  201942:	72 7c                	jb     2019c0 <convM2S+0xa0>
  201944:	44 8b 37             	mov    (%rdi),%r14d
  201947:	41 83 fe 06          	cmp    $0x6,%r14d
  20194b:	0f 86 a7 00 00 00    	jbe    2019f8 <convM2S+0xd8>
  201951:	0f b6 47 04          	movzbl 0x4(%rdi),%eax
  201955:	49 89 d4             	mov    %rdx,%r12
  201958:	88 02                	mov    %al,(%rdx)
  20195a:	0f b7 57 05          	movzwl 0x5(%rdi),%edx
  20195e:	83 e8 64             	sub    $0x64,%eax
  201961:	66 41 89 54 24 08    	mov    %dx,0x8(%r12)
  201967:	3c 69                	cmp    $0x69,%al
  201969:	77 38                	ja     2019a3 <convM2S+0x83>
  20196b:	48 8d 15 3a 1e 00 00 	lea    0x1e3a(%rip),%rdx        # 2037ac <_syscall+0x257>
  201972:	0f b6 c0             	movzbl %al,%eax
  201975:	48 63 04 82          	movslq (%rdx,%rax,4),%rax
  201979:	48 01 d0             	add    %rdx,%rax
  20197c:	3e ff e0             	notrack jmp *%rax
  20197f:	90                   	nop
  201980:	4c 8d 6f 0b          	lea    0xb(%rdi),%r13
  201984:	4c 39 ed             	cmp    %r13,%rbp
  201987:	72 1a                	jb     2019a3 <convM2S+0x83>
  201989:	8b 47 07             	mov    0x7(%rdi),%eax
  20198c:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  201991:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  201998:	44 89 f0             	mov    %r14d,%eax
  20199b:	48 01 c3             	add    %rax,%rbx
  20199e:	49 39 dd             	cmp    %rbx,%r13
  2019a1:	74 03                	je     2019a6 <convM2S+0x86>
  2019a3:	45 31 f6             	xor    %r14d,%r14d
  2019a6:	48 83 c4 18          	add    $0x18,%rsp
  2019aa:	44 89 f0             	mov    %r14d,%eax
  2019ad:	5b                   	pop    %rbx
  2019ae:	5d                   	pop    %rbp
  2019af:	41 5c                	pop    %r12
  2019b1:	41 5d                	pop    %r13
  2019b3:	41 5e                	pop    %r14
  2019b5:	41 5f                	pop    %r15
  2019b7:	c3                   	ret
  2019b8:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
  2019bf:	00 
  2019c0:	48 89 fe             	mov    %rdi,%rsi
  2019c3:	48 89 ea             	mov    %rbp,%rdx
  2019c6:	48 8d 3d 83 1d 00 00 	lea    0x1d83(%rip),%rdi        # 203750 <_syscall+0x1fb>
  2019cd:	31 c0                	xor    %eax,%eax
  2019cf:	e8 fc e7 ff ff       	call   2001d0 <print>
  2019d4:	eb cd                	jmp    2019a3 <convM2S+0x83>
  2019d6:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  2019dd:	00 00 00 
  2019e0:	4c 8d 6f 0b          	lea    0xb(%rdi),%r13
  2019e4:	4c 39 ed             	cmp    %r13,%rbp
  2019e7:	72 ba                	jb     2019a3 <convM2S+0x83>
  2019e9:	8b 47 07             	mov    0x7(%rdi),%eax
  2019ec:	41 89 44 24 18       	mov    %eax,0x18(%r12)
  2019f1:	eb a5                	jmp    201998 <convM2S+0x78>
  2019f3:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  2019f8:	ba 07 00 00 00       	mov    $0x7,%edx
  2019fd:	44 89 f6             	mov    %r14d,%esi
  201a00:	48 8d 3d 79 1d 00 00 	lea    0x1d79(%rip),%rdi        # 203780 <_syscall+0x22b>
  201a07:	31 c0                	xor    %eax,%eax
  201a09:	e8 c2 e7 ff ff       	call   2001d0 <print>
  201a0e:	eb 93                	jmp    2019a3 <convM2S+0x83>
  201a10:	48 8d 77 09          	lea    0x9(%rdi),%rsi
  201a14:	48 39 f5             	cmp    %rsi,%rbp
  201a17:	72 8a                	jb     2019a3 <convM2S+0x83>
  201a19:	44 0f b7 7f 07       	movzwl 0x7(%rdi),%r15d
  201a1e:	48 8d 4f 08          	lea    0x8(%rdi),%rcx
  201a22:	4e 8d 6c 3f 09       	lea    0x9(%rdi,%r15,1),%r13
  201a27:	4c 39 ed             	cmp    %r13,%rbp
  201a2a:	0f 82 73 ff ff ff    	jb     2019a3 <convM2S+0x83>
  201a30:	48 89 cf             	mov    %rcx,%rdi
  201a33:	4c 89 fa             	mov    %r15,%rdx
  201a36:	48 89 0c 24          	mov    %rcx,(%rsp)
  201a3a:	e8 91 fc ff ff       	call   2016d0 <memmove>
  201a3f:	42 c6 44 3b 08 00    	movb   $0x0,0x8(%rbx,%r15,1)
  201a45:	48 8b 0c 24          	mov    (%rsp),%rcx
  201a49:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  201a4e:	e9 45 ff ff ff       	jmp    201998 <convM2S+0x78>
  201a53:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  201a58:	4c 8d 6f 0f          	lea    0xf(%rdi),%r13
  201a5c:	4c 39 ed             	cmp    %r13,%rbp
  201a5f:	0f 82 3e ff ff ff    	jb     2019a3 <convM2S+0x83>
  201a65:	48 8b 47 07          	mov    0x7(%rdi),%rax
  201a69:	49 89 44 24 10       	mov    %rax,0x10(%r12)
  201a6e:	e9 25 ff ff ff       	jmp    201998 <convM2S+0x78>
  201a73:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  201a78:	48 8d 47 0b          	lea    0xb(%rdi),%rax
  201a7c:	48 39 c5             	cmp    %rax,%rbp
  201a7f:	0f 82 1e ff ff ff    	jb     2019a3 <convM2S+0x83>
  201a85:	8b 57 07             	mov    0x7(%rdi),%edx
  201a88:	4c 8d 2c 10          	lea    (%rax,%rdx,1),%r13
  201a8c:	41 89 54 24 18       	mov    %edx,0x18(%r12)
  201a91:	4c 39 ed             	cmp    %r13,%rbp
  201a94:	0f 82 09 ff ff ff    	jb     2019a3 <convM2S+0x83>
  201a9a:	49 89 44 24 20       	mov    %rax,0x20(%r12)
  201a9f:	e9 f4 fe ff ff       	jmp    201998 <convM2S+0x78>
  201aa4:	0f 1f 40 00          	nopl   0x0(%rax)
  201aa8:	48 8d 47 09          	lea    0x9(%rdi),%rax
  201aac:	48 39 c5             	cmp    %rax,%rbp
  201aaf:	0f 82 ee fe ff ff    	jb     2019a3 <convM2S+0x83>
  201ab5:	0f b7 57 07          	movzwl 0x7(%rdi),%edx
  201ab9:	4c 8d 2c 10          	lea    (%rax,%rdx,1),%r13
  201abd:	66 41 89 54 24 10    	mov    %dx,0x10(%r12)
  201ac3:	4c 39 ed             	cmp    %r13,%rbp
  201ac6:	0f 82 d7 fe ff ff    	jb     2019a3 <convM2S+0x83>
  201acc:	49 89 44 24 18       	mov    %rax,0x18(%r12)
  201ad1:	e9 c2 fe ff ff       	jmp    201998 <convM2S+0x78>
  201ad6:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  201add:	00 00 00 
  201ae0:	48 8d 47 17          	lea    0x17(%rdi),%rax
  201ae4:	48 39 c5             	cmp    %rax,%rbp
  201ae7:	0f 82 b6 fe ff ff    	jb     2019a3 <convM2S+0x83>
  201aed:	8b 57 07             	mov    0x7(%rdi),%edx
  201af0:	41 89 54 24 04       	mov    %edx,0x4(%r12)
  201af5:	48 8b 57 0b          	mov    0xb(%rdi),%rdx
  201af9:	49 89 54 24 10       	mov    %rdx,0x10(%r12)
  201afe:	8b 57 13             	mov    0x13(%rdi),%edx
  201b01:	4c 8d 2c 10          	lea    (%rax,%rdx,1),%r13
  201b05:	41 89 54 24 18       	mov    %edx,0x18(%r12)
  201b0a:	4c 39 ed             	cmp    %r13,%rbp
  201b0d:	73 8b                	jae    201a9a <convM2S+0x17a>
  201b0f:	e9 8f fe ff ff       	jmp    2019a3 <convM2S+0x83>
  201b14:	0f 1f 40 00          	nopl   0x0(%rax)
  201b18:	4c 8d 6f 17          	lea    0x17(%rdi),%r13
  201b1c:	4c 39 ed             	cmp    %r13,%rbp
  201b1f:	0f 82 7e fe ff ff    	jb     2019a3 <convM2S+0x83>
  201b25:	8b 47 07             	mov    0x7(%rdi),%eax
  201b28:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  201b2d:	48 8b 47 0b          	mov    0xb(%rdi),%rax
  201b31:	49 89 44 24 10       	mov    %rax,0x10(%r12)
  201b36:	8b 47 13             	mov    0x13(%rdi),%eax
  201b39:	41 89 44 24 18       	mov    %eax,0x18(%r12)
  201b3e:	e9 55 fe ff ff       	jmp    201998 <convM2S+0x78>
  201b43:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  201b48:	48 8d 47 0b          	lea    0xb(%rdi),%rax
  201b4c:	48 39 c5             	cmp    %rax,%rbp
  201b4f:	0f 82 4e fe ff ff    	jb     2019a3 <convM2S+0x83>
  201b55:	8b 47 07             	mov    0x7(%rdi),%eax
  201b58:	48 8d 77 0d          	lea    0xd(%rdi),%rsi
  201b5c:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  201b61:	48 39 f5             	cmp    %rsi,%rbp
  201b64:	0f 82 39 fe ff ff    	jb     2019a3 <convM2S+0x83>
  201b6a:	44 0f b7 6f 0b       	movzwl 0xb(%rdi),%r13d
  201b6f:	48 8d 4f 0c          	lea    0xc(%rdi),%rcx
  201b73:	4e 8d 7c 2f 0d       	lea    0xd(%rdi,%r13,1),%r15
  201b78:	4c 39 fd             	cmp    %r15,%rbp
  201b7b:	0f 82 22 fe ff ff    	jb     2019a3 <convM2S+0x83>
  201b81:	4c 89 ea             	mov    %r13,%rdx
  201b84:	48 89 cf             	mov    %rcx,%rdi
  201b87:	48 89 0c 24          	mov    %rcx,(%rsp)
  201b8b:	e8 40 fb ff ff       	call   2016d0 <memmove>
  201b90:	42 c6 44 2b 0c 00    	movb   $0x0,0xc(%rbx,%r13,1)
  201b96:	48 8b 0c 24          	mov    (%rsp),%rcx
  201b9a:	4d 8d 6f 05          	lea    0x5(%r15),%r13
  201b9e:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  201ba3:	4c 39 ed             	cmp    %r13,%rbp
  201ba6:	0f 82 f7 fd ff ff    	jb     2019a3 <convM2S+0x83>
  201bac:	41 8b 07             	mov    (%r15),%eax
  201baf:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  201bb4:	41 0f b6 47 04       	movzbl 0x4(%r15),%eax
  201bb9:	41 88 44 24 20       	mov    %al,0x20(%r12)
  201bbe:	e9 d5 fd ff ff       	jmp    201998 <convM2S+0x78>
  201bc3:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  201bc8:	48 8d 47 0b          	lea    0xb(%rdi),%rax
  201bcc:	48 39 c5             	cmp    %rax,%rbp
  201bcf:	0f 82 ce fd ff ff    	jb     2019a3 <convM2S+0x83>
  201bd5:	8b 47 07             	mov    0x7(%rdi),%eax
  201bd8:	48 8d 77 0d          	lea    0xd(%rdi),%rsi
  201bdc:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  201be1:	48 39 f5             	cmp    %rsi,%rbp
  201be4:	0f 82 b9 fd ff ff    	jb     2019a3 <convM2S+0x83>
  201bea:	44 0f b7 7f 0b       	movzwl 0xb(%rdi),%r15d
  201bef:	48 8d 4f 0c          	lea    0xc(%rdi),%rcx
  201bf3:	4e 8d 6c 3f 0d       	lea    0xd(%rdi,%r15,1),%r13
  201bf8:	4c 39 ed             	cmp    %r13,%rbp
  201bfb:	0f 82 a2 fd ff ff    	jb     2019a3 <convM2S+0x83>
  201c01:	48 89 cf             	mov    %rcx,%rdi
  201c04:	4c 89 fa             	mov    %r15,%rdx
  201c07:	48 89 0c 24          	mov    %rcx,(%rsp)
  201c0b:	e8 c0 fa ff ff       	call   2016d0 <memmove>
  201c10:	42 c6 44 3b 0c 00    	movb   $0x0,0xc(%rbx,%r15,1)
  201c16:	48 8b 0c 24          	mov    (%rsp),%rcx
  201c1a:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  201c1f:	e9 74 fd ff ff       	jmp    201998 <convM2S+0x78>
  201c24:	0f 1f 40 00          	nopl   0x0(%rax)
  201c28:	49 8d 54 24 10       	lea    0x10(%r12),%rdx
  201c2d:	4c 89 ef             	mov    %r13,%rdi
  201c30:	48 89 ee             	mov    %rbp,%rsi
  201c33:	e8 b8 fc ff ff       	call   2018f0 <gqid>
  201c38:	49 89 c5             	mov    %rax,%r13
  201c3b:	48 85 c0             	test   %rax,%rax
  201c3e:	0f 94 c0             	sete   %al
  201c41:	4c 39 ed             	cmp    %r13,%rbp
  201c44:	0f 92 c2             	setb   %dl
  201c47:	09 d0                	or     %edx,%eax
  201c49:	84 c0                	test   %al,%al
  201c4b:	0f 85 52 fd ff ff    	jne    2019a3 <convM2S+0x83>
  201c51:	e9 42 fd ff ff       	jmp    201998 <convM2S+0x78>
  201c56:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  201c5d:	00 00 00 
  201c60:	48 8d 77 09          	lea    0x9(%rdi),%rsi
  201c64:	48 39 f5             	cmp    %rsi,%rbp
  201c67:	0f 82 36 fd ff ff    	jb     2019a3 <convM2S+0x83>
  201c6d:	44 0f b7 7f 07       	movzwl 0x7(%rdi),%r15d
  201c72:	48 8d 4f 08          	lea    0x8(%rdi),%rcx
  201c76:	4e 8d 6c 3f 09       	lea    0x9(%rdi,%r15,1),%r13
  201c7b:	4c 39 ed             	cmp    %r13,%rbp
  201c7e:	0f 82 1f fd ff ff    	jb     2019a3 <convM2S+0x83>
  201c84:	48 89 cf             	mov    %rcx,%rdi
  201c87:	4c 89 fa             	mov    %r15,%rdx
  201c8a:	48 89 0c 24          	mov    %rcx,(%rsp)
  201c8e:	e8 3d fa ff ff       	call   2016d0 <memmove>
  201c93:	42 c6 44 3b 08 00    	movb   $0x0,0x8(%rbx,%r15,1)
  201c99:	48 8b 0c 24          	mov    (%rsp),%rcx
  201c9d:	49 89 4c 24 10       	mov    %rcx,0x10(%r12)
  201ca2:	e9 f1 fc ff ff       	jmp    201998 <convM2S+0x78>
  201ca7:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  201cae:	00 00 
  201cb0:	48 8d 47 0d          	lea    0xd(%rdi),%rax
  201cb4:	48 39 c5             	cmp    %rax,%rbp
  201cb7:	0f 82 e6 fc ff ff    	jb     2019a3 <convM2S+0x83>
  201cbd:	8b 57 07             	mov    0x7(%rdi),%edx
  201cc0:	41 89 54 24 04       	mov    %edx,0x4(%r12)
  201cc5:	0f b7 57 0b          	movzwl 0xb(%rdi),%edx
  201cc9:	4c 8d 2c 10          	lea    (%rax,%rdx,1),%r13
  201ccd:	66 41 89 54 24 10    	mov    %dx,0x10(%r12)
  201cd3:	4c 39 ed             	cmp    %r13,%rbp
  201cd6:	0f 83 f0 fd ff ff    	jae    201acc <convM2S+0x1ac>
  201cdc:	e9 c2 fc ff ff       	jmp    2019a3 <convM2S+0x83>
  201ce1:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  201ce8:	48 8d 7f 0b          	lea    0xb(%rdi),%rdi
  201cec:	48 39 fd             	cmp    %rdi,%rbp
  201cef:	0f 82 ae fc ff ff    	jb     2019a3 <convM2S+0x83>
  201cf5:	8b 43 07             	mov    0x7(%rbx),%eax
  201cf8:	49 8d 54 24 10       	lea    0x10(%r12),%rdx
  201cfd:	48 89 ee             	mov    %rbp,%rsi
  201d00:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  201d05:	e8 e6 fb ff ff       	call   2018f0 <gqid>
  201d0a:	48 85 c0             	test   %rax,%rax
  201d0d:	0f 84 90 fc ff ff    	je     2019a3 <convM2S+0x83>
  201d13:	4c 8d 68 04          	lea    0x4(%rax),%r13
  201d17:	4c 39 ed             	cmp    %r13,%rbp
  201d1a:	0f 82 83 fc ff ff    	jb     2019a3 <convM2S+0x83>
  201d20:	8b 00                	mov    (%rax),%eax
  201d22:	41 89 44 24 28       	mov    %eax,0x28(%r12)
  201d27:	e9 6c fc ff ff       	jmp    201998 <convM2S+0x78>
  201d2c:	0f 1f 40 00          	nopl   0x0(%rax)
  201d30:	49 8d 54 24 10       	lea    0x10(%r12),%rdx
  201d35:	48 89 ee             	mov    %rbp,%rsi
  201d38:	4c 89 ef             	mov    %r13,%rdi
  201d3b:	e8 b0 fb ff ff       	call   2018f0 <gqid>
  201d40:	48 85 c0             	test   %rax,%rax
  201d43:	75 ce                	jne    201d13 <convM2S+0x3f3>
  201d45:	e9 59 fc ff ff       	jmp    2019a3 <convM2S+0x83>
  201d4a:	4c 8d 6f 09          	lea    0x9(%rdi),%r13
  201d4e:	4c 39 ed             	cmp    %r13,%rbp
  201d51:	0f 82 4c fc ff ff    	jb     2019a3 <convM2S+0x83>
  201d57:	0f b7 47 07          	movzwl 0x7(%rdi),%eax
  201d5b:	66 41 89 44 24 10    	mov    %ax,0x10(%r12)
  201d61:	e9 32 fc ff ff       	jmp    201998 <convM2S+0x78>
  201d66:	48 8d 47 0f          	lea    0xf(%rdi),%rax
  201d6a:	48 39 c5             	cmp    %rax,%rbp
  201d6d:	0f 82 30 fc ff ff    	jb     2019a3 <convM2S+0x83>
  201d73:	8b 47 07             	mov    0x7(%rdi),%eax
  201d76:	48 8d 77 11          	lea    0x11(%rdi),%rsi
  201d7a:	41 89 44 24 18       	mov    %eax,0x18(%r12)
  201d7f:	8b 47 0b             	mov    0xb(%rdi),%eax
  201d82:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  201d87:	48 39 f5             	cmp    %rsi,%rbp
  201d8a:	0f 82 13 fc ff ff    	jb     2019a3 <convM2S+0x83>
  201d90:	44 0f b7 6f 0f       	movzwl 0xf(%rdi),%r13d
  201d95:	48 8d 4f 10          	lea    0x10(%rdi),%rcx
  201d99:	4e 8d 7c 2f 11       	lea    0x11(%rdi,%r13,1),%r15
  201d9e:	4c 39 fd             	cmp    %r15,%rbp
  201da1:	0f 82 fc fb ff ff    	jb     2019a3 <convM2S+0x83>
  201da7:	48 89 cf             	mov    %rcx,%rdi
  201daa:	4c 89 ea             	mov    %r13,%rdx
  201dad:	48 89 0c 24          	mov    %rcx,(%rsp)
  201db1:	e8 1a f9 ff ff       	call   2016d0 <memmove>
  201db6:	48 8b 0c 24          	mov    (%rsp),%rcx
  201dba:	49 8d 47 04          	lea    0x4(%r15),%rax
  201dbe:	42 c6 44 2b 10 00    	movb   $0x0,0x10(%rbx,%r13,1)
  201dc4:	49 89 4c 24 10       	mov    %rcx,0x10(%r12)
  201dc9:	48 39 c5             	cmp    %rax,%rbp
  201dcc:	0f 82 d1 fb ff ff    	jb     2019a3 <convM2S+0x83>
  201dd2:	41 8b 07             	mov    (%r15),%eax
  201dd5:	49 8d 77 06          	lea    0x6(%r15),%rsi
  201dd9:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  201dde:	48 39 f5             	cmp    %rsi,%rbp
  201de1:	0f 82 bc fb ff ff    	jb     2019a3 <convM2S+0x83>
  201de7:	41 0f b7 57 04       	movzwl 0x4(%r15),%edx
  201dec:	49 8d 4f 05          	lea    0x5(%r15),%rcx
  201df0:	4d 8d 6c 17 06       	lea    0x6(%r15,%rdx,1),%r13
  201df5:	4c 39 ed             	cmp    %r13,%rbp
  201df8:	0f 82 a5 fb ff ff    	jb     2019a3 <convM2S+0x83>
  201dfe:	48 89 cf             	mov    %rcx,%rdi
  201e01:	48 89 54 24 08       	mov    %rdx,0x8(%rsp)
  201e06:	48 89 0c 24          	mov    %rcx,(%rsp)
  201e0a:	e8 c1 f8 ff ff       	call   2016d0 <memmove>
  201e0f:	48 8b 54 24 08       	mov    0x8(%rsp),%rdx
  201e14:	48 8b 0c 24          	mov    (%rsp),%rcx
  201e18:	41 c6 44 17 05 00    	movb   $0x0,0x5(%r15,%rdx,1)
  201e1e:	49 89 4c 24 20       	mov    %rcx,0x20(%r12)
  201e23:	e9 70 fb ff ff       	jmp    201998 <convM2S+0x78>
  201e28:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
  201e2f:	00 
  201e30:	48 8d 77 09          	lea    0x9(%rdi),%rsi
  201e34:	48 39 f5             	cmp    %rsi,%rbp
  201e37:	0f 82 66 fb ff ff    	jb     2019a3 <convM2S+0x83>
  201e3d:	44 0f b7 6f 07       	movzwl 0x7(%rdi),%r13d
  201e42:	48 8d 4f 08          	lea    0x8(%rdi),%rcx
  201e46:	4e 8d 7c 2f 09       	lea    0x9(%rdi,%r13,1),%r15
  201e4b:	4c 39 fd             	cmp    %r15,%rbp
  201e4e:	0f 82 4f fb ff ff    	jb     2019a3 <convM2S+0x83>
  201e54:	4c 89 ea             	mov    %r13,%rdx
  201e57:	48 89 cf             	mov    %rcx,%rdi
  201e5a:	48 89 0c 24          	mov    %rcx,(%rsp)
  201e5e:	e8 6d f8 ff ff       	call   2016d0 <memmove>
  201e63:	48 8b 0c 24          	mov    (%rsp),%rcx
  201e67:	42 c6 44 2b 08 00    	movb   $0x0,0x8(%rbx,%r13,1)
  201e6d:	4d 8d 6f 04          	lea    0x4(%r15),%r13
  201e71:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  201e76:	4c 39 ed             	cmp    %r13,%rbp
  201e79:	0f 82 24 fb ff ff    	jb     2019a3 <convM2S+0x83>
  201e7f:	41 8b 07             	mov    (%r15),%eax
  201e82:	41 89 44 24 18       	mov    %eax,0x18(%r12)
  201e87:	e9 0c fb ff ff       	jmp    201998 <convM2S+0x78>
  201e8c:	48 8d 47 0b          	lea    0xb(%rdi),%rax
  201e90:	48 39 c5             	cmp    %rax,%rbp
  201e93:	0f 82 0a fb ff ff    	jb     2019a3 <convM2S+0x83>
  201e99:	8b 47 07             	mov    0x7(%rdi),%eax
  201e9c:	48 8d 77 0d          	lea    0xd(%rdi),%rsi
  201ea0:	41 89 44 24 14       	mov    %eax,0x14(%r12)
  201ea5:	48 39 f5             	cmp    %rsi,%rbp
  201ea8:	0f 82 f5 fa ff ff    	jb     2019a3 <convM2S+0x83>
  201eae:	44 0f b7 7f 0b       	movzwl 0xb(%rdi),%r15d
  201eb3:	48 8d 4f 0c          	lea    0xc(%rdi),%rcx
  201eb7:	4e 8d 6c 3f 0d       	lea    0xd(%rdi,%r15,1),%r13
  201ebc:	4c 39 ed             	cmp    %r13,%rbp
  201ebf:	0f 82 de fa ff ff    	jb     2019a3 <convM2S+0x83>
  201ec5:	48 89 cf             	mov    %rcx,%rdi
  201ec8:	4c 89 fa             	mov    %r15,%rdx
  201ecb:	48 89 0c 24          	mov    %rcx,(%rsp)
  201ecf:	e8 fc f7 ff ff       	call   2016d0 <memmove>
  201ed4:	48 8b 0c 24          	mov    (%rsp),%rcx
  201ed8:	42 c6 44 3b 0c 00    	movb   $0x0,0xc(%rbx,%r15,1)
  201ede:	49 89 4c 24 10       	mov    %rcx,0x10(%r12)
  201ee3:	e9 b0 fa ff ff       	jmp    201998 <convM2S+0x78>
  201ee8:	48 8d 47 13          	lea    0x13(%rdi),%rax
  201eec:	48 39 c5             	cmp    %rax,%rbp
  201eef:	0f 82 ae fa ff ff    	jb     2019a3 <convM2S+0x83>
  201ef5:	48 8b 57 07          	mov    0x7(%rdi),%rdx
  201ef9:	49 89 54 24 28       	mov    %rdx,0x28(%r12)
  201efe:	8b 53 0f             	mov    0xf(%rbx),%edx
  201f01:	4c 8d 2c 10          	lea    (%rax,%rdx,1),%r13
  201f05:	41 89 54 24 20       	mov    %edx,0x20(%r12)
  201f0a:	4c 39 ed             	cmp    %r13,%rbp
  201f0d:	0f 83 b9 fb ff ff    	jae    201acc <convM2S+0x1ac>
  201f13:	e9 8b fa ff ff       	jmp    2019a3 <convM2S+0x83>
  201f18:	48 8d 47 13          	lea    0x13(%rdi),%rax
  201f1c:	48 39 c5             	cmp    %rax,%rbp
  201f1f:	0f 82 7e fa ff ff    	jb     2019a3 <convM2S+0x83>
  201f25:	8b 57 07             	mov    0x7(%rdi),%edx
  201f28:	41 89 54 24 10       	mov    %edx,0x10(%r12)
  201f2d:	8b 57 0b             	mov    0xb(%rdi),%edx
  201f30:	41 89 54 24 14       	mov    %edx,0x14(%r12)
  201f35:	eb c7                	jmp    201efe <convM2S+0x5de>
  201f37:	48 8d 47 0b          	lea    0xb(%rdi),%rax
  201f3b:	48 39 c5             	cmp    %rax,%rbp
  201f3e:	0f 82 5f fa ff ff    	jb     2019a3 <convM2S+0x83>
  201f44:	8b 47 07             	mov    0x7(%rdi),%eax
  201f47:	48 8d 77 0d          	lea    0xd(%rdi),%rsi
  201f4b:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  201f50:	48 39 f5             	cmp    %rsi,%rbp
  201f53:	0f 82 4a fa ff ff    	jb     2019a3 <convM2S+0x83>
  201f59:	44 0f b7 6f 0b       	movzwl 0xb(%rdi),%r13d
  201f5e:	48 8d 4f 0c          	lea    0xc(%rdi),%rcx
  201f62:	4e 8d 7c 2f 0d       	lea    0xd(%rdi,%r13,1),%r15
  201f67:	4c 39 fd             	cmp    %r15,%rbp
  201f6a:	0f 82 33 fa ff ff    	jb     2019a3 <convM2S+0x83>
  201f70:	4c 89 ea             	mov    %r13,%rdx
  201f73:	48 89 cf             	mov    %rcx,%rdi
  201f76:	48 89 0c 24          	mov    %rcx,(%rsp)
  201f7a:	e8 51 f7 ff ff       	call   2016d0 <memmove>
  201f7f:	48 8b 0c 24          	mov    (%rsp),%rcx
  201f83:	42 c6 44 2b 0c 00    	movb   $0x0,0xc(%rbx,%r13,1)
  201f89:	4d 8d 6f 01          	lea    0x1(%r15),%r13
  201f8d:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  201f92:	4c 39 ed             	cmp    %r13,%rbp
  201f95:	0f 82 08 fa ff ff    	jb     2019a3 <convM2S+0x83>
  201f9b:	41 0f b6 07          	movzbl (%r15),%eax
  201f9f:	41 88 44 24 20       	mov    %al,0x20(%r12)
  201fa4:	e9 ef f9 ff ff       	jmp    201998 <convM2S+0x78>
  201fa9:	4c 8d 6f 0c          	lea    0xc(%rdi),%r13
  201fad:	4c 39 ed             	cmp    %r13,%rbp
  201fb0:	0f 82 ed f9 ff ff    	jb     2019a3 <convM2S+0x83>
  201fb6:	8b 47 07             	mov    0x7(%rdi),%eax
  201fb9:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  201fbe:	0f b6 47 0b          	movzbl 0xb(%rdi),%eax
  201fc2:	41 88 44 24 20       	mov    %al,0x20(%r12)
  201fc7:	e9 cc f9 ff ff       	jmp    201998 <convM2S+0x78>
  201fcc:	4c 8d 6f 09          	lea    0x9(%rdi),%r13
  201fd0:	4c 39 ed             	cmp    %r13,%rbp
  201fd3:	0f 82 ca f9 ff ff    	jb     2019a3 <convM2S+0x83>
  201fd9:	0f b7 47 07          	movzwl 0x7(%rdi),%eax
  201fdd:	66 41 89 44 24 10    	mov    %ax,0x10(%r12)
  201fe3:	66 83 f8 10          	cmp    $0x10,%ax
  201fe7:	0f 87 b6 f9 ff ff    	ja     2019a3 <convM2S+0x83>
  201fed:	66 85 c0             	test   %ax,%ax
  201ff0:	0f 84 a2 f9 ff ff    	je     201998 <convM2S+0x78>
  201ff6:	49 8d 54 24 18       	lea    0x18(%r12),%rdx
  201ffb:	45 31 c0             	xor    %r8d,%r8d
  201ffe:	66 90                	xchg   %ax,%ax
  202000:	4c 89 ef             	mov    %r13,%rdi
  202003:	48 89 ee             	mov    %rbp,%rsi
  202006:	e8 e5 f8 ff ff       	call   2018f0 <gqid>
  20200b:	49 89 c5             	mov    %rax,%r13
  20200e:	48 85 c0             	test   %rax,%rax
  202011:	0f 84 8c f9 ff ff    	je     2019a3 <convM2S+0x83>
  202017:	41 0f b7 44 24 10    	movzwl 0x10(%r12),%eax
  20201d:	41 83 c0 01          	add    $0x1,%r8d
  202021:	48 83 c2 18          	add    $0x18,%rdx
  202025:	41 39 c0             	cmp    %eax,%r8d
  202028:	72 d6                	jb     202000 <convM2S+0x6e0>
  20202a:	4c 39 ed             	cmp    %r13,%rbp
  20202d:	0f 82 70 f9 ff ff    	jb     2019a3 <convM2S+0x83>
  202033:	e9 60 f9 ff ff       	jmp    201998 <convM2S+0x78>
  202038:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
  20203f:	00 
  202040:	4c 8d 6f 11          	lea    0x11(%rdi),%r13
  202044:	4c 39 ed             	cmp    %r13,%rbp
  202047:	0f 82 56 f9 ff ff    	jb     2019a3 <convM2S+0x83>
  20204d:	8b 47 07             	mov    0x7(%rdi),%eax
  202050:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  202055:	8b 47 0b             	mov    0xb(%rdi),%eax
  202058:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  20205d:	0f b7 47 0f          	movzwl 0xf(%rdi),%eax
  202061:	66 41 89 44 24 14    	mov    %ax,0x14(%r12)
  202067:	66 83 f8 10          	cmp    $0x10,%ax
  20206b:	0f 87 32 f9 ff ff    	ja     2019a3 <convM2S+0x83>
  202071:	66 85 c0             	test   %ax,%ax
  202074:	0f 84 1e f9 ff ff    	je     201998 <convM2S+0x78>
  20207a:	48 8d 77 13          	lea    0x13(%rdi),%rsi
  20207e:	48 39 f5             	cmp    %rsi,%rbp
  202081:	0f 82 1c f9 ff ff    	jb     2019a3 <convM2S+0x83>
  202087:	0f b7 57 11          	movzwl 0x11(%rdi),%edx
  20208b:	4c 8d 7f 12          	lea    0x12(%rdi),%r15
  20208f:	31 c9                	xor    %ecx,%ecx
  202091:	4c 8d 6c 17 13       	lea    0x13(%rdi,%rdx,1),%r13
  202096:	4c 39 ed             	cmp    %r13,%rbp
  202099:	0f 82 04 f9 ff ff    	jb     2019a3 <convM2S+0x83>
  20209f:	90                   	nop
  2020a0:	4c 89 ff             	mov    %r15,%rdi
  2020a3:	89 4c 24 08          	mov    %ecx,0x8(%rsp)
  2020a7:	48 89 14 24          	mov    %rdx,(%rsp)
  2020ab:	e8 20 f6 ff ff       	call   2016d0 <memmove>
  2020b0:	8b 44 24 08          	mov    0x8(%rsp),%eax
  2020b4:	48 8b 14 24          	mov    (%rsp),%rdx
  2020b8:	48 89 c1             	mov    %rax,%rcx
  2020bb:	41 c6 04 17 00       	movb   $0x0,(%r15,%rdx,1)
  2020c0:	4d 89 7c c4 18       	mov    %r15,0x18(%r12,%rax,8)
  2020c5:	41 0f b7 44 24 14    	movzwl 0x14(%r12),%eax
  2020cb:	83 c1 01             	add    $0x1,%ecx
  2020ce:	39 c1                	cmp    %eax,%ecx
  2020d0:	0f 83 72 03 00 00    	jae    202448 <convM2S+0xb28>
  2020d6:	49 8d 75 02          	lea    0x2(%r13),%rsi
  2020da:	48 39 f5             	cmp    %rsi,%rbp
  2020dd:	0f 82 c0 f8 ff ff    	jb     2019a3 <convM2S+0x83>
  2020e3:	41 0f b7 55 00       	movzwl 0x0(%r13),%edx
  2020e8:	4d 8d 7d 01          	lea    0x1(%r13),%r15
  2020ec:	4d 8d 6c 17 01       	lea    0x1(%r15,%rdx,1),%r13
  2020f1:	4c 39 ed             	cmp    %r13,%rbp
  2020f4:	73 aa                	jae    2020a0 <convM2S+0x780>
  2020f6:	e9 a8 f8 ff ff       	jmp    2019a3 <convM2S+0x83>
  2020fb:	48 8d 47 0b          	lea    0xb(%rdi),%rax
  2020ff:	48 39 c5             	cmp    %rax,%rbp
  202102:	0f 82 9b f8 ff ff    	jb     2019a3 <convM2S+0x83>
  202108:	8b 47 07             	mov    0x7(%rdi),%eax
  20210b:	48 8d 77 0d          	lea    0xd(%rdi),%rsi
  20210f:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  202114:	48 39 f5             	cmp    %rsi,%rbp
  202117:	0f 82 86 f8 ff ff    	jb     2019a3 <convM2S+0x83>
  20211d:	44 0f b7 6f 0b       	movzwl 0xb(%rdi),%r13d
  202122:	48 8d 4f 0c          	lea    0xc(%rdi),%rcx
  202126:	4e 8d 7c 2f 0d       	lea    0xd(%rdi,%r13,1),%r15
  20212b:	4c 39 fd             	cmp    %r15,%rbp
  20212e:	0f 82 6f f8 ff ff    	jb     2019a3 <convM2S+0x83>
  202134:	4c 89 ea             	mov    %r13,%rdx
  202137:	48 89 cf             	mov    %rcx,%rdi
  20213a:	48 89 0c 24          	mov    %rcx,(%rsp)
  20213e:	e8 8d f5 ff ff       	call   2016d0 <memmove>
  202143:	42 c6 44 2b 0c 00    	movb   $0x0,0xc(%rbx,%r13,1)
  202149:	48 8b 0c 24          	mov    (%rsp),%rcx
  20214d:	49 8d 77 02          	lea    0x2(%r15),%rsi
  202151:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  202156:	48 39 f5             	cmp    %rsi,%rbp
  202159:	0f 82 44 f8 ff ff    	jb     2019a3 <convM2S+0x83>
  20215f:	41 0f b7 17          	movzwl (%r15),%edx
  202163:	49 8d 4f 01          	lea    0x1(%r15),%rcx
  202167:	4d 8d 6c 17 02       	lea    0x2(%r15,%rdx,1),%r13
  20216c:	4c 39 ed             	cmp    %r13,%rbp
  20216f:	0f 82 2e f8 ff ff    	jb     2019a3 <convM2S+0x83>
  202175:	48 89 cf             	mov    %rcx,%rdi
  202178:	48 89 54 24 08       	mov    %rdx,0x8(%rsp)
  20217d:	48 89 0c 24          	mov    %rcx,(%rsp)
  202181:	e8 4a f5 ff ff       	call   2016d0 <memmove>
  202186:	48 8b 54 24 08       	mov    0x8(%rsp),%rdx
  20218b:	41 c6 44 17 01 00    	movb   $0x0,0x1(%r15,%rdx,1)
  202191:	48 8b 0c 24          	mov    (%rsp),%rcx
  202195:	49 89 4c 24 20       	mov    %rcx,0x20(%r12)
  20219a:	e9 f9 f7 ff ff       	jmp    201998 <convM2S+0x78>
  20219f:	48 8d 77 09          	lea    0x9(%rdi),%rsi
  2021a3:	48 39 f5             	cmp    %rsi,%rbp
  2021a6:	0f 82 f7 f7 ff ff    	jb     2019a3 <convM2S+0x83>
  2021ac:	44 0f b7 7f 07       	movzwl 0x7(%rdi),%r15d
  2021b1:	48 8d 4f 08          	lea    0x8(%rdi),%rcx
  2021b5:	4e 8d 6c 3f 09       	lea    0x9(%rdi,%r15,1),%r13
  2021ba:	4c 39 ed             	cmp    %r13,%rbp
  2021bd:	0f 82 e0 f7 ff ff    	jb     2019a3 <convM2S+0x83>
  2021c3:	48 89 cf             	mov    %rcx,%rdi
  2021c6:	4c 89 fa             	mov    %r15,%rdx
  2021c9:	48 89 0c 24          	mov    %rcx,(%rsp)
  2021cd:	e8 fe f4 ff ff       	call   2016d0 <memmove>
  2021d2:	48 8b 0c 24          	mov    (%rsp),%rcx
  2021d6:	49 8d 75 02          	lea    0x2(%r13),%rsi
  2021da:	42 c6 44 3b 08 00    	movb   $0x0,0x8(%rbx,%r15,1)
  2021e0:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  2021e5:	48 39 f5             	cmp    %rsi,%rbp
  2021e8:	0f 82 b5 f7 ff ff    	jb     2019a3 <convM2S+0x83>
  2021ee:	45 0f b7 7d 00       	movzwl 0x0(%r13),%r15d
  2021f3:	49 8d 7d 01          	lea    0x1(%r13),%rdi
  2021f7:	4b 8d 4c 3d 02       	lea    0x2(%r13,%r15,1),%rcx
  2021fc:	48 39 cd             	cmp    %rcx,%rbp
  2021ff:	48 89 0c 24          	mov    %rcx,(%rsp)
  202203:	0f 82 9a f7 ff ff    	jb     2019a3 <convM2S+0x83>
  202209:	4c 89 fa             	mov    %r15,%rdx
  20220c:	48 89 7c 24 08       	mov    %rdi,0x8(%rsp)
  202211:	e8 ba f4 ff ff       	call   2016d0 <memmove>
  202216:	48 8b 0c 24          	mov    (%rsp),%rcx
  20221a:	48 8b 7c 24 08       	mov    0x8(%rsp),%rdi
  20221f:	43 c6 44 3d 01 00    	movb   $0x0,0x1(%r13,%r15,1)
  202225:	4c 8d 69 04          	lea    0x4(%rcx),%r13
  202229:	49 89 7c 24 10       	mov    %rdi,0x10(%r12)
  20222e:	4c 39 ed             	cmp    %r13,%rbp
  202231:	0f 82 6c f7 ff ff    	jb     2019a3 <convM2S+0x83>
  202237:	8b 01                	mov    (%rcx),%eax
  202239:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  20223e:	e9 55 f7 ff ff       	jmp    201998 <convM2S+0x78>
  202243:	4c 8d 6f 17          	lea    0x17(%rdi),%r13
  202247:	4c 39 ed             	cmp    %r13,%rbp
  20224a:	0f 82 53 f7 ff ff    	jb     2019a3 <convM2S+0x83>
  202250:	8b 47 07             	mov    0x7(%rdi),%eax
  202253:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  202258:	48 8b 47 0b          	mov    0xb(%rdi),%rax
  20225c:	49 89 44 24 10       	mov    %rax,0x10(%r12)
  202261:	8b 47 13             	mov    0x13(%rdi),%eax
  202264:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  202269:	e9 2a f7 ff ff       	jmp    201998 <convM2S+0x78>
  20226e:	4c 8d 6f 0b          	lea    0xb(%rdi),%r13
  202272:	4c 39 ed             	cmp    %r13,%rbp
  202275:	0f 82 28 f7 ff ff    	jb     2019a3 <convM2S+0x83>
  20227b:	8b 47 07             	mov    0x7(%rdi),%eax
  20227e:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  202283:	e9 10 f7 ff ff       	jmp    201998 <convM2S+0x78>
  202288:	48 8d 77 09          	lea    0x9(%rdi),%rsi
  20228c:	48 39 f5             	cmp    %rsi,%rbp
  20228f:	0f 82 0e f7 ff ff    	jb     2019a3 <convM2S+0x83>
  202295:	44 0f b7 6f 07       	movzwl 0x7(%rdi),%r13d
  20229a:	48 8d 4f 08          	lea    0x8(%rdi),%rcx
  20229e:	4e 8d 7c 2f 09       	lea    0x9(%rdi,%r13,1),%r15
  2022a3:	4c 39 fd             	cmp    %r15,%rbp
  2022a6:	0f 82 f7 f6 ff ff    	jb     2019a3 <convM2S+0x83>
  2022ac:	48 89 cf             	mov    %rcx,%rdi
  2022af:	4c 89 ea             	mov    %r13,%rdx
  2022b2:	48 89 0c 24          	mov    %rcx,(%rsp)
  2022b6:	e8 15 f4 ff ff       	call   2016d0 <memmove>
  2022bb:	48 8b 0c 24          	mov    (%rsp),%rcx
  2022bf:	49 8d 77 02          	lea    0x2(%r15),%rsi
  2022c3:	42 c6 44 2b 08 00    	movb   $0x0,0x8(%rbx,%r13,1)
  2022c9:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  2022ce:	48 39 f5             	cmp    %rsi,%rbp
  2022d1:	0f 82 cc f6 ff ff    	jb     2019a3 <convM2S+0x83>
  2022d7:	41 0f b7 17          	movzwl (%r15),%edx
  2022db:	49 8d 4f 01          	lea    0x1(%r15),%rcx
  2022df:	4d 8d 6c 17 02       	lea    0x2(%r15,%rdx,1),%r13
  2022e4:	4c 39 ed             	cmp    %r13,%rbp
  2022e7:	0f 82 b6 f6 ff ff    	jb     2019a3 <convM2S+0x83>
  2022ed:	48 89 cf             	mov    %rcx,%rdi
  2022f0:	48 89 54 24 08       	mov    %rdx,0x8(%rsp)
  2022f5:	48 89 0c 24          	mov    %rcx,(%rsp)
  2022f9:	e8 d2 f3 ff ff       	call   2016d0 <memmove>
  2022fe:	48 8b 54 24 08       	mov    0x8(%rsp),%rdx
  202303:	48 8b 0c 24          	mov    (%rsp),%rcx
  202307:	41 c6 44 17 01 00    	movb   $0x0,0x1(%r15,%rdx,1)
  20230d:	49 89 4c 24 10       	mov    %rcx,0x10(%r12)
  202312:	e9 81 f6 ff ff       	jmp    201998 <convM2S+0x78>
  202317:	4c 8d 6f 0f          	lea    0xf(%rdi),%r13
  20231b:	4c 39 ed             	cmp    %r13,%rbp
  20231e:	0f 82 7f f6 ff ff    	jb     2019a3 <convM2S+0x83>
  202324:	8b 47 07             	mov    0x7(%rdi),%eax
  202327:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  20232c:	8b 47 0b             	mov    0xb(%rdi),%eax
  20232f:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  202334:	e9 5f f6 ff ff       	jmp    201998 <convM2S+0x78>
  202339:	4c 8d 6f 0f          	lea    0xf(%rdi),%r13
  20233d:	4c 39 ed             	cmp    %r13,%rbp
  202340:	0f 82 5d f6 ff ff    	jb     2019a3 <convM2S+0x83>
  202346:	8b 47 07             	mov    0x7(%rdi),%eax
  202349:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  20234e:	8b 47 0b             	mov    0xb(%rdi),%eax
  202351:	41 89 44 24 14       	mov    %eax,0x14(%r12)
  202356:	e9 3d f6 ff ff       	jmp    201998 <convM2S+0x78>
  20235b:	4c 8d 6f 0b          	lea    0xb(%rdi),%r13
  20235f:	4c 39 ed             	cmp    %r13,%rbp
  202362:	0f 82 3b f6 ff ff    	jb     2019a3 <convM2S+0x83>
  202368:	8b 47 07             	mov    0x7(%rdi),%eax
  20236b:	41 89 44 24 14       	mov    %eax,0x14(%r12)
  202370:	e9 23 f6 ff ff       	jmp    201998 <convM2S+0x78>
  202375:	48 8d 77 09          	lea    0x9(%rdi),%rsi
  202379:	48 39 f5             	cmp    %rsi,%rbp
  20237c:	0f 82 21 f6 ff ff    	jb     2019a3 <convM2S+0x83>
  202382:	44 0f b7 6f 07       	movzwl 0x7(%rdi),%r13d
  202387:	48 8d 4f 08          	lea    0x8(%rdi),%rcx
  20238b:	4e 8d 7c 2f 09       	lea    0x9(%rdi,%r13,1),%r15
  202390:	4c 39 fd             	cmp    %r15,%rbp
  202393:	0f 82 0a f6 ff ff    	jb     2019a3 <convM2S+0x83>
  202399:	48 89 cf             	mov    %rcx,%rdi
  20239c:	4c 89 ea             	mov    %r13,%rdx
  20239f:	48 89 0c 24          	mov    %rcx,(%rsp)
  2023a3:	e8 28 f3 ff ff       	call   2016d0 <memmove>
  2023a8:	48 8b 0c 24          	mov    (%rsp),%rcx
  2023ac:	49 8d 47 02          	lea    0x2(%r15),%rax
  2023b0:	42 c6 44 2b 08 00    	movb   $0x0,0x8(%rbx,%r13,1)
  2023b6:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  2023bb:	48 39 c5             	cmp    %rax,%rbp
  2023be:	0f 82 df f5 ff ff    	jb     2019a3 <convM2S+0x83>
  2023c4:	41 0f b7 17          	movzwl (%r15),%edx
  2023c8:	4c 8d 2c 10          	lea    (%rax,%rdx,1),%r13
  2023cc:	66 41 89 54 24 10    	mov    %dx,0x10(%r12)
  2023d2:	4c 39 ed             	cmp    %r13,%rbp
  2023d5:	0f 83 f1 f6 ff ff    	jae    201acc <convM2S+0x1ac>
  2023db:	e9 c3 f5 ff ff       	jmp    2019a3 <convM2S+0x83>
  2023e0:	48 8d 47 0b          	lea    0xb(%rdi),%rax
  2023e4:	48 39 c5             	cmp    %rax,%rbp
  2023e7:	0f 82 b6 f5 ff ff    	jb     2019a3 <convM2S+0x83>
  2023ed:	8b 47 07             	mov    0x7(%rdi),%eax
  2023f0:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  2023f5:	48 8d 47 0f          	lea    0xf(%rdi),%rax
  2023f9:	48 39 c5             	cmp    %rax,%rbp
  2023fc:	0f 82 a1 f5 ff ff    	jb     2019a3 <convM2S+0x83>
  202402:	8b 47 0b             	mov    0xb(%rdi),%eax
  202405:	48 8d 77 11          	lea    0x11(%rdi),%rsi
  202409:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  20240e:	48 39 f5             	cmp    %rsi,%rbp
  202411:	0f 82 8c f5 ff ff    	jb     2019a3 <convM2S+0x83>
  202417:	44 0f b7 6f 0f       	movzwl 0xf(%rdi),%r13d
  20241c:	48 8d 4f 10          	lea    0x10(%rdi),%rcx
  202420:	4e 8d 7c 2f 11       	lea    0x11(%rdi,%r13,1),%r15
  202425:	4c 39 fd             	cmp    %r15,%rbp
  202428:	0f 82 75 f5 ff ff    	jb     2019a3 <convM2S+0x83>
  20242e:	4c 89 ea             	mov    %r13,%rdx
  202431:	48 89 cf             	mov    %rcx,%rdi
  202434:	48 89 0c 24          	mov    %rcx,(%rsp)
  202438:	e8 93 f2 ff ff       	call   2016d0 <memmove>
  20243d:	42 c6 44 2b 10 00    	movb   $0x0,0x10(%rbx,%r13,1)
  202443:	e9 01 fd ff ff       	jmp    202149 <convM2S+0x829>
  202448:	4c 39 ed             	cmp    %r13,%rbp
  20244b:	0f 92 c0             	setb   %al
  20244e:	e9 f6 f7 ff ff       	jmp    201c49 <convM2S+0x329>
  202453:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  20245a:	00 00 00 
  20245d:	0f 1f 00             	nopl   (%rax)

0000000000202460 <pqid>:
  202460:	0f b6 56 10          	movzbl 0x10(%rsi),%edx
  202464:	48 89 f8             	mov    %rdi,%rax
  202467:	48 83 c0 0d          	add    $0xd,%rax
  20246b:	88 17                	mov    %dl,(%rdi)
  20246d:	48 8b 56 08          	mov    0x8(%rsi),%rdx
  202471:	88 57 01             	mov    %dl,0x1(%rdi)
  202474:	48 8b 56 08          	mov    0x8(%rsi),%rdx
  202478:	88 77 02             	mov    %dh,0x2(%rdi)
  20247b:	48 8b 56 08          	mov    0x8(%rsi),%rdx
  20247f:	48 c1 ea 10          	shr    $0x10,%rdx
  202483:	88 57 03             	mov    %dl,0x3(%rdi)
  202486:	48 8b 56 08          	mov    0x8(%rsi),%rdx
  20248a:	48 c1 ea 18          	shr    $0x18,%rdx
  20248e:	88 57 04             	mov    %dl,0x4(%rdi)
  202491:	48 8b 16             	mov    (%rsi),%rdx
  202494:	88 57 05             	mov    %dl,0x5(%rdi)
  202497:	48 8b 16             	mov    (%rsi),%rdx
  20249a:	88 77 06             	mov    %dh,0x6(%rdi)
  20249d:	48 8b 16             	mov    (%rsi),%rdx
  2024a0:	48 c1 ea 10          	shr    $0x10,%rdx
  2024a4:	88 57 07             	mov    %dl,0x7(%rdi)
  2024a7:	48 8b 16             	mov    (%rsi),%rdx
  2024aa:	48 c1 ea 18          	shr    $0x18,%rdx
  2024ae:	88 57 08             	mov    %dl,0x8(%rdi)
  2024b1:	48 8b 16             	mov    (%rsi),%rdx
  2024b4:	48 c1 ea 20          	shr    $0x20,%rdx
  2024b8:	88 57 09             	mov    %dl,0x9(%rdi)
  2024bb:	48 8b 16             	mov    (%rsi),%rdx
  2024be:	48 c1 ea 28          	shr    $0x28,%rdx
  2024c2:	88 57 0a             	mov    %dl,0xa(%rdi)
  2024c5:	48 8b 16             	mov    (%rsi),%rdx
  2024c8:	48 c1 ea 30          	shr    $0x30,%rdx
  2024cc:	88 57 0b             	mov    %dl,0xb(%rdi)
  2024cf:	48 8b 16             	mov    (%rsi),%rdx
  2024d2:	48 c1 ea 38          	shr    $0x38,%rdx
  2024d6:	88 57 0c             	mov    %dl,0xc(%rdi)
  2024d9:	c3                   	ret
  2024da:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)

00000000002024e0 <sizeS2M>:
  2024e0:	f3 0f 1e fa          	endbr64
  2024e4:	41 54                	push   %r12
  2024e6:	55                   	push   %rbp
  2024e7:	53                   	push   %rbx
  2024e8:	0f b6 0f             	movzbl (%rdi),%ecx
  2024eb:	80 f9 cd             	cmp    $0xcd,%cl
  2024ee:	0f 87 9c 00 00 00    	ja     202590 <sizeS2M+0xb0>
  2024f4:	48 89 fb             	mov    %rdi,%rbx
  2024f7:	80 f9 7d             	cmp    $0x7d,%cl
  2024fa:	76 24                	jbe    202520 <sizeS2M+0x40>
  2024fc:	83 e9 7e             	sub    $0x7e,%ecx
  2024ff:	80 f9 4f             	cmp    $0x4f,%cl
  202502:	0f 87 88 00 00 00    	ja     202590 <sizeS2M+0xb0>
  202508:	48 8d 15 45 14 00 00 	lea    0x1445(%rip),%rdx        # 203954 <_syscall+0x3ff>
  20250f:	0f b6 c9             	movzbl %cl,%ecx
  202512:	48 63 04 8a          	movslq (%rdx,%rcx,4),%rax
  202516:	48 01 d0             	add    %rdx,%rax
  202519:	3e ff e0             	notrack jmp *%rax
  20251c:	0f 1f 40 00          	nopl   0x0(%rax)
  202520:	80 f9 76             	cmp    $0x76,%cl
  202523:	77 2b                	ja     202550 <sizeS2M+0x70>
  202525:	80 f9 63             	cmp    $0x63,%cl
  202528:	76 66                	jbe    202590 <sizeS2M+0xb0>
  20252a:	83 e9 64             	sub    $0x64,%ecx
  20252d:	80 f9 12             	cmp    $0x12,%cl
  202530:	77 5e                	ja     202590 <sizeS2M+0xb0>
  202532:	48 8d 15 5b 15 00 00 	lea    0x155b(%rip),%rdx        # 203a94 <_syscall+0x53f>
  202539:	0f b6 c9             	movzbl %cl,%ecx
  20253c:	48 63 04 8a          	movslq (%rdx,%rcx,4),%rax
  202540:	48 01 d0             	add    %rdx,%rax
  202543:	3e ff e0             	notrack jmp *%rax
  202546:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  20254d:	00 00 00 
  202550:	83 e9 77             	sub    $0x77,%ecx
  202553:	b8 01 00 00 00       	mov    $0x1,%eax
  202558:	bd 0b 00 00 00       	mov    $0xb,%ebp
  20255d:	48 d3 e0             	shl    %cl,%rax
  202560:	a8 2b                	test   $0x2b,%al
  202562:	75 11                	jne    202575 <sizeS2M+0x95>
  202564:	a8 14                	test   $0x14,%al
  202566:	0f 84 d4 00 00 00    	je     202640 <sizeS2M+0x160>
  20256c:	0f 1f 40 00          	nopl   0x0(%rax)
  202570:	bd 07 00 00 00       	mov    $0x7,%ebp
  202575:	89 e8                	mov    %ebp,%eax
  202577:	5b                   	pop    %rbx
  202578:	5d                   	pop    %rbp
  202579:	41 5c                	pop    %r12
  20257b:	c3                   	ret
  20257c:	0f 1f 40 00          	nopl   0x0(%rax)
  202580:	bd 0b 00 00 00       	mov    $0xb,%ebp
  202585:	eb ee                	jmp    202575 <sizeS2M+0x95>
  202587:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  20258e:	00 00 
  202590:	31 ed                	xor    %ebp,%ebp
  202592:	5b                   	pop    %rbx
  202593:	89 e8                	mov    %ebp,%eax
  202595:	5d                   	pop    %rbp
  202596:	41 5c                	pop    %r12
  202598:	c3                   	ret
  202599:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  2025a0:	48 8b 7b 18          	mov    0x18(%rbx),%rdi
  2025a4:	48 85 ff             	test   %rdi,%rdi
  2025a7:	0f 84 63 02 00 00    	je     202810 <sizeS2M+0x330>
  2025ad:	e8 be f1 ff ff       	call   201770 <strlen>
  2025b2:	8d 68 0d             	lea    0xd(%rax),%ebp
  2025b5:	eb be                	jmp    202575 <sizeS2M+0x95>
  2025b7:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  2025be:	00 00 
  2025c0:	48 8b 7f 18          	mov    0x18(%rdi),%rdi
  2025c4:	48 85 ff             	test   %rdi,%rdi
  2025c7:	74 18                	je     2025e1 <sizeS2M+0x101>
  2025c9:	e8 a2 f1 ff ff       	call   201770 <strlen>
  2025ce:	8d 68 09             	lea    0x9(%rax),%ebp
  2025d1:	eb a2                	jmp    202575 <sizeS2M+0x95>
  2025d3:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  2025d8:	48 8b 7b 10          	mov    0x10(%rbx),%rdi
  2025dc:	48 85 ff             	test   %rdi,%rdi
  2025df:	75 e8                	jne    2025c9 <sizeS2M+0xe9>
  2025e1:	bd 09 00 00 00       	mov    $0x9,%ebp
  2025e6:	eb 8d                	jmp    202575 <sizeS2M+0x95>
  2025e8:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
  2025ef:	00 
  2025f0:	48 8b 7b 18          	mov    0x18(%rbx),%rdi
  2025f4:	48 85 ff             	test   %rdi,%rdi
  2025f7:	0f 84 6d 01 00 00    	je     20276a <sizeS2M+0x28a>
  2025fd:	e8 6e f1 ff ff       	call   201770 <strlen>
  202602:	8d 68 12             	lea    0x12(%rax),%ebp
  202605:	e9 6b ff ff ff       	jmp    202575 <sizeS2M+0x95>
  20260a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  202610:	8b 43 18             	mov    0x18(%rbx),%eax
  202613:	8d 68 17             	lea    0x17(%rax),%ebp
  202616:	e9 5a ff ff ff       	jmp    202575 <sizeS2M+0x95>
  20261b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  202620:	8b 43 18             	mov    0x18(%rbx),%eax
  202623:	8d 68 0b             	lea    0xb(%rax),%ebp
  202626:	e9 4a ff ff ff       	jmp    202575 <sizeS2M+0x95>
  20262b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  202630:	0f b7 47 10          	movzwl 0x10(%rdi),%eax
  202634:	8d 68 0d             	lea    0xd(%rax),%ebp
  202637:	e9 39 ff ff ff       	jmp    202575 <sizeS2M+0x95>
  20263c:	0f 1f 40 00          	nopl   0x0(%rax)
  202640:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  202644:	8d 68 09             	lea    0x9(%rax),%ebp
  202647:	e9 29 ff ff ff       	jmp    202575 <sizeS2M+0x95>
  20264c:	0f 1f 40 00          	nopl   0x0(%rax)
  202650:	8b 47 20             	mov    0x20(%rdi),%eax
  202653:	8d 68 13             	lea    0x13(%rax),%ebp
  202656:	e9 1a ff ff ff       	jmp    202575 <sizeS2M+0x95>
  20265b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  202660:	bd 0f 00 00 00       	mov    $0xf,%ebp
  202665:	e9 0b ff ff ff       	jmp    202575 <sizeS2M+0x95>
  20266a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  202670:	bd 18 00 00 00       	mov    $0x18,%ebp
  202675:	e9 fb fe ff ff       	jmp    202575 <sizeS2M+0x95>
  20267a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  202680:	bd 14 00 00 00       	mov    $0x14,%ebp
  202685:	e9 eb fe ff ff       	jmp    202575 <sizeS2M+0x95>
  20268a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  202690:	bd 17 00 00 00       	mov    $0x17,%ebp
  202695:	e9 db fe ff ff       	jmp    202575 <sizeS2M+0x95>
  20269a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  2026a0:	45 31 e4             	xor    %r12d,%r12d
  2026a3:	66 83 7f 14 00       	cmpw   $0x0,0x14(%rdi)
  2026a8:	bd 11 00 00 00       	mov    $0x11,%ebp
  2026ad:	75 24                	jne    2026d3 <sizeS2M+0x1f3>
  2026af:	e9 c1 fe ff ff       	jmp    202575 <sizeS2M+0x95>
  2026b4:	0f 1f 40 00          	nopl   0x0(%rax)
  2026b8:	e8 b3 f0 ff ff       	call   201770 <strlen>
  2026bd:	83 c0 02             	add    $0x2,%eax
  2026c0:	01 c5                	add    %eax,%ebp
  2026c2:	0f b7 43 14          	movzwl 0x14(%rbx),%eax
  2026c6:	49 83 c4 01          	add    $0x1,%r12
  2026ca:	44 39 e0             	cmp    %r12d,%eax
  2026cd:	0f 8e a2 fe ff ff    	jle    202575 <sizeS2M+0x95>
  2026d3:	4a 8b 7c e3 18       	mov    0x18(%rbx,%r12,8),%rdi
  2026d8:	48 85 ff             	test   %rdi,%rdi
  2026db:	75 db                	jne    2026b8 <sizeS2M+0x1d8>
  2026dd:	b8 02 00 00 00       	mov    $0x2,%eax
  2026e2:	eb dc                	jmp    2026c0 <sizeS2M+0x1e0>
  2026e4:	0f 1f 40 00          	nopl   0x0(%rax)
  2026e8:	48 8b 7f 18          	mov    0x18(%rdi),%rdi
  2026ec:	48 85 ff             	test   %rdi,%rdi
  2026ef:	0f 84 40 01 00 00    	je     202835 <sizeS2M+0x355>
  2026f5:	e8 76 f0 ff ff       	call   201770 <strlen>
  2026fa:	8d 68 0d             	lea    0xd(%rax),%ebp
  2026fd:	48 8b 7b 20          	mov    0x20(%rbx),%rdi
  202701:	48 85 ff             	test   %rdi,%rdi
  202704:	0f 84 c4 00 00 00    	je     2027ce <sizeS2M+0x2ee>
  20270a:	e8 61 f0 ff ff       	call   201770 <strlen>
  20270f:	83 c0 02             	add    $0x2,%eax
  202712:	01 c5                	add    %eax,%ebp
  202714:	89 e8                	mov    %ebp,%eax
  202716:	5b                   	pop    %rbx
  202717:	5d                   	pop    %rbp
  202718:	41 5c                	pop    %r12
  20271a:	c3                   	ret
  20271b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  202720:	48 8b 7f 18          	mov    0x18(%rdi),%rdi
  202724:	48 85 ff             	test   %rdi,%rdi
  202727:	0f 84 f7 00 00 00    	je     202824 <sizeS2M+0x344>
  20272d:	e8 3e f0 ff ff       	call   201770 <strlen>
  202732:	8d 68 11             	lea    0x11(%rax),%ebp
  202735:	eb c6                	jmp    2026fd <sizeS2M+0x21d>
  202737:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  20273e:	00 00 
  202740:	0f b7 47 10          	movzwl 0x10(%rdi),%eax
  202744:	8d 14 40             	lea    (%rax,%rax,2),%edx
  202747:	8d 6c 90 09          	lea    0x9(%rax,%rdx,4),%ebp
  20274b:	e9 25 fe ff ff       	jmp    202575 <sizeS2M+0x95>
  202750:	bd 0c 00 00 00       	mov    $0xc,%ebp
  202755:	e9 1b fe ff ff       	jmp    202575 <sizeS2M+0x95>
  20275a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  202760:	bd 1c 00 00 00       	mov    $0x1c,%ebp
  202765:	e9 0b fe ff ff       	jmp    202575 <sizeS2M+0x95>
  20276a:	bd 12 00 00 00       	mov    $0x12,%ebp
  20276f:	e9 01 fe ff ff       	jmp    202575 <sizeS2M+0x95>
  202774:	48 8b 7f 18          	mov    0x18(%rdi),%rdi
  202778:	48 85 ff             	test   %rdi,%rdi
  20277b:	0f 84 99 00 00 00    	je     20281a <sizeS2M+0x33a>
  202781:	e8 ea ef ff ff       	call   201770 <strlen>
  202786:	83 c0 02             	add    $0x2,%eax
  202789:	0f b7 53 10          	movzwl 0x10(%rbx),%edx
  20278d:	8d 6c 10 09          	lea    0x9(%rax,%rdx,1),%ebp
  202791:	e9 df fd ff ff       	jmp    202575 <sizeS2M+0x95>
  202796:	48 8b 7f 18          	mov    0x18(%rdi),%rdi
  20279a:	48 85 ff             	test   %rdi,%rdi
  20279d:	0f 84 9c 00 00 00    	je     20283f <sizeS2M+0x35f>
  2027a3:	e8 c8 ef ff ff       	call   201770 <strlen>
  2027a8:	8d 68 0e             	lea    0xe(%rax),%ebp
  2027ab:	e9 c5 fd ff ff       	jmp    202575 <sizeS2M+0x95>
  2027b0:	48 8b 7f 18          	mov    0x18(%rdi),%rdi
  2027b4:	48 85 ff             	test   %rdi,%rdi
  2027b7:	74 75                	je     20282e <sizeS2M+0x34e>
  2027b9:	e8 b2 ef ff ff       	call   201770 <strlen>
  2027be:	8d 68 0d             	lea    0xd(%rax),%ebp
  2027c1:	48 8b 7b 10          	mov    0x10(%rbx),%rdi
  2027c5:	48 85 ff             	test   %rdi,%rdi
  2027c8:	0f 85 3c ff ff ff    	jne    20270a <sizeS2M+0x22a>
  2027ce:	b8 02 00 00 00       	mov    $0x2,%eax
  2027d3:	01 c5                	add    %eax,%ebp
  2027d5:	e9 3a ff ff ff       	jmp    202714 <sizeS2M+0x234>
  2027da:	48 8b 7f 18          	mov    0x18(%rdi),%rdi
  2027de:	48 85 ff             	test   %rdi,%rdi
  2027e1:	74 66                	je     202849 <sizeS2M+0x369>
  2027e3:	e8 88 ef ff ff       	call   201770 <strlen>
  2027e8:	8d 68 09             	lea    0x9(%rax),%ebp
  2027eb:	eb d4                	jmp    2027c1 <sizeS2M+0x2e1>
  2027ed:	48 8b 7f 10          	mov    0x10(%rdi),%rdi
  2027f1:	48 85 ff             	test   %rdi,%rdi
  2027f4:	74 5d                	je     202853 <sizeS2M+0x373>
  2027f6:	e8 75 ef ff ff       	call   201770 <strlen>
  2027fb:	8d 68 15             	lea    0x15(%rax),%ebp
  2027fe:	e9 fa fe ff ff       	jmp    2026fd <sizeS2M+0x21d>
  202803:	48 8b 7f 10          	mov    0x10(%rdi),%rdi
  202807:	48 85 ff             	test   %rdi,%rdi
  20280a:	0f 85 9d fd ff ff    	jne    2025ad <sizeS2M+0xcd>
  202810:	bd 0d 00 00 00       	mov    $0xd,%ebp
  202815:	e9 5b fd ff ff       	jmp    202575 <sizeS2M+0x95>
  20281a:	b8 02 00 00 00       	mov    $0x2,%eax
  20281f:	e9 65 ff ff ff       	jmp    202789 <sizeS2M+0x2a9>
  202824:	bd 11 00 00 00       	mov    $0x11,%ebp
  202829:	e9 cf fe ff ff       	jmp    2026fd <sizeS2M+0x21d>
  20282e:	bd 0d 00 00 00       	mov    $0xd,%ebp
  202833:	eb 8c                	jmp    2027c1 <sizeS2M+0x2e1>
  202835:	bd 0d 00 00 00       	mov    $0xd,%ebp
  20283a:	e9 be fe ff ff       	jmp    2026fd <sizeS2M+0x21d>
  20283f:	bd 0e 00 00 00       	mov    $0xe,%ebp
  202844:	e9 2c fd ff ff       	jmp    202575 <sizeS2M+0x95>
  202849:	bd 09 00 00 00       	mov    $0x9,%ebp
  20284e:	e9 6e ff ff ff       	jmp    2027c1 <sizeS2M+0x2e1>
  202853:	bd 15 00 00 00       	mov    $0x15,%ebp
  202858:	e9 a0 fe ff ff       	jmp    2026fd <sizeS2M+0x21d>
  20285d:	0f 1f 00             	nopl   (%rax)

0000000000202860 <convS2M>:
  202860:	f3 0f 1e fa          	endbr64
  202864:	41 57                	push   %r15
  202866:	41 56                	push   %r14
  202868:	41 55                	push   %r13
  20286a:	41 89 d5             	mov    %edx,%r13d
  20286d:	41 54                	push   %r12
  20286f:	55                   	push   %rbp
  202870:	48 89 f5             	mov    %rsi,%rbp
  202873:	53                   	push   %rbx
  202874:	48 89 fb             	mov    %rdi,%rbx
  202877:	48 83 ec 18          	sub    $0x18,%rsp
  20287b:	e8 60 fc ff ff       	call   2024e0 <sizeS2M>
  202880:	41 89 c4             	mov    %eax,%r12d
  202883:	83 e8 01             	sub    $0x1,%eax
  202886:	44 39 e8             	cmp    %r13d,%eax
  202889:	72 15                	jb     2028a0 <convS2M+0x40>
  20288b:	45 31 e4             	xor    %r12d,%r12d
  20288e:	48 83 c4 18          	add    $0x18,%rsp
  202892:	44 89 e0             	mov    %r12d,%eax
  202895:	5b                   	pop    %rbx
  202896:	5d                   	pop    %rbp
  202897:	41 5c                	pop    %r12
  202899:	41 5d                	pop    %r13
  20289b:	41 5e                	pop    %r14
  20289d:	41 5f                	pop    %r15
  20289f:	c3                   	ret
  2028a0:	44 89 65 00          	mov    %r12d,0x0(%rbp)
  2028a4:	0f b6 03             	movzbl (%rbx),%eax
  2028a7:	4c 8d 75 07          	lea    0x7(%rbp),%r14
  2028ab:	88 45 04             	mov    %al,0x4(%rbp)
  2028ae:	0f b7 43 08          	movzwl 0x8(%rbx),%eax
  2028b2:	88 45 05             	mov    %al,0x5(%rbp)
  2028b5:	0f b6 43 09          	movzbl 0x9(%rbx),%eax
  2028b9:	88 45 06             	mov    %al,0x6(%rbp)
  2028bc:	0f b6 03             	movzbl (%rbx),%eax
  2028bf:	83 e8 64             	sub    $0x64,%eax
  2028c2:	3c 69                	cmp    $0x69,%al
  2028c4:	77 c5                	ja     20288b <convS2M+0x2b>
  2028c6:	48 8d 15 13 12 00 00 	lea    0x1213(%rip),%rdx        # 203ae0 <_syscall+0x58b>
  2028cd:	0f b6 c0             	movzbl %al,%eax
  2028d0:	48 63 04 82          	movslq (%rdx,%rax,4),%rax
  2028d4:	48 01 d0             	add    %rdx,%rax
  2028d7:	3e ff e0             	notrack jmp *%rax
  2028da:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  2028e0:	b8 07 00 00 00       	mov    $0x7,%eax
  2028e5:	44 89 e2             	mov    %r12d,%edx
  2028e8:	48 39 c2             	cmp    %rax,%rdx
  2028eb:	75 9e                	jne    20288b <convS2M+0x2b>
  2028ed:	eb 9f                	jmp    20288e <convS2M+0x2e>
  2028ef:	90                   	nop
  2028f0:	8b 43 04             	mov    0x4(%rbx),%eax
  2028f3:	88 45 07             	mov    %al,0x7(%rbp)
  2028f6:	8b 43 04             	mov    0x4(%rbx),%eax
  2028f9:	88 65 08             	mov    %ah,0x8(%rbp)
  2028fc:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  202900:	88 45 09             	mov    %al,0x9(%rbp)
  202903:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  202907:	88 45 0a             	mov    %al,0xa(%rbp)
  20290a:	b8 0b 00 00 00       	mov    $0xb,%eax
  20290f:	eb d4                	jmp    2028e5 <convS2M+0x85>
  202911:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  202918:	8b 43 18             	mov    0x18(%rbx),%eax
  20291b:	88 45 07             	mov    %al,0x7(%rbp)
  20291e:	8b 43 18             	mov    0x18(%rbx),%eax
  202921:	88 65 08             	mov    %ah,0x8(%rbp)
  202924:	0f b7 43 1a          	movzwl 0x1a(%rbx),%eax
  202928:	88 45 09             	mov    %al,0x9(%rbp)
  20292b:	0f b6 43 1b          	movzbl 0x1b(%rbx),%eax
  20292f:	eb d6                	jmp    202907 <convS2M+0xa7>
  202931:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  202938:	4c 8b 6b 18          	mov    0x18(%rbx),%r13
  20293c:	4d 85 ed             	test   %r13,%r13
  20293f:	0f 84 45 02 00 00    	je     202b8a <convS2M+0x32a>
  202945:	4c 89 ef             	mov    %r13,%rdi
  202948:	e8 23 ee ff ff       	call   201770 <strlen>
  20294d:	48 8d 7d 09          	lea    0x9(%rbp),%rdi
  202951:	4c 89 ee             	mov    %r13,%rsi
  202954:	48 89 c3             	mov    %rax,%rbx
  202957:	89 c2                	mov    %eax,%edx
  202959:	e8 72 ed ff ff       	call   2016d0 <memmove>
  20295e:	8d 43 02             	lea    0x2(%rbx),%eax
  202961:	66 89 5d 07          	mov    %bx,0x7(%rbp)
  202965:	4c 01 f0             	add    %r14,%rax
  202968:	48 29 e8             	sub    %rbp,%rax
  20296b:	e9 75 ff ff ff       	jmp    2028e5 <convS2M+0x85>
  202970:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202974:	88 45 07             	mov    %al,0x7(%rbp)
  202977:	48 8b 43 10          	mov    0x10(%rbx),%rax
  20297b:	88 65 08             	mov    %ah,0x8(%rbp)
  20297e:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202982:	48 c1 e8 10          	shr    $0x10,%rax
  202986:	88 45 09             	mov    %al,0x9(%rbp)
  202989:	48 8b 43 10          	mov    0x10(%rbx),%rax
  20298d:	48 c1 e8 18          	shr    $0x18,%rax
  202991:	88 45 0a             	mov    %al,0xa(%rbp)
  202994:	8b 43 14             	mov    0x14(%rbx),%eax
  202997:	88 45 0b             	mov    %al,0xb(%rbp)
  20299a:	48 8b 43 10          	mov    0x10(%rbx),%rax
  20299e:	48 c1 e8 28          	shr    $0x28,%rax
  2029a2:	88 45 0c             	mov    %al,0xc(%rbp)
  2029a5:	0f b7 43 16          	movzwl 0x16(%rbx),%eax
  2029a9:	88 45 0d             	mov    %al,0xd(%rbp)
  2029ac:	0f b6 43 17          	movzbl 0x17(%rbx),%eax
  2029b0:	88 45 0e             	mov    %al,0xe(%rbp)
  2029b3:	b8 0f 00 00 00       	mov    $0xf,%eax
  2029b8:	e9 28 ff ff ff       	jmp    2028e5 <convS2M+0x85>
  2029bd:	0f 1f 00             	nopl   (%rax)
  2029c0:	8b 43 04             	mov    0x4(%rbx),%eax
  2029c3:	88 45 07             	mov    %al,0x7(%rbp)
  2029c6:	8b 43 04             	mov    0x4(%rbx),%eax
  2029c9:	88 65 08             	mov    %ah,0x8(%rbp)
  2029cc:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  2029d0:	88 45 09             	mov    %al,0x9(%rbp)
  2029d3:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  2029d7:	88 45 0a             	mov    %al,0xa(%rbp)
  2029da:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2029de:	88 45 0b             	mov    %al,0xb(%rbp)
  2029e1:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2029e5:	88 65 0c             	mov    %ah,0xc(%rbp)
  2029e8:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2029ec:	48 c1 f8 10          	sar    $0x10,%rax
  2029f0:	88 45 0d             	mov    %al,0xd(%rbp)
  2029f3:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2029f7:	48 c1 f8 18          	sar    $0x18,%rax
  2029fb:	88 45 0e             	mov    %al,0xe(%rbp)
  2029fe:	48 63 43 14          	movslq 0x14(%rbx),%rax
  202a02:	88 45 0f             	mov    %al,0xf(%rbp)
  202a05:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202a09:	48 c1 f8 28          	sar    $0x28,%rax
  202a0d:	88 45 10             	mov    %al,0x10(%rbp)
  202a10:	48 0f bf 43 16       	movswq 0x16(%rbx),%rax
  202a15:	88 45 11             	mov    %al,0x11(%rbp)
  202a18:	48 0f be 43 17       	movsbq 0x17(%rbx),%rax
  202a1d:	88 45 12             	mov    %al,0x12(%rbp)
  202a20:	8b 43 18             	mov    0x18(%rbx),%eax
  202a23:	88 45 13             	mov    %al,0x13(%rbp)
  202a26:	8b 43 18             	mov    0x18(%rbx),%eax
  202a29:	88 65 14             	mov    %ah,0x14(%rbp)
  202a2c:	0f b7 43 1a          	movzwl 0x1a(%rbx),%eax
  202a30:	88 45 15             	mov    %al,0x15(%rbp)
  202a33:	0f b6 43 1b          	movzbl 0x1b(%rbx),%eax
  202a37:	88 45 16             	mov    %al,0x16(%rbp)
  202a3a:	b8 17 00 00 00       	mov    $0x17,%eax
  202a3f:	e9 a1 fe ff ff       	jmp    2028e5 <convS2M+0x85>
  202a44:	0f 1f 40 00          	nopl   0x0(%rax)
  202a48:	8b 43 18             	mov    0x18(%rbx),%eax
  202a4b:	4c 8d 6d 0b          	lea    0xb(%rbp),%r13
  202a4f:	88 45 07             	mov    %al,0x7(%rbp)
  202a52:	8b 43 18             	mov    0x18(%rbx),%eax
  202a55:	88 65 08             	mov    %ah,0x8(%rbp)
  202a58:	0f b7 43 1a          	movzwl 0x1a(%rbx),%eax
  202a5c:	88 45 09             	mov    %al,0x9(%rbp)
  202a5f:	0f b6 43 1b          	movzbl 0x1b(%rbx),%eax
  202a63:	88 45 0a             	mov    %al,0xa(%rbp)
  202a66:	8b 53 18             	mov    0x18(%rbx),%edx
  202a69:	48 8b 73 20          	mov    0x20(%rbx),%rsi
  202a6d:	4c 89 ef             	mov    %r13,%rdi
  202a70:	e8 5b ec ff ff       	call   2016d0 <memmove>
  202a75:	8b 43 18             	mov    0x18(%rbx),%eax
  202a78:	4c 01 e8             	add    %r13,%rax
  202a7b:	48 29 e8             	sub    %rbp,%rax
  202a7e:	e9 62 fe ff ff       	jmp    2028e5 <convS2M+0x85>
  202a83:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  202a88:	8b 43 04             	mov    0x4(%rbx),%eax
  202a8b:	4c 8d 6d 17          	lea    0x17(%rbp),%r13
  202a8f:	88 45 07             	mov    %al,0x7(%rbp)
  202a92:	8b 43 04             	mov    0x4(%rbx),%eax
  202a95:	88 65 08             	mov    %ah,0x8(%rbp)
  202a98:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  202a9c:	88 45 09             	mov    %al,0x9(%rbp)
  202a9f:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  202aa3:	88 45 0a             	mov    %al,0xa(%rbp)
  202aa6:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202aaa:	88 45 0b             	mov    %al,0xb(%rbp)
  202aad:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202ab1:	88 65 0c             	mov    %ah,0xc(%rbp)
  202ab4:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202ab8:	48 c1 f8 10          	sar    $0x10,%rax
  202abc:	88 45 0d             	mov    %al,0xd(%rbp)
  202abf:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202ac3:	48 c1 f8 18          	sar    $0x18,%rax
  202ac7:	88 45 0e             	mov    %al,0xe(%rbp)
  202aca:	48 63 43 14          	movslq 0x14(%rbx),%rax
  202ace:	88 45 0f             	mov    %al,0xf(%rbp)
  202ad1:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202ad5:	48 c1 f8 28          	sar    $0x28,%rax
  202ad9:	88 45 10             	mov    %al,0x10(%rbp)
  202adc:	48 0f bf 43 16       	movswq 0x16(%rbx),%rax
  202ae1:	88 45 11             	mov    %al,0x11(%rbp)
  202ae4:	48 0f be 43 17       	movsbq 0x17(%rbx),%rax
  202ae9:	88 45 12             	mov    %al,0x12(%rbp)
  202aec:	8b 43 18             	mov    0x18(%rbx),%eax
  202aef:	88 45 13             	mov    %al,0x13(%rbp)
  202af2:	8b 43 18             	mov    0x18(%rbx),%eax
  202af5:	88 65 14             	mov    %ah,0x14(%rbp)
  202af8:	0f b7 43 1a          	movzwl 0x1a(%rbx),%eax
  202afc:	88 45 15             	mov    %al,0x15(%rbp)
  202aff:	0f b6 43 1b          	movzbl 0x1b(%rbx),%eax
  202b03:	88 45 16             	mov    %al,0x16(%rbp)
  202b06:	e9 5b ff ff ff       	jmp    202a66 <convS2M+0x206>
  202b0b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  202b10:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  202b14:	4c 8d 6d 09          	lea    0x9(%rbp),%r13
  202b18:	88 45 07             	mov    %al,0x7(%rbp)
  202b1b:	0f b6 43 11          	movzbl 0x11(%rbx),%eax
  202b1f:	88 45 08             	mov    %al,0x8(%rbp)
  202b22:	0f b7 53 10          	movzwl 0x10(%rbx),%edx
  202b26:	48 8b 73 18          	mov    0x18(%rbx),%rsi
  202b2a:	4c 89 ef             	mov    %r13,%rdi
  202b2d:	e8 9e eb ff ff       	call   2016d0 <memmove>
  202b32:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  202b36:	e9 3d ff ff ff       	jmp    202a78 <convS2M+0x218>
  202b3b:	8b 43 04             	mov    0x4(%rbx),%eax
  202b3e:	4c 8d 6d 0d          	lea    0xd(%rbp),%r13
  202b42:	88 45 07             	mov    %al,0x7(%rbp)
  202b45:	8b 43 04             	mov    0x4(%rbx),%eax
  202b48:	88 65 08             	mov    %ah,0x8(%rbp)
  202b4b:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  202b4f:	88 45 09             	mov    %al,0x9(%rbp)
  202b52:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  202b56:	88 45 0a             	mov    %al,0xa(%rbp)
  202b59:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  202b5d:	88 45 0b             	mov    %al,0xb(%rbp)
  202b60:	0f b6 43 11          	movzbl 0x11(%rbx),%eax
  202b64:	88 45 0c             	mov    %al,0xc(%rbp)
  202b67:	eb b9                	jmp    202b22 <convS2M+0x2c2>
  202b69:	48 8d 73 10          	lea    0x10(%rbx),%rsi
  202b6d:	4c 89 f7             	mov    %r14,%rdi
  202b70:	e8 eb f8 ff ff       	call   202460 <pqid>
  202b75:	48 29 e8             	sub    %rbp,%rax
  202b78:	e9 68 fd ff ff       	jmp    2028e5 <convS2M+0x85>
  202b7d:	4c 8b 6b 10          	mov    0x10(%rbx),%r13
  202b81:	4d 85 ed             	test   %r13,%r13
  202b84:	0f 85 bb fd ff ff    	jne    202945 <convS2M+0xe5>
  202b8a:	31 c0                	xor    %eax,%eax
  202b8c:	66 89 45 07          	mov    %ax,0x7(%rbp)
  202b90:	b8 09 00 00 00       	mov    $0x9,%eax
  202b95:	e9 4b fd ff ff       	jmp    2028e5 <convS2M+0x85>
  202b9a:	8b 43 04             	mov    0x4(%rbx),%eax
  202b9d:	4c 8d 6d 0d          	lea    0xd(%rbp),%r13
  202ba1:	88 45 07             	mov    %al,0x7(%rbp)
  202ba4:	8b 43 04             	mov    0x4(%rbx),%eax
  202ba7:	88 65 08             	mov    %ah,0x8(%rbp)
  202baa:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  202bae:	88 45 09             	mov    %al,0x9(%rbp)
  202bb1:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  202bb5:	88 45 0a             	mov    %al,0xa(%rbp)
  202bb8:	4c 8b 7b 18          	mov    0x18(%rbx),%r15
  202bbc:	4d 85 ff             	test   %r15,%r15
  202bbf:	0f 84 0f 09 00 00    	je     2034d4 <convS2M+0xc74>
  202bc5:	4c 89 ff             	mov    %r15,%rdi
  202bc8:	e8 a3 eb ff ff       	call   201770 <strlen>
  202bcd:	4c 89 ef             	mov    %r13,%rdi
  202bd0:	4c 89 fe             	mov    %r15,%rsi
  202bd3:	49 89 c6             	mov    %rax,%r14
  202bd6:	89 c2                	mov    %eax,%edx
  202bd8:	e8 f3 ea ff ff       	call   2016d0 <memmove>
  202bdd:	4c 89 f0             	mov    %r14,%rax
  202be0:	41 8d 4e 02          	lea    0x2(%r14),%ecx
  202be4:	44 89 f2             	mov    %r14d,%edx
  202be7:	0f b6 c4             	movzbl %ah,%eax
  202bea:	4c 8d 6c 0d 0b       	lea    0xb(%rbp,%rcx,1),%r13
  202bef:	88 55 0b             	mov    %dl,0xb(%rbp)
  202bf2:	88 45 0c             	mov    %al,0xc(%rbp)
  202bf5:	8b 43 10             	mov    0x10(%rbx),%eax
  202bf8:	41 88 45 00          	mov    %al,0x0(%r13)
  202bfc:	8b 43 10             	mov    0x10(%rbx),%eax
  202bff:	0f b6 c4             	movzbl %ah,%eax
  202c02:	41 88 45 01          	mov    %al,0x1(%r13)
  202c06:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  202c0a:	41 88 45 02          	mov    %al,0x2(%r13)
  202c0e:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202c12:	41 88 45 03          	mov    %al,0x3(%r13)
  202c16:	0f b6 43 20          	movzbl 0x20(%rbx),%eax
  202c1a:	41 88 45 04          	mov    %al,0x4(%r13)
  202c1e:	49 8d 45 05          	lea    0x5(%r13),%rax
  202c22:	48 29 e8             	sub    %rbp,%rax
  202c25:	e9 bb fc ff ff       	jmp    2028e5 <convS2M+0x85>
  202c2a:	8b 43 10             	mov    0x10(%rbx),%eax
  202c2d:	88 45 07             	mov    %al,0x7(%rbp)
  202c30:	8b 43 10             	mov    0x10(%rbx),%eax
  202c33:	88 65 08             	mov    %ah,0x8(%rbp)
  202c36:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  202c3a:	88 45 09             	mov    %al,0x9(%rbp)
  202c3d:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202c41:	88 45 0a             	mov    %al,0xa(%rbp)
  202c44:	4c 8b 6b 18          	mov    0x18(%rbx),%r13
  202c48:	4d 85 ed             	test   %r13,%r13
  202c4b:	0f 84 7b 01 00 00    	je     202dcc <convS2M+0x56c>
  202c51:	4c 89 ef             	mov    %r13,%rdi
  202c54:	e8 17 eb ff ff       	call   201770 <strlen>
  202c59:	48 8d 7d 0d          	lea    0xd(%rbp),%rdi
  202c5d:	4c 89 ee             	mov    %r13,%rsi
  202c60:	48 89 c3             	mov    %rax,%rbx
  202c63:	89 c2                	mov    %eax,%edx
  202c65:	e8 66 ea ff ff       	call   2016d0 <memmove>
  202c6a:	8d 43 02             	lea    0x2(%rbx),%eax
  202c6d:	66 89 5d 0b          	mov    %bx,0xb(%rbp)
  202c71:	48 83 c0 0b          	add    $0xb,%rax
  202c75:	e9 6b fc ff ff       	jmp    2028e5 <convS2M+0x85>
  202c7a:	48 8d 73 10          	lea    0x10(%rbx),%rsi
  202c7e:	4c 89 f7             	mov    %r14,%rdi
  202c81:	e8 da f7 ff ff       	call   202460 <pqid>
  202c86:	8b 53 28             	mov    0x28(%rbx),%edx
  202c89:	48 83 c0 04          	add    $0x4,%rax
  202c8d:	88 50 fc             	mov    %dl,-0x4(%rax)
  202c90:	8b 53 28             	mov    0x28(%rbx),%edx
  202c93:	88 70 fd             	mov    %dh,-0x3(%rax)
  202c96:	0f b7 53 2a          	movzwl 0x2a(%rbx),%edx
  202c9a:	88 50 fe             	mov    %dl,-0x2(%rax)
  202c9d:	0f b6 53 2b          	movzbl 0x2b(%rbx),%edx
  202ca1:	88 50 ff             	mov    %dl,-0x1(%rax)
  202ca4:	48 29 e8             	sub    %rbp,%rax
  202ca7:	e9 39 fc ff ff       	jmp    2028e5 <convS2M+0x85>
  202cac:	8b 43 04             	mov    0x4(%rbx),%eax
  202caf:	48 8d 7d 0b          	lea    0xb(%rbp),%rdi
  202cb3:	48 8d 73 10          	lea    0x10(%rbx),%rsi
  202cb7:	88 45 07             	mov    %al,0x7(%rbp)
  202cba:	8b 43 04             	mov    0x4(%rbx),%eax
  202cbd:	88 65 08             	mov    %ah,0x8(%rbp)
  202cc0:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  202cc4:	88 45 09             	mov    %al,0x9(%rbp)
  202cc7:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  202ccb:	88 45 0a             	mov    %al,0xa(%rbp)
  202cce:	eb b1                	jmp    202c81 <convS2M+0x421>
  202cd0:	8b 43 18             	mov    0x18(%rbx),%eax
  202cd3:	4c 8d 6d 11          	lea    0x11(%rbp),%r13
  202cd7:	88 45 07             	mov    %al,0x7(%rbp)
  202cda:	8b 43 18             	mov    0x18(%rbx),%eax
  202cdd:	88 65 08             	mov    %ah,0x8(%rbp)
  202ce0:	0f b7 43 1a          	movzwl 0x1a(%rbx),%eax
  202ce4:	88 45 09             	mov    %al,0x9(%rbp)
  202ce7:	0f b6 43 1b          	movzbl 0x1b(%rbx),%eax
  202ceb:	88 45 0a             	mov    %al,0xa(%rbp)
  202cee:	8b 43 10             	mov    0x10(%rbx),%eax
  202cf1:	88 45 0b             	mov    %al,0xb(%rbp)
  202cf4:	8b 43 10             	mov    0x10(%rbx),%eax
  202cf7:	88 65 0c             	mov    %ah,0xc(%rbp)
  202cfa:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  202cfe:	88 45 0d             	mov    %al,0xd(%rbp)
  202d01:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202d05:	88 45 0e             	mov    %al,0xe(%rbp)
  202d08:	4c 8b 7b 10          	mov    0x10(%rbx),%r15
  202d0c:	4d 85 ff             	test   %r15,%r15
  202d0f:	0f 84 ee 07 00 00    	je     203503 <convS2M+0xca3>
  202d15:	4c 89 ff             	mov    %r15,%rdi
  202d18:	e8 53 ea ff ff       	call   201770 <strlen>
  202d1d:	4c 89 ef             	mov    %r13,%rdi
  202d20:	4c 89 fe             	mov    %r15,%rsi
  202d23:	49 89 c6             	mov    %rax,%r14
  202d26:	89 c2                	mov    %eax,%edx
  202d28:	e8 a3 e9 ff ff       	call   2016d0 <memmove>
  202d2d:	4c 89 f0             	mov    %r14,%rax
  202d30:	41 8d 4e 02          	lea    0x2(%r14),%ecx
  202d34:	44 89 f2             	mov    %r14d,%edx
  202d37:	0f b6 c4             	movzbl %ah,%eax
  202d3a:	4c 8d 6c 0d 0f       	lea    0xf(%rbp,%rcx,1),%r13
  202d3f:	88 55 0f             	mov    %dl,0xf(%rbp)
  202d42:	88 45 10             	mov    %al,0x10(%rbp)
  202d45:	8b 43 10             	mov    0x10(%rbx),%eax
  202d48:	41 88 45 00          	mov    %al,0x0(%r13)
  202d4c:	8b 43 10             	mov    0x10(%rbx),%eax
  202d4f:	0f b6 c4             	movzbl %ah,%eax
  202d52:	41 88 45 01          	mov    %al,0x1(%r13)
  202d56:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  202d5a:	41 88 45 02          	mov    %al,0x2(%r13)
  202d5e:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202d62:	41 88 45 03          	mov    %al,0x3(%r13)
  202d66:	4c 8b 7b 20          	mov    0x20(%rbx),%r15
  202d6a:	49 8d 5d 06          	lea    0x6(%r13),%rbx
  202d6e:	4d 85 ff             	test   %r15,%r15
  202d71:	0f 84 7a 07 00 00    	je     2034f1 <convS2M+0xc91>
  202d77:	4c 89 ff             	mov    %r15,%rdi
  202d7a:	e8 f1 e9 ff ff       	call   201770 <strlen>
  202d7f:	4c 89 fe             	mov    %r15,%rsi
  202d82:	48 89 df             	mov    %rbx,%rdi
  202d85:	49 89 c6             	mov    %rax,%r14
  202d88:	89 c2                	mov    %eax,%edx
  202d8a:	e8 41 e9 ff ff       	call   2016d0 <memmove>
  202d8f:	41 8d 46 02          	lea    0x2(%r14),%eax
  202d93:	66 45 89 75 04       	mov    %r14w,0x4(%r13)
  202d98:	49 8d 44 05 04       	lea    0x4(%r13,%rax,1),%rax
  202d9d:	48 29 e8             	sub    %rbp,%rax
  202da0:	e9 40 fb ff ff       	jmp    2028e5 <convS2M+0x85>
  202da5:	8b 43 14             	mov    0x14(%rbx),%eax
  202da8:	88 45 07             	mov    %al,0x7(%rbp)
  202dab:	8b 43 14             	mov    0x14(%rbx),%eax
  202dae:	88 65 08             	mov    %ah,0x8(%rbp)
  202db1:	0f b7 43 16          	movzwl 0x16(%rbx),%eax
  202db5:	88 45 09             	mov    %al,0x9(%rbp)
  202db8:	0f b6 43 17          	movzbl 0x17(%rbx),%eax
  202dbc:	88 45 0a             	mov    %al,0xa(%rbp)
  202dbf:	4c 8b 6b 10          	mov    0x10(%rbx),%r13
  202dc3:	4d 85 ed             	test   %r13,%r13
  202dc6:	0f 85 85 fe ff ff    	jne    202c51 <convS2M+0x3f1>
  202dcc:	31 d2                	xor    %edx,%edx
  202dce:	b8 0d 00 00 00       	mov    $0xd,%eax
  202dd3:	66 89 55 0b          	mov    %dx,0xb(%rbp)
  202dd7:	e9 09 fb ff ff       	jmp    2028e5 <convS2M+0x85>
  202ddc:	48 8b 73 18          	mov    0x18(%rbx),%rsi
  202de0:	4c 8d 6d 09          	lea    0x9(%rbp),%r13
  202de4:	48 85 f6             	test   %rsi,%rsi
  202de7:	0f 84 55 07 00 00    	je     203542 <convS2M+0xce2>
  202ded:	48 89 f7             	mov    %rsi,%rdi
  202df0:	48 89 34 24          	mov    %rsi,(%rsp)
  202df4:	e8 77 e9 ff ff       	call   201770 <strlen>
  202df9:	48 8b 34 24          	mov    (%rsp),%rsi
  202dfd:	4c 89 ef             	mov    %r13,%rdi
  202e00:	49 89 c7             	mov    %rax,%r15
  202e03:	89 c2                	mov    %eax,%edx
  202e05:	e8 c6 e8 ff ff       	call   2016d0 <memmove>
  202e0a:	45 8d 6f 02          	lea    0x2(%r15),%r13d
  202e0e:	4c 89 f8             	mov    %r15,%rax
  202e11:	44 89 fa             	mov    %r15d,%edx
  202e14:	0f b6 c4             	movzbl %ah,%eax
  202e17:	4d 01 f5             	add    %r14,%r13
  202e1a:	88 55 07             	mov    %dl,0x7(%rbp)
  202e1d:	88 45 08             	mov    %al,0x8(%rbp)
  202e20:	4c 8b 7b 10          	mov    0x10(%rbx),%r15
  202e24:	49 8d 5d 02          	lea    0x2(%r13),%rbx
  202e28:	4d 85 ff             	test   %r15,%r15
  202e2b:	0f 84 cd 02 00 00    	je     2030fe <convS2M+0x89e>
  202e31:	4c 89 ff             	mov    %r15,%rdi
  202e34:	e8 37 e9 ff ff       	call   201770 <strlen>
  202e39:	4c 89 fe             	mov    %r15,%rsi
  202e3c:	48 89 df             	mov    %rbx,%rdi
  202e3f:	49 89 c6             	mov    %rax,%r14
  202e42:	89 c2                	mov    %eax,%edx
  202e44:	e8 87 e8 ff ff       	call   2016d0 <memmove>
  202e49:	41 8d 46 02          	lea    0x2(%r14),%eax
  202e4d:	66 45 89 75 00       	mov    %r14w,0x0(%r13)
  202e52:	4c 01 e8             	add    %r13,%rax
  202e55:	48 29 e8             	sub    %rbp,%rax
  202e58:	e9 88 fa ff ff       	jmp    2028e5 <convS2M+0x85>
  202e5d:	8b 43 04             	mov    0x4(%rbx),%eax
  202e60:	4c 8d 6d 0d          	lea    0xd(%rbp),%r13
  202e64:	88 45 07             	mov    %al,0x7(%rbp)
  202e67:	8b 43 04             	mov    0x4(%rbx),%eax
  202e6a:	88 65 08             	mov    %ah,0x8(%rbp)
  202e6d:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  202e71:	88 45 09             	mov    %al,0x9(%rbp)
  202e74:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  202e78:	88 45 0a             	mov    %al,0xa(%rbp)
  202e7b:	4c 8b 7b 18          	mov    0x18(%rbx),%r15
  202e7f:	4d 85 ff             	test   %r15,%r15
  202e82:	0f 84 a8 06 00 00    	je     203530 <convS2M+0xcd0>
  202e88:	4c 89 ff             	mov    %r15,%rdi
  202e8b:	e8 e0 e8 ff ff       	call   201770 <strlen>
  202e90:	4c 89 ef             	mov    %r13,%rdi
  202e93:	4c 89 fe             	mov    %r15,%rsi
  202e96:	49 89 c6             	mov    %rax,%r14
  202e99:	89 c2                	mov    %eax,%edx
  202e9b:	e8 30 e8 ff ff       	call   2016d0 <memmove>
  202ea0:	4c 89 f0             	mov    %r14,%rax
  202ea3:	41 8d 4e 02          	lea    0x2(%r14),%ecx
  202ea7:	44 89 f2             	mov    %r14d,%edx
  202eaa:	0f b6 c4             	movzbl %ah,%eax
  202ead:	4c 8d 6c 0d 0b       	lea    0xb(%rbp,%rcx,1),%r13
  202eb2:	88 55 0b             	mov    %dl,0xb(%rbp)
  202eb5:	88 45 0c             	mov    %al,0xc(%rbp)
  202eb8:	0f b6 43 20          	movzbl 0x20(%rbx),%eax
  202ebc:	41 88 45 00          	mov    %al,0x0(%r13)
  202ec0:	49 8d 45 01          	lea    0x1(%r13),%rax
  202ec4:	48 29 e8             	sub    %rbp,%rax
  202ec7:	e9 19 fa ff ff       	jmp    2028e5 <convS2M+0x85>
  202ecc:	8b 43 04             	mov    0x4(%rbx),%eax
  202ecf:	88 45 07             	mov    %al,0x7(%rbp)
  202ed2:	8b 43 04             	mov    0x4(%rbx),%eax
  202ed5:	88 65 08             	mov    %ah,0x8(%rbp)
  202ed8:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  202edc:	88 45 09             	mov    %al,0x9(%rbp)
  202edf:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  202ee3:	88 45 0a             	mov    %al,0xa(%rbp)
  202ee6:	0f b6 43 20          	movzbl 0x20(%rbx),%eax
  202eea:	88 45 0b             	mov    %al,0xb(%rbp)
  202eed:	b8 0c 00 00 00       	mov    $0xc,%eax
  202ef2:	e9 ee f9 ff ff       	jmp    2028e5 <convS2M+0x85>
  202ef7:	8b 43 10             	mov    0x10(%rbx),%eax
  202efa:	88 45 07             	mov    %al,0x7(%rbp)
  202efd:	8b 43 10             	mov    0x10(%rbx),%eax
  202f00:	88 65 08             	mov    %ah,0x8(%rbp)
  202f03:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  202f07:	88 45 09             	mov    %al,0x9(%rbp)
  202f0a:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202f0e:	e9 f4 f9 ff ff       	jmp    202907 <convS2M+0xa7>
  202f13:	8b 43 10             	mov    0x10(%rbx),%eax
  202f16:	88 45 07             	mov    %al,0x7(%rbp)
  202f19:	8b 43 10             	mov    0x10(%rbx),%eax
  202f1c:	88 65 08             	mov    %ah,0x8(%rbp)
  202f1f:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  202f23:	88 45 09             	mov    %al,0x9(%rbp)
  202f26:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202f2a:	88 45 0a             	mov    %al,0xa(%rbp)
  202f2d:	8b 43 14             	mov    0x14(%rbx),%eax
  202f30:	88 45 0b             	mov    %al,0xb(%rbp)
  202f33:	8b 43 14             	mov    0x14(%rbx),%eax
  202f36:	88 65 0c             	mov    %ah,0xc(%rbp)
  202f39:	0f b7 43 16          	movzwl 0x16(%rbx),%eax
  202f3d:	88 45 0d             	mov    %al,0xd(%rbp)
  202f40:	0f b6 43 17          	movzbl 0x17(%rbx),%eax
  202f44:	e9 67 fa ff ff       	jmp    2029b0 <convS2M+0x150>
  202f49:	8b 43 04             	mov    0x4(%rbx),%eax
  202f4c:	88 45 07             	mov    %al,0x7(%rbp)
  202f4f:	8b 43 04             	mov    0x4(%rbx),%eax
  202f52:	88 65 08             	mov    %ah,0x8(%rbp)
  202f55:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  202f59:	88 45 09             	mov    %al,0x9(%rbp)
  202f5c:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  202f60:	88 45 0a             	mov    %al,0xa(%rbp)
  202f63:	8b 43 10             	mov    0x10(%rbx),%eax
  202f66:	88 45 0b             	mov    %al,0xb(%rbp)
  202f69:	8b 43 10             	mov    0x10(%rbx),%eax
  202f6c:	88 65 0c             	mov    %ah,0xc(%rbp)
  202f6f:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  202f73:	88 45 0d             	mov    %al,0xd(%rbp)
  202f76:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202f7a:	e9 31 fa ff ff       	jmp    2029b0 <convS2M+0x150>
  202f7f:	48 8b 73 18          	mov    0x18(%rbx),%rsi
  202f83:	4c 8d 7d 09          	lea    0x9(%rbp),%r15
  202f87:	48 85 f6             	test   %rsi,%rsi
  202f8a:	0f 84 85 05 00 00    	je     203515 <convS2M+0xcb5>
  202f90:	48 89 f7             	mov    %rsi,%rdi
  202f93:	48 89 34 24          	mov    %rsi,(%rsp)
  202f97:	e8 d4 e7 ff ff       	call   201770 <strlen>
  202f9c:	48 8b 34 24          	mov    (%rsp),%rsi
  202fa0:	4c 89 ff             	mov    %r15,%rdi
  202fa3:	49 89 c5             	mov    %rax,%r13
  202fa6:	89 c2                	mov    %eax,%edx
  202fa8:	e8 23 e7 ff ff       	call   2016d0 <memmove>
  202fad:	45 8d 7d 02          	lea    0x2(%r13),%r15d
  202fb1:	4c 89 e8             	mov    %r13,%rax
  202fb4:	44 89 ea             	mov    %r13d,%edx
  202fb7:	0f b6 c4             	movzbl %ah,%eax
  202fba:	4d 01 f7             	add    %r14,%r15
  202fbd:	88 55 07             	mov    %dl,0x7(%rbp)
  202fc0:	4d 8d 6f 02          	lea    0x2(%r15),%r13
  202fc4:	88 45 08             	mov    %al,0x8(%rbp)
  202fc7:	48 8b 73 10          	mov    0x10(%rbx),%rsi
  202fcb:	48 85 f6             	test   %rsi,%rsi
  202fce:	0f 84 38 05 00 00    	je     20350c <convS2M+0xcac>
  202fd4:	48 89 f7             	mov    %rsi,%rdi
  202fd7:	48 89 34 24          	mov    %rsi,(%rsp)
  202fdb:	e8 90 e7 ff ff       	call   201770 <strlen>
  202fe0:	48 8b 34 24          	mov    (%rsp),%rsi
  202fe4:	4c 89 ef             	mov    %r13,%rdi
  202fe7:	49 89 c6             	mov    %rax,%r14
  202fea:	89 c2                	mov    %eax,%edx
  202fec:	e8 df e6 ff ff       	call   2016d0 <memmove>
  202ff1:	45 8d 6e 02          	lea    0x2(%r14),%r13d
  202ff5:	4c 89 f0             	mov    %r14,%rax
  202ff8:	44 89 f2             	mov    %r14d,%edx
  202ffb:	0f b6 c4             	movzbl %ah,%eax
  202ffe:	4d 01 fd             	add    %r15,%r13
  203001:	41 88 17             	mov    %dl,(%r15)
  203004:	41 88 47 01          	mov    %al,0x1(%r15)
  203008:	8b 43 10             	mov    0x10(%rbx),%eax
  20300b:	41 88 45 00          	mov    %al,0x0(%r13)
  20300f:	8b 43 10             	mov    0x10(%rbx),%eax
  203012:	0f b6 c4             	movzbl %ah,%eax
  203015:	41 88 45 01          	mov    %al,0x1(%r13)
  203019:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  20301d:	41 88 45 02          	mov    %al,0x2(%r13)
  203021:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  203025:	41 88 45 03          	mov    %al,0x3(%r13)
  203029:	49 8d 45 04          	lea    0x4(%r13),%rax
  20302d:	48 29 e8             	sub    %rbp,%rax
  203030:	e9 b0 f8 ff ff       	jmp    2028e5 <convS2M+0x85>
  203035:	48 8b 73 18          	mov    0x18(%rbx),%rsi
  203039:	4c 8d 6d 09          	lea    0x9(%rbp),%r13
  20303d:	48 85 f6             	test   %rsi,%rsi
  203040:	0f 84 f3 04 00 00    	je     203539 <convS2M+0xcd9>
  203046:	48 89 f7             	mov    %rsi,%rdi
  203049:	48 89 34 24          	mov    %rsi,(%rsp)
  20304d:	e8 1e e7 ff ff       	call   201770 <strlen>
  203052:	48 8b 34 24          	mov    (%rsp),%rsi
  203056:	4c 89 ef             	mov    %r13,%rdi
  203059:	49 89 c7             	mov    %rax,%r15
  20305c:	89 c2                	mov    %eax,%edx
  20305e:	e8 6d e6 ff ff       	call   2016d0 <memmove>
  203063:	45 8d 6f 02          	lea    0x2(%r15),%r13d
  203067:	4c 89 f8             	mov    %r15,%rax
  20306a:	44 89 fa             	mov    %r15d,%edx
  20306d:	0f b6 c4             	movzbl %ah,%eax
  203070:	4d 01 f5             	add    %r14,%r13
  203073:	88 55 07             	mov    %dl,0x7(%rbp)
  203076:	49 83 c5 02          	add    $0x2,%r13
  20307a:	88 45 08             	mov    %al,0x8(%rbp)
  20307d:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  203081:	41 88 45 fe          	mov    %al,-0x2(%r13)
  203085:	0f b6 43 11          	movzbl 0x11(%rbx),%eax
  203089:	41 88 45 ff          	mov    %al,-0x1(%r13)
  20308d:	e9 90 fa ff ff       	jmp    202b22 <convS2M+0x2c2>
  203092:	8b 43 10             	mov    0x10(%rbx),%eax
  203095:	4c 8d 6d 0d          	lea    0xd(%rbp),%r13
  203099:	88 45 07             	mov    %al,0x7(%rbp)
  20309c:	8b 43 10             	mov    0x10(%rbx),%eax
  20309f:	88 65 08             	mov    %ah,0x8(%rbp)
  2030a2:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  2030a6:	88 45 09             	mov    %al,0x9(%rbp)
  2030a9:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  2030ad:	88 45 0a             	mov    %al,0xa(%rbp)
  2030b0:	4c 8b 7b 18          	mov    0x18(%rbx),%r15
  2030b4:	4d 85 ff             	test   %r15,%r15
  2030b7:	0f 84 2b 04 00 00    	je     2034e8 <convS2M+0xc88>
  2030bd:	4c 89 ff             	mov    %r15,%rdi
  2030c0:	e8 ab e6 ff ff       	call   201770 <strlen>
  2030c5:	4c 89 ef             	mov    %r13,%rdi
  2030c8:	4c 89 fe             	mov    %r15,%rsi
  2030cb:	49 89 c6             	mov    %rax,%r14
  2030ce:	89 c2                	mov    %eax,%edx
  2030d0:	e8 fb e5 ff ff       	call   2016d0 <memmove>
  2030d5:	4c 89 f0             	mov    %r14,%rax
  2030d8:	41 8d 4e 02          	lea    0x2(%r14),%ecx
  2030dc:	44 89 f2             	mov    %r14d,%edx
  2030df:	0f b6 c4             	movzbl %ah,%eax
  2030e2:	4c 8d 6c 0d 0b       	lea    0xb(%rbp,%rcx,1),%r13
  2030e7:	88 55 0b             	mov    %dl,0xb(%rbp)
  2030ea:	88 45 0c             	mov    %al,0xc(%rbp)
  2030ed:	4c 8b 7b 20          	mov    0x20(%rbx),%r15
  2030f1:	49 8d 5d 02          	lea    0x2(%r13),%rbx
  2030f5:	4d 85 ff             	test   %r15,%r15
  2030f8:	0f 85 33 fd ff ff    	jne    202e31 <convS2M+0x5d1>
  2030fe:	31 c9                	xor    %ecx,%ecx
  203100:	48 89 d8             	mov    %rbx,%rax
  203103:	66 41 89 4d 00       	mov    %cx,0x0(%r13)
  203108:	48 29 e8             	sub    %rbp,%rax
  20310b:	e9 d5 f7 ff ff       	jmp    2028e5 <convS2M+0x85>
  203110:	8b 43 04             	mov    0x4(%rbx),%eax
  203113:	88 45 07             	mov    %al,0x7(%rbp)
  203116:	8b 43 04             	mov    0x4(%rbx),%eax
  203119:	88 65 08             	mov    %ah,0x8(%rbp)
  20311c:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  203120:	88 45 09             	mov    %al,0x9(%rbp)
  203123:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  203127:	88 45 0a             	mov    %al,0xa(%rbp)
  20312a:	48 8b 43 10          	mov    0x10(%rbx),%rax
  20312e:	88 45 0b             	mov    %al,0xb(%rbp)
  203131:	48 8b 43 10          	mov    0x10(%rbx),%rax
  203135:	88 65 0c             	mov    %ah,0xc(%rbp)
  203138:	48 8b 43 10          	mov    0x10(%rbx),%rax
  20313c:	48 c1 f8 10          	sar    $0x10,%rax
  203140:	88 45 0d             	mov    %al,0xd(%rbp)
  203143:	48 8b 43 10          	mov    0x10(%rbx),%rax
  203147:	48 c1 f8 18          	sar    $0x18,%rax
  20314b:	88 45 0e             	mov    %al,0xe(%rbp)
  20314e:	48 63 43 14          	movslq 0x14(%rbx),%rax
  203152:	88 45 0f             	mov    %al,0xf(%rbp)
  203155:	48 8b 43 10          	mov    0x10(%rbx),%rax
  203159:	48 c1 f8 28          	sar    $0x28,%rax
  20315d:	88 45 10             	mov    %al,0x10(%rbp)
  203160:	48 0f bf 43 16       	movswq 0x16(%rbx),%rax
  203165:	88 45 11             	mov    %al,0x11(%rbp)
  203168:	48 0f be 43 17       	movsbq 0x17(%rbx),%rax
  20316d:	88 45 12             	mov    %al,0x12(%rbp)
  203170:	8b 43 10             	mov    0x10(%rbx),%eax
  203173:	88 45 13             	mov    %al,0x13(%rbp)
  203176:	8b 43 10             	mov    0x10(%rbx),%eax
  203179:	88 65 14             	mov    %ah,0x14(%rbp)
  20317c:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  203180:	88 45 15             	mov    %al,0x15(%rbp)
  203183:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  203187:	e9 ab f8 ff ff       	jmp    202a37 <convS2M+0x1d7>
  20318c:	8b 43 04             	mov    0x4(%rbx),%eax
  20318f:	88 45 07             	mov    %al,0x7(%rbp)
  203192:	8b 43 04             	mov    0x4(%rbx),%eax
  203195:	88 65 08             	mov    %ah,0x8(%rbp)
  203198:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  20319c:	88 45 09             	mov    %al,0x9(%rbp)
  20319f:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  2031a3:	88 45 0a             	mov    %al,0xa(%rbp)
  2031a6:	8b 43 10             	mov    0x10(%rbx),%eax
  2031a9:	88 45 0b             	mov    %al,0xb(%rbp)
  2031ac:	8b 43 10             	mov    0x10(%rbx),%eax
  2031af:	88 65 0c             	mov    %ah,0xc(%rbp)
  2031b2:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  2031b6:	88 45 0d             	mov    %al,0xd(%rbp)
  2031b9:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  2031bd:	88 45 0e             	mov    %al,0xe(%rbp)
  2031c0:	0f b7 43 14          	movzwl 0x14(%rbx),%eax
  2031c4:	88 45 0f             	mov    %al,0xf(%rbp)
  2031c7:	0f b6 43 15          	movzbl 0x15(%rbx),%eax
  2031cb:	88 45 10             	mov    %al,0x10(%rbp)
  2031ce:	0f b7 43 14          	movzwl 0x14(%rbx),%eax
  2031d2:	66 83 f8 10          	cmp    $0x10,%ax
  2031d6:	0f 87 af f6 ff ff    	ja     20288b <convS2M+0x2b>
  2031dc:	4c 8d 7d 11          	lea    0x11(%rbp),%r15
  2031e0:	66 85 c0             	test   %ax,%ax
  2031e3:	0f 84 62 03 00 00    	je     20354b <convS2M+0xceb>
  2031e9:	31 c9                	xor    %ecx,%ecx
  2031eb:	eb 52                	jmp    20323f <convS2M+0x9df>
  2031ed:	0f 1f 00             	nopl   (%rax)
  2031f0:	48 89 f7             	mov    %rsi,%rdi
  2031f3:	89 4c 24 0c          	mov    %ecx,0xc(%rsp)
  2031f7:	48 89 34 24          	mov    %rsi,(%rsp)
  2031fb:	e8 70 e5 ff ff       	call   201770 <strlen>
  203200:	48 8b 34 24          	mov    (%rsp),%rsi
  203204:	4c 89 ef             	mov    %r13,%rdi
  203207:	49 89 c6             	mov    %rax,%r14
  20320a:	89 c2                	mov    %eax,%edx
  20320c:	e8 bf e4 ff ff       	call   2016d0 <memmove>
  203211:	4c 89 f0             	mov    %r14,%rax
  203214:	8b 4c 24 0c          	mov    0xc(%rsp),%ecx
  203218:	44 89 f6             	mov    %r14d,%esi
  20321b:	0f b6 d4             	movzbl %ah,%edx
  20321e:	41 8d 46 02          	lea    0x2(%r14),%eax
  203222:	4d 8d 2c 07          	lea    (%r15,%rax,1),%r13
  203226:	41 88 37             	mov    %sil,(%r15)
  203229:	83 c1 01             	add    $0x1,%ecx
  20322c:	41 88 57 01          	mov    %dl,0x1(%r15)
  203230:	0f b7 43 14          	movzwl 0x14(%rbx),%eax
  203234:	39 c1                	cmp    %eax,%ecx
  203236:	0f 83 a1 02 00 00    	jae    2034dd <convS2M+0xc7d>
  20323c:	4d 89 ef             	mov    %r13,%r15
  20323f:	89 c8                	mov    %ecx,%eax
  203241:	4d 8d 6f 02          	lea    0x2(%r15),%r13
  203245:	48 8b 74 c3 18       	mov    0x18(%rbx,%rax,8),%rsi
  20324a:	48 85 f6             	test   %rsi,%rsi
  20324d:	75 a1                	jne    2031f0 <convS2M+0x990>
  20324f:	31 f6                	xor    %esi,%esi
  203251:	31 d2                	xor    %edx,%edx
  203253:	eb d1                	jmp    203226 <convS2M+0x9c6>
  203255:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  203259:	88 45 07             	mov    %al,0x7(%rbp)
  20325c:	0f b6 43 11          	movzbl 0x11(%rbx),%eax
  203260:	88 45 08             	mov    %al,0x8(%rbp)
  203263:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  203267:	66 83 f8 10          	cmp    $0x10,%ax
  20326b:	0f 87 1a f6 ff ff    	ja     20288b <convS2M+0x2b>
  203271:	48 8d 7d 09          	lea    0x9(%rbp),%rdi
  203275:	66 85 c0             	test   %ax,%ax
  203278:	0f 84 12 f9 ff ff    	je     202b90 <convS2M+0x330>
  20327e:	48 8d 73 18          	lea    0x18(%rbx),%rsi
  203282:	31 c9                	xor    %ecx,%ecx
  203284:	0f 1f 40 00          	nopl   0x0(%rax)
  203288:	e8 d3 f1 ff ff       	call   202460 <pqid>
  20328d:	83 c1 01             	add    $0x1,%ecx
  203290:	48 83 c6 18          	add    $0x18,%rsi
  203294:	48 89 c7             	mov    %rax,%rdi
  203297:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  20329b:	39 c1                	cmp    %eax,%ecx
  20329d:	72 e9                	jb     203288 <convS2M+0xa28>
  20329f:	48 89 f8             	mov    %rdi,%rax
  2032a2:	48 29 e8             	sub    %rbp,%rax
  2032a5:	e9 3b f6 ff ff       	jmp    2028e5 <convS2M+0x85>
  2032aa:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  2032ae:	88 45 07             	mov    %al,0x7(%rbp)
  2032b1:	0f b6 43 11          	movzbl 0x11(%rbx),%eax
  2032b5:	88 45 08             	mov    %al,0x8(%rbp)
  2032b8:	e9 d3 f8 ff ff       	jmp    202b90 <convS2M+0x330>
  2032bd:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2032c1:	88 45 07             	mov    %al,0x7(%rbp)
  2032c4:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2032c8:	88 65 08             	mov    %ah,0x8(%rbp)
  2032cb:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2032cf:	48 c1 f8 10          	sar    $0x10,%rax
  2032d3:	88 45 09             	mov    %al,0x9(%rbp)
  2032d6:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2032da:	48 c1 f8 18          	sar    $0x18,%rax
  2032de:	88 45 0a             	mov    %al,0xa(%rbp)
  2032e1:	48 63 43 14          	movslq 0x14(%rbx),%rax
  2032e5:	88 45 0b             	mov    %al,0xb(%rbp)
  2032e8:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2032ec:	48 c1 f8 28          	sar    $0x28,%rax
  2032f0:	88 45 0c             	mov    %al,0xc(%rbp)
  2032f3:	48 0f bf 43 16       	movswq 0x16(%rbx),%rax
  2032f8:	88 45 0d             	mov    %al,0xd(%rbp)
  2032fb:	0f b6 43 17          	movzbl 0x17(%rbx),%eax
  2032ff:	e9 ac f6 ff ff       	jmp    2029b0 <convS2M+0x150>
  203304:	48 8b 73 18          	mov    0x18(%rbx),%rsi
  203308:	4c 8d 6d 09          	lea    0x9(%rbp),%r13
  20330c:	48 85 f6             	test   %rsi,%rsi
  20330f:	0f 84 09 02 00 00    	je     20351e <convS2M+0xcbe>
  203315:	48 89 f7             	mov    %rsi,%rdi
  203318:	48 89 34 24          	mov    %rsi,(%rsp)
  20331c:	e8 4f e4 ff ff       	call   201770 <strlen>
  203321:	48 8b 34 24          	mov    (%rsp),%rsi
  203325:	4c 89 ef             	mov    %r13,%rdi
  203328:	49 89 c7             	mov    %rax,%r15
  20332b:	89 c2                	mov    %eax,%edx
  20332d:	e8 9e e3 ff ff       	call   2016d0 <memmove>
  203332:	45 8d 6f 02          	lea    0x2(%r15),%r13d
  203336:	4c 89 f8             	mov    %r15,%rax
  203339:	44 89 fa             	mov    %r15d,%edx
  20333c:	0f b6 c4             	movzbl %ah,%eax
  20333f:	4d 01 f5             	add    %r14,%r13
  203342:	88 55 07             	mov    %dl,0x7(%rbp)
  203345:	88 45 08             	mov    %al,0x8(%rbp)
  203348:	8b 43 18             	mov    0x18(%rbx),%eax
  20334b:	41 88 45 00          	mov    %al,0x0(%r13)
  20334f:	8b 43 18             	mov    0x18(%rbx),%eax
  203352:	0f b6 c4             	movzbl %ah,%eax
  203355:	41 88 45 01          	mov    %al,0x1(%r13)
  203359:	0f b7 43 1a          	movzwl 0x1a(%rbx),%eax
  20335d:	41 88 45 02          	mov    %al,0x2(%r13)
  203361:	0f b6 43 1b          	movzbl 0x1b(%rbx),%eax
  203365:	41 88 45 03          	mov    %al,0x3(%r13)
  203369:	49 8d 45 04          	lea    0x4(%r13),%rax
  20336d:	48 29 e8             	sub    %rbp,%rax
  203370:	e9 70 f5 ff ff       	jmp    2028e5 <convS2M+0x85>
  203375:	8b 43 04             	mov    0x4(%rbx),%eax
  203378:	4c 8d 6d 11          	lea    0x11(%rbp),%r13
  20337c:	88 45 07             	mov    %al,0x7(%rbp)
  20337f:	8b 43 04             	mov    0x4(%rbx),%eax
  203382:	88 65 08             	mov    %ah,0x8(%rbp)
  203385:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  203389:	88 45 09             	mov    %al,0x9(%rbp)
  20338c:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  203390:	88 45 0a             	mov    %al,0xa(%rbp)
  203393:	8b 43 10             	mov    0x10(%rbx),%eax
  203396:	88 45 0b             	mov    %al,0xb(%rbp)
  203399:	8b 43 10             	mov    0x10(%rbx),%eax
  20339c:	88 65 0c             	mov    %ah,0xc(%rbp)
  20339f:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  2033a3:	88 45 0d             	mov    %al,0xd(%rbp)
  2033a6:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  2033aa:	88 45 0e             	mov    %al,0xe(%rbp)
  2033ad:	4c 8b 7b 18          	mov    0x18(%rbx),%r15
  2033b1:	4d 85 ff             	test   %r15,%r15
  2033b4:	0f 84 6d 01 00 00    	je     203527 <convS2M+0xcc7>
  2033ba:	4c 89 ff             	mov    %r15,%rdi
  2033bd:	e8 ae e3 ff ff       	call   201770 <strlen>
  2033c2:	4c 89 ef             	mov    %r13,%rdi
  2033c5:	4c 89 fe             	mov    %r15,%rsi
  2033c8:	49 89 c6             	mov    %rax,%r14
  2033cb:	89 c2                	mov    %eax,%edx
  2033cd:	e8 fe e2 ff ff       	call   2016d0 <memmove>
  2033d2:	4c 89 f0             	mov    %r14,%rax
  2033d5:	41 8d 4e 02          	lea    0x2(%r14),%ecx
  2033d9:	44 89 f2             	mov    %r14d,%edx
  2033dc:	0f b6 c4             	movzbl %ah,%eax
  2033df:	4c 8d 6c 0d 0f       	lea    0xf(%rbp,%rcx,1),%r13
  2033e4:	88 55 0f             	mov    %dl,0xf(%rbp)
  2033e7:	88 45 10             	mov    %al,0x10(%rbp)
  2033ea:	4c 8b 7b 20          	mov    0x20(%rbx),%r15
  2033ee:	49 8d 5d 02          	lea    0x2(%r13),%rbx
  2033f2:	4d 85 ff             	test   %r15,%r15
  2033f5:	0f 85 36 fa ff ff    	jne    202e31 <convS2M+0x5d1>
  2033fb:	e9 fe fc ff ff       	jmp    2030fe <convS2M+0x89e>
  203400:	8b 43 10             	mov    0x10(%rbx),%eax
  203403:	88 45 07             	mov    %al,0x7(%rbp)
  203406:	8b 43 10             	mov    0x10(%rbx),%eax
  203409:	88 65 08             	mov    %ah,0x8(%rbp)
  20340c:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  203410:	88 45 09             	mov    %al,0x9(%rbp)
  203413:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  203417:	88 45 0a             	mov    %al,0xa(%rbp)
  20341a:	8b 43 14             	mov    0x14(%rbx),%eax
  20341d:	88 45 0b             	mov    %al,0xb(%rbp)
  203420:	8b 43 14             	mov    0x14(%rbx),%eax
  203423:	88 65 0c             	mov    %ah,0xc(%rbp)
  203426:	0f b7 43 16          	movzwl 0x16(%rbx),%eax
  20342a:	88 45 0d             	mov    %al,0xd(%rbp)
  20342d:	0f b6 43 17          	movzbl 0x17(%rbx),%eax
  203431:	88 45 0e             	mov    %al,0xe(%rbp)
  203434:	8b 43 20             	mov    0x20(%rbx),%eax
  203437:	4c 8d 6d 13          	lea    0x13(%rbp),%r13
  20343b:	88 45 0f             	mov    %al,0xf(%rbp)
  20343e:	8b 43 20             	mov    0x20(%rbx),%eax
  203441:	88 65 10             	mov    %ah,0x10(%rbp)
  203444:	0f b7 43 22          	movzwl 0x22(%rbx),%eax
  203448:	88 45 11             	mov    %al,0x11(%rbp)
  20344b:	0f b6 43 23          	movzbl 0x23(%rbx),%eax
  20344f:	88 45 12             	mov    %al,0x12(%rbp)
  203452:	8b 43 20             	mov    0x20(%rbx),%eax
  203455:	85 c0                	test   %eax,%eax
  203457:	0f 84 1b f6 ff ff    	je     202a78 <convS2M+0x218>
  20345d:	48 8b 73 18          	mov    0x18(%rbx),%rsi
  203461:	89 c2                	mov    %eax,%edx
  203463:	4c 89 ef             	mov    %r13,%rdi
  203466:	e8 65 e2 ff ff       	call   2016d0 <memmove>
  20346b:	8b 43 20             	mov    0x20(%rbx),%eax
  20346e:	e9 05 f6 ff ff       	jmp    202a78 <convS2M+0x218>
  203473:	8b 43 14             	mov    0x14(%rbx),%eax
  203476:	88 45 07             	mov    %al,0x7(%rbp)
  203479:	8b 43 14             	mov    0x14(%rbx),%eax
  20347c:	88 65 08             	mov    %ah,0x8(%rbp)
  20347f:	0f b7 43 16          	movzwl 0x16(%rbx),%eax
  203483:	88 45 09             	mov    %al,0x9(%rbp)
  203486:	0f b6 43 17          	movzbl 0x17(%rbx),%eax
  20348a:	e9 78 f4 ff ff       	jmp    202907 <convS2M+0xa7>
  20348f:	48 8b 43 28          	mov    0x28(%rbx),%rax
  203493:	88 45 07             	mov    %al,0x7(%rbp)
  203496:	48 8b 43 28          	mov    0x28(%rbx),%rax
  20349a:	88 65 08             	mov    %ah,0x8(%rbp)
  20349d:	48 8b 43 28          	mov    0x28(%rbx),%rax
  2034a1:	48 c1 e8 10          	shr    $0x10,%rax
  2034a5:	88 45 09             	mov    %al,0x9(%rbp)
  2034a8:	48 8b 43 28          	mov    0x28(%rbx),%rax
  2034ac:	48 c1 e8 18          	shr    $0x18,%rax
  2034b0:	88 45 0a             	mov    %al,0xa(%rbp)
  2034b3:	8b 43 2c             	mov    0x2c(%rbx),%eax
  2034b6:	88 45 0b             	mov    %al,0xb(%rbp)
  2034b9:	48 8b 43 28          	mov    0x28(%rbx),%rax
  2034bd:	48 c1 e8 28          	shr    $0x28,%rax
  2034c1:	88 45 0c             	mov    %al,0xc(%rbp)
  2034c4:	0f b7 43 2e          	movzwl 0x2e(%rbx),%eax
  2034c8:	88 45 0d             	mov    %al,0xd(%rbp)
  2034cb:	0f b6 43 2f          	movzbl 0x2f(%rbx),%eax
  2034cf:	e9 5d ff ff ff       	jmp    203431 <convS2M+0xbd1>
  2034d4:	31 d2                	xor    %edx,%edx
  2034d6:	31 c0                	xor    %eax,%eax
  2034d8:	e9 12 f7 ff ff       	jmp    202bef <convS2M+0x38f>
  2034dd:	4c 89 e8             	mov    %r13,%rax
  2034e0:	48 29 e8             	sub    %rbp,%rax
  2034e3:	e9 fd f3 ff ff       	jmp    2028e5 <convS2M+0x85>
  2034e8:	31 d2                	xor    %edx,%edx
  2034ea:	31 c0                	xor    %eax,%eax
  2034ec:	e9 f6 fb ff ff       	jmp    2030e7 <convS2M+0x887>
  2034f1:	31 f6                	xor    %esi,%esi
  2034f3:	48 89 d8             	mov    %rbx,%rax
  2034f6:	66 41 89 75 04       	mov    %si,0x4(%r13)
  2034fb:	48 29 e8             	sub    %rbp,%rax
  2034fe:	e9 e2 f3 ff ff       	jmp    2028e5 <convS2M+0x85>
  203503:	31 d2                	xor    %edx,%edx
  203505:	31 c0                	xor    %eax,%eax
  203507:	e9 33 f8 ff ff       	jmp    202d3f <convS2M+0x4df>
  20350c:	31 d2                	xor    %edx,%edx
  20350e:	31 c0                	xor    %eax,%eax
  203510:	e9 ec fa ff ff       	jmp    203001 <convS2M+0x7a1>
  203515:	31 d2                	xor    %edx,%edx
  203517:	31 c0                	xor    %eax,%eax
  203519:	e9 9f fa ff ff       	jmp    202fbd <convS2M+0x75d>
  20351e:	31 d2                	xor    %edx,%edx
  203520:	31 c0                	xor    %eax,%eax
  203522:	e9 1b fe ff ff       	jmp    203342 <convS2M+0xae2>
  203527:	31 d2                	xor    %edx,%edx
  203529:	31 c0                	xor    %eax,%eax
  20352b:	e9 b4 fe ff ff       	jmp    2033e4 <convS2M+0xb84>
  203530:	31 d2                	xor    %edx,%edx
  203532:	31 c0                	xor    %eax,%eax
  203534:	e9 79 f9 ff ff       	jmp    202eb2 <convS2M+0x652>
  203539:	31 d2                	xor    %edx,%edx
  20353b:	31 c0                	xor    %eax,%eax
  20353d:	e9 31 fb ff ff       	jmp    203073 <convS2M+0x813>
  203542:	31 d2                	xor    %edx,%edx
  203544:	31 c0                	xor    %eax,%eax
  203546:	e9 cf f8 ff ff       	jmp    202e1a <convS2M+0x5ba>
  20354b:	b8 11 00 00 00       	mov    $0x11,%eax
  203550:	e9 90 f3 ff ff       	jmp    2028e5 <convS2M+0x85>

0000000000203555 <_syscall>:
  203555:	0f 05                	syscall
  203557:	c3                   	ret
  203558:	69 6e 69 74 3a 20 66 	imul   $0x66203a74,0x69(%rsi),%ebp
  20355f:	61                   	(bad)
  203560:	69 6c 65 64 20 74 6f 	imul   $0x206f7420,0x64(%rbp,%riz,2),%ebp
  203567:	20 
  203568:	63 72 65             	movsxd 0x65(%rdx),%esi
  20356b:	61                   	(bad)
  20356c:	74 65                	je     2035d3 <_syscall+0x7e>
  20356e:	20 00                	and    %al,(%rax)
  203570:	3d 3d 3d 20 4c       	cmp    $0x4c203d3d,%eax
  203575:	75 78                	jne    2035ef <_syscall+0x9a>
  203577:	39 20                	cmp    %esp,(%rax)
  203579:	49 6e                	rex.WB outsb %ds:(%rsi),(%dx)
  20357b:	69 74 20 53 74 61 72 	imul   $0x74726174,0x53(%rax,%riz,1),%esi
  203582:	74 
  203583:	69 6e 67 20 3d 3d 3d 	imul   $0x3d3d3d20,0x67(%rsi),%ebp
  20358a:	0a 00                	or     (%rax),%al
  20358c:	23 2f                	and    (%rdi),%ebp
  20358e:	2e 2f                	cs (bad)
  203590:	62 6f 6f 74 2f       	(bad)
  203595:	72 65                	jb     2035fc <_syscall+0xa7>
  203597:	73 75                	jae    20360e <_syscall+0xb9>
  203599:	72 72                	jb     20360d <_syscall+0xb8>
  20359b:	65 63 74 69 6f       	movsxd %gs:0x6f(%rcx,%rbp,2),%esi
  2035a0:	6e                   	outsb  %ds:(%rsi),(%dx)
  2035a1:	00 69 6e             	add    %ch,0x6e(%rcx)
  2035a4:	69 74 3a 20 66 6f 72 	imul   $0x6b726f66,0x20(%rdx,%rdi,1),%esi
  2035ab:	6b 
  2035ac:	20 66 61             	and    %ah,0x61(%rsi)
  2035af:	69 6c 65 64 0a 00 66 	imul   $0x6f66000a,0x64(%rbp,%riz,2),%ebp
  2035b6:	6f 
  2035b7:	72 6b                	jb     203624 <_syscall+0xcf>
  2035b9:	20 66 61             	and    %ah,0x61(%rsi)
  2035bc:	69 6c 65 64 00 29 0a 	imul   $0xa2900,0x64(%rbp,%riz,2),%ebp
  2035c3:	00 
  2035c4:	2f                   	(bad)
  2035c5:	62 6f 6f 74 2f       	(bad)
  2035ca:	72 75                	jb     203641 <_syscall+0xec>
  2035cc:	6d                   	insl   (%dx),%es:(%rdi)
  2035cd:	70 5f                	jo     20362e <_syscall+0xd9>
  2035cf:	73 65                	jae    203636 <_syscall+0xe1>
  2035d1:	72 76                	jb     203649 <_syscall+0xf4>
  2035d3:	65 72 00             	gs jb  2035d6 <_syscall+0x81>
  2035d6:	2f                   	(bad)
  2035d7:	62 6f 6f 74 2f       	(bad)
  2035dc:	74 75                	je     203653 <_syscall+0xfe>
  2035de:	72 62                	jb     203642 <_syscall+0xed>
  2035e0:	6f                   	outsl  %ds:(%rsi),(%dx)
  2035e1:	63 69 64             	movsxd 0x64(%rcx),%ebp
  2035e4:	00 2f                	add    %ch,(%rdi)
  2035e6:	62 6f 6f 74 2f       	(bad)
  2035eb:	77 61                	ja     20364e <_syscall+0xf9>
  2035ed:	73 6d                	jae    20365c <_syscall+0x107>
  2035ef:	5f                   	pop    %rdi
  2035f0:	74 65                	je     203657 <_syscall+0x102>
  2035f2:	73 74                	jae    203668 <_syscall+0x113>
  2035f4:	00 69 6e             	add    %ch,0x6e(%rcx)
  2035f7:	69 74 3a 20 45 6e 74 	imul   $0x65746e45,0x20(%rdx,%rdi,1),%esi
  2035fe:	65 
  2035ff:	72 69                	jb     20366a <_syscall+0x115>
  203601:	6e                   	outsb  %ds:(%rsi),(%dx)
  203602:	67 20 77 61          	and    %dh,0x61(%edi)
  203606:	69 74 20 6c 6f 6f 70 	imul   $0xa706f6f,0x6c(%rax,%riz,1),%esi
  20360d:	0a 
  20360e:	00 90 69 6e 69 74    	add    %dl,0x74696e69(%rax)
  203614:	3a 20                	cmp    (%rax),%ah
  203616:	66 61                	data16 (bad)
  203618:	69 6c 65 64 20 74 6f 	imul   $0x206f7420,0x64(%rbp,%riz,2),%ebp
  20361f:	20 
  203620:	77 72                	ja     203694 <_syscall+0x13f>
  203622:	69 74 65 20 63 6f 6e 	imul   $0x666e6f63,0x20(%rbp,%riz,2),%esi
  203629:	66 
  20362a:	69 67 20 74 6f 20 00 	imul   $0x206f74,0x20(%rdi),%esp
  203631:	00 00                	add    %al,(%rax)
  203633:	00 00                	add    %al,(%rax)
  203635:	00 00                	add    %al,(%rax)
  203637:	00 69 6e             	add    %ch,0x6e(%rcx)
  20363a:	69 74 3a 20 53 74 61 	imul   $0x72617453,0x20(%rdx,%rdi,1),%esi
  203641:	72 
  203642:	74 69                	je     2036ad <_syscall+0x158>
  203644:	6e                   	outsb  %ds:(%rsi),(%dx)
  203645:	67 20 72 65          	and    %dh,0x65(%edx)
  203649:	73 75                	jae    2036c0 <_syscall+0x16b>
  20364b:	72 72                	jb     2036bf <_syscall+0x16a>
  20364d:	65 63 74 69 6f       	movsxd %gs:0x6f(%rcx,%rbp,2),%esi
  203652:	6e                   	outsb  %ds:(%rsi),(%dx)
  203653:	20 73 65             	and    %dh,0x65(%rbx)
  203656:	72 76                	jb     2036ce <_syscall+0x179>
  203658:	65 72 2e             	gs jb  203689 <_syscall+0x134>
  20365b:	2e 2e 0a 00          	cs cs or (%rax),%al
  20365f:	00 69 6e             	add    %ch,0x6e(%rcx)
  203662:	69 74 3a 20 65 78 65 	imul   $0x63657865,0x20(%rdx,%rdi,1),%esi
  203669:	63 
  20366a:	20 72 65             	and    %dh,0x65(%rdx)
  20366d:	73 75                	jae    2036e4 <_syscall+0x18f>
  20366f:	72 72                	jb     2036e3 <_syscall+0x18e>
  203671:	65 63 74 69 6f       	movsxd %gs:0x6f(%rcx,%rbp,2),%esi
  203676:	6e                   	outsb  %ds:(%rsi),(%dx)
  203677:	20 66 61             	and    %ah,0x61(%rsi)
  20367a:	69 6c 65 64 0a 00 69 	imul   $0x6e69000a,0x64(%rbp,%riz,2),%ebp
  203681:	6e 
  203682:	69 74 3a 20 52 65 73 	imul   $0x75736552,0x20(%rdx,%rdi,1),%esi
  203689:	75 
  20368a:	72 72                	jb     2036fe <_syscall+0x1a9>
  20368c:	65 63 74 69 6f       	movsxd %gs:0x6f(%rcx,%rbp,2),%esi
  203691:	6e                   	outsb  %ds:(%rsi),(%dx)
  203692:	20 73 65             	and    %dh,0x65(%rbx)
  203695:	72 76                	jb     20370d <_syscall+0x1b8>
  203697:	65 72 20             	gs jb  2036ba <_syscall+0x165>
  20369a:	73 74                	jae    203710 <_syscall+0x1bb>
  20369c:	61                   	(bad)
  20369d:	72 74                	jb     203713 <_syscall+0x1be>
  20369f:	65 64 20 28          	gs and %ch,%fs:(%rax)
  2036a3:	50                   	push   %rax
  2036a4:	49                   	rex.WB
  2036a5:	44 20 00             	and    %r8b,(%rax)
  2036a8:	69 6e 69 74 3a 20 52 	imul   $0x52203a74,0x69(%rsi),%ebp
  2036af:	65 67 69 73 74 65 72 	imul   $0x6e697265,%gs:0x74(%ebx),%esi
  2036b6:	69 6e 
  2036b8:	67 20 73 65          	and    %dh,0x65(%ebx)
  2036bc:	72 76                	jb     203734 <_syscall+0x1df>
  2036be:	69 63 65 73 2e 2e 2e 	imul   $0x2e2e2e73,0x65(%rbx),%esp
  2036c5:	0a 00                	or     (%rax),%al
  2036c7:	00 69 6e             	add    %ch,0x6e(%rcx)
  2036ca:	69 74 3a 20 53 65 72 	imul   $0x76726553,0x20(%rdx,%rdi,1),%esi
  2036d1:	76 
  2036d2:	69 63 65 20 72 65 67 	imul   $0x67657220,0x65(%rbx),%esp
  2036d9:	69 73 74 72 61 74 69 	imul   $0x69746172,0x74(%rbx),%esi
  2036e0:	6f                   	outsl  %ds:(%rsi),(%dx)
  2036e1:	6e                   	outsb  %ds:(%rsi),(%dx)
  2036e2:	20 63 6f             	and    %ah,0x6f(%rbx)
  2036e5:	6d                   	insl   (%dx),%es:(%rdi)
  2036e6:	70 6c                	jo     203754 <_syscall+0x1ff>
  2036e8:	65 74 65             	gs je  203750 <_syscall+0x1fb>
  2036eb:	0a 00                	or     (%rax),%al
  2036ed:	00 00                	add    %al,(%rax)
  2036ef:	00 50 41             	add    %dl,0x41(%rax)
  2036f2:	4e                   	rex.WRX
  2036f3:	49                   	rex.WB
  2036f4:	43 3a 20             	rex.XB cmp (%r8),%spl
  2036f7:	6d                   	insl   (%dx),%es:(%rdi)
  2036f8:	61                   	(bad)
  2036f9:	6c                   	insb   (%dx),%es:(%rdi)
  2036fa:	6c                   	insb   (%dx),%es:(%rdi)
  2036fb:	6f                   	outsl  %ds:(%rsi),(%dx)
  2036fc:	63 28                	movsxd (%rax),%ebp
  2036fe:	29 20                	sub    %esp,(%rax)
  203700:	63 61 6c             	movsxd 0x6c(%rcx),%esp
  203703:	6c                   	insb   (%dx),%es:(%rdi)
  203704:	65 64 20 2d 20 75 73 	gs and %ch,%fs:0x65737520(%rip)        # 6593ac2c <stack_top+0x65732c2c>
  20370b:	65 
  20370c:	20 70 65             	and    %dh,0x65(%rax)
  20370f:	62 62 6c 65 5f       	(bad)
  203714:	61                   	(bad)
  203715:	6c                   	insb   (%dx),%es:(%rdi)
  203716:	6c                   	insb   (%dx),%es:(%rdi)
  203717:	6f                   	outsl  %ds:(%rsi),(%dx)
  203718:	63 28                	movsxd (%rax),%ebp
  20371a:	29 00                	sub    %eax,(%rax)
  20371c:	00 00                	add    %al,(%rax)
  20371e:	00 00                	add    %al,(%rax)
  203720:	50                   	push   %rax
  203721:	41                   	rex.B
  203722:	4e                   	rex.WRX
  203723:	49                   	rex.WB
  203724:	43 3a 20             	rex.XB cmp (%r8),%spl
  203727:	66 72 65             	data16 jb 20378f <_syscall+0x23a>
  20372a:	65 28 29             	sub    %ch,%gs:(%rcx)
  20372d:	20 63 61             	and    %ah,0x61(%rbx)
  203730:	6c                   	insb   (%dx),%es:(%rdi)
  203731:	6c                   	insb   (%dx),%es:(%rdi)
  203732:	65 64 20 2d 20 75 73 	gs and %ch,%fs:0x65737520(%rip)        # 6593ac5a <stack_top+0x65732c5a>
  203739:	65 
  20373a:	20 70 65             	and    %dh,0x65(%rax)
  20373d:	62 62 6c 65 5f       	(bad)
  203742:	66 72 65             	data16 jb 2037aa <_syscall+0x255>
  203745:	65 28 29             	sub    %ch,%gs:(%rcx)
	...
  203750:	63 6f 6e             	movsxd 0x6e(%rdi),%ebp
  203753:	76 4d                	jbe    2037a2 <_syscall+0x24d>
  203755:	32 53 3a             	xor    0x3a(%rbx),%dl
  203758:	20 68 65             	and    %ch,0x65(%rax)
  20375b:	61                   	(bad)
  20375c:	64 65 72 20          	fs gs jb 203780 <_syscall+0x22b>
  203760:	62 6f 75 6e 64       	(bad)
  203765:	73 20                	jae    203787 <_syscall+0x232>
  203767:	63 68 65             	movsxd 0x65(%rax),%ebp
  20376a:	63 6b 20             	movsxd 0x20(%rbx),%ebp
  20376d:	66 61                	data16 (bad)
  20376f:	69 6c 65 64 20 70 3d 	imul   $0x253d7020,0x64(%rbp,%riz,2),%ebp
  203776:	25 
  203777:	70 20                	jo     203799 <_syscall+0x244>
  203779:	65 70 3d             	gs jo  2037b9 <_syscall+0x264>
  20377c:	25 70 0a 00 63       	and    $0x63000a70,%eax
  203781:	6f                   	outsl  %ds:(%rsi),(%dx)
  203782:	6e                   	outsb  %ds:(%rsi),(%dx)
  203783:	76 4d                	jbe    2037d2 <_syscall+0x27d>
  203785:	32 53 3a             	xor    0x3a(%rbx),%dl
  203788:	20 73 69             	and    %dh,0x69(%rbx)
  20378b:	7a 65                	jp     2037f2 <_syscall+0x29d>
  20378d:	20 63 68             	and    %ah,0x68(%rbx)
  203790:	65 63 6b 20          	movsxd %gs:0x20(%rbx),%ebp
  203794:	66 61                	data16 (bad)
  203796:	69 6c 65 64 20 73 69 	imul   $0x7a697320,0x64(%rbp,%riz,2),%ebp
  20379d:	7a 
  20379e:	65 3d 25 64 20 6d    	gs cmp $0x6d206425,%eax
  2037a4:	69 6e 3d 25 64 0a 00 	imul   $0xa6425,0x3d(%rsi),%ebp
  2037ab:	90                   	nop
  2037ac:	1c e4                	sbb    $0xe4,%al
  2037ae:	ff                   	(bad)
  2037af:	ff 1c e4             	lcall  *(%rsp,%riz,8)
  2037b2:	ff                   	(bad)
  2037b3:	ff 4f e9             	decl   -0x17(%rdi)
  2037b6:	ff                   	(bad)
  2037b7:	ff                   	(bad)
  2037b8:	7c e4                	jl     20379e <_syscall+0x249>
  2037ba:	ff                   	(bad)
  2037bb:	ff 34 ec             	push   (%rsp,%rbp,8)
  2037be:	ff                   	(bad)
  2037bf:	ff                   	(bad)
  2037c0:	7c e4                	jl     2037a6 <_syscall+0x251>
  2037c2:	ff                   	(bad)
  2037c3:	ff f7                	push   %rdi
  2037c5:	e1 ff                	loope  2037c6 <_syscall+0x271>
  2037c7:	ff b4 e4 ff ff 9e e5 	push   -0x1a610001(%rsp,%riz,8)
  2037ce:	ff                   	(bad)
  2037cf:	ff                   	ljmp   (bad)
  2037d0:	ec                   	in     (%dx),%al
  2037d1:	e1 ff                	loope  2037d2 <_syscall+0x27d>
  2037d3:	ff 94 e8 ff ff 20 e8 	call   *-0x17df0001(%rax,%rbp,8)
  2037da:	ff                   	(bad)
  2037db:	ff                   	(bad)
  2037dc:	fd                   	std
  2037dd:	e7 ff                	out    %eax,$0xff
  2037df:	ff 84 e5 ff ff 9c e3 	incl   -0x1c630001(%rbp,%riz,8)
  2037e6:	ff                   	(bad)
  2037e7:	ff 84 e5 ff ff 6c e3 	incl   -0x1c930001(%rbp,%riz,8)
  2037ee:	ff                   	(bad)
  2037ef:	ff cc                	dec    %esp
  2037f1:	e2 ff                	loop   2037f2 <_syscall+0x29d>
  2037f3:	ff 34 e3             	push   (%rbx,%riz,8)
  2037f6:	ff                   	(bad)
  2037f7:	ff 34 e2             	push   (%rdx,%riz,8)
  2037fa:	ff                   	(bad)
  2037fb:	ff d4                	call   *%rsp
  2037fd:	e1 ff                	loope  2037fe <_syscall+0x2a9>
  2037ff:	ff                   	ljmp   (bad)
  203800:	ec                   	in     (%dx),%al
  203801:	e1 ff                	loope  203802 <_syscall+0x2ad>
  203803:	ff d4                	call   *%rsp
  203805:	e1 ff                	loope  203806 <_syscall+0x2b1>
  203807:	ff                   	ljmp   (bad)
  203808:	ec                   	in     (%dx),%al
  203809:	e1 ff                	loope  20380a <_syscall+0x2b5>
  20380b:	ff d4                	call   *%rsp
  20380d:	e1 ff                	loope  20380e <_syscall+0x2b9>
  20380f:	ff                   	(bad)
  203810:	fc                   	cld
  203811:	e2 ff                	loop   203812 <_syscall+0x2bd>
  203813:	ff 04 e5 ff ff ec e1 	incl   -0x1e130001(,%riz,8)
  20381a:	ff                   	(bad)
  20381b:	ff cc                	dec    %esp
  20381d:	e2 ff                	loop   20381e <_syscall+0x2c9>
  20381f:	ff f7                	push   %rdi
  203821:	e1 ff                	loope  203822 <_syscall+0x2cd>
  203823:	ff 6c e7 ff          	ljmp   *-0x1(%rdi,%riz,8)
  203827:	ff                   	(bad)
  203828:	3c e7                	cmp    $0xe7,%al
  20382a:	ff                   	(bad)
  20382b:	ff 8b e7 ff ff 3c    	decl   0x3cffffe7(%rbx)
  203831:	e5 ff                	in     $0xff,%eax
  203833:	ff 9c e3 ff ff 3c e5 	lcall  *-0x1ac30001(%rbx,%riz,8)
  20383a:	ff                   	(bad)
  20383b:	ff 6c e3 ff          	ljmp   *-0x1(%rbx,%riz,8)
  20383f:	ff cc                	dec    %esp
  203841:	e2 ff                	loop   203842 <_syscall+0x2ed>
  203843:	ff 34 e3             	push   (%rbx,%riz,8)
  203846:	ff                   	(bad)
  203847:	ff 34 e2             	push   (%rdx,%riz,8)
  20384a:	ff                   	(bad)
  20384b:	ff d4                	call   *%rsp
  20384d:	e1 ff                	loope  20384e <_syscall+0x2f9>
  20384f:	ff                   	ljmp   (bad)
  203850:	ec                   	in     (%dx),%al
  203851:	e1 ff                	loope  203852 <_syscall+0x2fd>
  203853:	ff 6c e3 ff          	ljmp   *-0x1(%rbx,%riz,8)
  203857:	ff cc                	dec    %esp
  203859:	e2 ff                	loop   20385a <_syscall+0x305>
  20385b:	ff 34 e3             	push   (%rbx,%riz,8)
  20385e:	ff                   	(bad)
  20385f:	ff 34 e2             	push   (%rdx,%riz,8)
  203862:	ff                   	(bad)
  203863:	ff 64 e2 ff          	jmp    *-0x1(%rdx,%riz,8)
  203867:	ff                   	ljmp   (bad)
  203868:	ec                   	in     (%dx),%al
  203869:	e1 ff                	loope  20386a <_syscall+0x315>
  20386b:	ff 64 e2 ff          	jmp    *-0x1(%rdx,%riz,8)
  20386f:	ff                   	(bad)
  203870:	fc                   	cld
  203871:	e2 ff                	loop   203872 <_syscall+0x31d>
  203873:	ff d4                	call   *%rsp
  203875:	e1 ff                	loope  203876 <_syscall+0x321>
  203877:	ff                   	(bad)
  203878:	fc                   	cld
  203879:	e2 ff                	loop   20387a <_syscall+0x325>
  20387b:	ff c9                	dec    %ecx
  20387d:	eb ff                	jmp    20387e <_syscall+0x329>
  20387f:	ff                   	ljmp   (bad)
  203880:	ec                   	in     (%dx),%al
  203881:	e1 ff                	loope  203882 <_syscall+0x32d>
  203883:	ff 04 e5 ff ff ec e1 	incl   -0x1e130001(,%riz,8)
  20388a:	ff                   	(bad)
  20388b:	ff f7                	push   %rdi
  20388d:	e1 ff                	loope  20388e <_syscall+0x339>
  20388f:	ff f7                	push   %rdi
  203891:	e1 ff                	loope  203892 <_syscall+0x33d>
  203893:	ff f7                	push   %rdi
  203895:	e1 ff                	loope  203896 <_syscall+0x341>
  203897:	ff f7                	push   %rdi
  203899:	e1 ff                	loope  20389a <_syscall+0x345>
  20389b:	ff c2                	inc    %edx
  20389d:	ea                   	(bad)
  20389e:	ff                   	(bad)
  20389f:	ff af eb ff ff 84    	ljmp   *-0x7b000015(%rdi)
  2038a5:	e6 ff                	out    %al,$0xff
  2038a7:	ff                   	ljmp   (bad)
  2038a8:	ec                   	in     (%dx),%al
  2038a9:	e1 ff                	loope  2038aa <_syscall+0x355>
  2038ab:	ff b4 e4 ff ff ec e1 	push   -0x1e130001(%rsp,%riz,8)
  2038b2:	ff                   	(bad)
  2038b3:	ff                   	ljmp   (bad)
  2038b4:	ec                   	in     (%dx),%al
  2038b5:	e1 ff                	loope  2038b6 <_syscall+0x361>
  2038b7:	ff e0                	jmp    *%rax
  2038b9:	e6 ff                	out    %al,$0xff
  2038bb:	ff ac e2 ff ff ac e2 	ljmp   *-0x1d530001(%rdx,%riz,8)
  2038c2:	ff                   	(bad)
  2038c3:	ff 34 e2             	push   (%rdx,%riz,8)
  2038c6:	ff                   	(bad)
  2038c7:	ff                   	ljmp   (bad)
  2038c8:	ec                   	in     (%dx),%al
  2038c9:	e1 ff                	loope  2038ca <_syscall+0x375>
  2038cb:	ff f7                	push   %rdi
  2038cd:	e1 ff                	loope  2038ce <_syscall+0x379>
  2038cf:	ff f7                	push   %rdi
  2038d1:	e1 ff                	loope  2038d2 <_syscall+0x37d>
  2038d3:	ff f7                	push   %rdi
  2038d5:	e1 ff                	loope  2038d6 <_syscall+0x381>
  2038d7:	ff f7                	push   %rdi
  2038d9:	e1 ff                	loope  2038da <_syscall+0x385>
  2038db:	ff f7                	push   %rdi
  2038dd:	e1 ff                	loope  2038de <_syscall+0x389>
  2038df:	ff f7                	push   %rdi
  2038e1:	e1 ff                	loope  2038e2 <_syscall+0x38d>
  2038e3:	ff f7                	push   %rdi
  2038e5:	e1 ff                	loope  2038e6 <_syscall+0x391>
  2038e7:	ff f7                	push   %rdi
  2038e9:	e1 ff                	loope  2038ea <_syscall+0x395>
  2038eb:	ff f3                	push   %rbx
  2038ed:	e9 ff ff ec e1       	jmp    ffffffffe20d38f1 <stack_top+0xffffffffe1ecb8f1>
  2038f2:	ff                   	(bad)
  2038f3:	ff                   	(bad)
  2038f4:	ba e5 ff ff ec       	mov    $0xecffffe5,%edx
  2038f9:	e1 ff                	loope  2038fa <_syscall+0x3a5>
  2038fb:	ff                   	lcall  (bad)
  2038fc:	dc ea                	fsubr  %st,%st(2)
  2038fe:	ff                   	(bad)
  2038ff:	ff                   	ljmp   (bad)
  203900:	ec                   	in     (%dx),%al
  203901:	e1 ff                	loope  203902 <_syscall+0x3ad>
  203903:	ff 64 e2 ff          	jmp    *-0x1(%rdx,%riz,8)
  203907:	ff                   	ljmp   (bad)
  203908:	ec                   	in     (%dx),%al
  203909:	e1 ff                	loope  20390a <_syscall+0x3b5>
  20390b:	ff f7                	push   %rdi
  20390d:	e1 ff                	loope  20390e <_syscall+0x3b9>
  20390f:	ff f7                	push   %rdi
  203911:	e1 ff                	loope  203912 <_syscall+0x3bd>
  203913:	ff 6b eb             	ljmp   *-0x15(%rbx)
  203916:	ff                   	(bad)
  203917:	ff d4                	call   *%rsp
  203919:	e1 ff                	loope  20391a <_syscall+0x3c5>
  20391b:	ff                   	ljmp   (bad)
  20391c:	ec                   	in     (%dx),%al
  20391d:	e1 ff                	loope  20391e <_syscall+0x3c9>
  20391f:	ff 8d eb ff ff d4    	decl   -0x2b000015(%rbp)
  203925:	e1 ff                	loope  203926 <_syscall+0x3d1>
  203927:	ff 64 e2 ff          	jmp    *-0x1(%rdx,%riz,8)
  20392b:	ff f7                	push   %rdi
  20392d:	e1 ff                	loope  20392e <_syscall+0x3d9>
  20392f:	ff f7                	push   %rdi
  203931:	e1 ff                	loope  203932 <_syscall+0x3dd>
  203933:	ff f7                	push   %rdi
  203935:	e1 ff                	loope  203936 <_syscall+0x3e1>
  203937:	ff f7                	push   %rdi
  203939:	e1 ff                	loope  20393a <_syscall+0x3e5>
  20393b:	ff 97 ea ff ff ac    	call   *-0x53000016(%rdi)
  203941:	e2 ff                	loop   203942 <_syscall+0x3ed>
  203943:	ff ac e2 ff ff ec e1 	ljmp   *-0x1e130001(%rdx,%riz,8)
  20394a:	ff                   	(bad)
  20394b:	ff 34 e2             	push   (%rdx,%riz,8)
  20394e:	ff                   	(bad)
  20394f:	ff 34 e2             	push   (%rdx,%riz,8)
  203952:	ff                   	(bad)
  203953:	ff                   	lcall  (bad)
  203954:	dc ec                	fsubr  %st,%st(4)
  203956:	ff                   	(bad)
  203957:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  20395a:	ff                   	(bad)
  20395b:	ff                   	(bad)
  20395c:	3c ec                	cmp    $0xec,%al
  20395e:	ff                   	(bad)
  20395f:	ff                   	(bad)
  203960:	3c ec                	cmp    $0xec,%al
  203962:	ff                   	(bad)
  203963:	ff                   	(bad)
  203964:	fc                   	cld
  203965:	ec                   	in     (%dx),%al
  203966:	ff                   	(bad)
  203967:	ff                   	(bad)
  203968:	fc                   	cld
  203969:	ec                   	in     (%dx),%al
  20396a:	ff                   	(bad)
  20396b:	ff 42 ee             	incl   -0x12(%rdx)
  20396e:	ff                   	(bad)
  20396f:	ff 0c ee             	decl   (%rsi,%rbp,8)
  203972:	ff                   	(bad)
  203973:	ff 9c ec ff ff 0c ee 	lcall  *-0x11f30001(%rsp,%rbp,8)
  20397a:	ff                   	(bad)
  20397b:	ff                   	(bad)
  20397c:	3c ed                	cmp    $0xed,%al
  20397e:	ff                   	(bad)
  20397f:	ff cc                	dec    %esp
  203981:	ec                   	in     (%dx),%al
  203982:	ff                   	(bad)
  203983:	ff                   	(bad)
  203984:	bc ec ff ff 2c       	mov    $0x2cffffec,%esp
  203989:	ec                   	in     (%dx),%al
  20398a:	ff                   	(bad)
  20398b:	ff 2c ec             	ljmp   *(%rsp,%rbp,8)
  20398e:	ff                   	(bad)
  20398f:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  203992:	ff                   	(bad)
  203993:	ff                   	(bad)
  203994:	3c ed                	cmp    $0xed,%al
  203996:	ff                   	(bad)
  203997:	ff cc                	dec    %esp
  203999:	ec                   	in     (%dx),%al
  20399a:	ff                   	(bad)
  20399b:	ff                   	(bad)
  20399c:	bc ec ff ff 2c       	mov    $0x2cffffec,%esp
  2039a1:	ec                   	in     (%dx),%al
  2039a2:	ff                   	(bad)
  2039a3:	ff 6c ec ff          	ljmp   *-0x1(%rsp,%rbp,8)
  2039a7:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  2039aa:	ff                   	(bad)
  2039ab:	ff 6c ec ff          	ljmp   *-0x1(%rsp,%rbp,8)
  2039af:	ff                   	ljmp   (bad)
  2039b0:	ec                   	in     (%dx),%al
  2039b1:	ec                   	in     (%dx),%al
  2039b2:	ff                   	(bad)
  2039b3:	ff 2c ec             	ljmp   *(%rsp,%rbp,8)
  2039b6:	ff                   	(bad)
  2039b7:	ff                   	ljmp   (bad)
  2039b8:	ec                   	in     (%dx),%al
  2039b9:	ec                   	in     (%dx),%al
  2039ba:	ff                   	(bad)
  2039bb:	ff 20                	jmp    *(%rax)
  2039bd:	ee                   	out    %al,(%dx)
  2039be:	ff                   	(bad)
  2039bf:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  2039c2:	ff                   	(bad)
  2039c3:	ff                   	lcall  (bad)
  2039c4:	dc ec                	fsubr  %st,%st(4)
  2039c6:	ff                   	(bad)
  2039c7:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  2039ca:	ff                   	(bad)
  2039cb:	ff                   	(bad)
  2039cc:	3c ec                	cmp    $0xec,%al
  2039ce:	ff                   	(bad)
  2039cf:	ff                   	(bad)
  2039d0:	3c ec                	cmp    $0xec,%al
  2039d2:	ff                   	(bad)
  2039d3:	ff                   	(bad)
  2039d4:	3c ec                	cmp    $0xec,%al
  2039d6:	ff                   	(bad)
  2039d7:	ff                   	(bad)
  2039d8:	3c ec                	cmp    $0xec,%al
  2039da:	ff                   	(bad)
  2039db:	ff 2c ec             	ljmp   *(%rsp,%rbp,8)
  2039de:	ff                   	(bad)
  2039df:	ff 2c ec             	ljmp   *(%rsp,%rbp,8)
  2039e2:	ff                   	(bad)
  2039e3:	ff 4c ec ff          	decl   -0x1(%rsp,%rbp,8)
  2039e7:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  2039ea:	ff                   	(bad)
  2039eb:	ff 84 ec ff ff 1c ec 	incl   -0x13e30001(%rsp,%rbp,8)
  2039f2:	ff                   	(bad)
  2039f3:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  2039f6:	ff                   	(bad)
  2039f7:	ff af ee ff ff 0c    	ljmp   *0xcffffee(%rdi)
  2039fd:	ed                   	in     (%dx),%eax
  2039fe:	ff                   	(bad)
  2039ff:	ff 0c ed ff ff 2c ec 	decl   -0x13d30001(,%rbp,8)
  203a06:	ff                   	(bad)
  203a07:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  203a0a:	ff                   	(bad)
  203a0b:	ff                   	(bad)
  203a0c:	3c ec                	cmp    $0xec,%al
  203a0e:	ff                   	(bad)
  203a0f:	ff                   	(bad)
  203a10:	3c ec                	cmp    $0xec,%al
  203a12:	ff                   	(bad)
  203a13:	ff                   	(bad)
  203a14:	3c ec                	cmp    $0xec,%al
  203a16:	ff                   	(bad)
  203a17:	ff                   	(bad)
  203a18:	3c ec                	cmp    $0xec,%al
  203a1a:	ff                   	(bad)
  203a1b:	ff                   	(bad)
  203a1c:	3c ec                	cmp    $0xec,%al
  203a1e:	ff                   	(bad)
  203a1f:	ff                   	(bad)
  203a20:	3c ec                	cmp    $0xec,%al
  203a22:	ff                   	(bad)
  203a23:	ff                   	(bad)
  203a24:	3c ec                	cmp    $0xec,%al
  203a26:	ff                   	(bad)
  203a27:	ff                   	(bad)
  203a28:	3c ec                	cmp    $0xec,%al
  203a2a:	ff                   	(bad)
  203a2b:	ff 5c ee ff          	lcall  *-0x1(%rsi,%rbp,8)
  203a2f:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  203a32:	ff                   	(bad)
  203a33:	ff 99 ee ff ff 1c    	lcall  *0x1cffffee(%rcx)
  203a39:	ec                   	in     (%dx),%al
  203a3a:	ff                   	(bad)
  203a3b:	ff 86 ee ff ff 1c    	incl   0x1cffffee(%rsi)
  203a41:	ec                   	in     (%dx),%al
  203a42:	ff                   	(bad)
  203a43:	ff 6c ec ff          	ljmp   *-0x1(%rsp,%rbp,8)
  203a47:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  203a4a:	ff                   	(bad)
  203a4b:	ff                   	(bad)
  203a4c:	3c ec                	cmp    $0xec,%al
  203a4e:	ff                   	(bad)
  203a4f:	ff                   	(bad)
  203a50:	3c ec                	cmp    $0xec,%al
  203a52:	ff                   	(bad)
  203a53:	ff 0c ed ff ff 2c ec 	decl   -0x13d30001(,%rbp,8)
  203a5a:	ff                   	(bad)
  203a5b:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  203a5e:	ff                   	(bad)
  203a5f:	ff 0c ed ff ff 2c ec 	decl   -0x13d30001(,%rbp,8)
  203a66:	ff                   	(bad)
  203a67:	ff 6c ec ff          	ljmp   *-0x1(%rsp,%rbp,8)
  203a6b:	ff                   	(bad)
  203a6c:	3c ec                	cmp    $0xec,%al
  203a6e:	ff                   	(bad)
  203a6f:	ff                   	(bad)
  203a70:	3c ec                	cmp    $0xec,%al
  203a72:	ff                   	(bad)
  203a73:	ff                   	(bad)
  203a74:	3c ec                	cmp    $0xec,%al
  203a76:	ff                   	(bad)
  203a77:	ff                   	(bad)
  203a78:	3c ec                	cmp    $0xec,%al
  203a7a:	ff                   	(bad)
  203a7b:	ff                   	(bad)
  203a7c:	3c ed                	cmp    $0xed,%al
  203a7e:	ff                   	(bad)
  203a7f:	ff 0c ed ff ff 0c ed 	decl   -0x12f30001(,%rbp,8)
  203a86:	ff                   	(bad)
  203a87:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  203a8a:	ff                   	(bad)
  203a8b:	ff 2c ec             	ljmp   *(%rsp,%rbp,8)
  203a8e:	ff                   	(bad)
  203a8f:	ff 2c ec             	ljmp   *(%rsp,%rbp,8)
  203a92:	ff                   	(bad)
  203a93:	ff 0c eb             	decl   (%rbx,%rbp,8)
  203a96:	ff                   	(bad)
  203a97:	ff 0c eb             	decl   (%rbx,%rbp,8)
  203a9a:	ff                   	(bad)
  203a9b:	ff 54 ec ff          	call   *-0x1(%rsp,%rbp,8)
  203a9f:	ff                   	ljmp   (bad)
  203aa0:	ec                   	in     (%dx),%al
  203aa1:	eb ff                	jmp    203aa2 <_syscall+0x54d>
  203aa3:	ff 8c ec ff ff ec eb 	decl   -0x14130001(%rsp,%rbp,8)
  203aaa:	ff                   	(bad)
  203aab:	ff                   	(bad)
  203aac:	fc                   	cld
  203aad:	ea                   	(bad)
  203aae:	ff                   	(bad)
  203aaf:	ff 44 eb ff          	incl   -0x1(%rbx,%rbp,8)
  203ab3:	ff 4d eb             	decl   -0x15(%rbp)
  203ab6:	ff                   	(bad)
  203ab7:	ff                   	lcall  (bad)
  203ab8:	dc ea                	fsubr  %st,%st(2)
  203aba:	ff                   	(bad)
  203abb:	ff 0c ec             	decl   (%rsp,%rbp,8)
  203abe:	ff                   	(bad)
  203abf:	ff ac ec ff ff bc ec 	ljmp   *-0x13430001(%rsp,%rbp,8)
  203ac6:	ff                   	(bad)
  203ac7:	ff                   	lcall  (bad)
  203ac8:	dc eb                	fsubr  %st,%st(3)
  203aca:	ff                   	(bad)
  203acb:	ff 5c eb ff          	lcall  *-0x1(%rbx,%rbp,8)
  203acf:	ff                   	lcall  (bad)
  203ad0:	dc eb                	fsubr  %st,%st(3)
  203ad2:	ff                   	(bad)
  203ad3:	ff                   	(bad)
  203ad4:	fc                   	cld
  203ad5:	eb ff                	jmp    203ad6 <_syscall+0x581>
  203ad7:	ff 8c eb ff ff 7c eb 	decl   -0x14830001(%rbx,%rbp,8)
  203ade:	ff                   	(bad)
  203adf:	ff 4a f1             	decl   -0xf(%rdx)
  203ae2:	ff                   	(bad)
  203ae3:	ff 4a f1             	decl   -0xf(%rdx)
  203ae6:	ff                   	(bad)
  203ae7:	ff b2 f5 ff ff 89    	push   -0x7600000b(%rdx)
  203aed:	f0 ff                	lock (bad)
  203aef:	ff 95 f8 ff ff 89    	call   *-0x76000008(%rbp)
  203af5:	f0 ff                	lock (bad)
  203af7:	ff ab ed ff ff 9d    	ljmp   *-0x62000013(%rbx)
  203afd:	f0 ff                	lock (bad)
  203aff:	ff ca                	dec    %edx
  203b01:	f7 ff                	idiv   %edi
  203b03:	ff 00                	incl   (%rax)
  203b05:	ee                   	out    %al,(%dx)
  203b06:	ff                   	(bad)
  203b07:	ff ac f6 ff ff 75 f7 	ljmp   *-0x88a0001(%rsi,%rsi,8)
  203b0e:	ff                   	(bad)
  203b0f:	ff                   	ljmp   (bad)
  203b10:	ec                   	in     (%dx),%al
  203b11:	f3 ff                	repz (bad)
  203b13:	ff 9a f1 ff ff ba    	lcall  *-0x4500000f(%rdx)
  203b19:	f0 ff                	lock (bad)
  203b1b:	ff 9a f1 ff ff e0    	lcall  *-0x1f00000f(%rdx)
  203b21:	ee                   	out    %al,(%dx)
  203b22:	ff                   	(bad)
  203b23:	ff 68 ef             	ljmp   *-0x11(%rax)
  203b26:	ff                   	(bad)
  203b27:	ff a8 ef ff ff 38    	ljmp   *0x38ffffef(%rax)
  203b2d:	ee                   	out    %al,(%dx)
  203b2e:	ff                   	(bad)
  203b2f:	ff 10                	call   *(%rax)
  203b31:	ee                   	out    %al,(%dx)
  203b32:	ff                   	(bad)
  203b33:	ff 00                	incl   (%rax)
  203b35:	ee                   	out    %al,(%dx)
  203b36:	ff                   	(bad)
  203b37:	ff 10                	call   *(%rax)
  203b39:	ee                   	out    %al,(%dx)
  203b3a:	ff                   	(bad)
  203b3b:	ff 00                	incl   (%rax)
  203b3d:	ee                   	out    %al,(%dx)
  203b3e:	ff                   	(bad)
  203b3f:	ff 10                	call   *(%rax)
  203b41:	ee                   	out    %al,(%dx)
  203b42:	ff                   	(bad)
  203b43:	ff 30                	push   (%rax)
  203b45:	f0 ff                	lock (bad)
  203b47:	ff 5b f0             	lcall  *-0x10(%rbx)
  203b4a:	ff                   	(bad)
  203b4b:	ff 00                	incl   (%rax)
  203b4d:	ee                   	out    %al,(%dx)
  203b4e:	ff                   	(bad)
  203b4f:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  203b55:	ed                   	in     (%dx),%eax
  203b56:	ff                   	(bad)
  203b57:	ff 20                	jmp    *(%rax)
  203b59:	f9                   	stc
  203b5a:	ff                   	(bad)
  203b5b:	ff af f9 ff ff 7d    	ljmp   *0x7dfffff9(%rdi)
  203b61:	f3 ff                	repz (bad)
  203b63:	ff cc                	dec    %esp
  203b65:	f1                   	int1
  203b66:	ff                   	(bad)
  203b67:	ff                   	(bad)
  203b68:	ba f0 ff ff cc       	mov    $0xccfffff0,%edx
  203b6d:	f1                   	int1
  203b6e:	ff                   	(bad)
  203b6f:	ff e0                	jmp    *%rax
  203b71:	ee                   	out    %al,(%dx)
  203b72:	ff                   	(bad)
  203b73:	ff 68 ef             	ljmp   *-0x11(%rax)
  203b76:	ff                   	(bad)
  203b77:	ff a8 ef ff ff 38    	ljmp   *0x38ffffef(%rax)
  203b7d:	ee                   	out    %al,(%dx)
  203b7e:	ff                   	(bad)
  203b7f:	ff 10                	call   *(%rax)
  203b81:	ee                   	out    %al,(%dx)
  203b82:	ff                   	(bad)
  203b83:	ff 00                	incl   (%rax)
  203b85:	ee                   	out    %al,(%dx)
  203b86:	ff                   	(bad)
  203b87:	ff e0                	jmp    *%rax
  203b89:	ee                   	out    %al,(%dx)
  203b8a:	ff                   	(bad)
  203b8b:	ff 68 ef             	ljmp   *-0x11(%rax)
  203b8e:	ff                   	(bad)
  203b8f:	ff a8 ef ff ff 38    	ljmp   *0x38ffffef(%rax)
  203b95:	ee                   	out    %al,(%dx)
  203b96:	ff                   	(bad)
  203b97:	ff 58 ee             	lcall  *-0x12(%rax)
  203b9a:	ff                   	(bad)
  203b9b:	ff 00                	incl   (%rax)
  203b9d:	ee                   	out    %al,(%dx)
  203b9e:	ff                   	(bad)
  203b9f:	ff 58 ee             	lcall  *-0x12(%rax)
  203ba2:	ff                   	(bad)
  203ba3:	ff 30                	push   (%rax)
  203ba5:	f0 ff                	lock (bad)
  203ba7:	ff 10                	call   *(%rax)
  203ba9:	ee                   	out    %al,(%dx)
  203baa:	ff                   	(bad)
  203bab:	ff 30                	push   (%rax)
  203bad:	f0 ff                	lock (bad)
  203baf:	ff 55 f5             	call   *-0xb(%rbp)
  203bb2:	ff                   	(bad)
  203bb3:	ff 00                	incl   (%rax)
  203bb5:	ee                   	out    %al,(%dx)
  203bb6:	ff                   	(bad)
  203bb7:	ff 5b f0             	lcall  *-0x10(%rbx)
  203bba:	ff                   	(bad)
  203bbb:	ff 00                	incl   (%rax)
  203bbd:	ee                   	out    %al,(%dx)
  203bbe:	ff                   	(bad)
  203bbf:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  203bc5:	ed                   	in     (%dx),%eax
  203bc6:	ff                   	(bad)
  203bc7:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  203bcd:	ed                   	in     (%dx),%eax
  203bce:	ff                   	(bad)
  203bcf:	ff 17                	call   *(%rdi)
  203bd1:	f4                   	hlt
  203bd2:	ff                   	(bad)
  203bd3:	ff 93 f9 ff ff 24    	call   *0x24fffff9(%rbx)
  203bd9:	f8                   	clc
  203bda:	ff                   	(bad)
  203bdb:	ff 00                	incl   (%rax)
  203bdd:	ee                   	out    %al,(%dx)
  203bde:	ff                   	(bad)
  203bdf:	ff 9d f0 ff ff 00    	lcall  *0xfffff0(%rbp)
  203be5:	ee                   	out    %al,(%dx)
  203be6:	ff                   	(bad)
  203be7:	ff 00                	incl   (%rax)
  203be9:	ee                   	out    %al,(%dx)
  203bea:	ff                   	(bad)
  203beb:	ff c5                	inc    %ebp
  203bed:	f2 ff                	repnz (bad)
  203bef:	ff 90 ee ff ff 90    	call   *-0x6f000012(%rax)
  203bf5:	ee                   	out    %al,(%dx)
  203bf6:	ff                   	(bad)
  203bf7:	ff                   	(bad)
  203bf8:	38 ee                	cmp    %ch,%dh
  203bfa:	ff                   	(bad)
  203bfb:	ff 00                	incl   (%rax)
  203bfd:	ee                   	out    %al,(%dx)
  203bfe:	ff                   	(bad)
  203bff:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  203c05:	ed                   	in     (%dx),%eax
  203c06:	ff                   	(bad)
  203c07:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  203c0d:	ed                   	in     (%dx),%eax
  203c0e:	ff                   	(bad)
  203c0f:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  203c15:	ed                   	in     (%dx),%eax
  203c16:	ff                   	(bad)
  203c17:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  203c1d:	ed                   	in     (%dx),%eax
  203c1e:	ff                   	(bad)
  203c1f:	ff 9f f4 ff ff 00    	lcall  *0xfffff4(%rdi)
  203c25:	ee                   	out    %al,(%dx)
  203c26:	ff                   	(bad)
  203c27:	ff f0                	push   %rax
  203c29:	f1                   	int1
  203c2a:	ff                   	(bad)
  203c2b:	ff 00                	incl   (%rax)
  203c2d:	ee                   	out    %al,(%dx)
  203c2e:	ff                   	(bad)
  203c2f:	ff                   	(bad)
  203c30:	fc                   	cld
  203c31:	f2 ff                	repnz (bad)
  203c33:	ff 00                	incl   (%rax)
  203c35:	ee                   	out    %al,(%dx)
  203c36:	ff                   	(bad)
  203c37:	ff 58 ee             	lcall  *-0x12(%rax)
  203c3a:	ff                   	(bad)
  203c3b:	ff 00                	incl   (%rax)
  203c3d:	ee                   	out    %al,(%dx)
  203c3e:	ff                   	(bad)
  203c3f:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  203c45:	ed                   	in     (%dx),%eax
  203c46:	ff                   	(bad)
  203c47:	ff 69 f4             	ljmp   *-0xc(%rcx)
  203c4a:	ff                   	(bad)
  203c4b:	ff 10                	call   *(%rax)
  203c4d:	ee                   	out    %al,(%dx)
  203c4e:	ff                   	(bad)
  203c4f:	ff 00                	incl   (%rax)
  203c51:	ee                   	out    %al,(%dx)
  203c52:	ff                   	(bad)
  203c53:	ff 33                	push   (%rbx)
  203c55:	f4                   	hlt
  203c56:	ff                   	(bad)
  203c57:	ff 10                	call   *(%rax)
  203c59:	ee                   	out    %al,(%dx)
  203c5a:	ff                   	(bad)
  203c5b:	ff 58 ee             	lcall  *-0x12(%rax)
  203c5e:	ff                   	(bad)
  203c5f:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  203c65:	ed                   	in     (%dx),%eax
  203c66:	ff                   	(bad)
  203c67:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  203c6d:	ed                   	in     (%dx),%eax
  203c6e:	ff                   	(bad)
  203c6f:	ff 30                	push   (%rax)
  203c71:	f6 ff                	idiv   %bh
  203c73:	ff                   	lcall  (bad)
  203c74:	dd f7                	(bad)
  203c76:	ff                   	(bad)
  203c77:	ff 90 ee ff ff 00    	call   *0xffffee(%rax)
  203c7d:	ee                   	out    %al,(%dx)
  203c7e:	ff                   	(bad)
  203c7f:	ff                   	(bad)
  203c80:	38 ee                	cmp    %ch,%dh
  203c82:	ff                   	(bad)
  203c83:	ff                   	(bad)
  203c84:	38 ee                	cmp    %ch,%dh
  203c86:	ff                   	(bad)
  203c87:	ff                   	.byte 0xff
