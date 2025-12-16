#!/bin/rc
# Master test script for all SIP devices

echo '=== SIP Device Integration Tests ==='
echo ''
echo 'Testing all 4 SIP kernel devices:'
echo '  1. /dev/pci - PCI enumeration'
echo '  2. /dev/mem - MMIO access'
echo '  3. /dev/dma - DMA buffer allocation'
echo '  4. /dev/irq - Interrupt delivery'
echo ''

# Run comprehensive AHCI test
if(test -e /bin/test_ahci) {
	echo '--- Running AHCI Integration Test ---'
	/bin/test_ahci
	echo ''
} else {
	echo 'ERROR: test_ahci not found'
}

# Run individual device tests
if(test -e /bin/test_devmem) {
	echo '--- Running /dev/mem Test ---'
	/bin/test_devmem
	echo ''
}

if(test -e /bin/test_devirq) {
	echo '--- Running /dev/irq Test ---'
	/bin/test_devirq
	echo ''
}

echo '=== All Tests Complete ==='
