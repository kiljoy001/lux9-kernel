
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
  200036:	e9 b5 05 00 00       	jmp    2005f0 <sys_write>
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
  2000f6:	e8 f5 07 00 00       	call   2008f0 <sys_create>
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
  200142:	e8 a9 04 00 00       	call   2005f0 <sys_write>
  200147:	48 39 c5             	cmp    %rax,%rbp
  20014a:	75 13                	jne    20015f <register_service+0x11f>
  20014c:	89 df                	mov    %ebx,%edi
  20014e:	e8 8d 03 00 00       	call   2004e0 <sys_close>
  200153:	48 81 c4 80 01 00 00 	add    $0x180,%rsp
  20015a:	5b                   	pop    %rbx
  20015b:	5d                   	pop    %rbp
  20015c:	41 5c                	pop    %r12
  20015e:	c3                   	ret
  20015f:	48 8d 3d 9a 30 00 00 	lea    0x309a(%rip),%rdi        # 203200 <_syscall+0xbb>
  200166:	e8 a5 fe ff ff       	call   200010 <init_print>
  20016b:	4c 89 e7             	mov    %r12,%rdi
  20016e:	e8 9d fe ff ff       	call   200010 <init_print>
  200173:	48 8d 3d 38 30 00 00 	lea    0x3038(%rip),%rdi        # 2031b2 <_syscall+0x6d>
  20017a:	e8 91 fe ff ff       	call   200010 <init_print>
  20017f:	eb cb                	jmp    20014c <register_service+0x10c>
  200181:	48 8d 3d c0 2f 00 00 	lea    0x2fc0(%rip),%rdi        # 203148 <_syscall+0x3>
  200188:	e8 83 fe ff ff       	call   200010 <init_print>
  20018d:	4c 89 e7             	mov    %r12,%rdi
  200190:	e8 7b fe ff ff       	call   200010 <init_print>
  200195:	48 8d 3d 16 30 00 00 	lea    0x3016(%rip),%rdi        # 2031b2 <_syscall+0x6d>
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
  2001f8:	e8 f3 03 00 00       	call   2005f0 <sys_write>
  2001fd:	89 d8                	mov    %ebx,%eax
  2001ff:	5b                   	pop    %rbx
  200200:	c3                   	ret
  200201:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  200208:	31 db                	xor    %ebx,%ebx
  20020a:	31 d2                	xor    %edx,%edx
  20020c:	bf 02 00 00 00       	mov    $0x2,%edi
  200211:	e8 da 03 00 00       	call   2005f0 <sys_write>
  200216:	89 d8                	mov    %ebx,%eax
  200218:	5b                   	pop    %rbx
  200219:	c3                   	ret
  20021a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)

0000000000200220 <main>:
  200220:	48 83 ec 08          	sub    $0x8,%rsp
  200224:	48 8d 3d 35 2f 00 00 	lea    0x2f35(%rip),%rdi        # 203160 <_syscall+0x1b>
  20022b:	e8 e0 fd ff ff       	call   200010 <init_print>
  200230:	48 8d 3d f1 2f 00 00 	lea    0x2ff1(%rip),%rdi        # 203228 <_syscall+0xe3>
  200237:	e8 d4 fd ff ff       	call   200010 <init_print>
  20023c:	bf 10 00 00 00       	mov    $0x10,%edi
  200241:	e8 ca 07 00 00       	call   200a10 <sys_rfork>
  200246:	85 c0                	test   %eax,%eax
  200248:	0f 84 89 00 00 00    	je     2002d7 <main+0xb7>
  20024e:	0f 88 9d 00 00 00    	js     2002f1 <main+0xd1>
  200254:	48 8d 3d 15 30 00 00 	lea    0x3015(%rip),%rdi        # 203270 <_syscall+0x12b>
  20025b:	e8 b0 fd ff ff       	call   200010 <init_print>
  200260:	48 8d 3d 4a 2f 00 00 	lea    0x2f4a(%rip),%rdi        # 2031b1 <_syscall+0x6c>
  200267:	e8 a4 fd ff ff       	call   200010 <init_print>
  20026c:	48 8d 3d 25 30 00 00 	lea    0x3025(%rip),%rdi        # 203298 <_syscall+0x153>
  200273:	e8 98 fd ff ff       	call   200010 <init_print>
  200278:	48 8d 35 35 2f 00 00 	lea    0x2f35(%rip),%rsi        # 2031b4 <_syscall+0x6f>
  20027f:	48 8d 3d 34 2f 00 00 	lea    0x2f34(%rip),%rdi        # 2031ba <_syscall+0x75>
  200286:	e8 b5 fd ff ff       	call   200040 <register_service>
  20028b:	48 8d 35 34 2f 00 00 	lea    0x2f34(%rip),%rsi        # 2031c6 <_syscall+0x81>
  200292:	48 8d 3d 33 2f 00 00 	lea    0x2f33(%rip),%rdi        # 2031cc <_syscall+0x87>
  200299:	e8 a2 fd ff ff       	call   200040 <register_service>
  20029e:	48 8d 35 30 2f 00 00 	lea    0x2f30(%rip),%rsi        # 2031d5 <_syscall+0x90>
  2002a5:	48 8d 3d 2f 2f 00 00 	lea    0x2f2f(%rip),%rdi        # 2031db <_syscall+0x96>
  2002ac:	e8 8f fd ff ff       	call   200040 <register_service>
  2002b1:	48 8d 3d 00 30 00 00 	lea    0x3000(%rip),%rdi        # 2032b8 <_syscall+0x173>
  2002b8:	e8 53 fd ff ff       	call   200010 <init_print>
  2002bd:	48 8d 3d 21 2f 00 00 	lea    0x2f21(%rip),%rdi        # 2031e5 <_syscall+0xa0>
  2002c4:	e8 47 fd ff ff       	call   200010 <init_print>
  2002c9:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  2002d0:	e8 db 09 00 00       	call   200cb0 <sys_wait>
  2002d5:	eb f9                	jmp    2002d0 <main+0xb0>
  2002d7:	48 8d 3d 9e 2e 00 00 	lea    0x2e9e(%rip),%rdi        # 20317c <_syscall+0x37>
  2002de:	e8 6d 07 00 00       	call   200a50 <sys_exec>
  2002e3:	48 8d 3d 66 2f 00 00 	lea    0x2f66(%rip),%rdi        # 203250 <_syscall+0x10b>
  2002ea:	e8 21 fd ff ff       	call   200010 <init_print>
  2002ef:	eb fe                	jmp    2002ef <main+0xcf>
  2002f1:	48 8d 3d 9a 2e 00 00 	lea    0x2e9a(%rip),%rdi        # 203192 <_syscall+0x4d>
  2002f8:	e8 13 fd ff ff       	call   200010 <init_print>
  2002fd:	48 8d 3d a1 2e 00 00 	lea    0x2ea1(%rip),%rdi        # 2031a5 <_syscall+0x60>
  200304:	e8 57 05 00 00       	call   200860 <sys_exit>
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
  20032f:	e8 1c 21 00 00       	call   202450 <convS2M>
  200334:	85 c0                	test   %eax,%eax
  200336:	7e 40                	jle    200378 <lux_call+0x68>
  200338:	e8 08 2e 00 00       	call   203145 <_syscall>
  20033d:	31 f6                	xor    %esi,%esi
  20033f:	ba 98 01 00 00       	mov    $0x198,%edx
  200344:	48 89 df             	mov    %rbx,%rdi
  200347:	e8 c4 0f 00 00       	call   201310 <memset>
  20034c:	48 89 da             	mov    %rbx,%rdx
  20034f:	be 00 0f 00 00       	mov    $0xf00,%esi
  200354:	48 89 ef             	mov    %rbp,%rdi
  200357:	e8 b4 11 00 00       	call   201510 <convM2S>
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
  200380:	41 57                	push   %r15
  200382:	41 56                	push   %r14
  200384:	41 55                	push   %r13
  200386:	41 89 fd             	mov    %edi,%r13d
  200389:	41 54                	push   %r12
  20038b:	49 89 f4             	mov    %rsi,%r12
  20038e:	31 f6                	xor    %esi,%esi
  200390:	55                   	push   %rbp
  200391:	89 d5                	mov    %edx,%ebp
  200393:	ba 98 01 00 00       	mov    $0x198,%edx
  200398:	53                   	push   %rbx
  200399:	48 89 cb             	mov    %rcx,%rbx
  20039c:	48 81 ec 48 03 00 00 	sub    $0x348,%rsp
  2003a3:	49 89 e6             	mov    %rsp,%r14
  2003a6:	4c 8d bc 24 a0 01 00 	lea    0x1a0(%rsp),%r15
  2003ad:	00 
  2003ae:	4c 89 f7             	mov    %r14,%rdi
  2003b1:	e8 5a 0f 00 00       	call   201310 <memset>
  2003b6:	31 f6                	xor    %esi,%esi
  2003b8:	4c 89 ff             	mov    %r15,%rdi
  2003bb:	ba 98 01 00 00       	mov    $0x198,%edx
  2003c0:	e8 4b 0f 00 00       	call   201310 <memset>
  2003c5:	b8 01 00 00 00       	mov    $0x1,%eax
  2003ca:	4c 89 fe             	mov    %r15,%rsi
  2003cd:	4c 89 f7             	mov    %r14,%rdi
  2003d0:	c6 04 24 82          	movb   $0x82,(%rsp)
  2003d4:	66 89 44 24 08       	mov    %ax,0x8(%rsp)
  2003d9:	44 89 6c 24 10       	mov    %r13d,0x10(%rsp)
  2003de:	c7 44 24 14 00 00 00 	movl   $0x0,0x14(%rsp)
  2003e5:	00 
  2003e6:	4c 89 64 24 18       	mov    %r12,0x18(%rsp)
  2003eb:	89 6c 24 20          	mov    %ebp,0x20(%rsp)
  2003ef:	e8 1c ff ff ff       	call   200310 <lux_call>
  2003f4:	85 c0                	test   %eax,%eax
  2003f6:	78 24                	js     20041c <do_syscall+0x9c>
  2003f8:	48 85 db             	test   %rbx,%rbx
  2003fb:	74 0b                	je     200408 <do_syscall+0x88>
  2003fd:	48 8b 84 24 c8 01 00 	mov    0x1c8(%rsp),%rax
  200404:	00 
  200405:	48 89 03             	mov    %rax,(%rbx)
  200408:	31 c0                	xor    %eax,%eax
  20040a:	48 81 c4 48 03 00 00 	add    $0x348,%rsp
  200411:	5b                   	pop    %rbx
  200412:	5d                   	pop    %rbp
  200413:	41 5c                	pop    %r12
  200415:	41 5d                	pop    %r13
  200417:	41 5e                	pop    %r14
  200419:	41 5f                	pop    %r15
  20041b:	c3                   	ret
  20041c:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  200421:	eb e7                	jmp    20040a <do_syscall+0x8a>
  200423:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  20042a:	00 00 00 00 
  20042e:	66 90                	xchg   %ax,%ax

0000000000200430 <sys_open>:
  200430:	f3 0f 1e fa          	endbr64
  200434:	41 54                	push   %r12
  200436:	41 89 f4             	mov    %esi,%r12d
  200439:	55                   	push   %rbp
  20043a:	53                   	push   %rbx
  20043b:	48 81 ec 10 04 00 00 	sub    $0x410,%rsp
  200442:	80 3f 00             	cmpb   $0x0,(%rdi)
  200445:	74 79                	je     2004c0 <sys_open+0x90>
  200447:	b8 01 00 00 00       	mov    $0x1,%eax
  20044c:	0f 1f 40 00          	nopl   0x0(%rax)
  200450:	48 89 c2             	mov    %rax,%rdx
  200453:	48 8d 40 01          	lea    0x1(%rax),%rax
  200457:	80 3c 17 00          	cmpb   $0x0,(%rdi,%rdx,1)
  20045b:	75 f3                	jne    200450 <sys_open+0x20>
  20045d:	8d 5a 02             	lea    0x2(%rdx),%ebx
  200460:	89 d1                	mov    %edx,%ecx
  200462:	0f b6 c6             	movzbl %dh,%eax
  200465:	48 63 db             	movslq %ebx,%rbx
  200468:	88 44 24 11          	mov    %al,0x11(%rsp)
  20046c:	48 8d 44 24 12       	lea    0x12(%rsp),%rax
  200471:	48 8d 6c 24 10       	lea    0x10(%rsp),%rbp
  200476:	48 89 fe             	mov    %rdi,%rsi
  200479:	48 89 c7             	mov    %rax,%rdi
  20047c:	88 4c 24 10          	mov    %cl,0x10(%rsp)
  200480:	e8 3b 0e 00 00       	call   2012c0 <memmove>
  200485:	48 8d 54 1d 00       	lea    0x0(%rbp,%rbx,1),%rdx
  20048a:	48 8d 4c 24 08       	lea    0x8(%rsp),%rcx
  20048f:	48 89 ee             	mov    %rbp,%rsi
  200492:	44 88 22             	mov    %r12b,(%rdx)
  200495:	48 83 c2 01          	add    $0x1,%rdx
  200499:	bf 01 00 00 00       	mov    $0x1,%edi
  20049e:	29 ea                	sub    %ebp,%edx
  2004a0:	e8 db fe ff ff       	call   200380 <do_syscall>
  2004a5:	85 c0                	test   %eax,%eax
  2004a7:	78 24                	js     2004cd <sys_open+0x9d>
  2004a9:	8b 44 24 08          	mov    0x8(%rsp),%eax
  2004ad:	48 81 c4 10 04 00 00 	add    $0x410,%rsp
  2004b4:	5b                   	pop    %rbx
  2004b5:	5d                   	pop    %rbp
  2004b6:	41 5c                	pop    %r12
  2004b8:	c3                   	ret
  2004b9:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  2004c0:	31 c0                	xor    %eax,%eax
  2004c2:	31 c9                	xor    %ecx,%ecx
  2004c4:	bb 02 00 00 00       	mov    $0x2,%ebx
  2004c9:	31 d2                	xor    %edx,%edx
  2004cb:	eb 9b                	jmp    200468 <sys_open+0x38>
  2004cd:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  2004d2:	eb d9                	jmp    2004ad <sys_open+0x7d>
  2004d4:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  2004db:	00 00 00 00 
  2004df:	90                   	nop

00000000002004e0 <sys_close>:
  2004e0:	f3 0f 1e fa          	endbr64
  2004e4:	48 83 ec 18          	sub    $0x18,%rsp
  2004e8:	31 c9                	xor    %ecx,%ecx
  2004ea:	ba 04 00 00 00       	mov    $0x4,%edx
  2004ef:	89 3c 24             	mov    %edi,(%rsp)
  2004f2:	48 89 e6             	mov    %rsp,%rsi
  2004f5:	bf 02 00 00 00       	mov    $0x2,%edi
  2004fa:	e8 81 fe ff ff       	call   200380 <do_syscall>
  2004ff:	48 83 c4 18          	add    $0x18,%rsp
  200503:	c3                   	ret
  200504:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  20050b:	00 00 00 00 
  20050f:	90                   	nop

0000000000200510 <sys_read>:
  200510:	f3 0f 1e fa          	endbr64
  200514:	41 55                	push   %r13
  200516:	41 54                	push   %r12
  200518:	55                   	push   %rbp
  200519:	48 89 f5             	mov    %rsi,%rbp
  20051c:	31 f6                	xor    %esi,%esi
  20051e:	53                   	push   %rbx
  20051f:	48 89 d3             	mov    %rdx,%rbx
  200522:	48 81 ec 68 03 00 00 	sub    $0x368,%rsp
  200529:	4c 8d 64 24 20       	lea    0x20(%rsp),%r12
  20052e:	89 3c 24             	mov    %edi,(%rsp)
  200531:	4c 8d ac 24 c0 01 00 	lea    0x1c0(%rsp),%r13
  200538:	00 
  200539:	89 54 24 0c          	mov    %edx,0xc(%rsp)
  20053d:	4c 89 e7             	mov    %r12,%rdi
  200540:	ba 98 01 00 00       	mov    $0x198,%edx
  200545:	48 c7 44 24 04 00 00 	movq   $0x0,0x4(%rsp)
  20054c:	00 00 
  20054e:	e8 bd 0d 00 00       	call   201310 <memset>
  200553:	31 f6                	xor    %esi,%esi
  200555:	4c 89 ef             	mov    %r13,%rdi
  200558:	ba 98 01 00 00       	mov    $0x198,%edx
  20055d:	e8 ae 0d 00 00       	call   201310 <memset>
  200562:	b8 01 00 00 00       	mov    $0x1,%eax
  200567:	4c 89 ee             	mov    %r13,%rsi
  20056a:	4c 89 e7             	mov    %r12,%rdi
  20056d:	66 89 44 24 28       	mov    %ax,0x28(%rsp)
  200572:	48 89 e0             	mov    %rsp,%rax
  200575:	c6 44 24 20 82       	movb   $0x82,0x20(%rsp)
  20057a:	c7 44 24 30 03 00 00 	movl   $0x3,0x30(%rsp)
  200581:	00 
  200582:	48 89 44 24 38       	mov    %rax,0x38(%rsp)
  200587:	c7 44 24 40 10 00 00 	movl   $0x10,0x40(%rsp)
  20058e:	00 
  20058f:	e8 7c fd ff ff       	call   200310 <lux_call>
  200594:	85 c0                	test   %eax,%eax
  200596:	78 4c                	js     2005e4 <sys_read+0xd4>
  200598:	8b 94 24 d8 01 00 00 	mov    0x1d8(%rsp),%edx
  20059f:	48 89 d0             	mov    %rdx,%rax
  2005a2:	48 39 da             	cmp    %rbx,%rdx
  2005a5:	7e 0c                	jle    2005b3 <sys_read+0xa3>
  2005a7:	89 9c 24 d8 01 00 00 	mov    %ebx,0x1d8(%rsp)
  2005ae:	89 da                	mov    %ebx,%edx
  2005b0:	48 89 d0             	mov    %rdx,%rax
  2005b3:	85 c0                	test   %eax,%eax
  2005b5:	74 1c                	je     2005d3 <sys_read+0xc3>
  2005b7:	48 8b b4 24 d8 01 00 	mov    0x1d8(%rsp),%rsi
  2005be:	00 
  2005bf:	48 85 f6             	test   %rsi,%rsi
  2005c2:	74 0f                	je     2005d3 <sys_read+0xc3>
  2005c4:	48 89 ef             	mov    %rbp,%rdi
  2005c7:	e8 f4 0c 00 00       	call   2012c0 <memmove>
  2005cc:	8b 94 24 d8 01 00 00 	mov    0x1d8(%rsp),%edx
  2005d3:	48 89 d0             	mov    %rdx,%rax
  2005d6:	48 81 c4 68 03 00 00 	add    $0x368,%rsp
  2005dd:	5b                   	pop    %rbx
  2005de:	5d                   	pop    %rbp
  2005df:	41 5c                	pop    %r12
  2005e1:	41 5d                	pop    %r13
  2005e3:	c3                   	ret
  2005e4:	48 c7 c0 ff ff ff ff 	mov    $0xffffffffffffffff,%rax
  2005eb:	eb e9                	jmp    2005d6 <sys_read+0xc6>
  2005ed:	0f 1f 00             	nopl   (%rax)

00000000002005f0 <sys_write>:
  2005f0:	f3 0f 1e fa          	endbr64
  2005f4:	41 55                	push   %r13
  2005f6:	49 89 f5             	mov    %rsi,%r13
  2005f9:	41 54                	push   %r12
  2005fb:	41 89 fc             	mov    %edi,%r12d
  2005fe:	48 8d 7a 10          	lea    0x10(%rdx),%rdi
  200602:	55                   	push   %rbp
  200603:	53                   	push   %rbx
  200604:	48 89 d3             	mov    %rdx,%rbx
  200607:	48 81 ec 18 04 00 00 	sub    $0x418,%rsp
  20060e:	48 c7 04 24 00 00 00 	movq   $0x0,(%rsp)
  200615:	00 
  200616:	48 81 ff 00 04 00 00 	cmp    $0x400,%rdi
  20061d:	76 19                	jbe    200638 <sys_write+0x48>
  20061f:	48 89 e6             	mov    %rsp,%rsi
  200622:	e8 d9 0a 00 00       	call   201100 <pebble_alloc>
  200627:	85 c0                	test   %eax,%eax
  200629:	78 75                	js     2006a0 <sys_write+0xb0>
  20062b:	48 8b 2c 24          	mov    (%rsp),%rbp
  20062f:	eb 0c                	jmp    20063d <sys_write+0x4d>
  200631:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  200638:	48 8d 6c 24 10       	lea    0x10(%rsp),%rbp
  20063d:	44 89 65 00          	mov    %r12d,0x0(%rbp)
  200641:	4c 8d 65 10          	lea    0x10(%rbp),%r12
  200645:	48 89 da             	mov    %rbx,%rdx
  200648:	4c 89 ee             	mov    %r13,%rsi
  20064b:	89 5d 0c             	mov    %ebx,0xc(%rbp)
  20064e:	4c 89 e7             	mov    %r12,%rdi
  200651:	48 c7 45 04 00 00 00 	movq   $0x0,0x4(%rbp)
  200658:	00 
  200659:	e8 62 0c 00 00       	call   2012c0 <memmove>
  20065e:	49 8d 14 1c          	lea    (%r12,%rbx,1),%rdx
  200662:	bf 04 00 00 00       	mov    $0x4,%edi
  200667:	48 89 ee             	mov    %rbp,%rsi
  20066a:	48 8d 4c 24 08       	lea    0x8(%rsp),%rcx
  20066f:	29 ea                	sub    %ebp,%edx
  200671:	e8 0a fd ff ff       	call   200380 <do_syscall>
  200676:	48 8b 3c 24          	mov    (%rsp),%rdi
  20067a:	89 c3                	mov    %eax,%ebx
  20067c:	48 85 ff             	test   %rdi,%rdi
  20067f:	74 05                	je     200686 <sys_write+0x96>
  200681:	e8 da 0b 00 00       	call   201260 <pebble_free>
  200686:	85 db                	test   %ebx,%ebx
  200688:	78 16                	js     2006a0 <sys_write+0xb0>
  20068a:	48 8b 44 24 08       	mov    0x8(%rsp),%rax
  20068f:	48 81 c4 18 04 00 00 	add    $0x418,%rsp
  200696:	5b                   	pop    %rbx
  200697:	5d                   	pop    %rbp
  200698:	41 5c                	pop    %r12
  20069a:	41 5d                	pop    %r13
  20069c:	c3                   	ret
  20069d:	0f 1f 00             	nopl   (%rax)
  2006a0:	48 c7 c0 ff ff ff ff 	mov    $0xffffffffffffffff,%rax
  2006a7:	eb e6                	jmp    20068f <sys_write+0x9f>
  2006a9:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)

00000000002006b0 <sys_pwrite>:
  2006b0:	f3 0f 1e fa          	endbr64
  2006b4:	41 57                	push   %r15
  2006b6:	41 56                	push   %r14
  2006b8:	41 89 fe             	mov    %edi,%r14d
  2006bb:	48 8d 7a 10          	lea    0x10(%rdx),%rdi
  2006bf:	41 55                	push   %r13
  2006c1:	49 89 cd             	mov    %rcx,%r13
  2006c4:	41 54                	push   %r12
  2006c6:	49 89 d4             	mov    %rdx,%r12
  2006c9:	55                   	push   %rbp
  2006ca:	48 89 f5             	mov    %rsi,%rbp
  2006cd:	53                   	push   %rbx
  2006ce:	48 81 ec 28 04 00 00 	sub    $0x428,%rsp
  2006d5:	48 c7 44 24 10 00 00 	movq   $0x0,0x10(%rsp)
  2006dc:	00 00 
  2006de:	48 81 ff 00 04 00 00 	cmp    $0x400,%rdi
  2006e5:	76 19                	jbe    200700 <sys_pwrite+0x50>
  2006e7:	48 8d 74 24 10       	lea    0x10(%rsp),%rsi
  2006ec:	e8 0f 0a 00 00       	call   201100 <pebble_alloc>
  2006f1:	85 c0                	test   %eax,%eax
  2006f3:	0f 88 57 01 00 00    	js     200850 <sys_pwrite+0x1a0>
  2006f9:	4c 8b 7c 24 10       	mov    0x10(%rsp),%r15
  2006fe:	eb 05                	jmp    200705 <sys_pwrite+0x55>
  200700:	4c 8d 7c 24 20       	lea    0x20(%rsp),%r15
  200705:	4d 89 eb             	mov    %r13,%r11
  200708:	4c 89 eb             	mov    %r13,%rbx
  20070b:	45 89 f2             	mov    %r14d,%r10d
  20070e:	44 89 e0             	mov    %r12d,%eax
  200711:	49 c1 eb 10          	shr    $0x10,%r11
  200715:	0f b6 df             	movzbl %bh,%ebx
  200718:	41 c1 fa 18          	sar    $0x18,%r10d
  20071c:	44 89 f1             	mov    %r14d,%ecx
  20071f:	4c 89 da             	mov    %r11,%rdx
  200722:	45 0f b6 db          	movzbl %r11b,%r11d
  200726:	45 0f b6 d2          	movzbl %r10b,%r10d
  20072a:	c1 f8 18             	sar    $0x18,%eax
  20072d:	81 e2 00 ff 00 00    	and    $0xff00,%edx
  200733:	45 89 e1             	mov    %r12d,%r9d
  200736:	c1 f9 10             	sar    $0x10,%ecx
  200739:	0f b6 c0             	movzbl %al,%eax
  20073c:	4c 09 da             	or     %r11,%rdx
  20073f:	45 0f b6 dd          	movzbl %r13b,%r11d
  200743:	41 c1 f9 10          	sar    $0x10,%r9d
  200747:	0f b6 c9             	movzbl %cl,%ecx
  20074a:	48 c1 e2 08          	shl    $0x8,%rdx
  20074e:	45 0f b6 c9          	movzbl %r9b,%r9d
  200752:	48 c1 e0 08          	shl    $0x8,%rax
  200756:	4d 89 e8             	mov    %r13,%r8
  200759:	48 09 da             	or     %rbx,%rdx
  20075c:	44 89 f3             	mov    %r14d,%ebx
  20075f:	4c 09 c8             	or     %r9,%rax
  200762:	45 0f b6 cc          	movzbl %r12b,%r9d
  200766:	48 c1 e2 08          	shl    $0x8,%rdx
  20076a:	48 c1 e0 08          	shl    $0x8,%rax
  20076e:	4c 89 ef             	mov    %r13,%rdi
  200771:	4c 89 ee             	mov    %r13,%rsi
  200774:	49 c1 e8 30          	shr    $0x30,%r8
  200778:	4c 09 da             	or     %r11,%rdx
  20077b:	48 c1 ef 28          	shr    $0x28,%rdi
  20077f:	48 c1 e2 08          	shl    $0x8,%rdx
  200783:	45 0f b6 c0          	movzbl %r8b,%r8d
  200787:	40 0f b6 ff          	movzbl %dil,%edi
  20078b:	48 c1 ee 20          	shr    $0x20,%rsi
  20078f:	4c 09 d2             	or     %r10,%rdx
  200792:	40 0f b6 f6          	movzbl %sil,%esi
  200796:	48 c1 e2 08          	shl    $0x8,%rdx
  20079a:	48 09 ca             	or     %rcx,%rdx
  20079d:	0f b6 cf             	movzbl %bh,%ecx
  2007a0:	4c 89 e3             	mov    %r12,%rbx
  2007a3:	0f b6 df             	movzbl %bh,%ebx
  2007a6:	48 c1 e2 08          	shl    $0x8,%rdx
  2007aa:	48 09 d8             	or     %rbx,%rax
  2007ad:	48 09 ca             	or     %rcx,%rdx
  2007b0:	41 0f b6 ce          	movzbl %r14b,%ecx
  2007b4:	48 c1 e0 08          	shl    $0x8,%rax
  2007b8:	48 c1 e2 08          	shl    $0x8,%rdx
  2007bc:	4c 09 c8             	or     %r9,%rax
  2007bf:	4c 0f a4 e8 08       	shld   $0x8,%r13,%rax
  2007c4:	4d 8d 6f 10          	lea    0x10(%r15),%r13
  2007c8:	48 c1 e0 08          	shl    $0x8,%rax
  2007cc:	4c 09 c0             	or     %r8,%rax
  2007cf:	48 c1 e0 08          	shl    $0x8,%rax
  2007d3:	48 09 f8             	or     %rdi,%rax
  2007d6:	4c 89 ef             	mov    %r13,%rdi
  2007d9:	48 c1 e0 08          	shl    $0x8,%rax
  2007dd:	48 09 ca             	or     %rcx,%rdx
  2007e0:	48 09 f0             	or     %rsi,%rax
  2007e3:	48 89 14 24          	mov    %rdx,(%rsp)
  2007e7:	48 89 ee             	mov    %rbp,%rsi
  2007ea:	4c 89 e2             	mov    %r12,%rdx
  2007ed:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
  2007f2:	66 0f 6f 04 24       	movdqa (%rsp),%xmm0
  2007f7:	41 0f 11 07          	movups %xmm0,(%r15)
  2007fb:	e8 c0 0a 00 00       	call   2012c0 <memmove>
  200800:	4b 8d 54 25 00       	lea    0x0(%r13,%r12,1),%rdx
  200805:	bf 06 00 00 00       	mov    $0x6,%edi
  20080a:	4c 89 fe             	mov    %r15,%rsi
  20080d:	48 8d 4c 24 18       	lea    0x18(%rsp),%rcx
  200812:	44 29 fa             	sub    %r15d,%edx
  200815:	e8 66 fb ff ff       	call   200380 <do_syscall>
  20081a:	48 8b 7c 24 10       	mov    0x10(%rsp),%rdi
  20081f:	89 c3                	mov    %eax,%ebx
  200821:	48 85 ff             	test   %rdi,%rdi
  200824:	74 05                	je     20082b <sys_pwrite+0x17b>
  200826:	e8 35 0a 00 00       	call   201260 <pebble_free>
  20082b:	85 db                	test   %ebx,%ebx
  20082d:	78 21                	js     200850 <sys_pwrite+0x1a0>
  20082f:	48 8b 44 24 18       	mov    0x18(%rsp),%rax
  200834:	48 81 c4 28 04 00 00 	add    $0x428,%rsp
  20083b:	5b                   	pop    %rbx
  20083c:	5d                   	pop    %rbp
  20083d:	41 5c                	pop    %r12
  20083f:	41 5d                	pop    %r13
  200841:	41 5e                	pop    %r14
  200843:	41 5f                	pop    %r15
  200845:	c3                   	ret
  200846:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  20084d:	00 00 00 
  200850:	48 c7 c0 ff ff ff ff 	mov    $0xffffffffffffffff,%rax
  200857:	eb db                	jmp    200834 <sys_pwrite+0x184>
  200859:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)

0000000000200860 <sys_exit>:
  200860:	f3 0f 1e fa          	endbr64
  200864:	55                   	push   %rbp
  200865:	53                   	push   %rbx
  200866:	48 81 ec 08 01 00 00 	sub    $0x108,%rsp
  20086d:	48 85 ff             	test   %rdi,%rdi
  200870:	74 4e                	je     2008c0 <sys_exit+0x60>
  200872:	80 3f 00             	cmpb   $0x0,(%rdi)
  200875:	48 89 fe             	mov    %rdi,%rsi
  200878:	74 5a                	je     2008d4 <sys_exit+0x74>
  20087a:	b8 01 00 00 00       	mov    $0x1,%eax
  20087f:	90                   	nop
  200880:	48 89 c2             	mov    %rax,%rdx
  200883:	48 8d 40 01          	lea    0x1(%rax),%rax
  200887:	80 3c 16 00          	cmpb   $0x0,(%rsi,%rdx,1)
  20088b:	75 f3                	jne    200880 <sys_exit+0x20>
  20088d:	89 d1                	mov    %edx,%ecx
  20088f:	0f b6 c6             	movzbl %dh,%eax
  200892:	8d 5a 02             	lea    0x2(%rdx),%ebx
  200895:	48 8d 7c 24 02       	lea    0x2(%rsp),%rdi
  20089a:	48 89 e5             	mov    %rsp,%rbp
  20089d:	88 0c 24             	mov    %cl,(%rsp)
  2008a0:	88 44 24 01          	mov    %al,0x1(%rsp)
  2008a4:	e8 17 0a 00 00       	call   2012c0 <memmove>
  2008a9:	31 c9                	xor    %ecx,%ecx
  2008ab:	89 da                	mov    %ebx,%edx
  2008ad:	48 89 ee             	mov    %rbp,%rsi
  2008b0:	bf 08 00 00 00       	mov    $0x8,%edi
  2008b5:	e8 c6 fa ff ff       	call   200380 <do_syscall>
  2008ba:	eb fe                	jmp    2008ba <sys_exit+0x5a>
  2008bc:	0f 1f 40 00          	nopl   0x0(%rax)
  2008c0:	bb 02 00 00 00       	mov    $0x2,%ebx
  2008c5:	31 c0                	xor    %eax,%eax
  2008c7:	31 c9                	xor    %ecx,%ecx
  2008c9:	31 d2                	xor    %edx,%edx
  2008cb:	48 8d 35 e1 28 00 00 	lea    0x28e1(%rip),%rsi        # 2031b3 <_syscall+0x6e>
  2008d2:	eb c1                	jmp    200895 <sys_exit+0x35>
  2008d4:	bb 02 00 00 00       	mov    $0x2,%ebx
  2008d9:	31 c0                	xor    %eax,%eax
  2008db:	31 c9                	xor    %ecx,%ecx
  2008dd:	31 d2                	xor    %edx,%edx
  2008df:	eb b4                	jmp    200895 <sys_exit+0x35>
  2008e1:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  2008e8:	00 00 00 00 
  2008ec:	0f 1f 40 00          	nopl   0x0(%rax)

00000000002008f0 <sys_create>:
  2008f0:	f3 0f 1e fa          	endbr64
  2008f4:	41 55                	push   %r13
  2008f6:	41 54                	push   %r12
  2008f8:	41 89 d4             	mov    %edx,%r12d
  2008fb:	55                   	push   %rbp
  2008fc:	89 f5                	mov    %esi,%ebp
  2008fe:	53                   	push   %rbx
  2008ff:	48 81 ec 18 04 00 00 	sub    $0x418,%rsp
  200906:	80 3f 00             	cmpb   $0x0,(%rdi)
  200909:	0f 84 e9 00 00 00    	je     2009f8 <sys_create+0x108>
  20090f:	b8 01 00 00 00       	mov    $0x1,%eax
  200914:	0f 1f 40 00          	nopl   0x0(%rax)
  200918:	48 89 c2             	mov    %rax,%rdx
  20091b:	48 8d 40 01          	lea    0x1(%rax),%rax
  20091f:	80 3c 17 00          	cmpb   $0x0,(%rdi,%rdx,1)
  200923:	75 f3                	jne    200918 <sys_create+0x28>
  200925:	44 8d 6a 02          	lea    0x2(%rdx),%r13d
  200929:	89 d1                	mov    %edx,%ecx
  20092b:	0f b6 c6             	movzbl %dh,%eax
  20092e:	4d 63 ed             	movslq %r13d,%r13
  200931:	88 44 24 11          	mov    %al,0x11(%rsp)
  200935:	48 8d 44 24 12       	lea    0x12(%rsp),%rax
  20093a:	48 89 fe             	mov    %rdi,%rsi
  20093d:	48 8d 5c 24 10       	lea    0x10(%rsp),%rbx
  200942:	48 89 c7             	mov    %rax,%rdi
  200945:	88 4c 24 10          	mov    %cl,0x10(%rsp)
  200949:	e8 72 09 00 00       	call   2012c0 <memmove>
  20094e:	44 89 e0             	mov    %r12d,%eax
  200951:	44 89 e7             	mov    %r12d,%edi
  200954:	89 ee                	mov    %ebp,%esi
  200956:	c1 f8 18             	sar    $0x18,%eax
  200959:	c1 ff 10             	sar    $0x10,%edi
  20095c:	89 e9                	mov    %ebp,%ecx
  20095e:	4a 8d 14 2b          	lea    (%rbx,%r13,1),%rdx
  200962:	0f b6 c0             	movzbl %al,%eax
  200965:	40 0f b6 ff          	movzbl %dil,%edi
  200969:	c1 fe 18             	sar    $0x18,%esi
  20096c:	48 83 c2 08          	add    $0x8,%rdx
  200970:	48 c1 e0 08          	shl    $0x8,%rax
  200974:	40 0f b6 f6          	movzbl %sil,%esi
  200978:	c1 f9 10             	sar    $0x10,%ecx
  20097b:	48 09 f8             	or     %rdi,%rax
  20097e:	0f b6 c9             	movzbl %cl,%ecx
  200981:	49 89 c0             	mov    %rax,%r8
  200984:	44 89 e0             	mov    %r12d,%eax
  200987:	45 0f b6 e4          	movzbl %r12b,%r12d
  20098b:	0f b6 fc             	movzbl %ah,%edi
  20098e:	4c 89 c0             	mov    %r8,%rax
  200991:	48 c1 e0 08          	shl    $0x8,%rax
  200995:	48 09 f8             	or     %rdi,%rax
  200998:	bf 07 00 00 00       	mov    $0x7,%edi
  20099d:	48 c1 e0 08          	shl    $0x8,%rax
  2009a1:	4c 09 e0             	or     %r12,%rax
  2009a4:	48 c1 e0 08          	shl    $0x8,%rax
  2009a8:	48 09 f0             	or     %rsi,%rax
  2009ab:	48 89 de             	mov    %rbx,%rsi
  2009ae:	48 c1 e0 08          	shl    $0x8,%rax
  2009b2:	48 09 c8             	or     %rcx,%rax
  2009b5:	89 e9                	mov    %ebp,%ecx
  2009b7:	40 0f b6 ed          	movzbl %bpl,%ebp
  2009bb:	0f b6 cd             	movzbl %ch,%ecx
  2009be:	48 c1 e0 08          	shl    $0x8,%rax
  2009c2:	48 09 c8             	or     %rcx,%rax
  2009c5:	48 8d 4c 24 08       	lea    0x8(%rsp),%rcx
  2009ca:	48 c1 e0 08          	shl    $0x8,%rax
  2009ce:	48 09 e8             	or     %rbp,%rax
  2009d1:	48 89 42 f8          	mov    %rax,-0x8(%rdx)
  2009d5:	29 da                	sub    %ebx,%edx
  2009d7:	e8 a4 f9 ff ff       	call   200380 <do_syscall>
  2009dc:	85 c0                	test   %eax,%eax
  2009de:	78 29                	js     200a09 <sys_create+0x119>
  2009e0:	8b 44 24 08          	mov    0x8(%rsp),%eax
  2009e4:	48 81 c4 18 04 00 00 	add    $0x418,%rsp
  2009eb:	5b                   	pop    %rbx
  2009ec:	5d                   	pop    %rbp
  2009ed:	41 5c                	pop    %r12
  2009ef:	41 5d                	pop    %r13
  2009f1:	c3                   	ret
  2009f2:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  2009f8:	31 c0                	xor    %eax,%eax
  2009fa:	31 c9                	xor    %ecx,%ecx
  2009fc:	41 bd 02 00 00 00    	mov    $0x2,%r13d
  200a02:	31 d2                	xor    %edx,%edx
  200a04:	e9 28 ff ff ff       	jmp    200931 <sys_create+0x41>
  200a09:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  200a0e:	eb d4                	jmp    2009e4 <sys_create+0xf4>

0000000000200a10 <sys_rfork>:
  200a10:	f3 0f 1e fa          	endbr64
  200a14:	48 83 ec 28          	sub    $0x28,%rsp
  200a18:	ba 04 00 00 00       	mov    $0x4,%edx
  200a1d:	89 7c 24 10          	mov    %edi,0x10(%rsp)
  200a21:	48 8d 4c 24 08       	lea    0x8(%rsp),%rcx
  200a26:	48 8d 74 24 10       	lea    0x10(%rsp),%rsi
  200a2b:	bf 13 00 00 00       	mov    $0x13,%edi
  200a30:	e8 4b f9 ff ff       	call   200380 <do_syscall>
  200a35:	85 c0                	test   %eax,%eax
  200a37:	78 09                	js     200a42 <sys_rfork+0x32>
  200a39:	8b 44 24 08          	mov    0x8(%rsp),%eax
  200a3d:	48 83 c4 28          	add    $0x28,%rsp
  200a41:	c3                   	ret
  200a42:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  200a47:	eb f4                	jmp    200a3d <sys_rfork+0x2d>
  200a49:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)

0000000000200a50 <sys_exec>:
  200a50:	f3 0f 1e fa          	endbr64
  200a54:	41 54                	push   %r12
  200a56:	ba 98 01 00 00       	mov    $0x198,%edx
  200a5b:	31 f6                	xor    %esi,%esi
  200a5d:	55                   	push   %rbp
  200a5e:	53                   	push   %rbx
  200a5f:	48 89 fb             	mov    %rdi,%rbx
  200a62:	48 81 ec 40 03 00 00 	sub    $0x340,%rsp
  200a69:	48 89 e5             	mov    %rsp,%rbp
  200a6c:	4c 8d a4 24 a0 01 00 	lea    0x1a0(%rsp),%r12
  200a73:	00 
  200a74:	48 89 ef             	mov    %rbp,%rdi
  200a77:	e8 94 08 00 00       	call   201310 <memset>
  200a7c:	4c 89 e7             	mov    %r12,%rdi
  200a7f:	ba 98 01 00 00       	mov    $0x198,%edx
  200a84:	31 f6                	xor    %esi,%esi
  200a86:	e8 85 08 00 00       	call   201310 <memset>
  200a8b:	b8 01 00 00 00       	mov    $0x1,%eax
  200a90:	4c 89 e6             	mov    %r12,%rsi
  200a93:	48 89 ef             	mov    %rbp,%rdi
  200a96:	48 89 5c 24 18       	mov    %rbx,0x18(%rsp)
  200a9b:	c6 04 24 80          	movb   $0x80,(%rsp)
  200a9f:	66 89 44 24 08       	mov    %ax,0x8(%rsp)
  200aa4:	e8 67 f8 ff ff       	call   200310 <lux_call>
  200aa9:	48 81 c4 40 03 00 00 	add    $0x340,%rsp
  200ab0:	5b                   	pop    %rbx
  200ab1:	5d                   	pop    %rbp
  200ab2:	41 5c                	pop    %r12
  200ab4:	c3                   	ret
  200ab5:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  200abc:	00 00 00 00 

0000000000200ac0 <sys_pipe>:
  200ac0:	f3 0f 1e fa          	endbr64
  200ac4:	41 54                	push   %r12
  200ac6:	31 f6                	xor    %esi,%esi
  200ac8:	ba 98 01 00 00       	mov    $0x198,%edx
  200acd:	55                   	push   %rbp
  200ace:	53                   	push   %rbx
  200acf:	48 89 fb             	mov    %rdi,%rbx
  200ad2:	48 81 ec 40 03 00 00 	sub    $0x340,%rsp
  200ad9:	48 89 e5             	mov    %rsp,%rbp
  200adc:	4c 8d a4 24 a0 01 00 	lea    0x1a0(%rsp),%r12
  200ae3:	00 
  200ae4:	48 89 ef             	mov    %rbp,%rdi
  200ae7:	e8 24 08 00 00       	call   201310 <memset>
  200aec:	31 f6                	xor    %esi,%esi
  200aee:	4c 89 e7             	mov    %r12,%rdi
  200af1:	ba 98 01 00 00       	mov    $0x198,%edx
  200af6:	e8 15 08 00 00       	call   201310 <memset>
  200afb:	b8 01 00 00 00       	mov    $0x1,%eax
  200b00:	4c 89 e6             	mov    %r12,%rsi
  200b03:	48 89 ef             	mov    %rbp,%rdi
  200b06:	c6 04 24 82          	movb   $0x82,(%rsp)
  200b0a:	66 89 44 24 08       	mov    %ax,0x8(%rsp)
  200b0f:	c7 44 24 10 15 00 00 	movl   $0x15,0x10(%rsp)
  200b16:	00 
  200b17:	48 c7 44 24 18 00 00 	movq   $0x0,0x18(%rsp)
  200b1e:	00 00 
  200b20:	c7 44 24 20 00 00 00 	movl   $0x0,0x20(%rsp)
  200b27:	00 
  200b28:	e8 e3 f7 ff ff       	call   200310 <lux_call>
  200b2d:	85 c0                	test   %eax,%eax
  200b2f:	78 2f                	js     200b60 <sys_pipe+0xa0>
  200b31:	4c 89 e6             	mov    %r12,%rsi
  200b34:	48 89 ef             	mov    %rbp,%rdi
  200b37:	c6 04 24 c0          	movb   $0xc0,(%rsp)
  200b3b:	e8 d0 f7 ff ff       	call   200310 <lux_call>
  200b40:	48 8b 84 24 b0 01 00 	mov    0x1b0(%rsp),%rax
  200b47:	00 
  200b48:	48 89 03             	mov    %rax,(%rbx)
  200b4b:	31 c0                	xor    %eax,%eax
  200b4d:	48 81 c4 40 03 00 00 	add    $0x340,%rsp
  200b54:	5b                   	pop    %rbx
  200b55:	5d                   	pop    %rbp
  200b56:	41 5c                	pop    %r12
  200b58:	c3                   	ret
  200b59:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  200b60:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  200b65:	eb e6                	jmp    200b4d <sys_pipe+0x8d>
  200b67:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  200b6e:	00 00 

0000000000200b70 <sys_seek>:
  200b70:	f3 0f 1e fa          	endbr64
  200b74:	41 56                	push   %r14
  200b76:	41 89 d1             	mov    %edx,%r9d
  200b79:	41 89 d2             	mov    %edx,%r10d
  200b7c:	89 d0                	mov    %edx,%eax
  200b7e:	41 54                	push   %r12
  200b80:	49 89 f4             	mov    %rsi,%r12
  200b83:	c1 f8 18             	sar    $0x18,%eax
  200b86:	41 89 fb             	mov    %edi,%r11d
  200b89:	49 c1 ec 10          	shr    $0x10,%r12
  200b8d:	55                   	push   %rbp
  200b8e:	41 89 c6             	mov    %eax,%r14d
  200b91:	89 fd                	mov    %edi,%ebp
  200b93:	4c 89 e2             	mov    %r12,%rdx
  200b96:	45 0f b6 e4          	movzbl %r12b,%r12d
  200b9a:	53                   	push   %rbx
  200b9b:	48 89 f3             	mov    %rsi,%rbx
  200b9e:	81 e2 00 ff 00 00    	and    $0xff00,%edx
  200ba4:	0f b6 c7             	movzbl %bh,%eax
  200ba7:	c1 fd 18             	sar    $0x18,%ebp
  200baa:	89 f9                	mov    %edi,%ecx
  200bac:	4c 09 e2             	or     %r12,%rdx
  200baf:	44 0f b6 e3          	movzbl %bl,%r12d
  200bb3:	40 0f b6 ed          	movzbl %bpl,%ebp
  200bb7:	41 c1 fb 10          	sar    $0x10,%r11d
  200bbb:	48 c1 e2 08          	shl    $0x8,%rdx
  200bbf:	45 0f b6 db          	movzbl %r11b,%r11d
  200bc3:	41 c1 fa 10          	sar    $0x10,%r10d
  200bc7:	49 89 d8             	mov    %rbx,%r8
  200bca:	48 09 c2             	or     %rax,%rdx
  200bcd:	0f b6 c5             	movzbl %ch,%eax
  200bd0:	45 0f b6 d2          	movzbl %r10b,%r10d
  200bd4:	49 c1 e8 30          	shr    $0x30,%r8
  200bd8:	48 c1 e2 08          	shl    $0x8,%rdx
  200bdc:	48 89 df             	mov    %rbx,%rdi
  200bdf:	45 0f b6 c0          	movzbl %r8b,%r8d
  200be3:	48 83 ec 48          	sub    $0x48,%rsp
  200be7:	48 c1 ef 28          	shr    $0x28,%rdi
  200beb:	4c 09 e2             	or     %r12,%rdx
  200bee:	48 c1 ee 20          	shr    $0x20,%rsi
  200bf2:	0f b6 c9             	movzbl %cl,%ecx
  200bf5:	48 c1 e2 08          	shl    $0x8,%rdx
  200bf9:	40 0f b6 ff          	movzbl %dil,%edi
  200bfd:	40 0f b6 f6          	movzbl %sil,%esi
  200c01:	48 09 ea             	or     %rbp,%rdx
  200c04:	48 c1 e2 08          	shl    $0x8,%rdx
  200c08:	4c 09 da             	or     %r11,%rdx
  200c0b:	48 c1 e2 08          	shl    $0x8,%rdx
  200c0f:	48 09 c2             	or     %rax,%rdx
  200c12:	41 0f b6 c6          	movzbl %r14b,%eax
  200c16:	48 c1 e0 08          	shl    $0x8,%rax
  200c1a:	48 c1 e2 08          	shl    $0x8,%rdx
  200c1e:	4c 09 d0             	or     %r10,%rax
  200c21:	49 89 c6             	mov    %rax,%r14
  200c24:	44 89 c8             	mov    %r9d,%eax
  200c27:	45 0f b6 c9          	movzbl %r9b,%r9d
  200c2b:	0f b6 c4             	movzbl %ah,%eax
  200c2e:	49 89 c2             	mov    %rax,%r10
  200c31:	4c 89 f0             	mov    %r14,%rax
  200c34:	48 c1 e0 08          	shl    $0x8,%rax
  200c38:	4c 09 d0             	or     %r10,%rax
  200c3b:	48 c1 e0 08          	shl    $0x8,%rax
  200c3f:	4c 09 c8             	or     %r9,%rax
  200c42:	48 0f a4 d8 08       	shld   $0x8,%rbx,%rax
  200c47:	48 c1 e0 08          	shl    $0x8,%rax
  200c4b:	4c 09 c0             	or     %r8,%rax
  200c4e:	48 c1 e0 08          	shl    $0x8,%rax
  200c52:	48 09 f8             	or     %rdi,%rax
  200c55:	48 09 ca             	or     %rcx,%rdx
  200c58:	48 8d 4c 24 18       	lea    0x18(%rsp),%rcx
  200c5d:	bf 27 00 00 00       	mov    $0x27,%edi
  200c62:	48 c1 e0 08          	shl    $0x8,%rax
  200c66:	48 89 14 24          	mov    %rdx,(%rsp)
  200c6a:	ba 10 00 00 00       	mov    $0x10,%edx
  200c6f:	48 09 f0             	or     %rsi,%rax
  200c72:	48 8d 74 24 20       	lea    0x20(%rsp),%rsi
  200c77:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
  200c7c:	66 0f 6f 04 24       	movdqa (%rsp),%xmm0
  200c81:	0f 29 44 24 20       	movaps %xmm0,0x20(%rsp)
  200c86:	e8 f5 f6 ff ff       	call   200380 <do_syscall>
  200c8b:	85 c0                	test   %eax,%eax
  200c8d:	78 10                	js     200c9f <sys_seek+0x12f>
  200c8f:	48 8b 44 24 18       	mov    0x18(%rsp),%rax
  200c94:	48 83 c4 48          	add    $0x48,%rsp
  200c98:	5b                   	pop    %rbx
  200c99:	5d                   	pop    %rbp
  200c9a:	41 5c                	pop    %r12
  200c9c:	41 5e                	pop    %r14
  200c9e:	c3                   	ret
  200c9f:	48 c7 c0 ff ff ff ff 	mov    $0xffffffffffffffff,%rax
  200ca6:	eb ec                	jmp    200c94 <sys_seek+0x124>
  200ca8:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
  200caf:	00 

0000000000200cb0 <sys_wait>:
  200cb0:	f3 0f 1e fa          	endbr64
  200cb4:	55                   	push   %rbp
  200cb5:	31 f6                	xor    %esi,%esi
  200cb7:	ba 98 01 00 00       	mov    $0x198,%edx
  200cbc:	53                   	push   %rbx
  200cbd:	48 81 ec 48 03 00 00 	sub    $0x348,%rsp
  200cc4:	48 89 e3             	mov    %rsp,%rbx
  200cc7:	48 8d ac 24 a0 01 00 	lea    0x1a0(%rsp),%rbp
  200cce:	00 
  200ccf:	48 89 df             	mov    %rbx,%rdi
  200cd2:	e8 39 06 00 00       	call   201310 <memset>
  200cd7:	31 f6                	xor    %esi,%esi
  200cd9:	48 89 ef             	mov    %rbp,%rdi
  200cdc:	ba 98 01 00 00       	mov    $0x198,%edx
  200ce1:	e8 2a 06 00 00       	call   201310 <memset>
  200ce6:	b8 01 00 00 00       	mov    $0x1,%eax
  200ceb:	48 89 ee             	mov    %rbp,%rsi
  200cee:	48 89 df             	mov    %rbx,%rdi
  200cf1:	c6 04 24 a6          	movb   $0xa6,(%rsp)
  200cf5:	66 89 44 24 08       	mov    %ax,0x8(%rsp)
  200cfa:	e8 11 f6 ff ff       	call   200310 <lux_call>
  200cff:	85 c0                	test   %eax,%eax
  200d01:	78 11                	js     200d14 <sys_wait+0x64>
  200d03:	8b 84 24 b4 01 00 00 	mov    0x1b4(%rsp),%eax
  200d0a:	48 81 c4 48 03 00 00 	add    $0x348,%rsp
  200d11:	5b                   	pop    %rbx
  200d12:	5d                   	pop    %rbp
  200d13:	c3                   	ret
  200d14:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  200d19:	eb ef                	jmp    200d0a <sys_wait+0x5a>
  200d1b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)

0000000000200d20 <sys_pread>:
  200d20:	f3 0f 1e fa          	endbr64
  200d24:	49 89 c9             	mov    %rcx,%r9
  200d27:	41 56                	push   %r14
  200d29:	41 89 d3             	mov    %edx,%r11d
  200d2c:	89 d0                	mov    %edx,%eax
  200d2e:	41 55                	push   %r13
  200d30:	4d 89 cd             	mov    %r9,%r13
  200d33:	89 f9                	mov    %edi,%ecx
  200d35:	c1 f8 18             	sar    $0x18,%eax
  200d38:	49 c1 ed 10          	shr    $0x10,%r13
  200d3c:	41 54                	push   %r12
  200d3e:	41 89 fc             	mov    %edi,%r12d
  200d41:	41 89 fa             	mov    %edi,%r10d
  200d44:	55                   	push   %rbp
  200d45:	48 89 d5             	mov    %rdx,%rbp
  200d48:	4c 89 ea             	mov    %r13,%rdx
  200d4b:	45 0f b6 ed          	movzbl %r13b,%r13d
  200d4f:	81 e2 00 ff 00 00    	and    $0xff00,%edx
  200d55:	53                   	push   %rbx
  200d56:	4c 89 cb             	mov    %r9,%rbx
  200d59:	41 c1 fc 18          	sar    $0x18,%r12d
  200d5d:	4c 09 ea             	or     %r13,%rdx
  200d60:	0f b6 df             	movzbl %bh,%ebx
  200d63:	45 0f b6 e9          	movzbl %r9b,%r13d
  200d67:	45 0f b6 e4          	movzbl %r12b,%r12d
  200d6b:	48 c1 e2 08          	shl    $0x8,%rdx
  200d6f:	c1 f9 10             	sar    $0x10,%ecx
  200d72:	0f b6 c0             	movzbl %al,%eax
  200d75:	4d 89 c8             	mov    %r9,%r8
  200d78:	48 09 da             	or     %rbx,%rdx
  200d7b:	41 c1 fb 10          	sar    $0x10,%r11d
  200d7f:	44 89 d3             	mov    %r10d,%ebx
  200d82:	0f b6 c9             	movzbl %cl,%ecx
  200d85:	48 c1 e2 08          	shl    $0x8,%rdx
  200d89:	45 0f b6 db          	movzbl %r11b,%r11d
  200d8d:	48 c1 e0 08          	shl    $0x8,%rax
  200d91:	4c 89 cf             	mov    %r9,%rdi
  200d94:	4c 09 ea             	or     %r13,%rdx
  200d97:	4c 09 d8             	or     %r11,%rax
  200d9a:	49 c1 e8 30          	shr    $0x30,%r8
  200d9e:	49 89 f6             	mov    %rsi,%r14
  200da1:	48 c1 e2 08          	shl    $0x8,%rdx
  200da5:	48 c1 e0 08          	shl    $0x8,%rax
  200da9:	45 0f b6 c0          	movzbl %r8b,%r8d
  200dad:	4c 89 ce             	mov    %r9,%rsi
  200db0:	4c 09 e2             	or     %r12,%rdx
  200db3:	48 c1 ef 28          	shr    $0x28,%rdi
  200db7:	48 81 ec 70 03 00 00 	sub    $0x370,%rsp
  200dbe:	48 c1 e2 08          	shl    $0x8,%rdx
  200dc2:	40 0f b6 ff          	movzbl %dil,%edi
  200dc6:	48 c1 ee 20          	shr    $0x20,%rsi
  200dca:	4c 8d 64 24 30       	lea    0x30(%rsp),%r12
  200dcf:	48 09 ca             	or     %rcx,%rdx
  200dd2:	0f b6 cf             	movzbl %bh,%ecx
  200dd5:	48 89 eb             	mov    %rbp,%rbx
  200dd8:	40 0f b6 f6          	movzbl %sil,%esi
  200ddc:	0f b6 df             	movzbl %bh,%ebx
  200ddf:	48 c1 e2 08          	shl    $0x8,%rdx
  200de3:	4c 8d ac 24 d0 01 00 	lea    0x1d0(%rsp),%r13
  200dea:	00 
  200deb:	48 09 d8             	or     %rbx,%rax
  200dee:	48 09 ca             	or     %rcx,%rdx
  200df1:	41 0f b6 ca          	movzbl %r10b,%ecx
  200df5:	44 0f b6 d5          	movzbl %bpl,%r10d
  200df9:	48 c1 e0 08          	shl    $0x8,%rax
  200dfd:	48 c1 e2 08          	shl    $0x8,%rdx
  200e01:	4c 09 d0             	or     %r10,%rax
  200e04:	4c 0f a4 c8 08       	shld   $0x8,%r9,%rax
  200e09:	48 c1 e0 08          	shl    $0x8,%rax
  200e0d:	4c 09 c0             	or     %r8,%rax
  200e10:	48 c1 e0 08          	shl    $0x8,%rax
  200e14:	48 09 f8             	or     %rdi,%rax
  200e17:	48 09 ca             	or     %rcx,%rdx
  200e1a:	4c 89 e7             	mov    %r12,%rdi
  200e1d:	48 c1 e0 08          	shl    $0x8,%rax
  200e21:	48 89 14 24          	mov    %rdx,(%rsp)
  200e25:	ba 98 01 00 00       	mov    $0x198,%edx
  200e2a:	48 09 f0             	or     %rsi,%rax
  200e2d:	31 f6                	xor    %esi,%esi
  200e2f:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
  200e34:	66 0f 6f 04 24       	movdqa (%rsp),%xmm0
  200e39:	0f 29 44 24 10       	movaps %xmm0,0x10(%rsp)
  200e3e:	e8 cd 04 00 00       	call   201310 <memset>
  200e43:	31 f6                	xor    %esi,%esi
  200e45:	4c 89 ef             	mov    %r13,%rdi
  200e48:	ba 98 01 00 00       	mov    $0x198,%edx
  200e4d:	e8 be 04 00 00       	call   201310 <memset>
  200e52:	b8 01 00 00 00       	mov    $0x1,%eax
  200e57:	4c 89 ee             	mov    %r13,%rsi
  200e5a:	4c 89 e7             	mov    %r12,%rdi
  200e5d:	66 89 44 24 38       	mov    %ax,0x38(%rsp)
  200e62:	48 8d 44 24 10       	lea    0x10(%rsp),%rax
  200e67:	c6 44 24 30 82       	movb   $0x82,0x30(%rsp)
  200e6c:	c7 44 24 40 05 00 00 	movl   $0x5,0x40(%rsp)
  200e73:	00 
  200e74:	48 89 44 24 48       	mov    %rax,0x48(%rsp)
  200e79:	c7 44 24 50 10 00 00 	movl   $0x10,0x50(%rsp)
  200e80:	00 
  200e81:	e8 8a f4 ff ff       	call   200310 <lux_call>
  200e86:	85 c0                	test   %eax,%eax
  200e88:	78 4e                	js     200ed8 <sys_pread+0x1b8>
  200e8a:	8b 94 24 e8 01 00 00 	mov    0x1e8(%rsp),%edx
  200e91:	48 89 d0             	mov    %rdx,%rax
  200e94:	48 39 ea             	cmp    %rbp,%rdx
  200e97:	7e 0c                	jle    200ea5 <sys_pread+0x185>
  200e99:	89 ac 24 e8 01 00 00 	mov    %ebp,0x1e8(%rsp)
  200ea0:	89 ea                	mov    %ebp,%edx
  200ea2:	48 89 d0             	mov    %rdx,%rax
  200ea5:	85 c0                	test   %eax,%eax
  200ea7:	74 1c                	je     200ec5 <sys_pread+0x1a5>
  200ea9:	48 8b b4 24 e8 01 00 	mov    0x1e8(%rsp),%rsi
  200eb0:	00 
  200eb1:	48 85 f6             	test   %rsi,%rsi
  200eb4:	74 0f                	je     200ec5 <sys_pread+0x1a5>
  200eb6:	4c 89 f7             	mov    %r14,%rdi
  200eb9:	e8 02 04 00 00       	call   2012c0 <memmove>
  200ebe:	8b 94 24 e8 01 00 00 	mov    0x1e8(%rsp),%edx
  200ec5:	48 89 d0             	mov    %rdx,%rax
  200ec8:	48 81 c4 70 03 00 00 	add    $0x370,%rsp
  200ecf:	5b                   	pop    %rbx
  200ed0:	5d                   	pop    %rbp
  200ed1:	41 5c                	pop    %r12
  200ed3:	41 5d                	pop    %r13
  200ed5:	41 5e                	pop    %r14
  200ed7:	c3                   	ret
  200ed8:	48 c7 c0 ff ff ff ff 	mov    $0xffffffffffffffff,%rax
  200edf:	eb e7                	jmp    200ec8 <sys_pread+0x1a8>
  200ee1:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  200ee8:	00 00 00 00 
  200eec:	0f 1f 40 00          	nopl   0x0(%rax)

0000000000200ef0 <sys_nsec>:
  200ef0:	f3 0f 1e fa          	endbr64
  200ef4:	55                   	push   %rbp
  200ef5:	ba 98 01 00 00       	mov    $0x198,%edx
  200efa:	31 f6                	xor    %esi,%esi
  200efc:	53                   	push   %rbx
  200efd:	48 81 ec 48 03 00 00 	sub    $0x348,%rsp
  200f04:	48 89 e3             	mov    %rsp,%rbx
  200f07:	48 8d ac 24 a0 01 00 	lea    0x1a0(%rsp),%rbp
  200f0e:	00 
  200f0f:	48 89 df             	mov    %rbx,%rdi
  200f12:	e8 f9 03 00 00       	call   201310 <memset>
  200f17:	ba 98 01 00 00       	mov    $0x198,%edx
  200f1c:	31 f6                	xor    %esi,%esi
  200f1e:	48 89 ef             	mov    %rbp,%rdi
  200f21:	e8 ea 03 00 00       	call   201310 <memset>
  200f26:	b8 01 00 00 00       	mov    $0x1,%eax
  200f2b:	48 89 ee             	mov    %rbp,%rsi
  200f2e:	48 89 df             	mov    %rbx,%rdi
  200f31:	c6 04 24 82          	movb   $0x82,(%rsp)
  200f35:	66 89 44 24 08       	mov    %ax,0x8(%rsp)
  200f3a:	c7 44 24 10 35 00 00 	movl   $0x35,0x10(%rsp)
  200f41:	00 
  200f42:	c7 44 24 20 00 00 00 	movl   $0x0,0x20(%rsp)
  200f49:	00 
  200f4a:	48 c7 44 24 18 00 00 	movq   $0x0,0x18(%rsp)
  200f51:	00 00 
  200f53:	e8 b8 f3 ff ff       	call   200310 <lux_call>
  200f58:	89 c2                	mov    %eax,%edx
  200f5a:	31 c0                	xor    %eax,%eax
  200f5c:	85 d2                	test   %edx,%edx
  200f5e:	78 08                	js     200f68 <sys_nsec+0x78>
  200f60:	48 8b 84 24 c8 01 00 	mov    0x1c8(%rsp),%rax
  200f67:	00 
  200f68:	48 81 c4 48 03 00 00 	add    $0x348,%rsp
  200f6f:	5b                   	pop    %rbx
  200f70:	5d                   	pop    %rbp
  200f71:	c3                   	ret
  200f72:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  200f79:	00 00 00 00 
  200f7d:	0f 1f 00             	nopl   (%rax)

0000000000200f80 <sys_stat>:
  200f80:	f3 0f 1e fa          	endbr64
  200f84:	41 54                	push   %r12
  200f86:	ba 98 01 00 00       	mov    $0x198,%edx
  200f8b:	31 f6                	xor    %esi,%esi
  200f8d:	55                   	push   %rbp
  200f8e:	53                   	push   %rbx
  200f8f:	48 89 fb             	mov    %rdi,%rbx
  200f92:	48 81 ec 40 03 00 00 	sub    $0x340,%rsp
  200f99:	48 89 e5             	mov    %rsp,%rbp
  200f9c:	4c 8d a4 24 a0 01 00 	lea    0x1a0(%rsp),%r12
  200fa3:	00 
  200fa4:	48 89 ef             	mov    %rbp,%rdi
  200fa7:	e8 64 03 00 00       	call   201310 <memset>
  200fac:	4c 89 e7             	mov    %r12,%rdi
  200faf:	ba 98 01 00 00       	mov    $0x198,%edx
  200fb4:	31 f6                	xor    %esi,%esi
  200fb6:	e8 55 03 00 00       	call   201310 <memset>
  200fbb:	b8 01 00 00 00       	mov    $0x1,%eax
  200fc0:	4c 89 e6             	mov    %r12,%rsi
  200fc3:	48 89 ef             	mov    %rbp,%rdi
  200fc6:	48 89 5c 24 18       	mov    %rbx,0x18(%rsp)
  200fcb:	c6 04 24 94          	movb   $0x94,(%rsp)
  200fcf:	66 89 44 24 08       	mov    %ax,0x8(%rsp)
  200fd4:	e8 37 f3 ff ff       	call   200310 <lux_call>
  200fd9:	48 81 c4 40 03 00 00 	add    $0x340,%rsp
  200fe0:	5b                   	pop    %rbx
  200fe1:	c1 f8 1f             	sar    $0x1f,%eax
  200fe4:	5d                   	pop    %rbp
  200fe5:	41 5c                	pop    %r12
  200fe7:	c3                   	ret
  200fe8:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
  200fef:	00 

0000000000200ff0 <sys_wstat>:
  200ff0:	f3 0f 1e fa          	endbr64
  200ff4:	41 55                	push   %r13
  200ff6:	41 54                	push   %r12
  200ff8:	55                   	push   %rbp
  200ff9:	89 d5                	mov    %edx,%ebp
  200ffb:	ba 98 01 00 00       	mov    $0x198,%edx
  201000:	53                   	push   %rbx
  201001:	48 89 f3             	mov    %rsi,%rbx
  201004:	31 f6                	xor    %esi,%esi
  201006:	48 81 ec 48 03 00 00 	sub    $0x348,%rsp
  20100d:	49 89 e4             	mov    %rsp,%r12
  201010:	4c 8d ac 24 a0 01 00 	lea    0x1a0(%rsp),%r13
  201017:	00 
  201018:	4c 89 e7             	mov    %r12,%rdi
  20101b:	e8 f0 02 00 00       	call   201310 <memset>
  201020:	4c 89 ef             	mov    %r13,%rdi
  201023:	ba 98 01 00 00       	mov    $0x198,%edx
  201028:	31 f6                	xor    %esi,%esi
  20102a:	e8 e1 02 00 00       	call   201310 <memset>
  20102f:	b8 01 00 00 00       	mov    $0x1,%eax
  201034:	4c 89 ee             	mov    %r13,%rsi
  201037:	4c 89 e7             	mov    %r12,%rdi
  20103a:	66 89 6c 24 10       	mov    %bp,0x10(%rsp)
  20103f:	48 89 5c 24 18       	mov    %rbx,0x18(%rsp)
  201044:	c6 04 24 98          	movb   $0x98,(%rsp)
  201048:	66 89 44 24 08       	mov    %ax,0x8(%rsp)
  20104d:	e8 be f2 ff ff       	call   200310 <lux_call>
  201052:	48 81 c4 48 03 00 00 	add    $0x348,%rsp
  201059:	5b                   	pop    %rbx
  20105a:	c1 f8 1f             	sar    $0x1f,%eax
  20105d:	5d                   	pop    %rbp
  20105e:	41 5c                	pop    %r12
  201060:	41 5d                	pop    %r13
  201062:	c3                   	ret
  201063:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  20106a:	00 00 00 00 
  20106e:	66 90                	xchg   %ax,%ax

0000000000201070 <sys_mount>:
  201070:	f3 0f 1e fa          	endbr64
  201074:	41 57                	push   %r15
  201076:	31 f6                	xor    %esi,%esi
  201078:	41 56                	push   %r14
  20107a:	41 55                	push   %r13
  20107c:	41 89 fd             	mov    %edi,%r13d
  20107f:	41 54                	push   %r12
  201081:	49 89 d4             	mov    %rdx,%r12
  201084:	ba 98 01 00 00       	mov    $0x198,%edx
  201089:	55                   	push   %rbp
  20108a:	89 cd                	mov    %ecx,%ebp
  20108c:	53                   	push   %rbx
  20108d:	4c 89 c3             	mov    %r8,%rbx
  201090:	48 81 ec 48 03 00 00 	sub    $0x348,%rsp
  201097:	49 89 e6             	mov    %rsp,%r14
  20109a:	4c 8d bc 24 a0 01 00 	lea    0x1a0(%rsp),%r15
  2010a1:	00 
  2010a2:	4c 89 f7             	mov    %r14,%rdi
  2010a5:	e8 66 02 00 00       	call   201310 <memset>
  2010aa:	4c 89 ff             	mov    %r15,%rdi
  2010ad:	ba 98 01 00 00       	mov    $0x198,%edx
  2010b2:	31 f6                	xor    %esi,%esi
  2010b4:	e8 57 02 00 00       	call   201310 <memset>
  2010b9:	b8 01 00 00 00       	mov    $0x1,%eax
  2010be:	4c 89 fe             	mov    %r15,%rsi
  2010c1:	4c 89 f7             	mov    %r14,%rdi
  2010c4:	4c 89 64 24 10       	mov    %r12,0x10(%rsp)
  2010c9:	44 89 6c 24 18       	mov    %r13d,0x18(%rsp)
  2010ce:	89 6c 24 10          	mov    %ebp,0x10(%rsp)
  2010d2:	48 89 5c 24 20       	mov    %rbx,0x20(%rsp)
  2010d7:	c6 04 24 b6          	movb   $0xb6,(%rsp)
  2010db:	66 89 44 24 08       	mov    %ax,0x8(%rsp)
  2010e0:	e8 2b f2 ff ff       	call   200310 <lux_call>
  2010e5:	48 81 c4 48 03 00 00 	add    $0x348,%rsp
  2010ec:	5b                   	pop    %rbx
  2010ed:	c1 f8 1f             	sar    $0x1f,%eax
  2010f0:	5d                   	pop    %rbp
  2010f1:	41 5c                	pop    %r12
  2010f3:	41 5d                	pop    %r13
  2010f5:	41 5e                	pop    %r14
  2010f7:	41 5f                	pop    %r15
  2010f9:	c3                   	ret
  2010fa:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)

0000000000201100 <pebble_alloc>:
  201100:	f3 0f 1e fa          	endbr64
  201104:	48 89 f9             	mov    %rdi,%rcx
  201107:	41 55                	push   %r13
  201109:	49 89 fd             	mov    %rdi,%r13
  20110c:	b8 01 00 00 00       	mov    $0x1,%eax
  201111:	48 89 ca             	mov    %rcx,%rdx
  201114:	49 c1 ed 30          	shr    $0x30,%r13
  201118:	41 54                	push   %r12
  20111a:	49 89 fc             	mov    %rdi,%r12
  20111d:	48 c1 ea 38          	shr    $0x38,%rdx
  201121:	45 0f b6 ed          	movzbl %r13b,%r13d
  201125:	55                   	push   %rbp
  201126:	49 c1 ec 28          	shr    $0x28,%r12
  20112a:	48 c1 e2 08          	shl    $0x8,%rdx
  20112e:	53                   	push   %rbx
  20112f:	48 89 fd             	mov    %rdi,%rbp
  201132:	45 0f b6 e4          	movzbl %r12b,%r12d
  201136:	4c 09 ea             	or     %r13,%rdx
  201139:	48 c1 ed 20          	shr    $0x20,%rbp
  20113d:	49 89 fb             	mov    %rdi,%r11
  201140:	48 89 f3             	mov    %rsi,%rbx
  201143:	48 c1 e2 08          	shl    $0x8,%rdx
  201147:	40 0f b6 ed          	movzbl %bpl,%ebp
  20114b:	49 c1 eb 18          	shr    $0x18,%r11
  20114f:	49 89 da             	mov    %rbx,%r10
  201152:	4c 09 e2             	or     %r12,%rdx
  201155:	48 81 ec 68 03 00 00 	sub    $0x368,%rsp
  20115c:	49 89 d9             	mov    %rbx,%r9
  20115f:	49 89 d8             	mov    %rbx,%r8
  201162:	48 c1 e2 08          	shl    $0x8,%rdx
  201166:	66 89 44 24 28       	mov    %ax,0x28(%rsp)
  20116b:	45 0f b6 db          	movzbl %r11b,%r11d
  20116f:	49 c1 ea 30          	shr    $0x30,%r10
  201173:	48 8d 44 24 10       	lea    0x10(%rsp),%rax
  201178:	48 09 ea             	or     %rbp,%rdx
  20117b:	45 0f b6 d2          	movzbl %r10b,%r10d
  20117f:	49 c1 e9 28          	shr    $0x28,%r9
  201183:	48 89 44 24 38       	mov    %rax,0x38(%rsp)
  201188:	48 c1 e2 08          	shl    $0x8,%rdx
  20118c:	48 89 f8             	mov    %rdi,%rax
  20118f:	45 0f b6 c9          	movzbl %r9b,%r9d
  201193:	4c 09 da             	or     %r11,%rdx
  201196:	48 c1 e8 10          	shr    $0x10,%rax
  20119a:	48 89 df             	mov    %rbx,%rdi
  20119d:	c6 44 24 20 82       	movb   $0x82,0x20(%rsp)
  2011a2:	0f b6 c0             	movzbl %al,%eax
  2011a5:	48 c1 e2 08          	shl    $0x8,%rdx
  2011a9:	c7 44 24 30 3b 00 00 	movl   $0x3b,0x30(%rsp)
  2011b0:	00 
  2011b1:	48 09 c2             	or     %rax,%rdx
  2011b4:	0f b6 c5             	movzbl %ch,%eax
  2011b7:	49 c1 e8 20          	shr    $0x20,%r8
  2011bb:	0f b6 c9             	movzbl %cl,%ecx
  2011be:	48 c1 e2 08          	shl    $0x8,%rdx
  2011c2:	45 0f b6 c0          	movzbl %r8b,%r8d
  2011c6:	48 c1 ef 18          	shr    $0x18,%rdi
  2011ca:	c7 44 24 40 10 00 00 	movl   $0x10,0x40(%rsp)
  2011d1:	00 
  2011d2:	48 09 c2             	or     %rax,%rdx
  2011d5:	48 89 d8             	mov    %rbx,%rax
  2011d8:	48 c1 ee 10          	shr    $0x10,%rsi
  2011dc:	40 0f b6 ff          	movzbl %dil,%edi
  2011e0:	48 c1 e8 38          	shr    $0x38,%rax
  2011e4:	48 c1 e2 08          	shl    $0x8,%rdx
  2011e8:	40 0f b6 f6          	movzbl %sil,%esi
  2011ec:	48 c1 e0 08          	shl    $0x8,%rax
  2011f0:	4c 09 d0             	or     %r10,%rax
  2011f3:	48 c1 e0 08          	shl    $0x8,%rax
  2011f7:	4c 09 c8             	or     %r9,%rax
  2011fa:	48 c1 e0 08          	shl    $0x8,%rax
  2011fe:	4c 09 c0             	or     %r8,%rax
  201201:	48 c1 e0 08          	shl    $0x8,%rax
  201205:	48 09 ca             	or     %rcx,%rdx
  201208:	48 09 f8             	or     %rdi,%rax
  20120b:	48 89 14 24          	mov    %rdx,(%rsp)
  20120f:	48 8d 7c 24 20       	lea    0x20(%rsp),%rdi
  201214:	48 c1 e0 08          	shl    $0x8,%rax
  201218:	48 09 f0             	or     %rsi,%rax
  20121b:	0f b6 f7             	movzbl %bh,%esi
  20121e:	0f b6 db             	movzbl %bl,%ebx
  201221:	48 c1 e0 08          	shl    $0x8,%rax
  201225:	48 09 f0             	or     %rsi,%rax
  201228:	48 8d b4 24 c0 01 00 	lea    0x1c0(%rsp),%rsi
  20122f:	00 
  201230:	48 c1 e0 08          	shl    $0x8,%rax
  201234:	48 09 c3             	or     %rax,%rbx
  201237:	48 89 5c 24 08       	mov    %rbx,0x8(%rsp)
  20123c:	66 0f 6f 04 24       	movdqa (%rsp),%xmm0
  201241:	0f 29 44 24 10       	movaps %xmm0,0x10(%rsp)
  201246:	e8 c5 f0 ff ff       	call   200310 <lux_call>
  20124b:	48 81 c4 68 03 00 00 	add    $0x368,%rsp
  201252:	5b                   	pop    %rbx
  201253:	c1 f8 1f             	sar    $0x1f,%eax
  201256:	5d                   	pop    %rbp
  201257:	41 5c                	pop    %r12
  201259:	41 5d                	pop    %r13
  20125b:	c3                   	ret
  20125c:	0f 1f 40 00          	nopl   0x0(%rax)

0000000000201260 <pebble_free>:
  201260:	f3 0f 1e fa          	endbr64
  201264:	48 81 ec 58 03 00 00 	sub    $0x358,%rsp
  20126b:	b8 01 00 00 00       	mov    $0x1,%eax
  201270:	66 89 44 24 18       	mov    %ax,0x18(%rsp)
  201275:	48 8d b4 24 b0 01 00 	lea    0x1b0(%rsp),%rsi
  20127c:	00 
  20127d:	48 8d 44 24 08       	lea    0x8(%rsp),%rax
  201282:	48 89 7c 24 08       	mov    %rdi,0x8(%rsp)
  201287:	48 8d 7c 24 10       	lea    0x10(%rsp),%rdi
  20128c:	c6 44 24 10 82       	movb   $0x82,0x10(%rsp)
  201291:	c7 44 24 20 3c 00 00 	movl   $0x3c,0x20(%rsp)
  201298:	00 
  201299:	48 89 44 24 28       	mov    %rax,0x28(%rsp)
  20129e:	c7 44 24 30 08 00 00 	movl   $0x8,0x30(%rsp)
  2012a5:	00 
  2012a6:	e8 65 f0 ff ff       	call   200310 <lux_call>
  2012ab:	48 81 c4 58 03 00 00 	add    $0x358,%rsp
  2012b2:	c1 f8 1f             	sar    $0x1f,%eax
  2012b5:	c3                   	ret
  2012b6:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  2012bd:	00 00 00 

00000000002012c0 <memmove>:
  2012c0:	f3 0f 1e fa          	endbr64
  2012c4:	48 89 f8             	mov    %rdi,%rax
  2012c7:	48 39 f7             	cmp    %rsi,%rdi
  2012ca:	73 24                	jae    2012f0 <memmove+0x30>
  2012cc:	31 c9                	xor    %ecx,%ecx
  2012ce:	48 85 d2             	test   %rdx,%rdx
  2012d1:	74 3a                	je     20130d <memmove+0x4d>
  2012d3:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  2012d8:	0f b6 3c 0e          	movzbl (%rsi,%rcx,1),%edi
  2012dc:	40 88 3c 08          	mov    %dil,(%rax,%rcx,1)
  2012e0:	48 83 c1 01          	add    $0x1,%rcx
  2012e4:	48 39 d1             	cmp    %rdx,%rcx
  2012e7:	75 ef                	jne    2012d8 <memmove+0x18>
  2012e9:	c3                   	ret
  2012ea:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  2012f0:	48 85 d2             	test   %rdx,%rdx
  2012f3:	74 18                	je     20130d <memmove+0x4d>
  2012f5:	48 83 ea 01          	sub    $0x1,%rdx
  2012f9:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  201300:	0f b6 0c 16          	movzbl (%rsi,%rdx,1),%ecx
  201304:	88 0c 10             	mov    %cl,(%rax,%rdx,1)
  201307:	48 83 ea 01          	sub    $0x1,%rdx
  20130b:	73 f3                	jae    201300 <memmove+0x40>
  20130d:	c3                   	ret
  20130e:	66 90                	xchg   %ax,%ax

0000000000201310 <memset>:
  201310:	f3 0f 1e fa          	endbr64
  201314:	48 89 f8             	mov    %rdi,%rax
  201317:	41 89 f0             	mov    %esi,%r8d
  20131a:	48 8d 3c 17          	lea    (%rdi,%rdx,1),%rdi
  20131e:	48 89 c1             	mov    %rax,%rcx
  201321:	48 85 d2             	test   %rdx,%rdx
  201324:	74 2a                	je     201350 <memset+0x40>
  201326:	48 89 fa             	mov    %rdi,%rdx
  201329:	48 29 c2             	sub    %rax,%rdx
  20132c:	83 e2 01             	and    $0x1,%edx
  20132f:	74 0f                	je     201340 <memset+0x30>
  201331:	48 8d 48 01          	lea    0x1(%rax),%rcx
  201335:	40 88 71 ff          	mov    %sil,-0x1(%rcx)
  201339:	48 39 cf             	cmp    %rcx,%rdi
  20133c:	74 13                	je     201351 <memset+0x41>
  20133e:	66 90                	xchg   %ax,%ax
  201340:	44 88 01             	mov    %r8b,(%rcx)
  201343:	48 83 c1 02          	add    $0x2,%rcx
  201347:	44 88 41 ff          	mov    %r8b,-0x1(%rcx)
  20134b:	48 39 cf             	cmp    %rcx,%rdi
  20134e:	75 f0                	jne    201340 <memset+0x30>
  201350:	c3                   	ret
  201351:	c3                   	ret
  201352:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  201359:	00 00 00 00 
  20135d:	0f 1f 00             	nopl   (%rax)

0000000000201360 <strlen>:
  201360:	f3 0f 1e fa          	endbr64
  201364:	80 3f 00             	cmpb   $0x0,(%rdi)
  201367:	74 17                	je     201380 <strlen+0x20>
  201369:	48 89 f8             	mov    %rdi,%rax
  20136c:	0f 1f 40 00          	nopl   0x0(%rax)
  201370:	48 83 c0 01          	add    $0x1,%rax
  201374:	80 38 00             	cmpb   $0x0,(%rax)
  201377:	75 f7                	jne    201370 <strlen+0x10>
  201379:	48 29 f8             	sub    %rdi,%rax
  20137c:	c3                   	ret
  20137d:	0f 1f 00             	nopl   (%rax)
  201380:	31 c0                	xor    %eax,%eax
  201382:	c3                   	ret
  201383:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  20138a:	00 00 00 00 
  20138e:	66 90                	xchg   %ax,%ax

0000000000201390 <malloc>:
  201390:	f3 0f 1e fa          	endbr64
  201394:	48 83 ec 08          	sub    $0x8,%rsp
  201398:	48 8d 3d 41 1f 00 00 	lea    0x1f41(%rip),%rdi        # 2032e0 <_syscall+0x19b>
  20139f:	e8 bc f4 ff ff       	call   200860 <sys_exit>
  2013a4:	31 c0                	xor    %eax,%eax
  2013a6:	48 83 c4 08          	add    $0x8,%rsp
  2013aa:	c3                   	ret
  2013ab:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)

00000000002013b0 <free>:
  2013b0:	f3 0f 1e fa          	endbr64
  2013b4:	48 8d 3d 55 1f 00 00 	lea    0x1f55(%rip),%rdi        # 203310 <_syscall+0x1cb>
  2013bb:	e9 a0 f4 ff ff       	jmp    200860 <sys_exit>

00000000002013c0 <atoi>:
  2013c0:	f3 0f 1e fa          	endbr64
  2013c4:	0f b6 07             	movzbl (%rdi),%eax
  2013c7:	3c 2d                	cmp    $0x2d,%al
  2013c9:	74 45                	je     201410 <atoi+0x50>
  2013cb:	8d 50 d0             	lea    -0x30(%rax),%edx
  2013ce:	31 f6                	xor    %esi,%esi
  2013d0:	80 fa 09             	cmp    $0x9,%dl
  2013d3:	77 5b                	ja     201430 <atoi+0x70>
  2013d5:	31 d2                	xor    %edx,%edx
  2013d7:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  2013de:	00 00 
  2013e0:	83 e8 30             	sub    $0x30,%eax
  2013e3:	8d 14 92             	lea    (%rdx,%rdx,4),%edx
  2013e6:	48 83 c7 01          	add    $0x1,%rdi
  2013ea:	0f be c0             	movsbl %al,%eax
  2013ed:	8d 14 50             	lea    (%rax,%rdx,2),%edx
  2013f0:	0f b6 07             	movzbl (%rdi),%eax
  2013f3:	8d 48 d0             	lea    -0x30(%rax),%ecx
  2013f6:	80 f9 09             	cmp    $0x9,%cl
  2013f9:	76 e5                	jbe    2013e0 <atoi+0x20>
  2013fb:	89 d0                	mov    %edx,%eax
  2013fd:	f7 d8                	neg    %eax
  2013ff:	85 f6                	test   %esi,%esi
  201401:	0f 45 d0             	cmovne %eax,%edx
  201404:	89 d0                	mov    %edx,%eax
  201406:	c3                   	ret
  201407:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  20140e:	00 00 
  201410:	0f b6 47 01          	movzbl 0x1(%rdi),%eax
  201414:	48 8d 4f 01          	lea    0x1(%rdi),%rcx
  201418:	8d 50 d0             	lea    -0x30(%rax),%edx
  20141b:	80 fa 09             	cmp    $0x9,%dl
  20141e:	77 10                	ja     201430 <atoi+0x70>
  201420:	48 89 cf             	mov    %rcx,%rdi
  201423:	be 01 00 00 00       	mov    $0x1,%esi
  201428:	eb ab                	jmp    2013d5 <atoi+0x15>
  20142a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  201430:	31 d2                	xor    %edx,%edx
  201432:	89 d0                	mov    %edx,%eax
  201434:	c3                   	ret
  201435:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  20143c:	00 00 00 00 

0000000000201440 <strtoull>:
  201440:	f3 0f 1e fa          	endbr64
  201444:	48 89 f9             	mov    %rdi,%rcx
  201447:	49 89 f1             	mov    %rsi,%r9
  20144a:	89 d7                	mov    %edx,%edi
  20144c:	0f b6 01             	movzbl (%rcx),%eax
  20144f:	85 d2                	test   %edx,%edx
  201451:	75 09                	jne    20145c <strtoull+0x1c>
  201453:	bf 0a 00 00 00       	mov    $0xa,%edi
  201458:	3c 30                	cmp    $0x30,%al
  20145a:	74 64                	je     2014c0 <strtoull+0x80>
  20145c:	31 c0                	xor    %eax,%eax
  20145e:	4c 63 c7             	movslq %edi,%r8
  201461:	eb 1a                	jmp    20147d <strtoull+0x3d>
  201463:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  201468:	83 ea 30             	sub    $0x30,%edx
  20146b:	39 fa                	cmp    %edi,%edx
  20146d:	7d 2a                	jge    201499 <strtoull+0x59>
  20146f:	49 0f af c0          	imul   %r8,%rax
  201473:	48 63 d2             	movslq %edx,%rdx
  201476:	48 83 c1 01          	add    $0x1,%rcx
  20147a:	48 01 d0             	add    %rdx,%rax
  20147d:	0f be 11             	movsbl (%rcx),%edx
  201480:	8d 72 d0             	lea    -0x30(%rdx),%esi
  201483:	40 80 fe 09          	cmp    $0x9,%sil
  201487:	76 df                	jbe    201468 <strtoull+0x28>
  201489:	8d 72 9f             	lea    -0x61(%rdx),%esi
  20148c:	40 80 fe 05          	cmp    $0x5,%sil
  201490:	77 16                	ja     2014a8 <strtoull+0x68>
  201492:	83 ea 57             	sub    $0x57,%edx
  201495:	39 fa                	cmp    %edi,%edx
  201497:	7c d6                	jl     20146f <strtoull+0x2f>
  201499:	4d 85 c9             	test   %r9,%r9
  20149c:	74 03                	je     2014a1 <strtoull+0x61>
  20149e:	49 89 09             	mov    %rcx,(%r9)
  2014a1:	c3                   	ret
  2014a2:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  2014a8:	8d 72 bf             	lea    -0x41(%rdx),%esi
  2014ab:	40 80 fe 05          	cmp    $0x5,%sil
  2014af:	77 e8                	ja     201499 <strtoull+0x59>
  2014b1:	83 ea 37             	sub    $0x37,%edx
  2014b4:	eb b5                	jmp    20146b <strtoull+0x2b>
  2014b6:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  2014bd:	00 00 00 
  2014c0:	0f b6 41 01          	movzbl 0x1(%rcx),%eax
  2014c4:	83 e0 df             	and    $0xffffffdf,%eax
  2014c7:	3c 58                	cmp    $0x58,%al
  2014c9:	75 91                	jne    20145c <strtoull+0x1c>
  2014cb:	48 83 c1 02          	add    $0x2,%rcx
  2014cf:	bf 10 00 00 00       	mov    $0x10,%edi
  2014d4:	eb 86                	jmp    20145c <strtoull+0x1c>
  2014d6:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  2014dd:	00 00 00 

00000000002014e0 <gqid>:
  2014e0:	48 8d 47 0d          	lea    0xd(%rdi),%rax
  2014e4:	48 39 f0             	cmp    %rsi,%rax
  2014e7:	77 17                	ja     201500 <gqid+0x20>
  2014e9:	0f b6 0f             	movzbl (%rdi),%ecx
  2014ec:	88 4a 10             	mov    %cl,0x10(%rdx)
  2014ef:	48 63 4f 01          	movslq 0x1(%rdi),%rcx
  2014f3:	48 89 4a 08          	mov    %rcx,0x8(%rdx)
  2014f7:	48 8b 4f 05          	mov    0x5(%rdi),%rcx
  2014fb:	48 89 0a             	mov    %rcx,(%rdx)
  2014fe:	c3                   	ret
  2014ff:	90                   	nop
  201500:	31 c0                	xor    %eax,%eax
  201502:	c3                   	ret
  201503:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  20150a:	00 00 00 00 
  20150e:	66 90                	xchg   %ax,%ax

0000000000201510 <convM2S>:
  201510:	f3 0f 1e fa          	endbr64
  201514:	41 57                	push   %r15
  201516:	89 f6                	mov    %esi,%esi
  201518:	41 56                	push   %r14
  20151a:	41 55                	push   %r13
  20151c:	4c 8d 6f 07          	lea    0x7(%rdi),%r13
  201520:	41 54                	push   %r12
  201522:	55                   	push   %rbp
  201523:	48 8d 2c 37          	lea    (%rdi,%rsi,1),%rbp
  201527:	53                   	push   %rbx
  201528:	48 89 fb             	mov    %rdi,%rbx
  20152b:	48 83 ec 18          	sub    $0x18,%rsp
  20152f:	4c 39 ed             	cmp    %r13,%rbp
  201532:	72 7c                	jb     2015b0 <convM2S+0xa0>
  201534:	44 8b 37             	mov    (%rdi),%r14d
  201537:	41 83 fe 06          	cmp    $0x6,%r14d
  20153b:	0f 86 a7 00 00 00    	jbe    2015e8 <convM2S+0xd8>
  201541:	0f b6 47 04          	movzbl 0x4(%rdi),%eax
  201545:	49 89 d4             	mov    %rdx,%r12
  201548:	88 02                	mov    %al,(%rdx)
  20154a:	0f b7 57 05          	movzwl 0x5(%rdi),%edx
  20154e:	83 e8 64             	sub    $0x64,%eax
  201551:	66 41 89 54 24 08    	mov    %dx,0x8(%r12)
  201557:	3c 69                	cmp    $0x69,%al
  201559:	77 38                	ja     201593 <convM2S+0x83>
  20155b:	48 8d 15 3a 1e 00 00 	lea    0x1e3a(%rip),%rdx        # 20339c <_syscall+0x257>
  201562:	0f b6 c0             	movzbl %al,%eax
  201565:	48 63 04 82          	movslq (%rdx,%rax,4),%rax
  201569:	48 01 d0             	add    %rdx,%rax
  20156c:	3e ff e0             	notrack jmp *%rax
  20156f:	90                   	nop
  201570:	4c 8d 6f 0b          	lea    0xb(%rdi),%r13
  201574:	4c 39 ed             	cmp    %r13,%rbp
  201577:	72 1a                	jb     201593 <convM2S+0x83>
  201579:	8b 47 07             	mov    0x7(%rdi),%eax
  20157c:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  201581:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  201588:	44 89 f0             	mov    %r14d,%eax
  20158b:	48 01 c3             	add    %rax,%rbx
  20158e:	49 39 dd             	cmp    %rbx,%r13
  201591:	74 03                	je     201596 <convM2S+0x86>
  201593:	45 31 f6             	xor    %r14d,%r14d
  201596:	48 83 c4 18          	add    $0x18,%rsp
  20159a:	44 89 f0             	mov    %r14d,%eax
  20159d:	5b                   	pop    %rbx
  20159e:	5d                   	pop    %rbp
  20159f:	41 5c                	pop    %r12
  2015a1:	41 5d                	pop    %r13
  2015a3:	41 5e                	pop    %r14
  2015a5:	41 5f                	pop    %r15
  2015a7:	c3                   	ret
  2015a8:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
  2015af:	00 
  2015b0:	48 89 fe             	mov    %rdi,%rsi
  2015b3:	48 89 ea             	mov    %rbp,%rdx
  2015b6:	48 8d 3d 83 1d 00 00 	lea    0x1d83(%rip),%rdi        # 203340 <_syscall+0x1fb>
  2015bd:	31 c0                	xor    %eax,%eax
  2015bf:	e8 0c ec ff ff       	call   2001d0 <print>
  2015c4:	eb cd                	jmp    201593 <convM2S+0x83>
  2015c6:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  2015cd:	00 00 00 
  2015d0:	4c 8d 6f 0b          	lea    0xb(%rdi),%r13
  2015d4:	4c 39 ed             	cmp    %r13,%rbp
  2015d7:	72 ba                	jb     201593 <convM2S+0x83>
  2015d9:	8b 47 07             	mov    0x7(%rdi),%eax
  2015dc:	41 89 44 24 18       	mov    %eax,0x18(%r12)
  2015e1:	eb a5                	jmp    201588 <convM2S+0x78>
  2015e3:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  2015e8:	ba 07 00 00 00       	mov    $0x7,%edx
  2015ed:	44 89 f6             	mov    %r14d,%esi
  2015f0:	48 8d 3d 79 1d 00 00 	lea    0x1d79(%rip),%rdi        # 203370 <_syscall+0x22b>
  2015f7:	31 c0                	xor    %eax,%eax
  2015f9:	e8 d2 eb ff ff       	call   2001d0 <print>
  2015fe:	eb 93                	jmp    201593 <convM2S+0x83>
  201600:	48 8d 77 09          	lea    0x9(%rdi),%rsi
  201604:	48 39 f5             	cmp    %rsi,%rbp
  201607:	72 8a                	jb     201593 <convM2S+0x83>
  201609:	44 0f b7 7f 07       	movzwl 0x7(%rdi),%r15d
  20160e:	48 8d 4f 08          	lea    0x8(%rdi),%rcx
  201612:	4e 8d 6c 3f 09       	lea    0x9(%rdi,%r15,1),%r13
  201617:	4c 39 ed             	cmp    %r13,%rbp
  20161a:	0f 82 73 ff ff ff    	jb     201593 <convM2S+0x83>
  201620:	48 89 cf             	mov    %rcx,%rdi
  201623:	4c 89 fa             	mov    %r15,%rdx
  201626:	48 89 0c 24          	mov    %rcx,(%rsp)
  20162a:	e8 91 fc ff ff       	call   2012c0 <memmove>
  20162f:	42 c6 44 3b 08 00    	movb   $0x0,0x8(%rbx,%r15,1)
  201635:	48 8b 0c 24          	mov    (%rsp),%rcx
  201639:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  20163e:	e9 45 ff ff ff       	jmp    201588 <convM2S+0x78>
  201643:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  201648:	4c 8d 6f 0f          	lea    0xf(%rdi),%r13
  20164c:	4c 39 ed             	cmp    %r13,%rbp
  20164f:	0f 82 3e ff ff ff    	jb     201593 <convM2S+0x83>
  201655:	48 8b 47 07          	mov    0x7(%rdi),%rax
  201659:	49 89 44 24 10       	mov    %rax,0x10(%r12)
  20165e:	e9 25 ff ff ff       	jmp    201588 <convM2S+0x78>
  201663:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  201668:	48 8d 47 0b          	lea    0xb(%rdi),%rax
  20166c:	48 39 c5             	cmp    %rax,%rbp
  20166f:	0f 82 1e ff ff ff    	jb     201593 <convM2S+0x83>
  201675:	8b 57 07             	mov    0x7(%rdi),%edx
  201678:	4c 8d 2c 10          	lea    (%rax,%rdx,1),%r13
  20167c:	41 89 54 24 18       	mov    %edx,0x18(%r12)
  201681:	4c 39 ed             	cmp    %r13,%rbp
  201684:	0f 82 09 ff ff ff    	jb     201593 <convM2S+0x83>
  20168a:	49 89 44 24 20       	mov    %rax,0x20(%r12)
  20168f:	e9 f4 fe ff ff       	jmp    201588 <convM2S+0x78>
  201694:	0f 1f 40 00          	nopl   0x0(%rax)
  201698:	48 8d 47 09          	lea    0x9(%rdi),%rax
  20169c:	48 39 c5             	cmp    %rax,%rbp
  20169f:	0f 82 ee fe ff ff    	jb     201593 <convM2S+0x83>
  2016a5:	0f b7 57 07          	movzwl 0x7(%rdi),%edx
  2016a9:	4c 8d 2c 10          	lea    (%rax,%rdx,1),%r13
  2016ad:	66 41 89 54 24 10    	mov    %dx,0x10(%r12)
  2016b3:	4c 39 ed             	cmp    %r13,%rbp
  2016b6:	0f 82 d7 fe ff ff    	jb     201593 <convM2S+0x83>
  2016bc:	49 89 44 24 18       	mov    %rax,0x18(%r12)
  2016c1:	e9 c2 fe ff ff       	jmp    201588 <convM2S+0x78>
  2016c6:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  2016cd:	00 00 00 
  2016d0:	48 8d 47 17          	lea    0x17(%rdi),%rax
  2016d4:	48 39 c5             	cmp    %rax,%rbp
  2016d7:	0f 82 b6 fe ff ff    	jb     201593 <convM2S+0x83>
  2016dd:	8b 57 07             	mov    0x7(%rdi),%edx
  2016e0:	41 89 54 24 04       	mov    %edx,0x4(%r12)
  2016e5:	48 8b 57 0b          	mov    0xb(%rdi),%rdx
  2016e9:	49 89 54 24 10       	mov    %rdx,0x10(%r12)
  2016ee:	8b 57 13             	mov    0x13(%rdi),%edx
  2016f1:	4c 8d 2c 10          	lea    (%rax,%rdx,1),%r13
  2016f5:	41 89 54 24 18       	mov    %edx,0x18(%r12)
  2016fa:	4c 39 ed             	cmp    %r13,%rbp
  2016fd:	73 8b                	jae    20168a <convM2S+0x17a>
  2016ff:	e9 8f fe ff ff       	jmp    201593 <convM2S+0x83>
  201704:	0f 1f 40 00          	nopl   0x0(%rax)
  201708:	4c 8d 6f 17          	lea    0x17(%rdi),%r13
  20170c:	4c 39 ed             	cmp    %r13,%rbp
  20170f:	0f 82 7e fe ff ff    	jb     201593 <convM2S+0x83>
  201715:	8b 47 07             	mov    0x7(%rdi),%eax
  201718:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  20171d:	48 8b 47 0b          	mov    0xb(%rdi),%rax
  201721:	49 89 44 24 10       	mov    %rax,0x10(%r12)
  201726:	8b 47 13             	mov    0x13(%rdi),%eax
  201729:	41 89 44 24 18       	mov    %eax,0x18(%r12)
  20172e:	e9 55 fe ff ff       	jmp    201588 <convM2S+0x78>
  201733:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  201738:	48 8d 47 0b          	lea    0xb(%rdi),%rax
  20173c:	48 39 c5             	cmp    %rax,%rbp
  20173f:	0f 82 4e fe ff ff    	jb     201593 <convM2S+0x83>
  201745:	8b 47 07             	mov    0x7(%rdi),%eax
  201748:	48 8d 77 0d          	lea    0xd(%rdi),%rsi
  20174c:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  201751:	48 39 f5             	cmp    %rsi,%rbp
  201754:	0f 82 39 fe ff ff    	jb     201593 <convM2S+0x83>
  20175a:	44 0f b7 6f 0b       	movzwl 0xb(%rdi),%r13d
  20175f:	48 8d 4f 0c          	lea    0xc(%rdi),%rcx
  201763:	4e 8d 7c 2f 0d       	lea    0xd(%rdi,%r13,1),%r15
  201768:	4c 39 fd             	cmp    %r15,%rbp
  20176b:	0f 82 22 fe ff ff    	jb     201593 <convM2S+0x83>
  201771:	4c 89 ea             	mov    %r13,%rdx
  201774:	48 89 cf             	mov    %rcx,%rdi
  201777:	48 89 0c 24          	mov    %rcx,(%rsp)
  20177b:	e8 40 fb ff ff       	call   2012c0 <memmove>
  201780:	42 c6 44 2b 0c 00    	movb   $0x0,0xc(%rbx,%r13,1)
  201786:	48 8b 0c 24          	mov    (%rsp),%rcx
  20178a:	4d 8d 6f 05          	lea    0x5(%r15),%r13
  20178e:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  201793:	4c 39 ed             	cmp    %r13,%rbp
  201796:	0f 82 f7 fd ff ff    	jb     201593 <convM2S+0x83>
  20179c:	41 8b 07             	mov    (%r15),%eax
  20179f:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  2017a4:	41 0f b6 47 04       	movzbl 0x4(%r15),%eax
  2017a9:	41 88 44 24 20       	mov    %al,0x20(%r12)
  2017ae:	e9 d5 fd ff ff       	jmp    201588 <convM2S+0x78>
  2017b3:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  2017b8:	48 8d 47 0b          	lea    0xb(%rdi),%rax
  2017bc:	48 39 c5             	cmp    %rax,%rbp
  2017bf:	0f 82 ce fd ff ff    	jb     201593 <convM2S+0x83>
  2017c5:	8b 47 07             	mov    0x7(%rdi),%eax
  2017c8:	48 8d 77 0d          	lea    0xd(%rdi),%rsi
  2017cc:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  2017d1:	48 39 f5             	cmp    %rsi,%rbp
  2017d4:	0f 82 b9 fd ff ff    	jb     201593 <convM2S+0x83>
  2017da:	44 0f b7 7f 0b       	movzwl 0xb(%rdi),%r15d
  2017df:	48 8d 4f 0c          	lea    0xc(%rdi),%rcx
  2017e3:	4e 8d 6c 3f 0d       	lea    0xd(%rdi,%r15,1),%r13
  2017e8:	4c 39 ed             	cmp    %r13,%rbp
  2017eb:	0f 82 a2 fd ff ff    	jb     201593 <convM2S+0x83>
  2017f1:	48 89 cf             	mov    %rcx,%rdi
  2017f4:	4c 89 fa             	mov    %r15,%rdx
  2017f7:	48 89 0c 24          	mov    %rcx,(%rsp)
  2017fb:	e8 c0 fa ff ff       	call   2012c0 <memmove>
  201800:	42 c6 44 3b 0c 00    	movb   $0x0,0xc(%rbx,%r15,1)
  201806:	48 8b 0c 24          	mov    (%rsp),%rcx
  20180a:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  20180f:	e9 74 fd ff ff       	jmp    201588 <convM2S+0x78>
  201814:	0f 1f 40 00          	nopl   0x0(%rax)
  201818:	49 8d 54 24 10       	lea    0x10(%r12),%rdx
  20181d:	4c 89 ef             	mov    %r13,%rdi
  201820:	48 89 ee             	mov    %rbp,%rsi
  201823:	e8 b8 fc ff ff       	call   2014e0 <gqid>
  201828:	49 89 c5             	mov    %rax,%r13
  20182b:	48 85 c0             	test   %rax,%rax
  20182e:	0f 94 c0             	sete   %al
  201831:	4c 39 ed             	cmp    %r13,%rbp
  201834:	0f 92 c2             	setb   %dl
  201837:	09 d0                	or     %edx,%eax
  201839:	84 c0                	test   %al,%al
  20183b:	0f 85 52 fd ff ff    	jne    201593 <convM2S+0x83>
  201841:	e9 42 fd ff ff       	jmp    201588 <convM2S+0x78>
  201846:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  20184d:	00 00 00 
  201850:	48 8d 77 09          	lea    0x9(%rdi),%rsi
  201854:	48 39 f5             	cmp    %rsi,%rbp
  201857:	0f 82 36 fd ff ff    	jb     201593 <convM2S+0x83>
  20185d:	44 0f b7 7f 07       	movzwl 0x7(%rdi),%r15d
  201862:	48 8d 4f 08          	lea    0x8(%rdi),%rcx
  201866:	4e 8d 6c 3f 09       	lea    0x9(%rdi,%r15,1),%r13
  20186b:	4c 39 ed             	cmp    %r13,%rbp
  20186e:	0f 82 1f fd ff ff    	jb     201593 <convM2S+0x83>
  201874:	48 89 cf             	mov    %rcx,%rdi
  201877:	4c 89 fa             	mov    %r15,%rdx
  20187a:	48 89 0c 24          	mov    %rcx,(%rsp)
  20187e:	e8 3d fa ff ff       	call   2012c0 <memmove>
  201883:	42 c6 44 3b 08 00    	movb   $0x0,0x8(%rbx,%r15,1)
  201889:	48 8b 0c 24          	mov    (%rsp),%rcx
  20188d:	49 89 4c 24 10       	mov    %rcx,0x10(%r12)
  201892:	e9 f1 fc ff ff       	jmp    201588 <convM2S+0x78>
  201897:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  20189e:	00 00 
  2018a0:	48 8d 47 0d          	lea    0xd(%rdi),%rax
  2018a4:	48 39 c5             	cmp    %rax,%rbp
  2018a7:	0f 82 e6 fc ff ff    	jb     201593 <convM2S+0x83>
  2018ad:	8b 57 07             	mov    0x7(%rdi),%edx
  2018b0:	41 89 54 24 04       	mov    %edx,0x4(%r12)
  2018b5:	0f b7 57 0b          	movzwl 0xb(%rdi),%edx
  2018b9:	4c 8d 2c 10          	lea    (%rax,%rdx,1),%r13
  2018bd:	66 41 89 54 24 10    	mov    %dx,0x10(%r12)
  2018c3:	4c 39 ed             	cmp    %r13,%rbp
  2018c6:	0f 83 f0 fd ff ff    	jae    2016bc <convM2S+0x1ac>
  2018cc:	e9 c2 fc ff ff       	jmp    201593 <convM2S+0x83>
  2018d1:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  2018d8:	48 8d 7f 0b          	lea    0xb(%rdi),%rdi
  2018dc:	48 39 fd             	cmp    %rdi,%rbp
  2018df:	0f 82 ae fc ff ff    	jb     201593 <convM2S+0x83>
  2018e5:	8b 43 07             	mov    0x7(%rbx),%eax
  2018e8:	49 8d 54 24 10       	lea    0x10(%r12),%rdx
  2018ed:	48 89 ee             	mov    %rbp,%rsi
  2018f0:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  2018f5:	e8 e6 fb ff ff       	call   2014e0 <gqid>
  2018fa:	48 85 c0             	test   %rax,%rax
  2018fd:	0f 84 90 fc ff ff    	je     201593 <convM2S+0x83>
  201903:	4c 8d 68 04          	lea    0x4(%rax),%r13
  201907:	4c 39 ed             	cmp    %r13,%rbp
  20190a:	0f 82 83 fc ff ff    	jb     201593 <convM2S+0x83>
  201910:	8b 00                	mov    (%rax),%eax
  201912:	41 89 44 24 28       	mov    %eax,0x28(%r12)
  201917:	e9 6c fc ff ff       	jmp    201588 <convM2S+0x78>
  20191c:	0f 1f 40 00          	nopl   0x0(%rax)
  201920:	49 8d 54 24 10       	lea    0x10(%r12),%rdx
  201925:	48 89 ee             	mov    %rbp,%rsi
  201928:	4c 89 ef             	mov    %r13,%rdi
  20192b:	e8 b0 fb ff ff       	call   2014e0 <gqid>
  201930:	48 85 c0             	test   %rax,%rax
  201933:	75 ce                	jne    201903 <convM2S+0x3f3>
  201935:	e9 59 fc ff ff       	jmp    201593 <convM2S+0x83>
  20193a:	4c 8d 6f 09          	lea    0x9(%rdi),%r13
  20193e:	4c 39 ed             	cmp    %r13,%rbp
  201941:	0f 82 4c fc ff ff    	jb     201593 <convM2S+0x83>
  201947:	0f b7 47 07          	movzwl 0x7(%rdi),%eax
  20194b:	66 41 89 44 24 10    	mov    %ax,0x10(%r12)
  201951:	e9 32 fc ff ff       	jmp    201588 <convM2S+0x78>
  201956:	48 8d 47 0f          	lea    0xf(%rdi),%rax
  20195a:	48 39 c5             	cmp    %rax,%rbp
  20195d:	0f 82 30 fc ff ff    	jb     201593 <convM2S+0x83>
  201963:	8b 47 07             	mov    0x7(%rdi),%eax
  201966:	48 8d 77 11          	lea    0x11(%rdi),%rsi
  20196a:	41 89 44 24 18       	mov    %eax,0x18(%r12)
  20196f:	8b 47 0b             	mov    0xb(%rdi),%eax
  201972:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  201977:	48 39 f5             	cmp    %rsi,%rbp
  20197a:	0f 82 13 fc ff ff    	jb     201593 <convM2S+0x83>
  201980:	44 0f b7 6f 0f       	movzwl 0xf(%rdi),%r13d
  201985:	48 8d 4f 10          	lea    0x10(%rdi),%rcx
  201989:	4e 8d 7c 2f 11       	lea    0x11(%rdi,%r13,1),%r15
  20198e:	4c 39 fd             	cmp    %r15,%rbp
  201991:	0f 82 fc fb ff ff    	jb     201593 <convM2S+0x83>
  201997:	48 89 cf             	mov    %rcx,%rdi
  20199a:	4c 89 ea             	mov    %r13,%rdx
  20199d:	48 89 0c 24          	mov    %rcx,(%rsp)
  2019a1:	e8 1a f9 ff ff       	call   2012c0 <memmove>
  2019a6:	48 8b 0c 24          	mov    (%rsp),%rcx
  2019aa:	49 8d 47 04          	lea    0x4(%r15),%rax
  2019ae:	42 c6 44 2b 10 00    	movb   $0x0,0x10(%rbx,%r13,1)
  2019b4:	49 89 4c 24 10       	mov    %rcx,0x10(%r12)
  2019b9:	48 39 c5             	cmp    %rax,%rbp
  2019bc:	0f 82 d1 fb ff ff    	jb     201593 <convM2S+0x83>
  2019c2:	41 8b 07             	mov    (%r15),%eax
  2019c5:	49 8d 77 06          	lea    0x6(%r15),%rsi
  2019c9:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  2019ce:	48 39 f5             	cmp    %rsi,%rbp
  2019d1:	0f 82 bc fb ff ff    	jb     201593 <convM2S+0x83>
  2019d7:	41 0f b7 57 04       	movzwl 0x4(%r15),%edx
  2019dc:	49 8d 4f 05          	lea    0x5(%r15),%rcx
  2019e0:	4d 8d 6c 17 06       	lea    0x6(%r15,%rdx,1),%r13
  2019e5:	4c 39 ed             	cmp    %r13,%rbp
  2019e8:	0f 82 a5 fb ff ff    	jb     201593 <convM2S+0x83>
  2019ee:	48 89 cf             	mov    %rcx,%rdi
  2019f1:	48 89 54 24 08       	mov    %rdx,0x8(%rsp)
  2019f6:	48 89 0c 24          	mov    %rcx,(%rsp)
  2019fa:	e8 c1 f8 ff ff       	call   2012c0 <memmove>
  2019ff:	48 8b 54 24 08       	mov    0x8(%rsp),%rdx
  201a04:	48 8b 0c 24          	mov    (%rsp),%rcx
  201a08:	41 c6 44 17 05 00    	movb   $0x0,0x5(%r15,%rdx,1)
  201a0e:	49 89 4c 24 20       	mov    %rcx,0x20(%r12)
  201a13:	e9 70 fb ff ff       	jmp    201588 <convM2S+0x78>
  201a18:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
  201a1f:	00 
  201a20:	48 8d 77 09          	lea    0x9(%rdi),%rsi
  201a24:	48 39 f5             	cmp    %rsi,%rbp
  201a27:	0f 82 66 fb ff ff    	jb     201593 <convM2S+0x83>
  201a2d:	44 0f b7 6f 07       	movzwl 0x7(%rdi),%r13d
  201a32:	48 8d 4f 08          	lea    0x8(%rdi),%rcx
  201a36:	4e 8d 7c 2f 09       	lea    0x9(%rdi,%r13,1),%r15
  201a3b:	4c 39 fd             	cmp    %r15,%rbp
  201a3e:	0f 82 4f fb ff ff    	jb     201593 <convM2S+0x83>
  201a44:	4c 89 ea             	mov    %r13,%rdx
  201a47:	48 89 cf             	mov    %rcx,%rdi
  201a4a:	48 89 0c 24          	mov    %rcx,(%rsp)
  201a4e:	e8 6d f8 ff ff       	call   2012c0 <memmove>
  201a53:	48 8b 0c 24          	mov    (%rsp),%rcx
  201a57:	42 c6 44 2b 08 00    	movb   $0x0,0x8(%rbx,%r13,1)
  201a5d:	4d 8d 6f 04          	lea    0x4(%r15),%r13
  201a61:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  201a66:	4c 39 ed             	cmp    %r13,%rbp
  201a69:	0f 82 24 fb ff ff    	jb     201593 <convM2S+0x83>
  201a6f:	41 8b 07             	mov    (%r15),%eax
  201a72:	41 89 44 24 18       	mov    %eax,0x18(%r12)
  201a77:	e9 0c fb ff ff       	jmp    201588 <convM2S+0x78>
  201a7c:	48 8d 47 0b          	lea    0xb(%rdi),%rax
  201a80:	48 39 c5             	cmp    %rax,%rbp
  201a83:	0f 82 0a fb ff ff    	jb     201593 <convM2S+0x83>
  201a89:	8b 47 07             	mov    0x7(%rdi),%eax
  201a8c:	48 8d 77 0d          	lea    0xd(%rdi),%rsi
  201a90:	41 89 44 24 14       	mov    %eax,0x14(%r12)
  201a95:	48 39 f5             	cmp    %rsi,%rbp
  201a98:	0f 82 f5 fa ff ff    	jb     201593 <convM2S+0x83>
  201a9e:	44 0f b7 7f 0b       	movzwl 0xb(%rdi),%r15d
  201aa3:	48 8d 4f 0c          	lea    0xc(%rdi),%rcx
  201aa7:	4e 8d 6c 3f 0d       	lea    0xd(%rdi,%r15,1),%r13
  201aac:	4c 39 ed             	cmp    %r13,%rbp
  201aaf:	0f 82 de fa ff ff    	jb     201593 <convM2S+0x83>
  201ab5:	48 89 cf             	mov    %rcx,%rdi
  201ab8:	4c 89 fa             	mov    %r15,%rdx
  201abb:	48 89 0c 24          	mov    %rcx,(%rsp)
  201abf:	e8 fc f7 ff ff       	call   2012c0 <memmove>
  201ac4:	48 8b 0c 24          	mov    (%rsp),%rcx
  201ac8:	42 c6 44 3b 0c 00    	movb   $0x0,0xc(%rbx,%r15,1)
  201ace:	49 89 4c 24 10       	mov    %rcx,0x10(%r12)
  201ad3:	e9 b0 fa ff ff       	jmp    201588 <convM2S+0x78>
  201ad8:	48 8d 47 13          	lea    0x13(%rdi),%rax
  201adc:	48 39 c5             	cmp    %rax,%rbp
  201adf:	0f 82 ae fa ff ff    	jb     201593 <convM2S+0x83>
  201ae5:	48 8b 57 07          	mov    0x7(%rdi),%rdx
  201ae9:	49 89 54 24 28       	mov    %rdx,0x28(%r12)
  201aee:	8b 53 0f             	mov    0xf(%rbx),%edx
  201af1:	4c 8d 2c 10          	lea    (%rax,%rdx,1),%r13
  201af5:	41 89 54 24 20       	mov    %edx,0x20(%r12)
  201afa:	4c 39 ed             	cmp    %r13,%rbp
  201afd:	0f 83 b9 fb ff ff    	jae    2016bc <convM2S+0x1ac>
  201b03:	e9 8b fa ff ff       	jmp    201593 <convM2S+0x83>
  201b08:	48 8d 47 13          	lea    0x13(%rdi),%rax
  201b0c:	48 39 c5             	cmp    %rax,%rbp
  201b0f:	0f 82 7e fa ff ff    	jb     201593 <convM2S+0x83>
  201b15:	8b 57 07             	mov    0x7(%rdi),%edx
  201b18:	41 89 54 24 10       	mov    %edx,0x10(%r12)
  201b1d:	8b 57 0b             	mov    0xb(%rdi),%edx
  201b20:	41 89 54 24 14       	mov    %edx,0x14(%r12)
  201b25:	eb c7                	jmp    201aee <convM2S+0x5de>
  201b27:	48 8d 47 0b          	lea    0xb(%rdi),%rax
  201b2b:	48 39 c5             	cmp    %rax,%rbp
  201b2e:	0f 82 5f fa ff ff    	jb     201593 <convM2S+0x83>
  201b34:	8b 47 07             	mov    0x7(%rdi),%eax
  201b37:	48 8d 77 0d          	lea    0xd(%rdi),%rsi
  201b3b:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  201b40:	48 39 f5             	cmp    %rsi,%rbp
  201b43:	0f 82 4a fa ff ff    	jb     201593 <convM2S+0x83>
  201b49:	44 0f b7 6f 0b       	movzwl 0xb(%rdi),%r13d
  201b4e:	48 8d 4f 0c          	lea    0xc(%rdi),%rcx
  201b52:	4e 8d 7c 2f 0d       	lea    0xd(%rdi,%r13,1),%r15
  201b57:	4c 39 fd             	cmp    %r15,%rbp
  201b5a:	0f 82 33 fa ff ff    	jb     201593 <convM2S+0x83>
  201b60:	4c 89 ea             	mov    %r13,%rdx
  201b63:	48 89 cf             	mov    %rcx,%rdi
  201b66:	48 89 0c 24          	mov    %rcx,(%rsp)
  201b6a:	e8 51 f7 ff ff       	call   2012c0 <memmove>
  201b6f:	48 8b 0c 24          	mov    (%rsp),%rcx
  201b73:	42 c6 44 2b 0c 00    	movb   $0x0,0xc(%rbx,%r13,1)
  201b79:	4d 8d 6f 01          	lea    0x1(%r15),%r13
  201b7d:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  201b82:	4c 39 ed             	cmp    %r13,%rbp
  201b85:	0f 82 08 fa ff ff    	jb     201593 <convM2S+0x83>
  201b8b:	41 0f b6 07          	movzbl (%r15),%eax
  201b8f:	41 88 44 24 20       	mov    %al,0x20(%r12)
  201b94:	e9 ef f9 ff ff       	jmp    201588 <convM2S+0x78>
  201b99:	4c 8d 6f 0c          	lea    0xc(%rdi),%r13
  201b9d:	4c 39 ed             	cmp    %r13,%rbp
  201ba0:	0f 82 ed f9 ff ff    	jb     201593 <convM2S+0x83>
  201ba6:	8b 47 07             	mov    0x7(%rdi),%eax
  201ba9:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  201bae:	0f b6 47 0b          	movzbl 0xb(%rdi),%eax
  201bb2:	41 88 44 24 20       	mov    %al,0x20(%r12)
  201bb7:	e9 cc f9 ff ff       	jmp    201588 <convM2S+0x78>
  201bbc:	4c 8d 6f 09          	lea    0x9(%rdi),%r13
  201bc0:	4c 39 ed             	cmp    %r13,%rbp
  201bc3:	0f 82 ca f9 ff ff    	jb     201593 <convM2S+0x83>
  201bc9:	0f b7 47 07          	movzwl 0x7(%rdi),%eax
  201bcd:	66 41 89 44 24 10    	mov    %ax,0x10(%r12)
  201bd3:	66 83 f8 10          	cmp    $0x10,%ax
  201bd7:	0f 87 b6 f9 ff ff    	ja     201593 <convM2S+0x83>
  201bdd:	66 85 c0             	test   %ax,%ax
  201be0:	0f 84 a2 f9 ff ff    	je     201588 <convM2S+0x78>
  201be6:	49 8d 54 24 18       	lea    0x18(%r12),%rdx
  201beb:	45 31 c0             	xor    %r8d,%r8d
  201bee:	66 90                	xchg   %ax,%ax
  201bf0:	4c 89 ef             	mov    %r13,%rdi
  201bf3:	48 89 ee             	mov    %rbp,%rsi
  201bf6:	e8 e5 f8 ff ff       	call   2014e0 <gqid>
  201bfb:	49 89 c5             	mov    %rax,%r13
  201bfe:	48 85 c0             	test   %rax,%rax
  201c01:	0f 84 8c f9 ff ff    	je     201593 <convM2S+0x83>
  201c07:	41 0f b7 44 24 10    	movzwl 0x10(%r12),%eax
  201c0d:	41 83 c0 01          	add    $0x1,%r8d
  201c11:	48 83 c2 18          	add    $0x18,%rdx
  201c15:	41 39 c0             	cmp    %eax,%r8d
  201c18:	72 d6                	jb     201bf0 <convM2S+0x6e0>
  201c1a:	4c 39 ed             	cmp    %r13,%rbp
  201c1d:	0f 82 70 f9 ff ff    	jb     201593 <convM2S+0x83>
  201c23:	e9 60 f9 ff ff       	jmp    201588 <convM2S+0x78>
  201c28:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
  201c2f:	00 
  201c30:	4c 8d 6f 11          	lea    0x11(%rdi),%r13
  201c34:	4c 39 ed             	cmp    %r13,%rbp
  201c37:	0f 82 56 f9 ff ff    	jb     201593 <convM2S+0x83>
  201c3d:	8b 47 07             	mov    0x7(%rdi),%eax
  201c40:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  201c45:	8b 47 0b             	mov    0xb(%rdi),%eax
  201c48:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  201c4d:	0f b7 47 0f          	movzwl 0xf(%rdi),%eax
  201c51:	66 41 89 44 24 14    	mov    %ax,0x14(%r12)
  201c57:	66 83 f8 10          	cmp    $0x10,%ax
  201c5b:	0f 87 32 f9 ff ff    	ja     201593 <convM2S+0x83>
  201c61:	66 85 c0             	test   %ax,%ax
  201c64:	0f 84 1e f9 ff ff    	je     201588 <convM2S+0x78>
  201c6a:	48 8d 77 13          	lea    0x13(%rdi),%rsi
  201c6e:	48 39 f5             	cmp    %rsi,%rbp
  201c71:	0f 82 1c f9 ff ff    	jb     201593 <convM2S+0x83>
  201c77:	0f b7 57 11          	movzwl 0x11(%rdi),%edx
  201c7b:	4c 8d 7f 12          	lea    0x12(%rdi),%r15
  201c7f:	31 c9                	xor    %ecx,%ecx
  201c81:	4c 8d 6c 17 13       	lea    0x13(%rdi,%rdx,1),%r13
  201c86:	4c 39 ed             	cmp    %r13,%rbp
  201c89:	0f 82 04 f9 ff ff    	jb     201593 <convM2S+0x83>
  201c8f:	90                   	nop
  201c90:	4c 89 ff             	mov    %r15,%rdi
  201c93:	89 4c 24 08          	mov    %ecx,0x8(%rsp)
  201c97:	48 89 14 24          	mov    %rdx,(%rsp)
  201c9b:	e8 20 f6 ff ff       	call   2012c0 <memmove>
  201ca0:	8b 44 24 08          	mov    0x8(%rsp),%eax
  201ca4:	48 8b 14 24          	mov    (%rsp),%rdx
  201ca8:	48 89 c1             	mov    %rax,%rcx
  201cab:	41 c6 04 17 00       	movb   $0x0,(%r15,%rdx,1)
  201cb0:	4d 89 7c c4 18       	mov    %r15,0x18(%r12,%rax,8)
  201cb5:	41 0f b7 44 24 14    	movzwl 0x14(%r12),%eax
  201cbb:	83 c1 01             	add    $0x1,%ecx
  201cbe:	39 c1                	cmp    %eax,%ecx
  201cc0:	0f 83 72 03 00 00    	jae    202038 <convM2S+0xb28>
  201cc6:	49 8d 75 02          	lea    0x2(%r13),%rsi
  201cca:	48 39 f5             	cmp    %rsi,%rbp
  201ccd:	0f 82 c0 f8 ff ff    	jb     201593 <convM2S+0x83>
  201cd3:	41 0f b7 55 00       	movzwl 0x0(%r13),%edx
  201cd8:	4d 8d 7d 01          	lea    0x1(%r13),%r15
  201cdc:	4d 8d 6c 17 01       	lea    0x1(%r15,%rdx,1),%r13
  201ce1:	4c 39 ed             	cmp    %r13,%rbp
  201ce4:	73 aa                	jae    201c90 <convM2S+0x780>
  201ce6:	e9 a8 f8 ff ff       	jmp    201593 <convM2S+0x83>
  201ceb:	48 8d 47 0b          	lea    0xb(%rdi),%rax
  201cef:	48 39 c5             	cmp    %rax,%rbp
  201cf2:	0f 82 9b f8 ff ff    	jb     201593 <convM2S+0x83>
  201cf8:	8b 47 07             	mov    0x7(%rdi),%eax
  201cfb:	48 8d 77 0d          	lea    0xd(%rdi),%rsi
  201cff:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  201d04:	48 39 f5             	cmp    %rsi,%rbp
  201d07:	0f 82 86 f8 ff ff    	jb     201593 <convM2S+0x83>
  201d0d:	44 0f b7 6f 0b       	movzwl 0xb(%rdi),%r13d
  201d12:	48 8d 4f 0c          	lea    0xc(%rdi),%rcx
  201d16:	4e 8d 7c 2f 0d       	lea    0xd(%rdi,%r13,1),%r15
  201d1b:	4c 39 fd             	cmp    %r15,%rbp
  201d1e:	0f 82 6f f8 ff ff    	jb     201593 <convM2S+0x83>
  201d24:	4c 89 ea             	mov    %r13,%rdx
  201d27:	48 89 cf             	mov    %rcx,%rdi
  201d2a:	48 89 0c 24          	mov    %rcx,(%rsp)
  201d2e:	e8 8d f5 ff ff       	call   2012c0 <memmove>
  201d33:	42 c6 44 2b 0c 00    	movb   $0x0,0xc(%rbx,%r13,1)
  201d39:	48 8b 0c 24          	mov    (%rsp),%rcx
  201d3d:	49 8d 77 02          	lea    0x2(%r15),%rsi
  201d41:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  201d46:	48 39 f5             	cmp    %rsi,%rbp
  201d49:	0f 82 44 f8 ff ff    	jb     201593 <convM2S+0x83>
  201d4f:	41 0f b7 17          	movzwl (%r15),%edx
  201d53:	49 8d 4f 01          	lea    0x1(%r15),%rcx
  201d57:	4d 8d 6c 17 02       	lea    0x2(%r15,%rdx,1),%r13
  201d5c:	4c 39 ed             	cmp    %r13,%rbp
  201d5f:	0f 82 2e f8 ff ff    	jb     201593 <convM2S+0x83>
  201d65:	48 89 cf             	mov    %rcx,%rdi
  201d68:	48 89 54 24 08       	mov    %rdx,0x8(%rsp)
  201d6d:	48 89 0c 24          	mov    %rcx,(%rsp)
  201d71:	e8 4a f5 ff ff       	call   2012c0 <memmove>
  201d76:	48 8b 54 24 08       	mov    0x8(%rsp),%rdx
  201d7b:	41 c6 44 17 01 00    	movb   $0x0,0x1(%r15,%rdx,1)
  201d81:	48 8b 0c 24          	mov    (%rsp),%rcx
  201d85:	49 89 4c 24 20       	mov    %rcx,0x20(%r12)
  201d8a:	e9 f9 f7 ff ff       	jmp    201588 <convM2S+0x78>
  201d8f:	48 8d 77 09          	lea    0x9(%rdi),%rsi
  201d93:	48 39 f5             	cmp    %rsi,%rbp
  201d96:	0f 82 f7 f7 ff ff    	jb     201593 <convM2S+0x83>
  201d9c:	44 0f b7 7f 07       	movzwl 0x7(%rdi),%r15d
  201da1:	48 8d 4f 08          	lea    0x8(%rdi),%rcx
  201da5:	4e 8d 6c 3f 09       	lea    0x9(%rdi,%r15,1),%r13
  201daa:	4c 39 ed             	cmp    %r13,%rbp
  201dad:	0f 82 e0 f7 ff ff    	jb     201593 <convM2S+0x83>
  201db3:	48 89 cf             	mov    %rcx,%rdi
  201db6:	4c 89 fa             	mov    %r15,%rdx
  201db9:	48 89 0c 24          	mov    %rcx,(%rsp)
  201dbd:	e8 fe f4 ff ff       	call   2012c0 <memmove>
  201dc2:	48 8b 0c 24          	mov    (%rsp),%rcx
  201dc6:	49 8d 75 02          	lea    0x2(%r13),%rsi
  201dca:	42 c6 44 3b 08 00    	movb   $0x0,0x8(%rbx,%r15,1)
  201dd0:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  201dd5:	48 39 f5             	cmp    %rsi,%rbp
  201dd8:	0f 82 b5 f7 ff ff    	jb     201593 <convM2S+0x83>
  201dde:	45 0f b7 7d 00       	movzwl 0x0(%r13),%r15d
  201de3:	49 8d 7d 01          	lea    0x1(%r13),%rdi
  201de7:	4b 8d 4c 3d 02       	lea    0x2(%r13,%r15,1),%rcx
  201dec:	48 39 cd             	cmp    %rcx,%rbp
  201def:	48 89 0c 24          	mov    %rcx,(%rsp)
  201df3:	0f 82 9a f7 ff ff    	jb     201593 <convM2S+0x83>
  201df9:	4c 89 fa             	mov    %r15,%rdx
  201dfc:	48 89 7c 24 08       	mov    %rdi,0x8(%rsp)
  201e01:	e8 ba f4 ff ff       	call   2012c0 <memmove>
  201e06:	48 8b 0c 24          	mov    (%rsp),%rcx
  201e0a:	48 8b 7c 24 08       	mov    0x8(%rsp),%rdi
  201e0f:	43 c6 44 3d 01 00    	movb   $0x0,0x1(%r13,%r15,1)
  201e15:	4c 8d 69 04          	lea    0x4(%rcx),%r13
  201e19:	49 89 7c 24 10       	mov    %rdi,0x10(%r12)
  201e1e:	4c 39 ed             	cmp    %r13,%rbp
  201e21:	0f 82 6c f7 ff ff    	jb     201593 <convM2S+0x83>
  201e27:	8b 01                	mov    (%rcx),%eax
  201e29:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  201e2e:	e9 55 f7 ff ff       	jmp    201588 <convM2S+0x78>
  201e33:	4c 8d 6f 17          	lea    0x17(%rdi),%r13
  201e37:	4c 39 ed             	cmp    %r13,%rbp
  201e3a:	0f 82 53 f7 ff ff    	jb     201593 <convM2S+0x83>
  201e40:	8b 47 07             	mov    0x7(%rdi),%eax
  201e43:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  201e48:	48 8b 47 0b          	mov    0xb(%rdi),%rax
  201e4c:	49 89 44 24 10       	mov    %rax,0x10(%r12)
  201e51:	8b 47 13             	mov    0x13(%rdi),%eax
  201e54:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  201e59:	e9 2a f7 ff ff       	jmp    201588 <convM2S+0x78>
  201e5e:	4c 8d 6f 0b          	lea    0xb(%rdi),%r13
  201e62:	4c 39 ed             	cmp    %r13,%rbp
  201e65:	0f 82 28 f7 ff ff    	jb     201593 <convM2S+0x83>
  201e6b:	8b 47 07             	mov    0x7(%rdi),%eax
  201e6e:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  201e73:	e9 10 f7 ff ff       	jmp    201588 <convM2S+0x78>
  201e78:	48 8d 77 09          	lea    0x9(%rdi),%rsi
  201e7c:	48 39 f5             	cmp    %rsi,%rbp
  201e7f:	0f 82 0e f7 ff ff    	jb     201593 <convM2S+0x83>
  201e85:	44 0f b7 6f 07       	movzwl 0x7(%rdi),%r13d
  201e8a:	48 8d 4f 08          	lea    0x8(%rdi),%rcx
  201e8e:	4e 8d 7c 2f 09       	lea    0x9(%rdi,%r13,1),%r15
  201e93:	4c 39 fd             	cmp    %r15,%rbp
  201e96:	0f 82 f7 f6 ff ff    	jb     201593 <convM2S+0x83>
  201e9c:	48 89 cf             	mov    %rcx,%rdi
  201e9f:	4c 89 ea             	mov    %r13,%rdx
  201ea2:	48 89 0c 24          	mov    %rcx,(%rsp)
  201ea6:	e8 15 f4 ff ff       	call   2012c0 <memmove>
  201eab:	48 8b 0c 24          	mov    (%rsp),%rcx
  201eaf:	49 8d 77 02          	lea    0x2(%r15),%rsi
  201eb3:	42 c6 44 2b 08 00    	movb   $0x0,0x8(%rbx,%r13,1)
  201eb9:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  201ebe:	48 39 f5             	cmp    %rsi,%rbp
  201ec1:	0f 82 cc f6 ff ff    	jb     201593 <convM2S+0x83>
  201ec7:	41 0f b7 17          	movzwl (%r15),%edx
  201ecb:	49 8d 4f 01          	lea    0x1(%r15),%rcx
  201ecf:	4d 8d 6c 17 02       	lea    0x2(%r15,%rdx,1),%r13
  201ed4:	4c 39 ed             	cmp    %r13,%rbp
  201ed7:	0f 82 b6 f6 ff ff    	jb     201593 <convM2S+0x83>
  201edd:	48 89 cf             	mov    %rcx,%rdi
  201ee0:	48 89 54 24 08       	mov    %rdx,0x8(%rsp)
  201ee5:	48 89 0c 24          	mov    %rcx,(%rsp)
  201ee9:	e8 d2 f3 ff ff       	call   2012c0 <memmove>
  201eee:	48 8b 54 24 08       	mov    0x8(%rsp),%rdx
  201ef3:	48 8b 0c 24          	mov    (%rsp),%rcx
  201ef7:	41 c6 44 17 01 00    	movb   $0x0,0x1(%r15,%rdx,1)
  201efd:	49 89 4c 24 10       	mov    %rcx,0x10(%r12)
  201f02:	e9 81 f6 ff ff       	jmp    201588 <convM2S+0x78>
  201f07:	4c 8d 6f 0f          	lea    0xf(%rdi),%r13
  201f0b:	4c 39 ed             	cmp    %r13,%rbp
  201f0e:	0f 82 7f f6 ff ff    	jb     201593 <convM2S+0x83>
  201f14:	8b 47 07             	mov    0x7(%rdi),%eax
  201f17:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  201f1c:	8b 47 0b             	mov    0xb(%rdi),%eax
  201f1f:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  201f24:	e9 5f f6 ff ff       	jmp    201588 <convM2S+0x78>
  201f29:	4c 8d 6f 0f          	lea    0xf(%rdi),%r13
  201f2d:	4c 39 ed             	cmp    %r13,%rbp
  201f30:	0f 82 5d f6 ff ff    	jb     201593 <convM2S+0x83>
  201f36:	8b 47 07             	mov    0x7(%rdi),%eax
  201f39:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  201f3e:	8b 47 0b             	mov    0xb(%rdi),%eax
  201f41:	41 89 44 24 14       	mov    %eax,0x14(%r12)
  201f46:	e9 3d f6 ff ff       	jmp    201588 <convM2S+0x78>
  201f4b:	4c 8d 6f 0b          	lea    0xb(%rdi),%r13
  201f4f:	4c 39 ed             	cmp    %r13,%rbp
  201f52:	0f 82 3b f6 ff ff    	jb     201593 <convM2S+0x83>
  201f58:	8b 47 07             	mov    0x7(%rdi),%eax
  201f5b:	41 89 44 24 14       	mov    %eax,0x14(%r12)
  201f60:	e9 23 f6 ff ff       	jmp    201588 <convM2S+0x78>
  201f65:	48 8d 77 09          	lea    0x9(%rdi),%rsi
  201f69:	48 39 f5             	cmp    %rsi,%rbp
  201f6c:	0f 82 21 f6 ff ff    	jb     201593 <convM2S+0x83>
  201f72:	44 0f b7 6f 07       	movzwl 0x7(%rdi),%r13d
  201f77:	48 8d 4f 08          	lea    0x8(%rdi),%rcx
  201f7b:	4e 8d 7c 2f 09       	lea    0x9(%rdi,%r13,1),%r15
  201f80:	4c 39 fd             	cmp    %r15,%rbp
  201f83:	0f 82 0a f6 ff ff    	jb     201593 <convM2S+0x83>
  201f89:	48 89 cf             	mov    %rcx,%rdi
  201f8c:	4c 89 ea             	mov    %r13,%rdx
  201f8f:	48 89 0c 24          	mov    %rcx,(%rsp)
  201f93:	e8 28 f3 ff ff       	call   2012c0 <memmove>
  201f98:	48 8b 0c 24          	mov    (%rsp),%rcx
  201f9c:	49 8d 47 02          	lea    0x2(%r15),%rax
  201fa0:	42 c6 44 2b 08 00    	movb   $0x0,0x8(%rbx,%r13,1)
  201fa6:	49 89 4c 24 18       	mov    %rcx,0x18(%r12)
  201fab:	48 39 c5             	cmp    %rax,%rbp
  201fae:	0f 82 df f5 ff ff    	jb     201593 <convM2S+0x83>
  201fb4:	41 0f b7 17          	movzwl (%r15),%edx
  201fb8:	4c 8d 2c 10          	lea    (%rax,%rdx,1),%r13
  201fbc:	66 41 89 54 24 10    	mov    %dx,0x10(%r12)
  201fc2:	4c 39 ed             	cmp    %r13,%rbp
  201fc5:	0f 83 f1 f6 ff ff    	jae    2016bc <convM2S+0x1ac>
  201fcb:	e9 c3 f5 ff ff       	jmp    201593 <convM2S+0x83>
  201fd0:	48 8d 47 0b          	lea    0xb(%rdi),%rax
  201fd4:	48 39 c5             	cmp    %rax,%rbp
  201fd7:	0f 82 b6 f5 ff ff    	jb     201593 <convM2S+0x83>
  201fdd:	8b 47 07             	mov    0x7(%rdi),%eax
  201fe0:	41 89 44 24 04       	mov    %eax,0x4(%r12)
  201fe5:	48 8d 47 0f          	lea    0xf(%rdi),%rax
  201fe9:	48 39 c5             	cmp    %rax,%rbp
  201fec:	0f 82 a1 f5 ff ff    	jb     201593 <convM2S+0x83>
  201ff2:	8b 47 0b             	mov    0xb(%rdi),%eax
  201ff5:	48 8d 77 11          	lea    0x11(%rdi),%rsi
  201ff9:	41 89 44 24 10       	mov    %eax,0x10(%r12)
  201ffe:	48 39 f5             	cmp    %rsi,%rbp
  202001:	0f 82 8c f5 ff ff    	jb     201593 <convM2S+0x83>
  202007:	44 0f b7 6f 0f       	movzwl 0xf(%rdi),%r13d
  20200c:	48 8d 4f 10          	lea    0x10(%rdi),%rcx
  202010:	4e 8d 7c 2f 11       	lea    0x11(%rdi,%r13,1),%r15
  202015:	4c 39 fd             	cmp    %r15,%rbp
  202018:	0f 82 75 f5 ff ff    	jb     201593 <convM2S+0x83>
  20201e:	4c 89 ea             	mov    %r13,%rdx
  202021:	48 89 cf             	mov    %rcx,%rdi
  202024:	48 89 0c 24          	mov    %rcx,(%rsp)
  202028:	e8 93 f2 ff ff       	call   2012c0 <memmove>
  20202d:	42 c6 44 2b 10 00    	movb   $0x0,0x10(%rbx,%r13,1)
  202033:	e9 01 fd ff ff       	jmp    201d39 <convM2S+0x829>
  202038:	4c 39 ed             	cmp    %r13,%rbp
  20203b:	0f 92 c0             	setb   %al
  20203e:	e9 f6 f7 ff ff       	jmp    201839 <convM2S+0x329>
  202043:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  20204a:	00 00 00 
  20204d:	0f 1f 00             	nopl   (%rax)

0000000000202050 <pqid>:
  202050:	0f b6 56 10          	movzbl 0x10(%rsi),%edx
  202054:	48 89 f8             	mov    %rdi,%rax
  202057:	48 83 c0 0d          	add    $0xd,%rax
  20205b:	88 17                	mov    %dl,(%rdi)
  20205d:	48 8b 56 08          	mov    0x8(%rsi),%rdx
  202061:	88 57 01             	mov    %dl,0x1(%rdi)
  202064:	48 8b 56 08          	mov    0x8(%rsi),%rdx
  202068:	88 77 02             	mov    %dh,0x2(%rdi)
  20206b:	48 8b 56 08          	mov    0x8(%rsi),%rdx
  20206f:	48 c1 ea 10          	shr    $0x10,%rdx
  202073:	88 57 03             	mov    %dl,0x3(%rdi)
  202076:	48 8b 56 08          	mov    0x8(%rsi),%rdx
  20207a:	48 c1 ea 18          	shr    $0x18,%rdx
  20207e:	88 57 04             	mov    %dl,0x4(%rdi)
  202081:	48 8b 16             	mov    (%rsi),%rdx
  202084:	88 57 05             	mov    %dl,0x5(%rdi)
  202087:	48 8b 16             	mov    (%rsi),%rdx
  20208a:	88 77 06             	mov    %dh,0x6(%rdi)
  20208d:	48 8b 16             	mov    (%rsi),%rdx
  202090:	48 c1 ea 10          	shr    $0x10,%rdx
  202094:	88 57 07             	mov    %dl,0x7(%rdi)
  202097:	48 8b 16             	mov    (%rsi),%rdx
  20209a:	48 c1 ea 18          	shr    $0x18,%rdx
  20209e:	88 57 08             	mov    %dl,0x8(%rdi)
  2020a1:	48 8b 16             	mov    (%rsi),%rdx
  2020a4:	48 c1 ea 20          	shr    $0x20,%rdx
  2020a8:	88 57 09             	mov    %dl,0x9(%rdi)
  2020ab:	48 8b 16             	mov    (%rsi),%rdx
  2020ae:	48 c1 ea 28          	shr    $0x28,%rdx
  2020b2:	88 57 0a             	mov    %dl,0xa(%rdi)
  2020b5:	48 8b 16             	mov    (%rsi),%rdx
  2020b8:	48 c1 ea 30          	shr    $0x30,%rdx
  2020bc:	88 57 0b             	mov    %dl,0xb(%rdi)
  2020bf:	48 8b 16             	mov    (%rsi),%rdx
  2020c2:	48 c1 ea 38          	shr    $0x38,%rdx
  2020c6:	88 57 0c             	mov    %dl,0xc(%rdi)
  2020c9:	c3                   	ret
  2020ca:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)

00000000002020d0 <sizeS2M>:
  2020d0:	f3 0f 1e fa          	endbr64
  2020d4:	41 54                	push   %r12
  2020d6:	55                   	push   %rbp
  2020d7:	53                   	push   %rbx
  2020d8:	0f b6 0f             	movzbl (%rdi),%ecx
  2020db:	80 f9 cd             	cmp    $0xcd,%cl
  2020de:	0f 87 9c 00 00 00    	ja     202180 <sizeS2M+0xb0>
  2020e4:	48 89 fb             	mov    %rdi,%rbx
  2020e7:	80 f9 7d             	cmp    $0x7d,%cl
  2020ea:	76 24                	jbe    202110 <sizeS2M+0x40>
  2020ec:	83 e9 7e             	sub    $0x7e,%ecx
  2020ef:	80 f9 4f             	cmp    $0x4f,%cl
  2020f2:	0f 87 88 00 00 00    	ja     202180 <sizeS2M+0xb0>
  2020f8:	48 8d 15 45 14 00 00 	lea    0x1445(%rip),%rdx        # 203544 <_syscall+0x3ff>
  2020ff:	0f b6 c9             	movzbl %cl,%ecx
  202102:	48 63 04 8a          	movslq (%rdx,%rcx,4),%rax
  202106:	48 01 d0             	add    %rdx,%rax
  202109:	3e ff e0             	notrack jmp *%rax
  20210c:	0f 1f 40 00          	nopl   0x0(%rax)
  202110:	80 f9 76             	cmp    $0x76,%cl
  202113:	77 2b                	ja     202140 <sizeS2M+0x70>
  202115:	80 f9 63             	cmp    $0x63,%cl
  202118:	76 66                	jbe    202180 <sizeS2M+0xb0>
  20211a:	83 e9 64             	sub    $0x64,%ecx
  20211d:	80 f9 12             	cmp    $0x12,%cl
  202120:	77 5e                	ja     202180 <sizeS2M+0xb0>
  202122:	48 8d 15 5b 15 00 00 	lea    0x155b(%rip),%rdx        # 203684 <_syscall+0x53f>
  202129:	0f b6 c9             	movzbl %cl,%ecx
  20212c:	48 63 04 8a          	movslq (%rdx,%rcx,4),%rax
  202130:	48 01 d0             	add    %rdx,%rax
  202133:	3e ff e0             	notrack jmp *%rax
  202136:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  20213d:	00 00 00 
  202140:	83 e9 77             	sub    $0x77,%ecx
  202143:	b8 01 00 00 00       	mov    $0x1,%eax
  202148:	bd 0b 00 00 00       	mov    $0xb,%ebp
  20214d:	48 d3 e0             	shl    %cl,%rax
  202150:	a8 2b                	test   $0x2b,%al
  202152:	75 11                	jne    202165 <sizeS2M+0x95>
  202154:	a8 14                	test   $0x14,%al
  202156:	0f 84 d4 00 00 00    	je     202230 <sizeS2M+0x160>
  20215c:	0f 1f 40 00          	nopl   0x0(%rax)
  202160:	bd 07 00 00 00       	mov    $0x7,%ebp
  202165:	89 e8                	mov    %ebp,%eax
  202167:	5b                   	pop    %rbx
  202168:	5d                   	pop    %rbp
  202169:	41 5c                	pop    %r12
  20216b:	c3                   	ret
  20216c:	0f 1f 40 00          	nopl   0x0(%rax)
  202170:	bd 0b 00 00 00       	mov    $0xb,%ebp
  202175:	eb ee                	jmp    202165 <sizeS2M+0x95>
  202177:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  20217e:	00 00 
  202180:	31 ed                	xor    %ebp,%ebp
  202182:	5b                   	pop    %rbx
  202183:	89 e8                	mov    %ebp,%eax
  202185:	5d                   	pop    %rbp
  202186:	41 5c                	pop    %r12
  202188:	c3                   	ret
  202189:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  202190:	48 8b 7b 18          	mov    0x18(%rbx),%rdi
  202194:	48 85 ff             	test   %rdi,%rdi
  202197:	0f 84 63 02 00 00    	je     202400 <sizeS2M+0x330>
  20219d:	e8 be f1 ff ff       	call   201360 <strlen>
  2021a2:	8d 68 0d             	lea    0xd(%rax),%ebp
  2021a5:	eb be                	jmp    202165 <sizeS2M+0x95>
  2021a7:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  2021ae:	00 00 
  2021b0:	48 8b 7f 18          	mov    0x18(%rdi),%rdi
  2021b4:	48 85 ff             	test   %rdi,%rdi
  2021b7:	74 18                	je     2021d1 <sizeS2M+0x101>
  2021b9:	e8 a2 f1 ff ff       	call   201360 <strlen>
  2021be:	8d 68 09             	lea    0x9(%rax),%ebp
  2021c1:	eb a2                	jmp    202165 <sizeS2M+0x95>
  2021c3:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  2021c8:	48 8b 7b 10          	mov    0x10(%rbx),%rdi
  2021cc:	48 85 ff             	test   %rdi,%rdi
  2021cf:	75 e8                	jne    2021b9 <sizeS2M+0xe9>
  2021d1:	bd 09 00 00 00       	mov    $0x9,%ebp
  2021d6:	eb 8d                	jmp    202165 <sizeS2M+0x95>
  2021d8:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
  2021df:	00 
  2021e0:	48 8b 7b 18          	mov    0x18(%rbx),%rdi
  2021e4:	48 85 ff             	test   %rdi,%rdi
  2021e7:	0f 84 6d 01 00 00    	je     20235a <sizeS2M+0x28a>
  2021ed:	e8 6e f1 ff ff       	call   201360 <strlen>
  2021f2:	8d 68 12             	lea    0x12(%rax),%ebp
  2021f5:	e9 6b ff ff ff       	jmp    202165 <sizeS2M+0x95>
  2021fa:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  202200:	8b 43 18             	mov    0x18(%rbx),%eax
  202203:	8d 68 17             	lea    0x17(%rax),%ebp
  202206:	e9 5a ff ff ff       	jmp    202165 <sizeS2M+0x95>
  20220b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  202210:	8b 43 18             	mov    0x18(%rbx),%eax
  202213:	8d 68 0b             	lea    0xb(%rax),%ebp
  202216:	e9 4a ff ff ff       	jmp    202165 <sizeS2M+0x95>
  20221b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  202220:	0f b7 47 10          	movzwl 0x10(%rdi),%eax
  202224:	8d 68 0d             	lea    0xd(%rax),%ebp
  202227:	e9 39 ff ff ff       	jmp    202165 <sizeS2M+0x95>
  20222c:	0f 1f 40 00          	nopl   0x0(%rax)
  202230:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  202234:	8d 68 09             	lea    0x9(%rax),%ebp
  202237:	e9 29 ff ff ff       	jmp    202165 <sizeS2M+0x95>
  20223c:	0f 1f 40 00          	nopl   0x0(%rax)
  202240:	8b 47 20             	mov    0x20(%rdi),%eax
  202243:	8d 68 13             	lea    0x13(%rax),%ebp
  202246:	e9 1a ff ff ff       	jmp    202165 <sizeS2M+0x95>
  20224b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  202250:	bd 0f 00 00 00       	mov    $0xf,%ebp
  202255:	e9 0b ff ff ff       	jmp    202165 <sizeS2M+0x95>
  20225a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  202260:	bd 18 00 00 00       	mov    $0x18,%ebp
  202265:	e9 fb fe ff ff       	jmp    202165 <sizeS2M+0x95>
  20226a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  202270:	bd 14 00 00 00       	mov    $0x14,%ebp
  202275:	e9 eb fe ff ff       	jmp    202165 <sizeS2M+0x95>
  20227a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  202280:	bd 17 00 00 00       	mov    $0x17,%ebp
  202285:	e9 db fe ff ff       	jmp    202165 <sizeS2M+0x95>
  20228a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  202290:	45 31 e4             	xor    %r12d,%r12d
  202293:	66 83 7f 14 00       	cmpw   $0x0,0x14(%rdi)
  202298:	bd 11 00 00 00       	mov    $0x11,%ebp
  20229d:	75 24                	jne    2022c3 <sizeS2M+0x1f3>
  20229f:	e9 c1 fe ff ff       	jmp    202165 <sizeS2M+0x95>
  2022a4:	0f 1f 40 00          	nopl   0x0(%rax)
  2022a8:	e8 b3 f0 ff ff       	call   201360 <strlen>
  2022ad:	83 c0 02             	add    $0x2,%eax
  2022b0:	01 c5                	add    %eax,%ebp
  2022b2:	0f b7 43 14          	movzwl 0x14(%rbx),%eax
  2022b6:	49 83 c4 01          	add    $0x1,%r12
  2022ba:	44 39 e0             	cmp    %r12d,%eax
  2022bd:	0f 8e a2 fe ff ff    	jle    202165 <sizeS2M+0x95>
  2022c3:	4a 8b 7c e3 18       	mov    0x18(%rbx,%r12,8),%rdi
  2022c8:	48 85 ff             	test   %rdi,%rdi
  2022cb:	75 db                	jne    2022a8 <sizeS2M+0x1d8>
  2022cd:	b8 02 00 00 00       	mov    $0x2,%eax
  2022d2:	eb dc                	jmp    2022b0 <sizeS2M+0x1e0>
  2022d4:	0f 1f 40 00          	nopl   0x0(%rax)
  2022d8:	48 8b 7f 18          	mov    0x18(%rdi),%rdi
  2022dc:	48 85 ff             	test   %rdi,%rdi
  2022df:	0f 84 40 01 00 00    	je     202425 <sizeS2M+0x355>
  2022e5:	e8 76 f0 ff ff       	call   201360 <strlen>
  2022ea:	8d 68 0d             	lea    0xd(%rax),%ebp
  2022ed:	48 8b 7b 20          	mov    0x20(%rbx),%rdi
  2022f1:	48 85 ff             	test   %rdi,%rdi
  2022f4:	0f 84 c4 00 00 00    	je     2023be <sizeS2M+0x2ee>
  2022fa:	e8 61 f0 ff ff       	call   201360 <strlen>
  2022ff:	83 c0 02             	add    $0x2,%eax
  202302:	01 c5                	add    %eax,%ebp
  202304:	89 e8                	mov    %ebp,%eax
  202306:	5b                   	pop    %rbx
  202307:	5d                   	pop    %rbp
  202308:	41 5c                	pop    %r12
  20230a:	c3                   	ret
  20230b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  202310:	48 8b 7f 18          	mov    0x18(%rdi),%rdi
  202314:	48 85 ff             	test   %rdi,%rdi
  202317:	0f 84 f7 00 00 00    	je     202414 <sizeS2M+0x344>
  20231d:	e8 3e f0 ff ff       	call   201360 <strlen>
  202322:	8d 68 11             	lea    0x11(%rax),%ebp
  202325:	eb c6                	jmp    2022ed <sizeS2M+0x21d>
  202327:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  20232e:	00 00 
  202330:	0f b7 47 10          	movzwl 0x10(%rdi),%eax
  202334:	8d 14 40             	lea    (%rax,%rax,2),%edx
  202337:	8d 6c 90 09          	lea    0x9(%rax,%rdx,4),%ebp
  20233b:	e9 25 fe ff ff       	jmp    202165 <sizeS2M+0x95>
  202340:	bd 0c 00 00 00       	mov    $0xc,%ebp
  202345:	e9 1b fe ff ff       	jmp    202165 <sizeS2M+0x95>
  20234a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  202350:	bd 1c 00 00 00       	mov    $0x1c,%ebp
  202355:	e9 0b fe ff ff       	jmp    202165 <sizeS2M+0x95>
  20235a:	bd 12 00 00 00       	mov    $0x12,%ebp
  20235f:	e9 01 fe ff ff       	jmp    202165 <sizeS2M+0x95>
  202364:	48 8b 7f 18          	mov    0x18(%rdi),%rdi
  202368:	48 85 ff             	test   %rdi,%rdi
  20236b:	0f 84 99 00 00 00    	je     20240a <sizeS2M+0x33a>
  202371:	e8 ea ef ff ff       	call   201360 <strlen>
  202376:	83 c0 02             	add    $0x2,%eax
  202379:	0f b7 53 10          	movzwl 0x10(%rbx),%edx
  20237d:	8d 6c 10 09          	lea    0x9(%rax,%rdx,1),%ebp
  202381:	e9 df fd ff ff       	jmp    202165 <sizeS2M+0x95>
  202386:	48 8b 7f 18          	mov    0x18(%rdi),%rdi
  20238a:	48 85 ff             	test   %rdi,%rdi
  20238d:	0f 84 9c 00 00 00    	je     20242f <sizeS2M+0x35f>
  202393:	e8 c8 ef ff ff       	call   201360 <strlen>
  202398:	8d 68 0e             	lea    0xe(%rax),%ebp
  20239b:	e9 c5 fd ff ff       	jmp    202165 <sizeS2M+0x95>
  2023a0:	48 8b 7f 18          	mov    0x18(%rdi),%rdi
  2023a4:	48 85 ff             	test   %rdi,%rdi
  2023a7:	74 75                	je     20241e <sizeS2M+0x34e>
  2023a9:	e8 b2 ef ff ff       	call   201360 <strlen>
  2023ae:	8d 68 0d             	lea    0xd(%rax),%ebp
  2023b1:	48 8b 7b 10          	mov    0x10(%rbx),%rdi
  2023b5:	48 85 ff             	test   %rdi,%rdi
  2023b8:	0f 85 3c ff ff ff    	jne    2022fa <sizeS2M+0x22a>
  2023be:	b8 02 00 00 00       	mov    $0x2,%eax
  2023c3:	01 c5                	add    %eax,%ebp
  2023c5:	e9 3a ff ff ff       	jmp    202304 <sizeS2M+0x234>
  2023ca:	48 8b 7f 18          	mov    0x18(%rdi),%rdi
  2023ce:	48 85 ff             	test   %rdi,%rdi
  2023d1:	74 66                	je     202439 <sizeS2M+0x369>
  2023d3:	e8 88 ef ff ff       	call   201360 <strlen>
  2023d8:	8d 68 09             	lea    0x9(%rax),%ebp
  2023db:	eb d4                	jmp    2023b1 <sizeS2M+0x2e1>
  2023dd:	48 8b 7f 10          	mov    0x10(%rdi),%rdi
  2023e1:	48 85 ff             	test   %rdi,%rdi
  2023e4:	74 5d                	je     202443 <sizeS2M+0x373>
  2023e6:	e8 75 ef ff ff       	call   201360 <strlen>
  2023eb:	8d 68 15             	lea    0x15(%rax),%ebp
  2023ee:	e9 fa fe ff ff       	jmp    2022ed <sizeS2M+0x21d>
  2023f3:	48 8b 7f 10          	mov    0x10(%rdi),%rdi
  2023f7:	48 85 ff             	test   %rdi,%rdi
  2023fa:	0f 85 9d fd ff ff    	jne    20219d <sizeS2M+0xcd>
  202400:	bd 0d 00 00 00       	mov    $0xd,%ebp
  202405:	e9 5b fd ff ff       	jmp    202165 <sizeS2M+0x95>
  20240a:	b8 02 00 00 00       	mov    $0x2,%eax
  20240f:	e9 65 ff ff ff       	jmp    202379 <sizeS2M+0x2a9>
  202414:	bd 11 00 00 00       	mov    $0x11,%ebp
  202419:	e9 cf fe ff ff       	jmp    2022ed <sizeS2M+0x21d>
  20241e:	bd 0d 00 00 00       	mov    $0xd,%ebp
  202423:	eb 8c                	jmp    2023b1 <sizeS2M+0x2e1>
  202425:	bd 0d 00 00 00       	mov    $0xd,%ebp
  20242a:	e9 be fe ff ff       	jmp    2022ed <sizeS2M+0x21d>
  20242f:	bd 0e 00 00 00       	mov    $0xe,%ebp
  202434:	e9 2c fd ff ff       	jmp    202165 <sizeS2M+0x95>
  202439:	bd 09 00 00 00       	mov    $0x9,%ebp
  20243e:	e9 6e ff ff ff       	jmp    2023b1 <sizeS2M+0x2e1>
  202443:	bd 15 00 00 00       	mov    $0x15,%ebp
  202448:	e9 a0 fe ff ff       	jmp    2022ed <sizeS2M+0x21d>
  20244d:	0f 1f 00             	nopl   (%rax)

0000000000202450 <convS2M>:
  202450:	f3 0f 1e fa          	endbr64
  202454:	41 57                	push   %r15
  202456:	41 56                	push   %r14
  202458:	41 55                	push   %r13
  20245a:	41 89 d5             	mov    %edx,%r13d
  20245d:	41 54                	push   %r12
  20245f:	55                   	push   %rbp
  202460:	48 89 f5             	mov    %rsi,%rbp
  202463:	53                   	push   %rbx
  202464:	48 89 fb             	mov    %rdi,%rbx
  202467:	48 83 ec 18          	sub    $0x18,%rsp
  20246b:	e8 60 fc ff ff       	call   2020d0 <sizeS2M>
  202470:	41 89 c4             	mov    %eax,%r12d
  202473:	83 e8 01             	sub    $0x1,%eax
  202476:	44 39 e8             	cmp    %r13d,%eax
  202479:	72 15                	jb     202490 <convS2M+0x40>
  20247b:	45 31 e4             	xor    %r12d,%r12d
  20247e:	48 83 c4 18          	add    $0x18,%rsp
  202482:	44 89 e0             	mov    %r12d,%eax
  202485:	5b                   	pop    %rbx
  202486:	5d                   	pop    %rbp
  202487:	41 5c                	pop    %r12
  202489:	41 5d                	pop    %r13
  20248b:	41 5e                	pop    %r14
  20248d:	41 5f                	pop    %r15
  20248f:	c3                   	ret
  202490:	44 89 65 00          	mov    %r12d,0x0(%rbp)
  202494:	0f b6 03             	movzbl (%rbx),%eax
  202497:	4c 8d 75 07          	lea    0x7(%rbp),%r14
  20249b:	88 45 04             	mov    %al,0x4(%rbp)
  20249e:	0f b7 43 08          	movzwl 0x8(%rbx),%eax
  2024a2:	88 45 05             	mov    %al,0x5(%rbp)
  2024a5:	0f b6 43 09          	movzbl 0x9(%rbx),%eax
  2024a9:	88 45 06             	mov    %al,0x6(%rbp)
  2024ac:	0f b6 03             	movzbl (%rbx),%eax
  2024af:	83 e8 64             	sub    $0x64,%eax
  2024b2:	3c 69                	cmp    $0x69,%al
  2024b4:	77 c5                	ja     20247b <convS2M+0x2b>
  2024b6:	48 8d 15 13 12 00 00 	lea    0x1213(%rip),%rdx        # 2036d0 <_syscall+0x58b>
  2024bd:	0f b6 c0             	movzbl %al,%eax
  2024c0:	48 63 04 82          	movslq (%rdx,%rax,4),%rax
  2024c4:	48 01 d0             	add    %rdx,%rax
  2024c7:	3e ff e0             	notrack jmp *%rax
  2024ca:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  2024d0:	b8 07 00 00 00       	mov    $0x7,%eax
  2024d5:	44 89 e2             	mov    %r12d,%edx
  2024d8:	48 39 c2             	cmp    %rax,%rdx
  2024db:	75 9e                	jne    20247b <convS2M+0x2b>
  2024dd:	eb 9f                	jmp    20247e <convS2M+0x2e>
  2024df:	90                   	nop
  2024e0:	8b 43 04             	mov    0x4(%rbx),%eax
  2024e3:	88 45 07             	mov    %al,0x7(%rbp)
  2024e6:	8b 43 04             	mov    0x4(%rbx),%eax
  2024e9:	88 65 08             	mov    %ah,0x8(%rbp)
  2024ec:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  2024f0:	88 45 09             	mov    %al,0x9(%rbp)
  2024f3:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  2024f7:	88 45 0a             	mov    %al,0xa(%rbp)
  2024fa:	b8 0b 00 00 00       	mov    $0xb,%eax
  2024ff:	eb d4                	jmp    2024d5 <convS2M+0x85>
  202501:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  202508:	8b 43 18             	mov    0x18(%rbx),%eax
  20250b:	88 45 07             	mov    %al,0x7(%rbp)
  20250e:	8b 43 18             	mov    0x18(%rbx),%eax
  202511:	88 65 08             	mov    %ah,0x8(%rbp)
  202514:	0f b7 43 1a          	movzwl 0x1a(%rbx),%eax
  202518:	88 45 09             	mov    %al,0x9(%rbp)
  20251b:	0f b6 43 1b          	movzbl 0x1b(%rbx),%eax
  20251f:	eb d6                	jmp    2024f7 <convS2M+0xa7>
  202521:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  202528:	4c 8b 6b 18          	mov    0x18(%rbx),%r13
  20252c:	4d 85 ed             	test   %r13,%r13
  20252f:	0f 84 45 02 00 00    	je     20277a <convS2M+0x32a>
  202535:	4c 89 ef             	mov    %r13,%rdi
  202538:	e8 23 ee ff ff       	call   201360 <strlen>
  20253d:	48 8d 7d 09          	lea    0x9(%rbp),%rdi
  202541:	4c 89 ee             	mov    %r13,%rsi
  202544:	48 89 c3             	mov    %rax,%rbx
  202547:	89 c2                	mov    %eax,%edx
  202549:	e8 72 ed ff ff       	call   2012c0 <memmove>
  20254e:	8d 43 02             	lea    0x2(%rbx),%eax
  202551:	66 89 5d 07          	mov    %bx,0x7(%rbp)
  202555:	4c 01 f0             	add    %r14,%rax
  202558:	48 29 e8             	sub    %rbp,%rax
  20255b:	e9 75 ff ff ff       	jmp    2024d5 <convS2M+0x85>
  202560:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202564:	88 45 07             	mov    %al,0x7(%rbp)
  202567:	48 8b 43 10          	mov    0x10(%rbx),%rax
  20256b:	88 65 08             	mov    %ah,0x8(%rbp)
  20256e:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202572:	48 c1 e8 10          	shr    $0x10,%rax
  202576:	88 45 09             	mov    %al,0x9(%rbp)
  202579:	48 8b 43 10          	mov    0x10(%rbx),%rax
  20257d:	48 c1 e8 18          	shr    $0x18,%rax
  202581:	88 45 0a             	mov    %al,0xa(%rbp)
  202584:	8b 43 14             	mov    0x14(%rbx),%eax
  202587:	88 45 0b             	mov    %al,0xb(%rbp)
  20258a:	48 8b 43 10          	mov    0x10(%rbx),%rax
  20258e:	48 c1 e8 28          	shr    $0x28,%rax
  202592:	88 45 0c             	mov    %al,0xc(%rbp)
  202595:	0f b7 43 16          	movzwl 0x16(%rbx),%eax
  202599:	88 45 0d             	mov    %al,0xd(%rbp)
  20259c:	0f b6 43 17          	movzbl 0x17(%rbx),%eax
  2025a0:	88 45 0e             	mov    %al,0xe(%rbp)
  2025a3:	b8 0f 00 00 00       	mov    $0xf,%eax
  2025a8:	e9 28 ff ff ff       	jmp    2024d5 <convS2M+0x85>
  2025ad:	0f 1f 00             	nopl   (%rax)
  2025b0:	8b 43 04             	mov    0x4(%rbx),%eax
  2025b3:	88 45 07             	mov    %al,0x7(%rbp)
  2025b6:	8b 43 04             	mov    0x4(%rbx),%eax
  2025b9:	88 65 08             	mov    %ah,0x8(%rbp)
  2025bc:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  2025c0:	88 45 09             	mov    %al,0x9(%rbp)
  2025c3:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  2025c7:	88 45 0a             	mov    %al,0xa(%rbp)
  2025ca:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2025ce:	88 45 0b             	mov    %al,0xb(%rbp)
  2025d1:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2025d5:	88 65 0c             	mov    %ah,0xc(%rbp)
  2025d8:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2025dc:	48 c1 f8 10          	sar    $0x10,%rax
  2025e0:	88 45 0d             	mov    %al,0xd(%rbp)
  2025e3:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2025e7:	48 c1 f8 18          	sar    $0x18,%rax
  2025eb:	88 45 0e             	mov    %al,0xe(%rbp)
  2025ee:	48 63 43 14          	movslq 0x14(%rbx),%rax
  2025f2:	88 45 0f             	mov    %al,0xf(%rbp)
  2025f5:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2025f9:	48 c1 f8 28          	sar    $0x28,%rax
  2025fd:	88 45 10             	mov    %al,0x10(%rbp)
  202600:	48 0f bf 43 16       	movswq 0x16(%rbx),%rax
  202605:	88 45 11             	mov    %al,0x11(%rbp)
  202608:	48 0f be 43 17       	movsbq 0x17(%rbx),%rax
  20260d:	88 45 12             	mov    %al,0x12(%rbp)
  202610:	8b 43 18             	mov    0x18(%rbx),%eax
  202613:	88 45 13             	mov    %al,0x13(%rbp)
  202616:	8b 43 18             	mov    0x18(%rbx),%eax
  202619:	88 65 14             	mov    %ah,0x14(%rbp)
  20261c:	0f b7 43 1a          	movzwl 0x1a(%rbx),%eax
  202620:	88 45 15             	mov    %al,0x15(%rbp)
  202623:	0f b6 43 1b          	movzbl 0x1b(%rbx),%eax
  202627:	88 45 16             	mov    %al,0x16(%rbp)
  20262a:	b8 17 00 00 00       	mov    $0x17,%eax
  20262f:	e9 a1 fe ff ff       	jmp    2024d5 <convS2M+0x85>
  202634:	0f 1f 40 00          	nopl   0x0(%rax)
  202638:	8b 43 18             	mov    0x18(%rbx),%eax
  20263b:	4c 8d 6d 0b          	lea    0xb(%rbp),%r13
  20263f:	88 45 07             	mov    %al,0x7(%rbp)
  202642:	8b 43 18             	mov    0x18(%rbx),%eax
  202645:	88 65 08             	mov    %ah,0x8(%rbp)
  202648:	0f b7 43 1a          	movzwl 0x1a(%rbx),%eax
  20264c:	88 45 09             	mov    %al,0x9(%rbp)
  20264f:	0f b6 43 1b          	movzbl 0x1b(%rbx),%eax
  202653:	88 45 0a             	mov    %al,0xa(%rbp)
  202656:	8b 53 18             	mov    0x18(%rbx),%edx
  202659:	48 8b 73 20          	mov    0x20(%rbx),%rsi
  20265d:	4c 89 ef             	mov    %r13,%rdi
  202660:	e8 5b ec ff ff       	call   2012c0 <memmove>
  202665:	8b 43 18             	mov    0x18(%rbx),%eax
  202668:	4c 01 e8             	add    %r13,%rax
  20266b:	48 29 e8             	sub    %rbp,%rax
  20266e:	e9 62 fe ff ff       	jmp    2024d5 <convS2M+0x85>
  202673:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  202678:	8b 43 04             	mov    0x4(%rbx),%eax
  20267b:	4c 8d 6d 17          	lea    0x17(%rbp),%r13
  20267f:	88 45 07             	mov    %al,0x7(%rbp)
  202682:	8b 43 04             	mov    0x4(%rbx),%eax
  202685:	88 65 08             	mov    %ah,0x8(%rbp)
  202688:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  20268c:	88 45 09             	mov    %al,0x9(%rbp)
  20268f:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  202693:	88 45 0a             	mov    %al,0xa(%rbp)
  202696:	48 8b 43 10          	mov    0x10(%rbx),%rax
  20269a:	88 45 0b             	mov    %al,0xb(%rbp)
  20269d:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2026a1:	88 65 0c             	mov    %ah,0xc(%rbp)
  2026a4:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2026a8:	48 c1 f8 10          	sar    $0x10,%rax
  2026ac:	88 45 0d             	mov    %al,0xd(%rbp)
  2026af:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2026b3:	48 c1 f8 18          	sar    $0x18,%rax
  2026b7:	88 45 0e             	mov    %al,0xe(%rbp)
  2026ba:	48 63 43 14          	movslq 0x14(%rbx),%rax
  2026be:	88 45 0f             	mov    %al,0xf(%rbp)
  2026c1:	48 8b 43 10          	mov    0x10(%rbx),%rax
  2026c5:	48 c1 f8 28          	sar    $0x28,%rax
  2026c9:	88 45 10             	mov    %al,0x10(%rbp)
  2026cc:	48 0f bf 43 16       	movswq 0x16(%rbx),%rax
  2026d1:	88 45 11             	mov    %al,0x11(%rbp)
  2026d4:	48 0f be 43 17       	movsbq 0x17(%rbx),%rax
  2026d9:	88 45 12             	mov    %al,0x12(%rbp)
  2026dc:	8b 43 18             	mov    0x18(%rbx),%eax
  2026df:	88 45 13             	mov    %al,0x13(%rbp)
  2026e2:	8b 43 18             	mov    0x18(%rbx),%eax
  2026e5:	88 65 14             	mov    %ah,0x14(%rbp)
  2026e8:	0f b7 43 1a          	movzwl 0x1a(%rbx),%eax
  2026ec:	88 45 15             	mov    %al,0x15(%rbp)
  2026ef:	0f b6 43 1b          	movzbl 0x1b(%rbx),%eax
  2026f3:	88 45 16             	mov    %al,0x16(%rbp)
  2026f6:	e9 5b ff ff ff       	jmp    202656 <convS2M+0x206>
  2026fb:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  202700:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  202704:	4c 8d 6d 09          	lea    0x9(%rbp),%r13
  202708:	88 45 07             	mov    %al,0x7(%rbp)
  20270b:	0f b6 43 11          	movzbl 0x11(%rbx),%eax
  20270f:	88 45 08             	mov    %al,0x8(%rbp)
  202712:	0f b7 53 10          	movzwl 0x10(%rbx),%edx
  202716:	48 8b 73 18          	mov    0x18(%rbx),%rsi
  20271a:	4c 89 ef             	mov    %r13,%rdi
  20271d:	e8 9e eb ff ff       	call   2012c0 <memmove>
  202722:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  202726:	e9 3d ff ff ff       	jmp    202668 <convS2M+0x218>
  20272b:	8b 43 04             	mov    0x4(%rbx),%eax
  20272e:	4c 8d 6d 0d          	lea    0xd(%rbp),%r13
  202732:	88 45 07             	mov    %al,0x7(%rbp)
  202735:	8b 43 04             	mov    0x4(%rbx),%eax
  202738:	88 65 08             	mov    %ah,0x8(%rbp)
  20273b:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  20273f:	88 45 09             	mov    %al,0x9(%rbp)
  202742:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  202746:	88 45 0a             	mov    %al,0xa(%rbp)
  202749:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  20274d:	88 45 0b             	mov    %al,0xb(%rbp)
  202750:	0f b6 43 11          	movzbl 0x11(%rbx),%eax
  202754:	88 45 0c             	mov    %al,0xc(%rbp)
  202757:	eb b9                	jmp    202712 <convS2M+0x2c2>
  202759:	48 8d 73 10          	lea    0x10(%rbx),%rsi
  20275d:	4c 89 f7             	mov    %r14,%rdi
  202760:	e8 eb f8 ff ff       	call   202050 <pqid>
  202765:	48 29 e8             	sub    %rbp,%rax
  202768:	e9 68 fd ff ff       	jmp    2024d5 <convS2M+0x85>
  20276d:	4c 8b 6b 10          	mov    0x10(%rbx),%r13
  202771:	4d 85 ed             	test   %r13,%r13
  202774:	0f 85 bb fd ff ff    	jne    202535 <convS2M+0xe5>
  20277a:	31 c0                	xor    %eax,%eax
  20277c:	66 89 45 07          	mov    %ax,0x7(%rbp)
  202780:	b8 09 00 00 00       	mov    $0x9,%eax
  202785:	e9 4b fd ff ff       	jmp    2024d5 <convS2M+0x85>
  20278a:	8b 43 04             	mov    0x4(%rbx),%eax
  20278d:	4c 8d 6d 0d          	lea    0xd(%rbp),%r13
  202791:	88 45 07             	mov    %al,0x7(%rbp)
  202794:	8b 43 04             	mov    0x4(%rbx),%eax
  202797:	88 65 08             	mov    %ah,0x8(%rbp)
  20279a:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  20279e:	88 45 09             	mov    %al,0x9(%rbp)
  2027a1:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  2027a5:	88 45 0a             	mov    %al,0xa(%rbp)
  2027a8:	4c 8b 7b 18          	mov    0x18(%rbx),%r15
  2027ac:	4d 85 ff             	test   %r15,%r15
  2027af:	0f 84 0f 09 00 00    	je     2030c4 <convS2M+0xc74>
  2027b5:	4c 89 ff             	mov    %r15,%rdi
  2027b8:	e8 a3 eb ff ff       	call   201360 <strlen>
  2027bd:	4c 89 ef             	mov    %r13,%rdi
  2027c0:	4c 89 fe             	mov    %r15,%rsi
  2027c3:	49 89 c6             	mov    %rax,%r14
  2027c6:	89 c2                	mov    %eax,%edx
  2027c8:	e8 f3 ea ff ff       	call   2012c0 <memmove>
  2027cd:	4c 89 f0             	mov    %r14,%rax
  2027d0:	41 8d 4e 02          	lea    0x2(%r14),%ecx
  2027d4:	44 89 f2             	mov    %r14d,%edx
  2027d7:	0f b6 c4             	movzbl %ah,%eax
  2027da:	4c 8d 6c 0d 0b       	lea    0xb(%rbp,%rcx,1),%r13
  2027df:	88 55 0b             	mov    %dl,0xb(%rbp)
  2027e2:	88 45 0c             	mov    %al,0xc(%rbp)
  2027e5:	8b 43 10             	mov    0x10(%rbx),%eax
  2027e8:	41 88 45 00          	mov    %al,0x0(%r13)
  2027ec:	8b 43 10             	mov    0x10(%rbx),%eax
  2027ef:	0f b6 c4             	movzbl %ah,%eax
  2027f2:	41 88 45 01          	mov    %al,0x1(%r13)
  2027f6:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  2027fa:	41 88 45 02          	mov    %al,0x2(%r13)
  2027fe:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202802:	41 88 45 03          	mov    %al,0x3(%r13)
  202806:	0f b6 43 20          	movzbl 0x20(%rbx),%eax
  20280a:	41 88 45 04          	mov    %al,0x4(%r13)
  20280e:	49 8d 45 05          	lea    0x5(%r13),%rax
  202812:	48 29 e8             	sub    %rbp,%rax
  202815:	e9 bb fc ff ff       	jmp    2024d5 <convS2M+0x85>
  20281a:	8b 43 10             	mov    0x10(%rbx),%eax
  20281d:	88 45 07             	mov    %al,0x7(%rbp)
  202820:	8b 43 10             	mov    0x10(%rbx),%eax
  202823:	88 65 08             	mov    %ah,0x8(%rbp)
  202826:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  20282a:	88 45 09             	mov    %al,0x9(%rbp)
  20282d:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202831:	88 45 0a             	mov    %al,0xa(%rbp)
  202834:	4c 8b 6b 18          	mov    0x18(%rbx),%r13
  202838:	4d 85 ed             	test   %r13,%r13
  20283b:	0f 84 7b 01 00 00    	je     2029bc <convS2M+0x56c>
  202841:	4c 89 ef             	mov    %r13,%rdi
  202844:	e8 17 eb ff ff       	call   201360 <strlen>
  202849:	48 8d 7d 0d          	lea    0xd(%rbp),%rdi
  20284d:	4c 89 ee             	mov    %r13,%rsi
  202850:	48 89 c3             	mov    %rax,%rbx
  202853:	89 c2                	mov    %eax,%edx
  202855:	e8 66 ea ff ff       	call   2012c0 <memmove>
  20285a:	8d 43 02             	lea    0x2(%rbx),%eax
  20285d:	66 89 5d 0b          	mov    %bx,0xb(%rbp)
  202861:	48 83 c0 0b          	add    $0xb,%rax
  202865:	e9 6b fc ff ff       	jmp    2024d5 <convS2M+0x85>
  20286a:	48 8d 73 10          	lea    0x10(%rbx),%rsi
  20286e:	4c 89 f7             	mov    %r14,%rdi
  202871:	e8 da f7 ff ff       	call   202050 <pqid>
  202876:	8b 53 28             	mov    0x28(%rbx),%edx
  202879:	48 83 c0 04          	add    $0x4,%rax
  20287d:	88 50 fc             	mov    %dl,-0x4(%rax)
  202880:	8b 53 28             	mov    0x28(%rbx),%edx
  202883:	88 70 fd             	mov    %dh,-0x3(%rax)
  202886:	0f b7 53 2a          	movzwl 0x2a(%rbx),%edx
  20288a:	88 50 fe             	mov    %dl,-0x2(%rax)
  20288d:	0f b6 53 2b          	movzbl 0x2b(%rbx),%edx
  202891:	88 50 ff             	mov    %dl,-0x1(%rax)
  202894:	48 29 e8             	sub    %rbp,%rax
  202897:	e9 39 fc ff ff       	jmp    2024d5 <convS2M+0x85>
  20289c:	8b 43 04             	mov    0x4(%rbx),%eax
  20289f:	48 8d 7d 0b          	lea    0xb(%rbp),%rdi
  2028a3:	48 8d 73 10          	lea    0x10(%rbx),%rsi
  2028a7:	88 45 07             	mov    %al,0x7(%rbp)
  2028aa:	8b 43 04             	mov    0x4(%rbx),%eax
  2028ad:	88 65 08             	mov    %ah,0x8(%rbp)
  2028b0:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  2028b4:	88 45 09             	mov    %al,0x9(%rbp)
  2028b7:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  2028bb:	88 45 0a             	mov    %al,0xa(%rbp)
  2028be:	eb b1                	jmp    202871 <convS2M+0x421>
  2028c0:	8b 43 18             	mov    0x18(%rbx),%eax
  2028c3:	4c 8d 6d 11          	lea    0x11(%rbp),%r13
  2028c7:	88 45 07             	mov    %al,0x7(%rbp)
  2028ca:	8b 43 18             	mov    0x18(%rbx),%eax
  2028cd:	88 65 08             	mov    %ah,0x8(%rbp)
  2028d0:	0f b7 43 1a          	movzwl 0x1a(%rbx),%eax
  2028d4:	88 45 09             	mov    %al,0x9(%rbp)
  2028d7:	0f b6 43 1b          	movzbl 0x1b(%rbx),%eax
  2028db:	88 45 0a             	mov    %al,0xa(%rbp)
  2028de:	8b 43 10             	mov    0x10(%rbx),%eax
  2028e1:	88 45 0b             	mov    %al,0xb(%rbp)
  2028e4:	8b 43 10             	mov    0x10(%rbx),%eax
  2028e7:	88 65 0c             	mov    %ah,0xc(%rbp)
  2028ea:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  2028ee:	88 45 0d             	mov    %al,0xd(%rbp)
  2028f1:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  2028f5:	88 45 0e             	mov    %al,0xe(%rbp)
  2028f8:	4c 8b 7b 10          	mov    0x10(%rbx),%r15
  2028fc:	4d 85 ff             	test   %r15,%r15
  2028ff:	0f 84 ee 07 00 00    	je     2030f3 <convS2M+0xca3>
  202905:	4c 89 ff             	mov    %r15,%rdi
  202908:	e8 53 ea ff ff       	call   201360 <strlen>
  20290d:	4c 89 ef             	mov    %r13,%rdi
  202910:	4c 89 fe             	mov    %r15,%rsi
  202913:	49 89 c6             	mov    %rax,%r14
  202916:	89 c2                	mov    %eax,%edx
  202918:	e8 a3 e9 ff ff       	call   2012c0 <memmove>
  20291d:	4c 89 f0             	mov    %r14,%rax
  202920:	41 8d 4e 02          	lea    0x2(%r14),%ecx
  202924:	44 89 f2             	mov    %r14d,%edx
  202927:	0f b6 c4             	movzbl %ah,%eax
  20292a:	4c 8d 6c 0d 0f       	lea    0xf(%rbp,%rcx,1),%r13
  20292f:	88 55 0f             	mov    %dl,0xf(%rbp)
  202932:	88 45 10             	mov    %al,0x10(%rbp)
  202935:	8b 43 10             	mov    0x10(%rbx),%eax
  202938:	41 88 45 00          	mov    %al,0x0(%r13)
  20293c:	8b 43 10             	mov    0x10(%rbx),%eax
  20293f:	0f b6 c4             	movzbl %ah,%eax
  202942:	41 88 45 01          	mov    %al,0x1(%r13)
  202946:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  20294a:	41 88 45 02          	mov    %al,0x2(%r13)
  20294e:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202952:	41 88 45 03          	mov    %al,0x3(%r13)
  202956:	4c 8b 7b 20          	mov    0x20(%rbx),%r15
  20295a:	49 8d 5d 06          	lea    0x6(%r13),%rbx
  20295e:	4d 85 ff             	test   %r15,%r15
  202961:	0f 84 7a 07 00 00    	je     2030e1 <convS2M+0xc91>
  202967:	4c 89 ff             	mov    %r15,%rdi
  20296a:	e8 f1 e9 ff ff       	call   201360 <strlen>
  20296f:	4c 89 fe             	mov    %r15,%rsi
  202972:	48 89 df             	mov    %rbx,%rdi
  202975:	49 89 c6             	mov    %rax,%r14
  202978:	89 c2                	mov    %eax,%edx
  20297a:	e8 41 e9 ff ff       	call   2012c0 <memmove>
  20297f:	41 8d 46 02          	lea    0x2(%r14),%eax
  202983:	66 45 89 75 04       	mov    %r14w,0x4(%r13)
  202988:	49 8d 44 05 04       	lea    0x4(%r13,%rax,1),%rax
  20298d:	48 29 e8             	sub    %rbp,%rax
  202990:	e9 40 fb ff ff       	jmp    2024d5 <convS2M+0x85>
  202995:	8b 43 14             	mov    0x14(%rbx),%eax
  202998:	88 45 07             	mov    %al,0x7(%rbp)
  20299b:	8b 43 14             	mov    0x14(%rbx),%eax
  20299e:	88 65 08             	mov    %ah,0x8(%rbp)
  2029a1:	0f b7 43 16          	movzwl 0x16(%rbx),%eax
  2029a5:	88 45 09             	mov    %al,0x9(%rbp)
  2029a8:	0f b6 43 17          	movzbl 0x17(%rbx),%eax
  2029ac:	88 45 0a             	mov    %al,0xa(%rbp)
  2029af:	4c 8b 6b 10          	mov    0x10(%rbx),%r13
  2029b3:	4d 85 ed             	test   %r13,%r13
  2029b6:	0f 85 85 fe ff ff    	jne    202841 <convS2M+0x3f1>
  2029bc:	31 d2                	xor    %edx,%edx
  2029be:	b8 0d 00 00 00       	mov    $0xd,%eax
  2029c3:	66 89 55 0b          	mov    %dx,0xb(%rbp)
  2029c7:	e9 09 fb ff ff       	jmp    2024d5 <convS2M+0x85>
  2029cc:	48 8b 73 18          	mov    0x18(%rbx),%rsi
  2029d0:	4c 8d 6d 09          	lea    0x9(%rbp),%r13
  2029d4:	48 85 f6             	test   %rsi,%rsi
  2029d7:	0f 84 55 07 00 00    	je     203132 <convS2M+0xce2>
  2029dd:	48 89 f7             	mov    %rsi,%rdi
  2029e0:	48 89 34 24          	mov    %rsi,(%rsp)
  2029e4:	e8 77 e9 ff ff       	call   201360 <strlen>
  2029e9:	48 8b 34 24          	mov    (%rsp),%rsi
  2029ed:	4c 89 ef             	mov    %r13,%rdi
  2029f0:	49 89 c7             	mov    %rax,%r15
  2029f3:	89 c2                	mov    %eax,%edx
  2029f5:	e8 c6 e8 ff ff       	call   2012c0 <memmove>
  2029fa:	45 8d 6f 02          	lea    0x2(%r15),%r13d
  2029fe:	4c 89 f8             	mov    %r15,%rax
  202a01:	44 89 fa             	mov    %r15d,%edx
  202a04:	0f b6 c4             	movzbl %ah,%eax
  202a07:	4d 01 f5             	add    %r14,%r13
  202a0a:	88 55 07             	mov    %dl,0x7(%rbp)
  202a0d:	88 45 08             	mov    %al,0x8(%rbp)
  202a10:	4c 8b 7b 10          	mov    0x10(%rbx),%r15
  202a14:	49 8d 5d 02          	lea    0x2(%r13),%rbx
  202a18:	4d 85 ff             	test   %r15,%r15
  202a1b:	0f 84 cd 02 00 00    	je     202cee <convS2M+0x89e>
  202a21:	4c 89 ff             	mov    %r15,%rdi
  202a24:	e8 37 e9 ff ff       	call   201360 <strlen>
  202a29:	4c 89 fe             	mov    %r15,%rsi
  202a2c:	48 89 df             	mov    %rbx,%rdi
  202a2f:	49 89 c6             	mov    %rax,%r14
  202a32:	89 c2                	mov    %eax,%edx
  202a34:	e8 87 e8 ff ff       	call   2012c0 <memmove>
  202a39:	41 8d 46 02          	lea    0x2(%r14),%eax
  202a3d:	66 45 89 75 00       	mov    %r14w,0x0(%r13)
  202a42:	4c 01 e8             	add    %r13,%rax
  202a45:	48 29 e8             	sub    %rbp,%rax
  202a48:	e9 88 fa ff ff       	jmp    2024d5 <convS2M+0x85>
  202a4d:	8b 43 04             	mov    0x4(%rbx),%eax
  202a50:	4c 8d 6d 0d          	lea    0xd(%rbp),%r13
  202a54:	88 45 07             	mov    %al,0x7(%rbp)
  202a57:	8b 43 04             	mov    0x4(%rbx),%eax
  202a5a:	88 65 08             	mov    %ah,0x8(%rbp)
  202a5d:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  202a61:	88 45 09             	mov    %al,0x9(%rbp)
  202a64:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  202a68:	88 45 0a             	mov    %al,0xa(%rbp)
  202a6b:	4c 8b 7b 18          	mov    0x18(%rbx),%r15
  202a6f:	4d 85 ff             	test   %r15,%r15
  202a72:	0f 84 a8 06 00 00    	je     203120 <convS2M+0xcd0>
  202a78:	4c 89 ff             	mov    %r15,%rdi
  202a7b:	e8 e0 e8 ff ff       	call   201360 <strlen>
  202a80:	4c 89 ef             	mov    %r13,%rdi
  202a83:	4c 89 fe             	mov    %r15,%rsi
  202a86:	49 89 c6             	mov    %rax,%r14
  202a89:	89 c2                	mov    %eax,%edx
  202a8b:	e8 30 e8 ff ff       	call   2012c0 <memmove>
  202a90:	4c 89 f0             	mov    %r14,%rax
  202a93:	41 8d 4e 02          	lea    0x2(%r14),%ecx
  202a97:	44 89 f2             	mov    %r14d,%edx
  202a9a:	0f b6 c4             	movzbl %ah,%eax
  202a9d:	4c 8d 6c 0d 0b       	lea    0xb(%rbp,%rcx,1),%r13
  202aa2:	88 55 0b             	mov    %dl,0xb(%rbp)
  202aa5:	88 45 0c             	mov    %al,0xc(%rbp)
  202aa8:	0f b6 43 20          	movzbl 0x20(%rbx),%eax
  202aac:	41 88 45 00          	mov    %al,0x0(%r13)
  202ab0:	49 8d 45 01          	lea    0x1(%r13),%rax
  202ab4:	48 29 e8             	sub    %rbp,%rax
  202ab7:	e9 19 fa ff ff       	jmp    2024d5 <convS2M+0x85>
  202abc:	8b 43 04             	mov    0x4(%rbx),%eax
  202abf:	88 45 07             	mov    %al,0x7(%rbp)
  202ac2:	8b 43 04             	mov    0x4(%rbx),%eax
  202ac5:	88 65 08             	mov    %ah,0x8(%rbp)
  202ac8:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  202acc:	88 45 09             	mov    %al,0x9(%rbp)
  202acf:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  202ad3:	88 45 0a             	mov    %al,0xa(%rbp)
  202ad6:	0f b6 43 20          	movzbl 0x20(%rbx),%eax
  202ada:	88 45 0b             	mov    %al,0xb(%rbp)
  202add:	b8 0c 00 00 00       	mov    $0xc,%eax
  202ae2:	e9 ee f9 ff ff       	jmp    2024d5 <convS2M+0x85>
  202ae7:	8b 43 10             	mov    0x10(%rbx),%eax
  202aea:	88 45 07             	mov    %al,0x7(%rbp)
  202aed:	8b 43 10             	mov    0x10(%rbx),%eax
  202af0:	88 65 08             	mov    %ah,0x8(%rbp)
  202af3:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  202af7:	88 45 09             	mov    %al,0x9(%rbp)
  202afa:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202afe:	e9 f4 f9 ff ff       	jmp    2024f7 <convS2M+0xa7>
  202b03:	8b 43 10             	mov    0x10(%rbx),%eax
  202b06:	88 45 07             	mov    %al,0x7(%rbp)
  202b09:	8b 43 10             	mov    0x10(%rbx),%eax
  202b0c:	88 65 08             	mov    %ah,0x8(%rbp)
  202b0f:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  202b13:	88 45 09             	mov    %al,0x9(%rbp)
  202b16:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202b1a:	88 45 0a             	mov    %al,0xa(%rbp)
  202b1d:	8b 43 14             	mov    0x14(%rbx),%eax
  202b20:	88 45 0b             	mov    %al,0xb(%rbp)
  202b23:	8b 43 14             	mov    0x14(%rbx),%eax
  202b26:	88 65 0c             	mov    %ah,0xc(%rbp)
  202b29:	0f b7 43 16          	movzwl 0x16(%rbx),%eax
  202b2d:	88 45 0d             	mov    %al,0xd(%rbp)
  202b30:	0f b6 43 17          	movzbl 0x17(%rbx),%eax
  202b34:	e9 67 fa ff ff       	jmp    2025a0 <convS2M+0x150>
  202b39:	8b 43 04             	mov    0x4(%rbx),%eax
  202b3c:	88 45 07             	mov    %al,0x7(%rbp)
  202b3f:	8b 43 04             	mov    0x4(%rbx),%eax
  202b42:	88 65 08             	mov    %ah,0x8(%rbp)
  202b45:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  202b49:	88 45 09             	mov    %al,0x9(%rbp)
  202b4c:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  202b50:	88 45 0a             	mov    %al,0xa(%rbp)
  202b53:	8b 43 10             	mov    0x10(%rbx),%eax
  202b56:	88 45 0b             	mov    %al,0xb(%rbp)
  202b59:	8b 43 10             	mov    0x10(%rbx),%eax
  202b5c:	88 65 0c             	mov    %ah,0xc(%rbp)
  202b5f:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  202b63:	88 45 0d             	mov    %al,0xd(%rbp)
  202b66:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202b6a:	e9 31 fa ff ff       	jmp    2025a0 <convS2M+0x150>
  202b6f:	48 8b 73 18          	mov    0x18(%rbx),%rsi
  202b73:	4c 8d 7d 09          	lea    0x9(%rbp),%r15
  202b77:	48 85 f6             	test   %rsi,%rsi
  202b7a:	0f 84 85 05 00 00    	je     203105 <convS2M+0xcb5>
  202b80:	48 89 f7             	mov    %rsi,%rdi
  202b83:	48 89 34 24          	mov    %rsi,(%rsp)
  202b87:	e8 d4 e7 ff ff       	call   201360 <strlen>
  202b8c:	48 8b 34 24          	mov    (%rsp),%rsi
  202b90:	4c 89 ff             	mov    %r15,%rdi
  202b93:	49 89 c5             	mov    %rax,%r13
  202b96:	89 c2                	mov    %eax,%edx
  202b98:	e8 23 e7 ff ff       	call   2012c0 <memmove>
  202b9d:	45 8d 7d 02          	lea    0x2(%r13),%r15d
  202ba1:	4c 89 e8             	mov    %r13,%rax
  202ba4:	44 89 ea             	mov    %r13d,%edx
  202ba7:	0f b6 c4             	movzbl %ah,%eax
  202baa:	4d 01 f7             	add    %r14,%r15
  202bad:	88 55 07             	mov    %dl,0x7(%rbp)
  202bb0:	4d 8d 6f 02          	lea    0x2(%r15),%r13
  202bb4:	88 45 08             	mov    %al,0x8(%rbp)
  202bb7:	48 8b 73 10          	mov    0x10(%rbx),%rsi
  202bbb:	48 85 f6             	test   %rsi,%rsi
  202bbe:	0f 84 38 05 00 00    	je     2030fc <convS2M+0xcac>
  202bc4:	48 89 f7             	mov    %rsi,%rdi
  202bc7:	48 89 34 24          	mov    %rsi,(%rsp)
  202bcb:	e8 90 e7 ff ff       	call   201360 <strlen>
  202bd0:	48 8b 34 24          	mov    (%rsp),%rsi
  202bd4:	4c 89 ef             	mov    %r13,%rdi
  202bd7:	49 89 c6             	mov    %rax,%r14
  202bda:	89 c2                	mov    %eax,%edx
  202bdc:	e8 df e6 ff ff       	call   2012c0 <memmove>
  202be1:	45 8d 6e 02          	lea    0x2(%r14),%r13d
  202be5:	4c 89 f0             	mov    %r14,%rax
  202be8:	44 89 f2             	mov    %r14d,%edx
  202beb:	0f b6 c4             	movzbl %ah,%eax
  202bee:	4d 01 fd             	add    %r15,%r13
  202bf1:	41 88 17             	mov    %dl,(%r15)
  202bf4:	41 88 47 01          	mov    %al,0x1(%r15)
  202bf8:	8b 43 10             	mov    0x10(%rbx),%eax
  202bfb:	41 88 45 00          	mov    %al,0x0(%r13)
  202bff:	8b 43 10             	mov    0x10(%rbx),%eax
  202c02:	0f b6 c4             	movzbl %ah,%eax
  202c05:	41 88 45 01          	mov    %al,0x1(%r13)
  202c09:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  202c0d:	41 88 45 02          	mov    %al,0x2(%r13)
  202c11:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202c15:	41 88 45 03          	mov    %al,0x3(%r13)
  202c19:	49 8d 45 04          	lea    0x4(%r13),%rax
  202c1d:	48 29 e8             	sub    %rbp,%rax
  202c20:	e9 b0 f8 ff ff       	jmp    2024d5 <convS2M+0x85>
  202c25:	48 8b 73 18          	mov    0x18(%rbx),%rsi
  202c29:	4c 8d 6d 09          	lea    0x9(%rbp),%r13
  202c2d:	48 85 f6             	test   %rsi,%rsi
  202c30:	0f 84 f3 04 00 00    	je     203129 <convS2M+0xcd9>
  202c36:	48 89 f7             	mov    %rsi,%rdi
  202c39:	48 89 34 24          	mov    %rsi,(%rsp)
  202c3d:	e8 1e e7 ff ff       	call   201360 <strlen>
  202c42:	48 8b 34 24          	mov    (%rsp),%rsi
  202c46:	4c 89 ef             	mov    %r13,%rdi
  202c49:	49 89 c7             	mov    %rax,%r15
  202c4c:	89 c2                	mov    %eax,%edx
  202c4e:	e8 6d e6 ff ff       	call   2012c0 <memmove>
  202c53:	45 8d 6f 02          	lea    0x2(%r15),%r13d
  202c57:	4c 89 f8             	mov    %r15,%rax
  202c5a:	44 89 fa             	mov    %r15d,%edx
  202c5d:	0f b6 c4             	movzbl %ah,%eax
  202c60:	4d 01 f5             	add    %r14,%r13
  202c63:	88 55 07             	mov    %dl,0x7(%rbp)
  202c66:	49 83 c5 02          	add    $0x2,%r13
  202c6a:	88 45 08             	mov    %al,0x8(%rbp)
  202c6d:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  202c71:	41 88 45 fe          	mov    %al,-0x2(%r13)
  202c75:	0f b6 43 11          	movzbl 0x11(%rbx),%eax
  202c79:	41 88 45 ff          	mov    %al,-0x1(%r13)
  202c7d:	e9 90 fa ff ff       	jmp    202712 <convS2M+0x2c2>
  202c82:	8b 43 10             	mov    0x10(%rbx),%eax
  202c85:	4c 8d 6d 0d          	lea    0xd(%rbp),%r13
  202c89:	88 45 07             	mov    %al,0x7(%rbp)
  202c8c:	8b 43 10             	mov    0x10(%rbx),%eax
  202c8f:	88 65 08             	mov    %ah,0x8(%rbp)
  202c92:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  202c96:	88 45 09             	mov    %al,0x9(%rbp)
  202c99:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202c9d:	88 45 0a             	mov    %al,0xa(%rbp)
  202ca0:	4c 8b 7b 18          	mov    0x18(%rbx),%r15
  202ca4:	4d 85 ff             	test   %r15,%r15
  202ca7:	0f 84 2b 04 00 00    	je     2030d8 <convS2M+0xc88>
  202cad:	4c 89 ff             	mov    %r15,%rdi
  202cb0:	e8 ab e6 ff ff       	call   201360 <strlen>
  202cb5:	4c 89 ef             	mov    %r13,%rdi
  202cb8:	4c 89 fe             	mov    %r15,%rsi
  202cbb:	49 89 c6             	mov    %rax,%r14
  202cbe:	89 c2                	mov    %eax,%edx
  202cc0:	e8 fb e5 ff ff       	call   2012c0 <memmove>
  202cc5:	4c 89 f0             	mov    %r14,%rax
  202cc8:	41 8d 4e 02          	lea    0x2(%r14),%ecx
  202ccc:	44 89 f2             	mov    %r14d,%edx
  202ccf:	0f b6 c4             	movzbl %ah,%eax
  202cd2:	4c 8d 6c 0d 0b       	lea    0xb(%rbp,%rcx,1),%r13
  202cd7:	88 55 0b             	mov    %dl,0xb(%rbp)
  202cda:	88 45 0c             	mov    %al,0xc(%rbp)
  202cdd:	4c 8b 7b 20          	mov    0x20(%rbx),%r15
  202ce1:	49 8d 5d 02          	lea    0x2(%r13),%rbx
  202ce5:	4d 85 ff             	test   %r15,%r15
  202ce8:	0f 85 33 fd ff ff    	jne    202a21 <convS2M+0x5d1>
  202cee:	31 c9                	xor    %ecx,%ecx
  202cf0:	48 89 d8             	mov    %rbx,%rax
  202cf3:	66 41 89 4d 00       	mov    %cx,0x0(%r13)
  202cf8:	48 29 e8             	sub    %rbp,%rax
  202cfb:	e9 d5 f7 ff ff       	jmp    2024d5 <convS2M+0x85>
  202d00:	8b 43 04             	mov    0x4(%rbx),%eax
  202d03:	88 45 07             	mov    %al,0x7(%rbp)
  202d06:	8b 43 04             	mov    0x4(%rbx),%eax
  202d09:	88 65 08             	mov    %ah,0x8(%rbp)
  202d0c:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  202d10:	88 45 09             	mov    %al,0x9(%rbp)
  202d13:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  202d17:	88 45 0a             	mov    %al,0xa(%rbp)
  202d1a:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202d1e:	88 45 0b             	mov    %al,0xb(%rbp)
  202d21:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202d25:	88 65 0c             	mov    %ah,0xc(%rbp)
  202d28:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202d2c:	48 c1 f8 10          	sar    $0x10,%rax
  202d30:	88 45 0d             	mov    %al,0xd(%rbp)
  202d33:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202d37:	48 c1 f8 18          	sar    $0x18,%rax
  202d3b:	88 45 0e             	mov    %al,0xe(%rbp)
  202d3e:	48 63 43 14          	movslq 0x14(%rbx),%rax
  202d42:	88 45 0f             	mov    %al,0xf(%rbp)
  202d45:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202d49:	48 c1 f8 28          	sar    $0x28,%rax
  202d4d:	88 45 10             	mov    %al,0x10(%rbp)
  202d50:	48 0f bf 43 16       	movswq 0x16(%rbx),%rax
  202d55:	88 45 11             	mov    %al,0x11(%rbp)
  202d58:	48 0f be 43 17       	movsbq 0x17(%rbx),%rax
  202d5d:	88 45 12             	mov    %al,0x12(%rbp)
  202d60:	8b 43 10             	mov    0x10(%rbx),%eax
  202d63:	88 45 13             	mov    %al,0x13(%rbp)
  202d66:	8b 43 10             	mov    0x10(%rbx),%eax
  202d69:	88 65 14             	mov    %ah,0x14(%rbp)
  202d6c:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  202d70:	88 45 15             	mov    %al,0x15(%rbp)
  202d73:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202d77:	e9 ab f8 ff ff       	jmp    202627 <convS2M+0x1d7>
  202d7c:	8b 43 04             	mov    0x4(%rbx),%eax
  202d7f:	88 45 07             	mov    %al,0x7(%rbp)
  202d82:	8b 43 04             	mov    0x4(%rbx),%eax
  202d85:	88 65 08             	mov    %ah,0x8(%rbp)
  202d88:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  202d8c:	88 45 09             	mov    %al,0x9(%rbp)
  202d8f:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  202d93:	88 45 0a             	mov    %al,0xa(%rbp)
  202d96:	8b 43 10             	mov    0x10(%rbx),%eax
  202d99:	88 45 0b             	mov    %al,0xb(%rbp)
  202d9c:	8b 43 10             	mov    0x10(%rbx),%eax
  202d9f:	88 65 0c             	mov    %ah,0xc(%rbp)
  202da2:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  202da6:	88 45 0d             	mov    %al,0xd(%rbp)
  202da9:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202dad:	88 45 0e             	mov    %al,0xe(%rbp)
  202db0:	0f b7 43 14          	movzwl 0x14(%rbx),%eax
  202db4:	88 45 0f             	mov    %al,0xf(%rbp)
  202db7:	0f b6 43 15          	movzbl 0x15(%rbx),%eax
  202dbb:	88 45 10             	mov    %al,0x10(%rbp)
  202dbe:	0f b7 43 14          	movzwl 0x14(%rbx),%eax
  202dc2:	66 83 f8 10          	cmp    $0x10,%ax
  202dc6:	0f 87 af f6 ff ff    	ja     20247b <convS2M+0x2b>
  202dcc:	4c 8d 7d 11          	lea    0x11(%rbp),%r15
  202dd0:	66 85 c0             	test   %ax,%ax
  202dd3:	0f 84 62 03 00 00    	je     20313b <convS2M+0xceb>
  202dd9:	31 c9                	xor    %ecx,%ecx
  202ddb:	eb 52                	jmp    202e2f <convS2M+0x9df>
  202ddd:	0f 1f 00             	nopl   (%rax)
  202de0:	48 89 f7             	mov    %rsi,%rdi
  202de3:	89 4c 24 0c          	mov    %ecx,0xc(%rsp)
  202de7:	48 89 34 24          	mov    %rsi,(%rsp)
  202deb:	e8 70 e5 ff ff       	call   201360 <strlen>
  202df0:	48 8b 34 24          	mov    (%rsp),%rsi
  202df4:	4c 89 ef             	mov    %r13,%rdi
  202df7:	49 89 c6             	mov    %rax,%r14
  202dfa:	89 c2                	mov    %eax,%edx
  202dfc:	e8 bf e4 ff ff       	call   2012c0 <memmove>
  202e01:	4c 89 f0             	mov    %r14,%rax
  202e04:	8b 4c 24 0c          	mov    0xc(%rsp),%ecx
  202e08:	44 89 f6             	mov    %r14d,%esi
  202e0b:	0f b6 d4             	movzbl %ah,%edx
  202e0e:	41 8d 46 02          	lea    0x2(%r14),%eax
  202e12:	4d 8d 2c 07          	lea    (%r15,%rax,1),%r13
  202e16:	41 88 37             	mov    %sil,(%r15)
  202e19:	83 c1 01             	add    $0x1,%ecx
  202e1c:	41 88 57 01          	mov    %dl,0x1(%r15)
  202e20:	0f b7 43 14          	movzwl 0x14(%rbx),%eax
  202e24:	39 c1                	cmp    %eax,%ecx
  202e26:	0f 83 a1 02 00 00    	jae    2030cd <convS2M+0xc7d>
  202e2c:	4d 89 ef             	mov    %r13,%r15
  202e2f:	89 c8                	mov    %ecx,%eax
  202e31:	4d 8d 6f 02          	lea    0x2(%r15),%r13
  202e35:	48 8b 74 c3 18       	mov    0x18(%rbx,%rax,8),%rsi
  202e3a:	48 85 f6             	test   %rsi,%rsi
  202e3d:	75 a1                	jne    202de0 <convS2M+0x990>
  202e3f:	31 f6                	xor    %esi,%esi
  202e41:	31 d2                	xor    %edx,%edx
  202e43:	eb d1                	jmp    202e16 <convS2M+0x9c6>
  202e45:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  202e49:	88 45 07             	mov    %al,0x7(%rbp)
  202e4c:	0f b6 43 11          	movzbl 0x11(%rbx),%eax
  202e50:	88 45 08             	mov    %al,0x8(%rbp)
  202e53:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  202e57:	66 83 f8 10          	cmp    $0x10,%ax
  202e5b:	0f 87 1a f6 ff ff    	ja     20247b <convS2M+0x2b>
  202e61:	48 8d 7d 09          	lea    0x9(%rbp),%rdi
  202e65:	66 85 c0             	test   %ax,%ax
  202e68:	0f 84 12 f9 ff ff    	je     202780 <convS2M+0x330>
  202e6e:	48 8d 73 18          	lea    0x18(%rbx),%rsi
  202e72:	31 c9                	xor    %ecx,%ecx
  202e74:	0f 1f 40 00          	nopl   0x0(%rax)
  202e78:	e8 d3 f1 ff ff       	call   202050 <pqid>
  202e7d:	83 c1 01             	add    $0x1,%ecx
  202e80:	48 83 c6 18          	add    $0x18,%rsi
  202e84:	48 89 c7             	mov    %rax,%rdi
  202e87:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  202e8b:	39 c1                	cmp    %eax,%ecx
  202e8d:	72 e9                	jb     202e78 <convS2M+0xa28>
  202e8f:	48 89 f8             	mov    %rdi,%rax
  202e92:	48 29 e8             	sub    %rbp,%rax
  202e95:	e9 3b f6 ff ff       	jmp    2024d5 <convS2M+0x85>
  202e9a:	0f b7 43 10          	movzwl 0x10(%rbx),%eax
  202e9e:	88 45 07             	mov    %al,0x7(%rbp)
  202ea1:	0f b6 43 11          	movzbl 0x11(%rbx),%eax
  202ea5:	88 45 08             	mov    %al,0x8(%rbp)
  202ea8:	e9 d3 f8 ff ff       	jmp    202780 <convS2M+0x330>
  202ead:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202eb1:	88 45 07             	mov    %al,0x7(%rbp)
  202eb4:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202eb8:	88 65 08             	mov    %ah,0x8(%rbp)
  202ebb:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202ebf:	48 c1 f8 10          	sar    $0x10,%rax
  202ec3:	88 45 09             	mov    %al,0x9(%rbp)
  202ec6:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202eca:	48 c1 f8 18          	sar    $0x18,%rax
  202ece:	88 45 0a             	mov    %al,0xa(%rbp)
  202ed1:	48 63 43 14          	movslq 0x14(%rbx),%rax
  202ed5:	88 45 0b             	mov    %al,0xb(%rbp)
  202ed8:	48 8b 43 10          	mov    0x10(%rbx),%rax
  202edc:	48 c1 f8 28          	sar    $0x28,%rax
  202ee0:	88 45 0c             	mov    %al,0xc(%rbp)
  202ee3:	48 0f bf 43 16       	movswq 0x16(%rbx),%rax
  202ee8:	88 45 0d             	mov    %al,0xd(%rbp)
  202eeb:	0f b6 43 17          	movzbl 0x17(%rbx),%eax
  202eef:	e9 ac f6 ff ff       	jmp    2025a0 <convS2M+0x150>
  202ef4:	48 8b 73 18          	mov    0x18(%rbx),%rsi
  202ef8:	4c 8d 6d 09          	lea    0x9(%rbp),%r13
  202efc:	48 85 f6             	test   %rsi,%rsi
  202eff:	0f 84 09 02 00 00    	je     20310e <convS2M+0xcbe>
  202f05:	48 89 f7             	mov    %rsi,%rdi
  202f08:	48 89 34 24          	mov    %rsi,(%rsp)
  202f0c:	e8 4f e4 ff ff       	call   201360 <strlen>
  202f11:	48 8b 34 24          	mov    (%rsp),%rsi
  202f15:	4c 89 ef             	mov    %r13,%rdi
  202f18:	49 89 c7             	mov    %rax,%r15
  202f1b:	89 c2                	mov    %eax,%edx
  202f1d:	e8 9e e3 ff ff       	call   2012c0 <memmove>
  202f22:	45 8d 6f 02          	lea    0x2(%r15),%r13d
  202f26:	4c 89 f8             	mov    %r15,%rax
  202f29:	44 89 fa             	mov    %r15d,%edx
  202f2c:	0f b6 c4             	movzbl %ah,%eax
  202f2f:	4d 01 f5             	add    %r14,%r13
  202f32:	88 55 07             	mov    %dl,0x7(%rbp)
  202f35:	88 45 08             	mov    %al,0x8(%rbp)
  202f38:	8b 43 18             	mov    0x18(%rbx),%eax
  202f3b:	41 88 45 00          	mov    %al,0x0(%r13)
  202f3f:	8b 43 18             	mov    0x18(%rbx),%eax
  202f42:	0f b6 c4             	movzbl %ah,%eax
  202f45:	41 88 45 01          	mov    %al,0x1(%r13)
  202f49:	0f b7 43 1a          	movzwl 0x1a(%rbx),%eax
  202f4d:	41 88 45 02          	mov    %al,0x2(%r13)
  202f51:	0f b6 43 1b          	movzbl 0x1b(%rbx),%eax
  202f55:	41 88 45 03          	mov    %al,0x3(%r13)
  202f59:	49 8d 45 04          	lea    0x4(%r13),%rax
  202f5d:	48 29 e8             	sub    %rbp,%rax
  202f60:	e9 70 f5 ff ff       	jmp    2024d5 <convS2M+0x85>
  202f65:	8b 43 04             	mov    0x4(%rbx),%eax
  202f68:	4c 8d 6d 11          	lea    0x11(%rbp),%r13
  202f6c:	88 45 07             	mov    %al,0x7(%rbp)
  202f6f:	8b 43 04             	mov    0x4(%rbx),%eax
  202f72:	88 65 08             	mov    %ah,0x8(%rbp)
  202f75:	0f b7 43 06          	movzwl 0x6(%rbx),%eax
  202f79:	88 45 09             	mov    %al,0x9(%rbp)
  202f7c:	0f b6 43 07          	movzbl 0x7(%rbx),%eax
  202f80:	88 45 0a             	mov    %al,0xa(%rbp)
  202f83:	8b 43 10             	mov    0x10(%rbx),%eax
  202f86:	88 45 0b             	mov    %al,0xb(%rbp)
  202f89:	8b 43 10             	mov    0x10(%rbx),%eax
  202f8c:	88 65 0c             	mov    %ah,0xc(%rbp)
  202f8f:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  202f93:	88 45 0d             	mov    %al,0xd(%rbp)
  202f96:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  202f9a:	88 45 0e             	mov    %al,0xe(%rbp)
  202f9d:	4c 8b 7b 18          	mov    0x18(%rbx),%r15
  202fa1:	4d 85 ff             	test   %r15,%r15
  202fa4:	0f 84 6d 01 00 00    	je     203117 <convS2M+0xcc7>
  202faa:	4c 89 ff             	mov    %r15,%rdi
  202fad:	e8 ae e3 ff ff       	call   201360 <strlen>
  202fb2:	4c 89 ef             	mov    %r13,%rdi
  202fb5:	4c 89 fe             	mov    %r15,%rsi
  202fb8:	49 89 c6             	mov    %rax,%r14
  202fbb:	89 c2                	mov    %eax,%edx
  202fbd:	e8 fe e2 ff ff       	call   2012c0 <memmove>
  202fc2:	4c 89 f0             	mov    %r14,%rax
  202fc5:	41 8d 4e 02          	lea    0x2(%r14),%ecx
  202fc9:	44 89 f2             	mov    %r14d,%edx
  202fcc:	0f b6 c4             	movzbl %ah,%eax
  202fcf:	4c 8d 6c 0d 0f       	lea    0xf(%rbp,%rcx,1),%r13
  202fd4:	88 55 0f             	mov    %dl,0xf(%rbp)
  202fd7:	88 45 10             	mov    %al,0x10(%rbp)
  202fda:	4c 8b 7b 20          	mov    0x20(%rbx),%r15
  202fde:	49 8d 5d 02          	lea    0x2(%r13),%rbx
  202fe2:	4d 85 ff             	test   %r15,%r15
  202fe5:	0f 85 36 fa ff ff    	jne    202a21 <convS2M+0x5d1>
  202feb:	e9 fe fc ff ff       	jmp    202cee <convS2M+0x89e>
  202ff0:	8b 43 10             	mov    0x10(%rbx),%eax
  202ff3:	88 45 07             	mov    %al,0x7(%rbp)
  202ff6:	8b 43 10             	mov    0x10(%rbx),%eax
  202ff9:	88 65 08             	mov    %ah,0x8(%rbp)
  202ffc:	0f b7 43 12          	movzwl 0x12(%rbx),%eax
  203000:	88 45 09             	mov    %al,0x9(%rbp)
  203003:	0f b6 43 13          	movzbl 0x13(%rbx),%eax
  203007:	88 45 0a             	mov    %al,0xa(%rbp)
  20300a:	8b 43 14             	mov    0x14(%rbx),%eax
  20300d:	88 45 0b             	mov    %al,0xb(%rbp)
  203010:	8b 43 14             	mov    0x14(%rbx),%eax
  203013:	88 65 0c             	mov    %ah,0xc(%rbp)
  203016:	0f b7 43 16          	movzwl 0x16(%rbx),%eax
  20301a:	88 45 0d             	mov    %al,0xd(%rbp)
  20301d:	0f b6 43 17          	movzbl 0x17(%rbx),%eax
  203021:	88 45 0e             	mov    %al,0xe(%rbp)
  203024:	8b 43 20             	mov    0x20(%rbx),%eax
  203027:	4c 8d 6d 13          	lea    0x13(%rbp),%r13
  20302b:	88 45 0f             	mov    %al,0xf(%rbp)
  20302e:	8b 43 20             	mov    0x20(%rbx),%eax
  203031:	88 65 10             	mov    %ah,0x10(%rbp)
  203034:	0f b7 43 22          	movzwl 0x22(%rbx),%eax
  203038:	88 45 11             	mov    %al,0x11(%rbp)
  20303b:	0f b6 43 23          	movzbl 0x23(%rbx),%eax
  20303f:	88 45 12             	mov    %al,0x12(%rbp)
  203042:	8b 43 20             	mov    0x20(%rbx),%eax
  203045:	85 c0                	test   %eax,%eax
  203047:	0f 84 1b f6 ff ff    	je     202668 <convS2M+0x218>
  20304d:	48 8b 73 18          	mov    0x18(%rbx),%rsi
  203051:	89 c2                	mov    %eax,%edx
  203053:	4c 89 ef             	mov    %r13,%rdi
  203056:	e8 65 e2 ff ff       	call   2012c0 <memmove>
  20305b:	8b 43 20             	mov    0x20(%rbx),%eax
  20305e:	e9 05 f6 ff ff       	jmp    202668 <convS2M+0x218>
  203063:	8b 43 14             	mov    0x14(%rbx),%eax
  203066:	88 45 07             	mov    %al,0x7(%rbp)
  203069:	8b 43 14             	mov    0x14(%rbx),%eax
  20306c:	88 65 08             	mov    %ah,0x8(%rbp)
  20306f:	0f b7 43 16          	movzwl 0x16(%rbx),%eax
  203073:	88 45 09             	mov    %al,0x9(%rbp)
  203076:	0f b6 43 17          	movzbl 0x17(%rbx),%eax
  20307a:	e9 78 f4 ff ff       	jmp    2024f7 <convS2M+0xa7>
  20307f:	48 8b 43 28          	mov    0x28(%rbx),%rax
  203083:	88 45 07             	mov    %al,0x7(%rbp)
  203086:	48 8b 43 28          	mov    0x28(%rbx),%rax
  20308a:	88 65 08             	mov    %ah,0x8(%rbp)
  20308d:	48 8b 43 28          	mov    0x28(%rbx),%rax
  203091:	48 c1 e8 10          	shr    $0x10,%rax
  203095:	88 45 09             	mov    %al,0x9(%rbp)
  203098:	48 8b 43 28          	mov    0x28(%rbx),%rax
  20309c:	48 c1 e8 18          	shr    $0x18,%rax
  2030a0:	88 45 0a             	mov    %al,0xa(%rbp)
  2030a3:	8b 43 2c             	mov    0x2c(%rbx),%eax
  2030a6:	88 45 0b             	mov    %al,0xb(%rbp)
  2030a9:	48 8b 43 28          	mov    0x28(%rbx),%rax
  2030ad:	48 c1 e8 28          	shr    $0x28,%rax
  2030b1:	88 45 0c             	mov    %al,0xc(%rbp)
  2030b4:	0f b7 43 2e          	movzwl 0x2e(%rbx),%eax
  2030b8:	88 45 0d             	mov    %al,0xd(%rbp)
  2030bb:	0f b6 43 2f          	movzbl 0x2f(%rbx),%eax
  2030bf:	e9 5d ff ff ff       	jmp    203021 <convS2M+0xbd1>
  2030c4:	31 d2                	xor    %edx,%edx
  2030c6:	31 c0                	xor    %eax,%eax
  2030c8:	e9 12 f7 ff ff       	jmp    2027df <convS2M+0x38f>
  2030cd:	4c 89 e8             	mov    %r13,%rax
  2030d0:	48 29 e8             	sub    %rbp,%rax
  2030d3:	e9 fd f3 ff ff       	jmp    2024d5 <convS2M+0x85>
  2030d8:	31 d2                	xor    %edx,%edx
  2030da:	31 c0                	xor    %eax,%eax
  2030dc:	e9 f6 fb ff ff       	jmp    202cd7 <convS2M+0x887>
  2030e1:	31 f6                	xor    %esi,%esi
  2030e3:	48 89 d8             	mov    %rbx,%rax
  2030e6:	66 41 89 75 04       	mov    %si,0x4(%r13)
  2030eb:	48 29 e8             	sub    %rbp,%rax
  2030ee:	e9 e2 f3 ff ff       	jmp    2024d5 <convS2M+0x85>
  2030f3:	31 d2                	xor    %edx,%edx
  2030f5:	31 c0                	xor    %eax,%eax
  2030f7:	e9 33 f8 ff ff       	jmp    20292f <convS2M+0x4df>
  2030fc:	31 d2                	xor    %edx,%edx
  2030fe:	31 c0                	xor    %eax,%eax
  203100:	e9 ec fa ff ff       	jmp    202bf1 <convS2M+0x7a1>
  203105:	31 d2                	xor    %edx,%edx
  203107:	31 c0                	xor    %eax,%eax
  203109:	e9 9f fa ff ff       	jmp    202bad <convS2M+0x75d>
  20310e:	31 d2                	xor    %edx,%edx
  203110:	31 c0                	xor    %eax,%eax
  203112:	e9 1b fe ff ff       	jmp    202f32 <convS2M+0xae2>
  203117:	31 d2                	xor    %edx,%edx
  203119:	31 c0                	xor    %eax,%eax
  20311b:	e9 b4 fe ff ff       	jmp    202fd4 <convS2M+0xb84>
  203120:	31 d2                	xor    %edx,%edx
  203122:	31 c0                	xor    %eax,%eax
  203124:	e9 79 f9 ff ff       	jmp    202aa2 <convS2M+0x652>
  203129:	31 d2                	xor    %edx,%edx
  20312b:	31 c0                	xor    %eax,%eax
  20312d:	e9 31 fb ff ff       	jmp    202c63 <convS2M+0x813>
  203132:	31 d2                	xor    %edx,%edx
  203134:	31 c0                	xor    %eax,%eax
  203136:	e9 cf f8 ff ff       	jmp    202a0a <convS2M+0x5ba>
  20313b:	b8 11 00 00 00       	mov    $0x11,%eax
  203140:	e9 90 f3 ff ff       	jmp    2024d5 <convS2M+0x85>

0000000000203145 <_syscall>:
  203145:	0f 05                	syscall
  203147:	c3                   	ret
  203148:	69 6e 69 74 3a 20 66 	imul   $0x66203a74,0x69(%rsi),%ebp
  20314f:	61                   	(bad)
  203150:	69 6c 65 64 20 74 6f 	imul   $0x206f7420,0x64(%rbp,%riz,2),%ebp
  203157:	20 
  203158:	63 72 65             	movsxd 0x65(%rdx),%esi
  20315b:	61                   	(bad)
  20315c:	74 65                	je     2031c3 <_syscall+0x7e>
  20315e:	20 00                	and    %al,(%rax)
  203160:	3d 3d 3d 20 4c       	cmp    $0x4c203d3d,%eax
  203165:	75 78                	jne    2031df <_syscall+0x9a>
  203167:	39 20                	cmp    %esp,(%rax)
  203169:	49 6e                	rex.WB outsb %ds:(%rsi),(%dx)
  20316b:	69 74 20 53 74 61 72 	imul   $0x74726174,0x53(%rax,%riz,1),%esi
  203172:	74 
  203173:	69 6e 67 20 3d 3d 3d 	imul   $0x3d3d3d20,0x67(%rsi),%ebp
  20317a:	0a 00                	or     (%rax),%al
  20317c:	23 2f                	and    (%rdi),%ebp
  20317e:	2e 2f                	cs (bad)
  203180:	62 6f 6f 74 2f       	(bad)
  203185:	72 65                	jb     2031ec <_syscall+0xa7>
  203187:	73 75                	jae    2031fe <_syscall+0xb9>
  203189:	72 72                	jb     2031fd <_syscall+0xb8>
  20318b:	65 63 74 69 6f       	movsxd %gs:0x6f(%rcx,%rbp,2),%esi
  203190:	6e                   	outsb  %ds:(%rsi),(%dx)
  203191:	00 69 6e             	add    %ch,0x6e(%rcx)
  203194:	69 74 3a 20 66 6f 72 	imul   $0x6b726f66,0x20(%rdx,%rdi,1),%esi
  20319b:	6b 
  20319c:	20 66 61             	and    %ah,0x61(%rsi)
  20319f:	69 6c 65 64 0a 00 66 	imul   $0x6f66000a,0x64(%rbp,%riz,2),%ebp
  2031a6:	6f 
  2031a7:	72 6b                	jb     203214 <_syscall+0xcf>
  2031a9:	20 66 61             	and    %ah,0x61(%rsi)
  2031ac:	69 6c 65 64 00 29 0a 	imul   $0xa2900,0x64(%rbp,%riz,2),%ebp
  2031b3:	00 
  2031b4:	2f                   	(bad)
  2031b5:	62 6f 6f 74 2f       	(bad)
  2031ba:	72 75                	jb     203231 <_syscall+0xec>
  2031bc:	6d                   	insl   (%dx),%es:(%rdi)
  2031bd:	70 5f                	jo     20321e <_syscall+0xd9>
  2031bf:	73 65                	jae    203226 <_syscall+0xe1>
  2031c1:	72 76                	jb     203239 <_syscall+0xf4>
  2031c3:	65 72 00             	gs jb  2031c6 <_syscall+0x81>
  2031c6:	2f                   	(bad)
  2031c7:	62 6f 6f 74 2f       	(bad)
  2031cc:	74 75                	je     203243 <_syscall+0xfe>
  2031ce:	72 62                	jb     203232 <_syscall+0xed>
  2031d0:	6f                   	outsl  %ds:(%rsi),(%dx)
  2031d1:	63 69 64             	movsxd 0x64(%rcx),%ebp
  2031d4:	00 2f                	add    %ch,(%rdi)
  2031d6:	62 6f 6f 74 2f       	(bad)
  2031db:	77 61                	ja     20323e <_syscall+0xf9>
  2031dd:	73 6d                	jae    20324c <_syscall+0x107>
  2031df:	5f                   	pop    %rdi
  2031e0:	74 65                	je     203247 <_syscall+0x102>
  2031e2:	73 74                	jae    203258 <_syscall+0x113>
  2031e4:	00 69 6e             	add    %ch,0x6e(%rcx)
  2031e7:	69 74 3a 20 45 6e 74 	imul   $0x65746e45,0x20(%rdx,%rdi,1),%esi
  2031ee:	65 
  2031ef:	72 69                	jb     20325a <_syscall+0x115>
  2031f1:	6e                   	outsb  %ds:(%rsi),(%dx)
  2031f2:	67 20 77 61          	and    %dh,0x61(%edi)
  2031f6:	69 74 20 6c 6f 6f 70 	imul   $0xa706f6f,0x6c(%rax,%riz,1),%esi
  2031fd:	0a 
  2031fe:	00 90 69 6e 69 74    	add    %dl,0x74696e69(%rax)
  203204:	3a 20                	cmp    (%rax),%ah
  203206:	66 61                	data16 (bad)
  203208:	69 6c 65 64 20 74 6f 	imul   $0x206f7420,0x64(%rbp,%riz,2),%ebp
  20320f:	20 
  203210:	77 72                	ja     203284 <_syscall+0x13f>
  203212:	69 74 65 20 63 6f 6e 	imul   $0x666e6f63,0x20(%rbp,%riz,2),%esi
  203219:	66 
  20321a:	69 67 20 74 6f 20 00 	imul   $0x206f74,0x20(%rdi),%esp
  203221:	00 00                	add    %al,(%rax)
  203223:	00 00                	add    %al,(%rax)
  203225:	00 00                	add    %al,(%rax)
  203227:	00 69 6e             	add    %ch,0x6e(%rcx)
  20322a:	69 74 3a 20 53 74 61 	imul   $0x72617453,0x20(%rdx,%rdi,1),%esi
  203231:	72 
  203232:	74 69                	je     20329d <_syscall+0x158>
  203234:	6e                   	outsb  %ds:(%rsi),(%dx)
  203235:	67 20 72 65          	and    %dh,0x65(%edx)
  203239:	73 75                	jae    2032b0 <_syscall+0x16b>
  20323b:	72 72                	jb     2032af <_syscall+0x16a>
  20323d:	65 63 74 69 6f       	movsxd %gs:0x6f(%rcx,%rbp,2),%esi
  203242:	6e                   	outsb  %ds:(%rsi),(%dx)
  203243:	20 73 65             	and    %dh,0x65(%rbx)
  203246:	72 76                	jb     2032be <_syscall+0x179>
  203248:	65 72 2e             	gs jb  203279 <_syscall+0x134>
  20324b:	2e 2e 0a 00          	cs cs or (%rax),%al
  20324f:	00 69 6e             	add    %ch,0x6e(%rcx)
  203252:	69 74 3a 20 65 78 65 	imul   $0x63657865,0x20(%rdx,%rdi,1),%esi
  203259:	63 
  20325a:	20 72 65             	and    %dh,0x65(%rdx)
  20325d:	73 75                	jae    2032d4 <_syscall+0x18f>
  20325f:	72 72                	jb     2032d3 <_syscall+0x18e>
  203261:	65 63 74 69 6f       	movsxd %gs:0x6f(%rcx,%rbp,2),%esi
  203266:	6e                   	outsb  %ds:(%rsi),(%dx)
  203267:	20 66 61             	and    %ah,0x61(%rsi)
  20326a:	69 6c 65 64 0a 00 69 	imul   $0x6e69000a,0x64(%rbp,%riz,2),%ebp
  203271:	6e 
  203272:	69 74 3a 20 52 65 73 	imul   $0x75736552,0x20(%rdx,%rdi,1),%esi
  203279:	75 
  20327a:	72 72                	jb     2032ee <_syscall+0x1a9>
  20327c:	65 63 74 69 6f       	movsxd %gs:0x6f(%rcx,%rbp,2),%esi
  203281:	6e                   	outsb  %ds:(%rsi),(%dx)
  203282:	20 73 65             	and    %dh,0x65(%rbx)
  203285:	72 76                	jb     2032fd <_syscall+0x1b8>
  203287:	65 72 20             	gs jb  2032aa <_syscall+0x165>
  20328a:	73 74                	jae    203300 <_syscall+0x1bb>
  20328c:	61                   	(bad)
  20328d:	72 74                	jb     203303 <_syscall+0x1be>
  20328f:	65 64 20 28          	gs and %ch,%fs:(%rax)
  203293:	50                   	push   %rax
  203294:	49                   	rex.WB
  203295:	44 20 00             	and    %r8b,(%rax)
  203298:	69 6e 69 74 3a 20 52 	imul   $0x52203a74,0x69(%rsi),%ebp
  20329f:	65 67 69 73 74 65 72 	imul   $0x6e697265,%gs:0x74(%ebx),%esi
  2032a6:	69 6e 
  2032a8:	67 20 73 65          	and    %dh,0x65(%ebx)
  2032ac:	72 76                	jb     203324 <_syscall+0x1df>
  2032ae:	69 63 65 73 2e 2e 2e 	imul   $0x2e2e2e73,0x65(%rbx),%esp
  2032b5:	0a 00                	or     (%rax),%al
  2032b7:	00 69 6e             	add    %ch,0x6e(%rcx)
  2032ba:	69 74 3a 20 53 65 72 	imul   $0x76726553,0x20(%rdx,%rdi,1),%esi
  2032c1:	76 
  2032c2:	69 63 65 20 72 65 67 	imul   $0x67657220,0x65(%rbx),%esp
  2032c9:	69 73 74 72 61 74 69 	imul   $0x69746172,0x74(%rbx),%esi
  2032d0:	6f                   	outsl  %ds:(%rsi),(%dx)
  2032d1:	6e                   	outsb  %ds:(%rsi),(%dx)
  2032d2:	20 63 6f             	and    %ah,0x6f(%rbx)
  2032d5:	6d                   	insl   (%dx),%es:(%rdi)
  2032d6:	70 6c                	jo     203344 <_syscall+0x1ff>
  2032d8:	65 74 65             	gs je  203340 <_syscall+0x1fb>
  2032db:	0a 00                	or     (%rax),%al
  2032dd:	00 00                	add    %al,(%rax)
  2032df:	00 50 41             	add    %dl,0x41(%rax)
  2032e2:	4e                   	rex.WRX
  2032e3:	49                   	rex.WB
  2032e4:	43 3a 20             	rex.XB cmp (%r8),%spl
  2032e7:	6d                   	insl   (%dx),%es:(%rdi)
  2032e8:	61                   	(bad)
  2032e9:	6c                   	insb   (%dx),%es:(%rdi)
  2032ea:	6c                   	insb   (%dx),%es:(%rdi)
  2032eb:	6f                   	outsl  %ds:(%rsi),(%dx)
  2032ec:	63 28                	movsxd (%rax),%ebp
  2032ee:	29 20                	sub    %esp,(%rax)
  2032f0:	63 61 6c             	movsxd 0x6c(%rcx),%esp
  2032f3:	6c                   	insb   (%dx),%es:(%rdi)
  2032f4:	65 64 20 2d 20 75 73 	gs and %ch,%fs:0x65737520(%rip)        # 6593a81c <stack_top+0x6573281c>
  2032fb:	65 
  2032fc:	20 70 65             	and    %dh,0x65(%rax)
  2032ff:	62 62 6c 65 5f       	(bad)
  203304:	61                   	(bad)
  203305:	6c                   	insb   (%dx),%es:(%rdi)
  203306:	6c                   	insb   (%dx),%es:(%rdi)
  203307:	6f                   	outsl  %ds:(%rsi),(%dx)
  203308:	63 28                	movsxd (%rax),%ebp
  20330a:	29 00                	sub    %eax,(%rax)
  20330c:	00 00                	add    %al,(%rax)
  20330e:	00 00                	add    %al,(%rax)
  203310:	50                   	push   %rax
  203311:	41                   	rex.B
  203312:	4e                   	rex.WRX
  203313:	49                   	rex.WB
  203314:	43 3a 20             	rex.XB cmp (%r8),%spl
  203317:	66 72 65             	data16 jb 20337f <_syscall+0x23a>
  20331a:	65 28 29             	sub    %ch,%gs:(%rcx)
  20331d:	20 63 61             	and    %ah,0x61(%rbx)
  203320:	6c                   	insb   (%dx),%es:(%rdi)
  203321:	6c                   	insb   (%dx),%es:(%rdi)
  203322:	65 64 20 2d 20 75 73 	gs and %ch,%fs:0x65737520(%rip)        # 6593a84a <stack_top+0x6573284a>
  203329:	65 
  20332a:	20 70 65             	and    %dh,0x65(%rax)
  20332d:	62 62 6c 65 5f       	(bad)
  203332:	66 72 65             	data16 jb 20339a <_syscall+0x255>
  203335:	65 28 29             	sub    %ch,%gs:(%rcx)
	...
  203340:	63 6f 6e             	movsxd 0x6e(%rdi),%ebp
  203343:	76 4d                	jbe    203392 <_syscall+0x24d>
  203345:	32 53 3a             	xor    0x3a(%rbx),%dl
  203348:	20 68 65             	and    %ch,0x65(%rax)
  20334b:	61                   	(bad)
  20334c:	64 65 72 20          	fs gs jb 203370 <_syscall+0x22b>
  203350:	62 6f 75 6e 64       	(bad)
  203355:	73 20                	jae    203377 <_syscall+0x232>
  203357:	63 68 65             	movsxd 0x65(%rax),%ebp
  20335a:	63 6b 20             	movsxd 0x20(%rbx),%ebp
  20335d:	66 61                	data16 (bad)
  20335f:	69 6c 65 64 20 70 3d 	imul   $0x253d7020,0x64(%rbp,%riz,2),%ebp
  203366:	25 
  203367:	70 20                	jo     203389 <_syscall+0x244>
  203369:	65 70 3d             	gs jo  2033a9 <_syscall+0x264>
  20336c:	25 70 0a 00 63       	and    $0x63000a70,%eax
  203371:	6f                   	outsl  %ds:(%rsi),(%dx)
  203372:	6e                   	outsb  %ds:(%rsi),(%dx)
  203373:	76 4d                	jbe    2033c2 <_syscall+0x27d>
  203375:	32 53 3a             	xor    0x3a(%rbx),%dl
  203378:	20 73 69             	and    %dh,0x69(%rbx)
  20337b:	7a 65                	jp     2033e2 <_syscall+0x29d>
  20337d:	20 63 68             	and    %ah,0x68(%rbx)
  203380:	65 63 6b 20          	movsxd %gs:0x20(%rbx),%ebp
  203384:	66 61                	data16 (bad)
  203386:	69 6c 65 64 20 73 69 	imul   $0x7a697320,0x64(%rbp,%riz,2),%ebp
  20338d:	7a 
  20338e:	65 3d 25 64 20 6d    	gs cmp $0x6d206425,%eax
  203394:	69 6e 3d 25 64 0a 00 	imul   $0xa6425,0x3d(%rsi),%ebp
  20339b:	90                   	nop
  20339c:	1c e4                	sbb    $0xe4,%al
  20339e:	ff                   	(bad)
  20339f:	ff 1c e4             	lcall  *(%rsp,%riz,8)
  2033a2:	ff                   	(bad)
  2033a3:	ff 4f e9             	decl   -0x17(%rdi)
  2033a6:	ff                   	(bad)
  2033a7:	ff                   	(bad)
  2033a8:	7c e4                	jl     20338e <_syscall+0x249>
  2033aa:	ff                   	(bad)
  2033ab:	ff 34 ec             	push   (%rsp,%rbp,8)
  2033ae:	ff                   	(bad)
  2033af:	ff                   	(bad)
  2033b0:	7c e4                	jl     203396 <_syscall+0x251>
  2033b2:	ff                   	(bad)
  2033b3:	ff f7                	push   %rdi
  2033b5:	e1 ff                	loope  2033b6 <_syscall+0x271>
  2033b7:	ff b4 e4 ff ff 9e e5 	push   -0x1a610001(%rsp,%riz,8)
  2033be:	ff                   	(bad)
  2033bf:	ff                   	ljmp   (bad)
  2033c0:	ec                   	in     (%dx),%al
  2033c1:	e1 ff                	loope  2033c2 <_syscall+0x27d>
  2033c3:	ff 94 e8 ff ff 20 e8 	call   *-0x17df0001(%rax,%rbp,8)
  2033ca:	ff                   	(bad)
  2033cb:	ff                   	(bad)
  2033cc:	fd                   	std
  2033cd:	e7 ff                	out    %eax,$0xff
  2033cf:	ff 84 e5 ff ff 9c e3 	incl   -0x1c630001(%rbp,%riz,8)
  2033d6:	ff                   	(bad)
  2033d7:	ff 84 e5 ff ff 6c e3 	incl   -0x1c930001(%rbp,%riz,8)
  2033de:	ff                   	(bad)
  2033df:	ff cc                	dec    %esp
  2033e1:	e2 ff                	loop   2033e2 <_syscall+0x29d>
  2033e3:	ff 34 e3             	push   (%rbx,%riz,8)
  2033e6:	ff                   	(bad)
  2033e7:	ff 34 e2             	push   (%rdx,%riz,8)
  2033ea:	ff                   	(bad)
  2033eb:	ff d4                	call   *%rsp
  2033ed:	e1 ff                	loope  2033ee <_syscall+0x2a9>
  2033ef:	ff                   	ljmp   (bad)
  2033f0:	ec                   	in     (%dx),%al
  2033f1:	e1 ff                	loope  2033f2 <_syscall+0x2ad>
  2033f3:	ff d4                	call   *%rsp
  2033f5:	e1 ff                	loope  2033f6 <_syscall+0x2b1>
  2033f7:	ff                   	ljmp   (bad)
  2033f8:	ec                   	in     (%dx),%al
  2033f9:	e1 ff                	loope  2033fa <_syscall+0x2b5>
  2033fb:	ff d4                	call   *%rsp
  2033fd:	e1 ff                	loope  2033fe <_syscall+0x2b9>
  2033ff:	ff                   	(bad)
  203400:	fc                   	cld
  203401:	e2 ff                	loop   203402 <_syscall+0x2bd>
  203403:	ff 04 e5 ff ff ec e1 	incl   -0x1e130001(,%riz,8)
  20340a:	ff                   	(bad)
  20340b:	ff cc                	dec    %esp
  20340d:	e2 ff                	loop   20340e <_syscall+0x2c9>
  20340f:	ff f7                	push   %rdi
  203411:	e1 ff                	loope  203412 <_syscall+0x2cd>
  203413:	ff 6c e7 ff          	ljmp   *-0x1(%rdi,%riz,8)
  203417:	ff                   	(bad)
  203418:	3c e7                	cmp    $0xe7,%al
  20341a:	ff                   	(bad)
  20341b:	ff 8b e7 ff ff 3c    	decl   0x3cffffe7(%rbx)
  203421:	e5 ff                	in     $0xff,%eax
  203423:	ff 9c e3 ff ff 3c e5 	lcall  *-0x1ac30001(%rbx,%riz,8)
  20342a:	ff                   	(bad)
  20342b:	ff 6c e3 ff          	ljmp   *-0x1(%rbx,%riz,8)
  20342f:	ff cc                	dec    %esp
  203431:	e2 ff                	loop   203432 <_syscall+0x2ed>
  203433:	ff 34 e3             	push   (%rbx,%riz,8)
  203436:	ff                   	(bad)
  203437:	ff 34 e2             	push   (%rdx,%riz,8)
  20343a:	ff                   	(bad)
  20343b:	ff d4                	call   *%rsp
  20343d:	e1 ff                	loope  20343e <_syscall+0x2f9>
  20343f:	ff                   	ljmp   (bad)
  203440:	ec                   	in     (%dx),%al
  203441:	e1 ff                	loope  203442 <_syscall+0x2fd>
  203443:	ff 6c e3 ff          	ljmp   *-0x1(%rbx,%riz,8)
  203447:	ff cc                	dec    %esp
  203449:	e2 ff                	loop   20344a <_syscall+0x305>
  20344b:	ff 34 e3             	push   (%rbx,%riz,8)
  20344e:	ff                   	(bad)
  20344f:	ff 34 e2             	push   (%rdx,%riz,8)
  203452:	ff                   	(bad)
  203453:	ff 64 e2 ff          	jmp    *-0x1(%rdx,%riz,8)
  203457:	ff                   	ljmp   (bad)
  203458:	ec                   	in     (%dx),%al
  203459:	e1 ff                	loope  20345a <_syscall+0x315>
  20345b:	ff 64 e2 ff          	jmp    *-0x1(%rdx,%riz,8)
  20345f:	ff                   	(bad)
  203460:	fc                   	cld
  203461:	e2 ff                	loop   203462 <_syscall+0x31d>
  203463:	ff d4                	call   *%rsp
  203465:	e1 ff                	loope  203466 <_syscall+0x321>
  203467:	ff                   	(bad)
  203468:	fc                   	cld
  203469:	e2 ff                	loop   20346a <_syscall+0x325>
  20346b:	ff c9                	dec    %ecx
  20346d:	eb ff                	jmp    20346e <_syscall+0x329>
  20346f:	ff                   	ljmp   (bad)
  203470:	ec                   	in     (%dx),%al
  203471:	e1 ff                	loope  203472 <_syscall+0x32d>
  203473:	ff 04 e5 ff ff ec e1 	incl   -0x1e130001(,%riz,8)
  20347a:	ff                   	(bad)
  20347b:	ff f7                	push   %rdi
  20347d:	e1 ff                	loope  20347e <_syscall+0x339>
  20347f:	ff f7                	push   %rdi
  203481:	e1 ff                	loope  203482 <_syscall+0x33d>
  203483:	ff f7                	push   %rdi
  203485:	e1 ff                	loope  203486 <_syscall+0x341>
  203487:	ff f7                	push   %rdi
  203489:	e1 ff                	loope  20348a <_syscall+0x345>
  20348b:	ff c2                	inc    %edx
  20348d:	ea                   	(bad)
  20348e:	ff                   	(bad)
  20348f:	ff af eb ff ff 84    	ljmp   *-0x7b000015(%rdi)
  203495:	e6 ff                	out    %al,$0xff
  203497:	ff                   	ljmp   (bad)
  203498:	ec                   	in     (%dx),%al
  203499:	e1 ff                	loope  20349a <_syscall+0x355>
  20349b:	ff b4 e4 ff ff ec e1 	push   -0x1e130001(%rsp,%riz,8)
  2034a2:	ff                   	(bad)
  2034a3:	ff                   	ljmp   (bad)
  2034a4:	ec                   	in     (%dx),%al
  2034a5:	e1 ff                	loope  2034a6 <_syscall+0x361>
  2034a7:	ff e0                	jmp    *%rax
  2034a9:	e6 ff                	out    %al,$0xff
  2034ab:	ff ac e2 ff ff ac e2 	ljmp   *-0x1d530001(%rdx,%riz,8)
  2034b2:	ff                   	(bad)
  2034b3:	ff 34 e2             	push   (%rdx,%riz,8)
  2034b6:	ff                   	(bad)
  2034b7:	ff                   	ljmp   (bad)
  2034b8:	ec                   	in     (%dx),%al
  2034b9:	e1 ff                	loope  2034ba <_syscall+0x375>
  2034bb:	ff f7                	push   %rdi
  2034bd:	e1 ff                	loope  2034be <_syscall+0x379>
  2034bf:	ff f7                	push   %rdi
  2034c1:	e1 ff                	loope  2034c2 <_syscall+0x37d>
  2034c3:	ff f7                	push   %rdi
  2034c5:	e1 ff                	loope  2034c6 <_syscall+0x381>
  2034c7:	ff f7                	push   %rdi
  2034c9:	e1 ff                	loope  2034ca <_syscall+0x385>
  2034cb:	ff f7                	push   %rdi
  2034cd:	e1 ff                	loope  2034ce <_syscall+0x389>
  2034cf:	ff f7                	push   %rdi
  2034d1:	e1 ff                	loope  2034d2 <_syscall+0x38d>
  2034d3:	ff f7                	push   %rdi
  2034d5:	e1 ff                	loope  2034d6 <_syscall+0x391>
  2034d7:	ff f7                	push   %rdi
  2034d9:	e1 ff                	loope  2034da <_syscall+0x395>
  2034db:	ff f3                	push   %rbx
  2034dd:	e9 ff ff ec e1       	jmp    ffffffffe20d34e1 <stack_top+0xffffffffe1ecb4e1>
  2034e2:	ff                   	(bad)
  2034e3:	ff                   	(bad)
  2034e4:	ba e5 ff ff ec       	mov    $0xecffffe5,%edx
  2034e9:	e1 ff                	loope  2034ea <_syscall+0x3a5>
  2034eb:	ff                   	lcall  (bad)
  2034ec:	dc ea                	fsubr  %st,%st(2)
  2034ee:	ff                   	(bad)
  2034ef:	ff                   	ljmp   (bad)
  2034f0:	ec                   	in     (%dx),%al
  2034f1:	e1 ff                	loope  2034f2 <_syscall+0x3ad>
  2034f3:	ff 64 e2 ff          	jmp    *-0x1(%rdx,%riz,8)
  2034f7:	ff                   	ljmp   (bad)
  2034f8:	ec                   	in     (%dx),%al
  2034f9:	e1 ff                	loope  2034fa <_syscall+0x3b5>
  2034fb:	ff f7                	push   %rdi
  2034fd:	e1 ff                	loope  2034fe <_syscall+0x3b9>
  2034ff:	ff f7                	push   %rdi
  203501:	e1 ff                	loope  203502 <_syscall+0x3bd>
  203503:	ff 6b eb             	ljmp   *-0x15(%rbx)
  203506:	ff                   	(bad)
  203507:	ff d4                	call   *%rsp
  203509:	e1 ff                	loope  20350a <_syscall+0x3c5>
  20350b:	ff                   	ljmp   (bad)
  20350c:	ec                   	in     (%dx),%al
  20350d:	e1 ff                	loope  20350e <_syscall+0x3c9>
  20350f:	ff 8d eb ff ff d4    	decl   -0x2b000015(%rbp)
  203515:	e1 ff                	loope  203516 <_syscall+0x3d1>
  203517:	ff 64 e2 ff          	jmp    *-0x1(%rdx,%riz,8)
  20351b:	ff f7                	push   %rdi
  20351d:	e1 ff                	loope  20351e <_syscall+0x3d9>
  20351f:	ff f7                	push   %rdi
  203521:	e1 ff                	loope  203522 <_syscall+0x3dd>
  203523:	ff f7                	push   %rdi
  203525:	e1 ff                	loope  203526 <_syscall+0x3e1>
  203527:	ff f7                	push   %rdi
  203529:	e1 ff                	loope  20352a <_syscall+0x3e5>
  20352b:	ff 97 ea ff ff ac    	call   *-0x53000016(%rdi)
  203531:	e2 ff                	loop   203532 <_syscall+0x3ed>
  203533:	ff ac e2 ff ff ec e1 	ljmp   *-0x1e130001(%rdx,%riz,8)
  20353a:	ff                   	(bad)
  20353b:	ff 34 e2             	push   (%rdx,%riz,8)
  20353e:	ff                   	(bad)
  20353f:	ff 34 e2             	push   (%rdx,%riz,8)
  203542:	ff                   	(bad)
  203543:	ff                   	lcall  (bad)
  203544:	dc ec                	fsubr  %st,%st(4)
  203546:	ff                   	(bad)
  203547:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  20354a:	ff                   	(bad)
  20354b:	ff                   	(bad)
  20354c:	3c ec                	cmp    $0xec,%al
  20354e:	ff                   	(bad)
  20354f:	ff                   	(bad)
  203550:	3c ec                	cmp    $0xec,%al
  203552:	ff                   	(bad)
  203553:	ff                   	(bad)
  203554:	fc                   	cld
  203555:	ec                   	in     (%dx),%al
  203556:	ff                   	(bad)
  203557:	ff                   	(bad)
  203558:	fc                   	cld
  203559:	ec                   	in     (%dx),%al
  20355a:	ff                   	(bad)
  20355b:	ff 42 ee             	incl   -0x12(%rdx)
  20355e:	ff                   	(bad)
  20355f:	ff 0c ee             	decl   (%rsi,%rbp,8)
  203562:	ff                   	(bad)
  203563:	ff 9c ec ff ff 0c ee 	lcall  *-0x11f30001(%rsp,%rbp,8)
  20356a:	ff                   	(bad)
  20356b:	ff                   	(bad)
  20356c:	3c ed                	cmp    $0xed,%al
  20356e:	ff                   	(bad)
  20356f:	ff cc                	dec    %esp
  203571:	ec                   	in     (%dx),%al
  203572:	ff                   	(bad)
  203573:	ff                   	(bad)
  203574:	bc ec ff ff 2c       	mov    $0x2cffffec,%esp
  203579:	ec                   	in     (%dx),%al
  20357a:	ff                   	(bad)
  20357b:	ff 2c ec             	ljmp   *(%rsp,%rbp,8)
  20357e:	ff                   	(bad)
  20357f:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  203582:	ff                   	(bad)
  203583:	ff                   	(bad)
  203584:	3c ed                	cmp    $0xed,%al
  203586:	ff                   	(bad)
  203587:	ff cc                	dec    %esp
  203589:	ec                   	in     (%dx),%al
  20358a:	ff                   	(bad)
  20358b:	ff                   	(bad)
  20358c:	bc ec ff ff 2c       	mov    $0x2cffffec,%esp
  203591:	ec                   	in     (%dx),%al
  203592:	ff                   	(bad)
  203593:	ff 6c ec ff          	ljmp   *-0x1(%rsp,%rbp,8)
  203597:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  20359a:	ff                   	(bad)
  20359b:	ff 6c ec ff          	ljmp   *-0x1(%rsp,%rbp,8)
  20359f:	ff                   	ljmp   (bad)
  2035a0:	ec                   	in     (%dx),%al
  2035a1:	ec                   	in     (%dx),%al
  2035a2:	ff                   	(bad)
  2035a3:	ff 2c ec             	ljmp   *(%rsp,%rbp,8)
  2035a6:	ff                   	(bad)
  2035a7:	ff                   	ljmp   (bad)
  2035a8:	ec                   	in     (%dx),%al
  2035a9:	ec                   	in     (%dx),%al
  2035aa:	ff                   	(bad)
  2035ab:	ff 20                	jmp    *(%rax)
  2035ad:	ee                   	out    %al,(%dx)
  2035ae:	ff                   	(bad)
  2035af:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  2035b2:	ff                   	(bad)
  2035b3:	ff                   	lcall  (bad)
  2035b4:	dc ec                	fsubr  %st,%st(4)
  2035b6:	ff                   	(bad)
  2035b7:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  2035ba:	ff                   	(bad)
  2035bb:	ff                   	(bad)
  2035bc:	3c ec                	cmp    $0xec,%al
  2035be:	ff                   	(bad)
  2035bf:	ff                   	(bad)
  2035c0:	3c ec                	cmp    $0xec,%al
  2035c2:	ff                   	(bad)
  2035c3:	ff                   	(bad)
  2035c4:	3c ec                	cmp    $0xec,%al
  2035c6:	ff                   	(bad)
  2035c7:	ff                   	(bad)
  2035c8:	3c ec                	cmp    $0xec,%al
  2035ca:	ff                   	(bad)
  2035cb:	ff 2c ec             	ljmp   *(%rsp,%rbp,8)
  2035ce:	ff                   	(bad)
  2035cf:	ff 2c ec             	ljmp   *(%rsp,%rbp,8)
  2035d2:	ff                   	(bad)
  2035d3:	ff 4c ec ff          	decl   -0x1(%rsp,%rbp,8)
  2035d7:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  2035da:	ff                   	(bad)
  2035db:	ff 84 ec ff ff 1c ec 	incl   -0x13e30001(%rsp,%rbp,8)
  2035e2:	ff                   	(bad)
  2035e3:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  2035e6:	ff                   	(bad)
  2035e7:	ff af ee ff ff 0c    	ljmp   *0xcffffee(%rdi)
  2035ed:	ed                   	in     (%dx),%eax
  2035ee:	ff                   	(bad)
  2035ef:	ff 0c ed ff ff 2c ec 	decl   -0x13d30001(,%rbp,8)
  2035f6:	ff                   	(bad)
  2035f7:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  2035fa:	ff                   	(bad)
  2035fb:	ff                   	(bad)
  2035fc:	3c ec                	cmp    $0xec,%al
  2035fe:	ff                   	(bad)
  2035ff:	ff                   	(bad)
  203600:	3c ec                	cmp    $0xec,%al
  203602:	ff                   	(bad)
  203603:	ff                   	(bad)
  203604:	3c ec                	cmp    $0xec,%al
  203606:	ff                   	(bad)
  203607:	ff                   	(bad)
  203608:	3c ec                	cmp    $0xec,%al
  20360a:	ff                   	(bad)
  20360b:	ff                   	(bad)
  20360c:	3c ec                	cmp    $0xec,%al
  20360e:	ff                   	(bad)
  20360f:	ff                   	(bad)
  203610:	3c ec                	cmp    $0xec,%al
  203612:	ff                   	(bad)
  203613:	ff                   	(bad)
  203614:	3c ec                	cmp    $0xec,%al
  203616:	ff                   	(bad)
  203617:	ff                   	(bad)
  203618:	3c ec                	cmp    $0xec,%al
  20361a:	ff                   	(bad)
  20361b:	ff 5c ee ff          	lcall  *-0x1(%rsi,%rbp,8)
  20361f:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  203622:	ff                   	(bad)
  203623:	ff 99 ee ff ff 1c    	lcall  *0x1cffffee(%rcx)
  203629:	ec                   	in     (%dx),%al
  20362a:	ff                   	(bad)
  20362b:	ff 86 ee ff ff 1c    	incl   0x1cffffee(%rsi)
  203631:	ec                   	in     (%dx),%al
  203632:	ff                   	(bad)
  203633:	ff 6c ec ff          	ljmp   *-0x1(%rsp,%rbp,8)
  203637:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  20363a:	ff                   	(bad)
  20363b:	ff                   	(bad)
  20363c:	3c ec                	cmp    $0xec,%al
  20363e:	ff                   	(bad)
  20363f:	ff                   	(bad)
  203640:	3c ec                	cmp    $0xec,%al
  203642:	ff                   	(bad)
  203643:	ff 0c ed ff ff 2c ec 	decl   -0x13d30001(,%rbp,8)
  20364a:	ff                   	(bad)
  20364b:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  20364e:	ff                   	(bad)
  20364f:	ff 0c ed ff ff 2c ec 	decl   -0x13d30001(,%rbp,8)
  203656:	ff                   	(bad)
  203657:	ff 6c ec ff          	ljmp   *-0x1(%rsp,%rbp,8)
  20365b:	ff                   	(bad)
  20365c:	3c ec                	cmp    $0xec,%al
  20365e:	ff                   	(bad)
  20365f:	ff                   	(bad)
  203660:	3c ec                	cmp    $0xec,%al
  203662:	ff                   	(bad)
  203663:	ff                   	(bad)
  203664:	3c ec                	cmp    $0xec,%al
  203666:	ff                   	(bad)
  203667:	ff                   	(bad)
  203668:	3c ec                	cmp    $0xec,%al
  20366a:	ff                   	(bad)
  20366b:	ff                   	(bad)
  20366c:	3c ed                	cmp    $0xed,%al
  20366e:	ff                   	(bad)
  20366f:	ff 0c ed ff ff 0c ed 	decl   -0x12f30001(,%rbp,8)
  203676:	ff                   	(bad)
  203677:	ff 1c ec             	lcall  *(%rsp,%rbp,8)
  20367a:	ff                   	(bad)
  20367b:	ff 2c ec             	ljmp   *(%rsp,%rbp,8)
  20367e:	ff                   	(bad)
  20367f:	ff 2c ec             	ljmp   *(%rsp,%rbp,8)
  203682:	ff                   	(bad)
  203683:	ff 0c eb             	decl   (%rbx,%rbp,8)
  203686:	ff                   	(bad)
  203687:	ff 0c eb             	decl   (%rbx,%rbp,8)
  20368a:	ff                   	(bad)
  20368b:	ff 54 ec ff          	call   *-0x1(%rsp,%rbp,8)
  20368f:	ff                   	ljmp   (bad)
  203690:	ec                   	in     (%dx),%al
  203691:	eb ff                	jmp    203692 <_syscall+0x54d>
  203693:	ff 8c ec ff ff ec eb 	decl   -0x14130001(%rsp,%rbp,8)
  20369a:	ff                   	(bad)
  20369b:	ff                   	(bad)
  20369c:	fc                   	cld
  20369d:	ea                   	(bad)
  20369e:	ff                   	(bad)
  20369f:	ff 44 eb ff          	incl   -0x1(%rbx,%rbp,8)
  2036a3:	ff 4d eb             	decl   -0x15(%rbp)
  2036a6:	ff                   	(bad)
  2036a7:	ff                   	lcall  (bad)
  2036a8:	dc ea                	fsubr  %st,%st(2)
  2036aa:	ff                   	(bad)
  2036ab:	ff 0c ec             	decl   (%rsp,%rbp,8)
  2036ae:	ff                   	(bad)
  2036af:	ff ac ec ff ff bc ec 	ljmp   *-0x13430001(%rsp,%rbp,8)
  2036b6:	ff                   	(bad)
  2036b7:	ff                   	lcall  (bad)
  2036b8:	dc eb                	fsubr  %st,%st(3)
  2036ba:	ff                   	(bad)
  2036bb:	ff 5c eb ff          	lcall  *-0x1(%rbx,%rbp,8)
  2036bf:	ff                   	lcall  (bad)
  2036c0:	dc eb                	fsubr  %st,%st(3)
  2036c2:	ff                   	(bad)
  2036c3:	ff                   	(bad)
  2036c4:	fc                   	cld
  2036c5:	eb ff                	jmp    2036c6 <_syscall+0x581>
  2036c7:	ff 8c eb ff ff 7c eb 	decl   -0x14830001(%rbx,%rbp,8)
  2036ce:	ff                   	(bad)
  2036cf:	ff 4a f1             	decl   -0xf(%rdx)
  2036d2:	ff                   	(bad)
  2036d3:	ff 4a f1             	decl   -0xf(%rdx)
  2036d6:	ff                   	(bad)
  2036d7:	ff b2 f5 ff ff 89    	push   -0x7600000b(%rdx)
  2036dd:	f0 ff                	lock (bad)
  2036df:	ff 95 f8 ff ff 89    	call   *-0x76000008(%rbp)
  2036e5:	f0 ff                	lock (bad)
  2036e7:	ff ab ed ff ff 9d    	ljmp   *-0x62000013(%rbx)
  2036ed:	f0 ff                	lock (bad)
  2036ef:	ff ca                	dec    %edx
  2036f1:	f7 ff                	idiv   %edi
  2036f3:	ff 00                	incl   (%rax)
  2036f5:	ee                   	out    %al,(%dx)
  2036f6:	ff                   	(bad)
  2036f7:	ff ac f6 ff ff 75 f7 	ljmp   *-0x88a0001(%rsi,%rsi,8)
  2036fe:	ff                   	(bad)
  2036ff:	ff                   	ljmp   (bad)
  203700:	ec                   	in     (%dx),%al
  203701:	f3 ff                	repz (bad)
  203703:	ff 9a f1 ff ff ba    	lcall  *-0x4500000f(%rdx)
  203709:	f0 ff                	lock (bad)
  20370b:	ff 9a f1 ff ff e0    	lcall  *-0x1f00000f(%rdx)
  203711:	ee                   	out    %al,(%dx)
  203712:	ff                   	(bad)
  203713:	ff 68 ef             	ljmp   *-0x11(%rax)
  203716:	ff                   	(bad)
  203717:	ff a8 ef ff ff 38    	ljmp   *0x38ffffef(%rax)
  20371d:	ee                   	out    %al,(%dx)
  20371e:	ff                   	(bad)
  20371f:	ff 10                	call   *(%rax)
  203721:	ee                   	out    %al,(%dx)
  203722:	ff                   	(bad)
  203723:	ff 00                	incl   (%rax)
  203725:	ee                   	out    %al,(%dx)
  203726:	ff                   	(bad)
  203727:	ff 10                	call   *(%rax)
  203729:	ee                   	out    %al,(%dx)
  20372a:	ff                   	(bad)
  20372b:	ff 00                	incl   (%rax)
  20372d:	ee                   	out    %al,(%dx)
  20372e:	ff                   	(bad)
  20372f:	ff 10                	call   *(%rax)
  203731:	ee                   	out    %al,(%dx)
  203732:	ff                   	(bad)
  203733:	ff 30                	push   (%rax)
  203735:	f0 ff                	lock (bad)
  203737:	ff 5b f0             	lcall  *-0x10(%rbx)
  20373a:	ff                   	(bad)
  20373b:	ff 00                	incl   (%rax)
  20373d:	ee                   	out    %al,(%dx)
  20373e:	ff                   	(bad)
  20373f:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  203745:	ed                   	in     (%dx),%eax
  203746:	ff                   	(bad)
  203747:	ff 20                	jmp    *(%rax)
  203749:	f9                   	stc
  20374a:	ff                   	(bad)
  20374b:	ff af f9 ff ff 7d    	ljmp   *0x7dfffff9(%rdi)
  203751:	f3 ff                	repz (bad)
  203753:	ff cc                	dec    %esp
  203755:	f1                   	int1
  203756:	ff                   	(bad)
  203757:	ff                   	(bad)
  203758:	ba f0 ff ff cc       	mov    $0xccfffff0,%edx
  20375d:	f1                   	int1
  20375e:	ff                   	(bad)
  20375f:	ff e0                	jmp    *%rax
  203761:	ee                   	out    %al,(%dx)
  203762:	ff                   	(bad)
  203763:	ff 68 ef             	ljmp   *-0x11(%rax)
  203766:	ff                   	(bad)
  203767:	ff a8 ef ff ff 38    	ljmp   *0x38ffffef(%rax)
  20376d:	ee                   	out    %al,(%dx)
  20376e:	ff                   	(bad)
  20376f:	ff 10                	call   *(%rax)
  203771:	ee                   	out    %al,(%dx)
  203772:	ff                   	(bad)
  203773:	ff 00                	incl   (%rax)
  203775:	ee                   	out    %al,(%dx)
  203776:	ff                   	(bad)
  203777:	ff e0                	jmp    *%rax
  203779:	ee                   	out    %al,(%dx)
  20377a:	ff                   	(bad)
  20377b:	ff 68 ef             	ljmp   *-0x11(%rax)
  20377e:	ff                   	(bad)
  20377f:	ff a8 ef ff ff 38    	ljmp   *0x38ffffef(%rax)
  203785:	ee                   	out    %al,(%dx)
  203786:	ff                   	(bad)
  203787:	ff 58 ee             	lcall  *-0x12(%rax)
  20378a:	ff                   	(bad)
  20378b:	ff 00                	incl   (%rax)
  20378d:	ee                   	out    %al,(%dx)
  20378e:	ff                   	(bad)
  20378f:	ff 58 ee             	lcall  *-0x12(%rax)
  203792:	ff                   	(bad)
  203793:	ff 30                	push   (%rax)
  203795:	f0 ff                	lock (bad)
  203797:	ff 10                	call   *(%rax)
  203799:	ee                   	out    %al,(%dx)
  20379a:	ff                   	(bad)
  20379b:	ff 30                	push   (%rax)
  20379d:	f0 ff                	lock (bad)
  20379f:	ff 55 f5             	call   *-0xb(%rbp)
  2037a2:	ff                   	(bad)
  2037a3:	ff 00                	incl   (%rax)
  2037a5:	ee                   	out    %al,(%dx)
  2037a6:	ff                   	(bad)
  2037a7:	ff 5b f0             	lcall  *-0x10(%rbx)
  2037aa:	ff                   	(bad)
  2037ab:	ff 00                	incl   (%rax)
  2037ad:	ee                   	out    %al,(%dx)
  2037ae:	ff                   	(bad)
  2037af:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  2037b5:	ed                   	in     (%dx),%eax
  2037b6:	ff                   	(bad)
  2037b7:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  2037bd:	ed                   	in     (%dx),%eax
  2037be:	ff                   	(bad)
  2037bf:	ff 17                	call   *(%rdi)
  2037c1:	f4                   	hlt
  2037c2:	ff                   	(bad)
  2037c3:	ff 93 f9 ff ff 24    	call   *0x24fffff9(%rbx)
  2037c9:	f8                   	clc
  2037ca:	ff                   	(bad)
  2037cb:	ff 00                	incl   (%rax)
  2037cd:	ee                   	out    %al,(%dx)
  2037ce:	ff                   	(bad)
  2037cf:	ff 9d f0 ff ff 00    	lcall  *0xfffff0(%rbp)
  2037d5:	ee                   	out    %al,(%dx)
  2037d6:	ff                   	(bad)
  2037d7:	ff 00                	incl   (%rax)
  2037d9:	ee                   	out    %al,(%dx)
  2037da:	ff                   	(bad)
  2037db:	ff c5                	inc    %ebp
  2037dd:	f2 ff                	repnz (bad)
  2037df:	ff 90 ee ff ff 90    	call   *-0x6f000012(%rax)
  2037e5:	ee                   	out    %al,(%dx)
  2037e6:	ff                   	(bad)
  2037e7:	ff                   	(bad)
  2037e8:	38 ee                	cmp    %ch,%dh
  2037ea:	ff                   	(bad)
  2037eb:	ff 00                	incl   (%rax)
  2037ed:	ee                   	out    %al,(%dx)
  2037ee:	ff                   	(bad)
  2037ef:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  2037f5:	ed                   	in     (%dx),%eax
  2037f6:	ff                   	(bad)
  2037f7:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  2037fd:	ed                   	in     (%dx),%eax
  2037fe:	ff                   	(bad)
  2037ff:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  203805:	ed                   	in     (%dx),%eax
  203806:	ff                   	(bad)
  203807:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  20380d:	ed                   	in     (%dx),%eax
  20380e:	ff                   	(bad)
  20380f:	ff 9f f4 ff ff 00    	lcall  *0xfffff4(%rdi)
  203815:	ee                   	out    %al,(%dx)
  203816:	ff                   	(bad)
  203817:	ff f0                	push   %rax
  203819:	f1                   	int1
  20381a:	ff                   	(bad)
  20381b:	ff 00                	incl   (%rax)
  20381d:	ee                   	out    %al,(%dx)
  20381e:	ff                   	(bad)
  20381f:	ff                   	(bad)
  203820:	fc                   	cld
  203821:	f2 ff                	repnz (bad)
  203823:	ff 00                	incl   (%rax)
  203825:	ee                   	out    %al,(%dx)
  203826:	ff                   	(bad)
  203827:	ff 58 ee             	lcall  *-0x12(%rax)
  20382a:	ff                   	(bad)
  20382b:	ff 00                	incl   (%rax)
  20382d:	ee                   	out    %al,(%dx)
  20382e:	ff                   	(bad)
  20382f:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  203835:	ed                   	in     (%dx),%eax
  203836:	ff                   	(bad)
  203837:	ff 69 f4             	ljmp   *-0xc(%rcx)
  20383a:	ff                   	(bad)
  20383b:	ff 10                	call   *(%rax)
  20383d:	ee                   	out    %al,(%dx)
  20383e:	ff                   	(bad)
  20383f:	ff 00                	incl   (%rax)
  203841:	ee                   	out    %al,(%dx)
  203842:	ff                   	(bad)
  203843:	ff 33                	push   (%rbx)
  203845:	f4                   	hlt
  203846:	ff                   	(bad)
  203847:	ff 10                	call   *(%rax)
  203849:	ee                   	out    %al,(%dx)
  20384a:	ff                   	(bad)
  20384b:	ff 58 ee             	lcall  *-0x12(%rax)
  20384e:	ff                   	(bad)
  20384f:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  203855:	ed                   	in     (%dx),%eax
  203856:	ff                   	(bad)
  203857:	ff ab ed ff ff ab    	ljmp   *-0x54000013(%rbx)
  20385d:	ed                   	in     (%dx),%eax
  20385e:	ff                   	(bad)
  20385f:	ff 30                	push   (%rax)
  203861:	f6 ff                	idiv   %bh
  203863:	ff                   	lcall  (bad)
  203864:	dd f7                	(bad)
  203866:	ff                   	(bad)
  203867:	ff 90 ee ff ff 00    	call   *0xffffee(%rax)
  20386d:	ee                   	out    %al,(%dx)
  20386e:	ff                   	(bad)
  20386f:	ff                   	(bad)
  203870:	38 ee                	cmp    %ch,%dh
  203872:	ff                   	(bad)
  203873:	ff                   	(bad)
  203874:	38 ee                	cmp    %ch,%dh
  203876:	ff                   	(bad)
  203877:	ff                   	.byte 0xff
