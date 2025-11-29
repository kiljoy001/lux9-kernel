package main

import (
	"encoding/binary"
	"fmt"
	"io"
	"os"
	"os/exec"
)

func main() {
	fmt.Println("Starting Secure MemFS Test Client...")
	// Start memfs server
	cmd := exec.Command("memfs/memfs")
	stdin, _ := cmd.StdinPipe()
	stdout, _ := cmd.StdoutPipe()
	if err := cmd.Start(); err != nil {
		fmt.Printf("Failed to start memfs: %v\n", err)
		os.Exit(1)
	}

	// 1. Tversion
	fmt.Println("-> Tversion")
	sendTversion(stdin, 8192, "9P2000")
	readRversion(stdout)

	// 2. Tattach (fid 1 = root)
	fmt.Println("-> Tattach (fid 1)")
	sendTattach(stdin, 1, 0xFFFFFFFF, "user", "")
	readRattach(stdout)

	// 3. Tcreate "secret.txt" (fid 1 becomes "secret.txt")
	fmt.Println("-> Tcreate 'secret.txt' (fid 1)")
	sendTcreate(stdin, 1, "secret.txt", 0600, 1) // 1 = OWRITE
	readRcreate(stdout)

	// 4. Twrite
	secret := "My Secret Data"
	fmt.Printf("-> Twrite '%s'\n", secret)
	sendTwrite(stdin, 1, 0, []byte(secret))
	count := readRwrite(stdout)
	if count != uint32(len(secret)) {
		fmt.Printf("Error: Write count mismatch. Expected %d, got %d\n", len(secret), count)
		os.Exit(1)
	}

	// 5. Tclunk (close fid 1)
	fmt.Println("-> Tclunk (fid 1)")
	sendTclunk(stdin, 1)
	readRclunk(stdout)

	// 6. Get root again (fid 1)
	fmt.Println("-> Tattach (fid 1 = root)")
	sendTattach(stdin, 1, 0xFFFFFFFF, "user", "")
	readRattach(stdout)

	// 7. Twalk root -> "secret.txt" (fid 2)
	fmt.Println("-> Twalk (fid 1 -> fid 2 'secret.txt')")
	sendTwalk(stdin, 1, 2, []string{"secret.txt"})
	readRwalk(stdout)

	// 8. Topen (fid 2, OREAD)
	fmt.Println("-> Topen (fid 2)")
	sendTopen(stdin, 2, 0)
	readRopen(stdout)

	// 9. Tread
	fmt.Println("-> Tread")
	sendTread(stdin, 2, 0, 100)
	data := readRread(stdout)
	fmt.Printf("Read: '%s'\n", string(data))
	if string(data) != secret {
		fmt.Printf("Error: Data mismatch. Expected '%s', got '%s'\n", secret, string(data))
		os.Exit(1)
	}

	// 10. Tremove (fid 2) - Should wipe memory
	fmt.Println("-> Tremove (fid 2) - Initiating Secure Wipe")
	sendTremove(stdin, 2)
	readRremove(stdout)

	// 11. Verify removal: Twalk root -> "secret.txt" should fail
	fmt.Println("-> Twalk (verify removal)")
	sendTwalk(stdin, 1, 2, []string{"secret.txt"})
	err := readRwalkExpectError(stdout)
	if err {
		fmt.Println("Success: File not found (as expected)")
	} else {
		fmt.Println("Error: File still exists after remove!")
		os.Exit(1)
	}

	stdin.Close()
	cmd.Wait()
	fmt.Println("Test Sequence Completed Successfully.")
}

// --- Helper Functions ---

func sendTversion(w io.Writer, msize uint32, version string) {
	buf := make([]byte, 4+1+2+4+2+len(version))
	binary.LittleEndian.PutUint32(buf[0:4], uint32(len(buf)))
	buf[4] = 100
	binary.LittleEndian.PutUint16(buf[5:7], 0xFFFF)
	binary.LittleEndian.PutUint32(buf[7:11], msize)
	binary.LittleEndian.PutUint16(buf[11:13], uint16(len(version)))
	copy(buf[13:], version)
	w.Write(buf)
}

func readRversion(r io.Reader) {
	readMsg(r)
}

func sendTattach(w io.Writer, fid, afid uint32, uname, aname string) {
	size := 4 + 1 + 2 + 4 + 4 + 2 + len(uname) + 2 + len(aname)
	buf := make([]byte, size)
	binary.LittleEndian.PutUint32(buf[0:4], uint32(size))
	buf[4] = 104
	binary.LittleEndian.PutUint16(buf[5:7], 1)
	binary.LittleEndian.PutUint32(buf[7:11], fid)
	binary.LittleEndian.PutUint32(buf[11:15], afid)
	binary.LittleEndian.PutUint16(buf[15:17], uint16(len(uname)))
	copy(buf[17:], uname)
	off := 17 + len(uname)
	binary.LittleEndian.PutUint16(buf[off:off+2], uint16(len(aname)))
	copy(buf[off+2:], aname)
	w.Write(buf)
}

func readRattach(r io.Reader) {
	readMsg(r)
}

func sendTcreate(w io.Writer, fid uint32, name string, perm uint32, mode uint8) {
	size := 4 + 1 + 2 + 4 + 2 + len(name) + 4 + 1
	buf := make([]byte, size)
	binary.LittleEndian.PutUint32(buf[0:4], uint32(size))
	buf[4] = 114 // Tcreate
	binary.LittleEndian.PutUint16(buf[5:7], 2)
	binary.LittleEndian.PutUint32(buf[7:11], fid)
	binary.LittleEndian.PutUint16(buf[11:13], uint16(len(name)))
	copy(buf[13:], name)
	off := 13 + len(name)
	binary.LittleEndian.PutUint32(buf[off:off+4], perm)
	buf[off+4] = mode
	w.Write(buf)
}

func readRcreate(r io.Reader) {
	readMsg(r)
}

func sendTwrite(w io.Writer, fid uint32, offset uint64, data []byte) {
	size := 4 + 1 + 2 + 4 + 8 + 4 + len(data)
	buf := make([]byte, size)
	binary.LittleEndian.PutUint32(buf[0:4], uint32(size))
	buf[4] = 118 // Twrite
	binary.LittleEndian.PutUint16(buf[5:7], 3)
	binary.LittleEndian.PutUint32(buf[7:11], fid)
	binary.LittleEndian.PutUint64(buf[11:19], offset)
	binary.LittleEndian.PutUint32(buf[19:23], uint32(len(data)))
	copy(buf[23:], data)
	w.Write(buf)
}

func readRwrite(r io.Reader) uint32 {
	buf := readMsg(r)
	return binary.LittleEndian.Uint32(buf[3:7])
}

func sendTclunk(w io.Writer, fid uint32) {
	buf := make([]byte, 4+1+2+4)
	binary.LittleEndian.PutUint32(buf[0:4], uint32(len(buf)))
	buf[4] = 120 // Tclunk
	binary.LittleEndian.PutUint16(buf[5:7], 4)
	binary.LittleEndian.PutUint32(buf[7:11], fid)
	w.Write(buf)
}

func readRclunk(r io.Reader) {
	readMsg(r)
}

func sendTwalk(w io.Writer, fid, newfid uint32, names []string) {
	// Calculate size
	size := 4 + 1 + 2 + 4 + 4 + 2
	for _, n := range names {
		size += 2 + len(n)
	}
	buf := make([]byte, size)
	binary.LittleEndian.PutUint32(buf[0:4], uint32(size))
	buf[4] = 110 // Twalk
	binary.LittleEndian.PutUint16(buf[5:7], 5)
	binary.LittleEndian.PutUint32(buf[7:11], fid)
	binary.LittleEndian.PutUint32(buf[11:15], newfid)
	binary.LittleEndian.PutUint16(buf[15:17], uint16(len(names)))
	off := 17
	for _, n := range names {
		binary.LittleEndian.PutUint16(buf[off:off+2], uint16(len(n)))
		copy(buf[off+2:], n)
		off += 2 + len(n)
	}
	w.Write(buf)
}

func readRwalk(r io.Reader) {
	buf := readMsg(r)
	if buf[0] == 107 { // Rerror
		fmt.Printf("Rwalk Error: %s\n", string(buf[5:]))
		os.Exit(1)
	}
}

func readRwalkExpectError(r io.Reader) bool {
	buf := readMsg(r)
	return buf[0] == 107 // Rerror
}

func sendTopen(w io.Writer, fid uint32, mode byte) {
	buf := make([]byte, 4+1+2+4+1)
	binary.LittleEndian.PutUint32(buf[0:4], uint32(len(buf)))
	buf[4] = 112 // Topen
	binary.LittleEndian.PutUint16(buf[5:7], 6)
	binary.LittleEndian.PutUint32(buf[7:11], fid)
	buf[11] = mode
	w.Write(buf)
}

func readRopen(r io.Reader) {
	readMsg(r)
}

func sendTread(w io.Writer, fid uint32, offset uint64, count uint32) {
	buf := make([]byte, 4+1+2+4+8+4)
	binary.LittleEndian.PutUint32(buf[0:4], uint32(len(buf)))
	buf[4] = 116 // Tread
	binary.LittleEndian.PutUint16(buf[5:7], 7)
	binary.LittleEndian.PutUint32(buf[7:11], fid)
	binary.LittleEndian.PutUint64(buf[11:19], offset)
	binary.LittleEndian.PutUint32(buf[19:23], count)
	w.Write(buf)
}

func readRread(r io.Reader) []byte {
	buf := readMsg(r)
	count := binary.LittleEndian.Uint32(buf[3:7])
	return buf[7 : 7+count]
}

func sendTremove(w io.Writer, fid uint32) {
	buf := make([]byte, 4+1+2+4)
	binary.LittleEndian.PutUint32(buf[0:4], uint32(len(buf)))
	buf[4] = 122 // Tremove
	binary.LittleEndian.PutUint16(buf[5:7], 8)
	binary.LittleEndian.PutUint32(buf[7:11], fid)
	w.Write(buf)
}

func readRremove(r io.Reader) {
	readMsg(r)
}

func readMsg(r io.Reader) []byte {
	var size uint32
	if err := binary.Read(r, binary.LittleEndian, &size); err != nil {
		fmt.Printf("Read error: %v\n", err)
		os.Exit(1)
	}
	buf := make([]byte, size-4)
	if _, err := io.ReadFull(r, buf); err != nil {
		fmt.Printf("Read body error: %v\n", err)
		os.Exit(1)
	}
	if buf[0] == 107 { // Rerror
		// length of error string at buf[3:5]
		errLen := binary.LittleEndian.Uint16(buf[3:5])
		errMsg := string(buf[5 : 5+errLen])
		fmt.Printf("Received Rerror: %s\n", errMsg)
		// Don't exit for Rwalk tests
	}
	return buf
}
