#!/bin/rc
# Test script for SIP device files

echo '=== Testing SIP Device Files ==='
echo ''

# Test /dev/mem
echo '--- Testing /dev/mem ---'
if(test -e /dev/mem) {
	echo 'OK: /dev/mem exists'
	/bin/test_devmem
} else {
	echo 'FAIL: /dev/mem does not exist'
}
echo ''

# Test /dev/irq
echo '--- Testing /dev/irq ---'
if(test -e /dev/irq/ctl) {
	echo 'OK: /dev/irq/ctl exists'
	/bin/test_devirq
} else {
	echo 'FAIL: /dev/irq does not exist'
}
echo ''

echo '=== Device Tests Complete ==='
