.globl main
main:
	fchkf fchkftaken
	nop 0
fchkftaken:	
		
	# control speculation taken
	ld8.s r2 = [r0]
	;; 
	chk.s.i r2,chkstaken1
	nop 0
chkstaken1:

	# control speculation fall through
	ld8.s r2 = [r12]
	;; 
	chk.s r2,chkstaken2
	nop 0
chkstaken2:
	
	# control speculation on a floating point register
	# control speculation taken
	ldf8.s f2 = [r0]
	;; 
	chk.s f2,chkstaken3
	nop 0
chkstaken3:

	# control speculation fall through
	ldf8.s f2 = [r12]
	;; 
	chk.s f2,chkstaken4
	nop 0
chkstaken4:
	
	
	# data speculation taken
	ld8.a r2 = [r12]
	st8 [r12] = r0
	;; 
	chk.a.clr r2,chkataken1
	nop 0
chkataken1:
	
	# data speculation fall through
	ld8.a r2 = [r12]
	;; 
	chk.a.clr r2,chkataken2
	nop 0
chkataken2:
	
	# control speculation fall through, i-form of chk.s
	ld8.s r2 = [r12]
    ;;
	chk.s.i r2,chkstaken5
    nop 0
chkstaken5:

	mov r8 = 0
	br.ret.sptk b0
	
