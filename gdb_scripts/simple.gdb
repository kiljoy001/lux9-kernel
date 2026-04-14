set auto-load safe-path /home/scott/Repo/lux9-kernel
file lux9.elf
target remote tcp:localhost:1234
break borrow_lock if (uintptr_t)bl == (uintptr_t)&mmupool
commands
silent
printf "borrow_lock mmupool: up=%p\n", up
bt
continue
end
break borrow_unlock if (uintptr_t)bl == (uintptr_t)&mmupool
commands
silent
printf "borrow_unlock mmupool: up=%p\n", up
bt
continue
end
continue