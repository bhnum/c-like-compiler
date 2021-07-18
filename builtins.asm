
# compiler builtin functions, mostly syscall wrappers

.text
print_string:
    # $a0 : string address
    li $v0, 4
    syscall
    jr $ra

print_char:
    # $a0 : character value
    li $v0, 11
    syscall
    jr $ra

print_int:
    # $a0 : integer value
    li $v0, 1
    syscall
    jr $ra


read_string:
    # $a0 : input buffer address
    # $a1 : maximum number of characters to read
    li $v0, 8
    syscall
    jr $ra

read_char:
    li $v0, 12
    syscall
    # $v0 contains character read
    jr $ra
    
read_int:
    li $v0, 5
    syscall
    # $v0 contains integer read
    jr $ra


exit: # terminate without value
    li $v0, 10
    syscall

exit2: # terminate with value
    # $a0 : termination result
    li $v0, 17
    syscall


$out_of_bounds_error:
    la $a0, $out_of_bounds_error_msg
    jal print_string
    li $a0, 1
    j exit2

.data
$out_of_bounds_error_msg:
    .asciiz "index out of bounds error!\n"
