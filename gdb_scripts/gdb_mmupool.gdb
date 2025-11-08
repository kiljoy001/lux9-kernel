set auto-load safe-path /home/scott/Repo/lux9-kernel
file lux9.elf
target remote tcp:localhost:1234
delete
break borrow_lock if bl == (BorrowLock*)&mmupool
commands
silent
printf "borrow_lock mmupool: up=%p caller=%p\n", up, *(void**)(\$sp)
bt
continue
end
break borrow_unlock if bl == (BorrowLock*)&mmupool
commands
silent
printf "borrow_unlock mmupool: up=%p caller=%p\n", up, *(void**)(\$sp)
bt
continue
end
continue