.global Addr16Func
.type Addr16Func, @function


// use of segment register is not an ERROR


Addr16Func:
    
    
	
	addr16 mov %fs:5,%edx
        addr16 mov %edx,%fs:5
        addr16 mov %fs:0(%bx),%edx
        addr16 mov %edx,%fs:0(%bx)
        addr16 mov %fs:5(%bx),%edx
        addr16 mov %edx,%fs:5(%bx)
        addr16 mov %fs:0(%bx,%si),%edx
        addr16 mov %edx,%fs:0(%bx,%si)
        addr16 mov %fs:5(%bx,%si),%edx
        addr16 mov %edx,%fs:5(%bx,%si)
        addr16 lea 5(%bx,%si),%edx
        addr16 mov %fs:5(%bp,%si),%edx
        addr16 mov %edx,%fs:5(%bp,%si)
        addr16 mov %edx,5(%bx,%si)
        addr16 mov 5(%bx,%si),%edx
        addr16 mov %edx,5(%bp,%si)
        addr16 mov 5(%bp,%si),%edx
        addr16 mov %edx,5(%bx)
        addr16 mov 5(%bx),%edx
        addr16 mov %edx,5(%bp)
        addr16 mov 5(%bp),%edx
    
    ret



